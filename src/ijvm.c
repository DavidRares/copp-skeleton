#include <stdio.h>  // for getc, printf
#include <stdlib.h> // malloc, free
#include "ijvm.h"
#include "util.h" // read this file for debug prints, endianness helper functions


// see ijvm.h for descriptions of the below functions
ijvm* m;
uint32_t text_size;
uint32_t constant_size;
word_t *constant;
word_t *local_variables;
byte_t *text;
unsigned int programCounter = 0;
unsigned int firstPush = 1;
unsigned int activeMethods = 0;
word_t *lv;
word_t *sp;
word_t prevLV;

struct customStack_t{
    word_t element[65];// daca il fac 64 nu mai merge test 5
    unsigned long size;
    unsigned long max;
}*myStack;

// Stack functions
void push(word_t element)
{
    if(myStack->size == 0 && firstPush == 1) {
        firstPush = 0;
        myStack->element[myStack->size] = element;
    }
     
    else if(myStack->size < myStack->max - 1)
    {
        myStack->size += 1;
        myStack->element[myStack->size] = element;
        sp = &myStack->element[myStack->size];
    }
    else
    {
        myStack = (struct customStack_t *)realloc(myStack, sizeof(myStack->max) + 1);
        myStack->size += 1;
        myStack ->max +=1;
        myStack->element[myStack->size] = element;
        sp = &myStack->element[myStack->size];
    }
}

word_t pop(void)
{
    //assert(myStack->size >= 0);
    word_t returnee = myStack->element[myStack->size];
    myStack->element[myStack->size] = 0;
    if (myStack->size > 0) myStack->size -= 1;
    else if(myStack->size == 0) firstPush = 1;
    sp = &myStack->element[myStack->size];
    return returnee;
}
//end of Stack functions

//Instruction functions
void iadd(void)
{
    word_t a, b;
    a = pop();
    b = pop();
    push(a + b);
}

void isub(void)
{
    word_t a, b;
    a = pop();
    b = pop();
    push(b - a);
}

void iand(void)
{
    word_t a, b;
    a = pop();
    b = pop();
    //a = a&b;
    push(a&b);
}

void ior(void)
{
    word_t a, b;
    a = pop();
    b = pop();
    push(a|b);
}

void swap(void)
{
    word_t a, b;
    a = pop();
    b = pop();
    push(a);
    push(b);
}

void branch(byte_t instruction , byte_t argument)
{
    short address;
    byte_t arg2 = text[programCounter + 2];
    address = argument * 0x100u + arg2;
    word_t compElement = pop();
    
    switch(instruction)
    {
        case OP_GOTO: push(compElement); programCounter += (int16_t)address; break;
        case OP_IFEQ: if(compElement == 0)
            programCounter += address;
        else programCounter += 3;
        break;
        case OP_IFLT:
            if(compElement < 0) programCounter += address;
            else programCounter +=3;
        break;
        case OP_IF_ICMPEQ: 
            if(compElement == pop()) programCounter += address;
            else programCounter += 3;
        break;
        default: break;
    }
}

void iirc(byte_t argument)
{
    byte_t argument2 = text[programCounter + 2];
    int32_t argument2_conv = argument2;
    if (argument2_conv >= 128) { argument2_conv -= 256;}
    local_variables[argument] += (word_t)argument2_conv;
    //d2printf("local variable after %02x and argument2 = %d\n",local_variables[argument],(byte_t)argument2_conv);
}

void wide_iirc(word_t address)
{
    byte_t increment = text[programCounter + 4];
    int32_t increment_conv = increment;
    if (increment_conv >= 128) { increment_conv -= 256;}
    if(lv == &myStack->element[0] && activeMethods == 0) local_variables[address] += (word_t)increment_conv;
    else lv[address] += (word_t)increment_conv;
    programCounter++;
    //d2printf("local variable after %02x and argument2 = %d\n",local_variables[argument],(byte_t)argument2_conv);
}

void wide_instructions(byte_t instruction)
{
    byte_t part1 = text[programCounter + 2];
    byte_t part2 = text[programCounter + 3];
    int32_t address = part1 * 0x100u + part2;
    
    switch(instruction)
    {
        case OP_ILOAD: if(lv == &myStack->element[0] && activeMethods == 0) push(local_variables[address]); else push(lv[address]); break;
        case OP_ISTORE: if(lv == &myStack->element[0] && activeMethods == 0) local_variables[address] = pop();else lv[address] = pop(); break;
        case OP_IINC: wide_iirc(address);break;
    }
}
//End of Instruction functions

ijvm* init_ijvm(char *binary_path, FILE* input , FILE* output)
{
    // do not change these first three lines
    ijvm* m = (ijvm *) malloc(sizeof(ijvm));
    // note that malloc gives you memory, but gives no guarantees on the initial
    // values of that memory. It might be all zeroes, or be random data.
    // It is hence important that you initialize all variables in the ijvm
    // struct and do not assume these are set to zero.
    m->in = input;
    m->out = output;
    
    FILE *fp = fopen(binary_path, "rb");
    
    uint32_t magicNumber;
    fread(&magicNumber, sizeof(uint32_t), 1, fp);
    magicNumber = swap_uint32(magicNumber);
    if(magicNumber != 0x1DEADFAD) {return NULL;}
    
    fseek(fp, 4, SEEK_CUR);
       
    fread(&constant_size, sizeof(uint32_t), 1, fp);
    constant_size = swap_uint32(constant_size);
       
    constant = (word_t *)malloc(sizeof(word_t) * constant_size);
    local_variables = (word_t *)malloc(sizeof(word_t) * 256);
    
    for(int i = 0; i < constant_size; i++)
    {
        fread(&constant[i], sizeof(word_t), 1, fp);
        constant[i] = swap_uint32(constant[i]);
    }
       
    fseek(fp, constant_size + 16, SEEK_SET);

    fread(&text_size, sizeof(uint32_t), 1, fp);
    text_size = swap_uint32(text_size);
       
    text = (byte_t *)malloc(sizeof(byte_t) * text_size);
       
    for(int i = 0; i < text_size; i++)
    {
        fread(&text[i], sizeof(byte_t), 1, fp);
    }
    //Stack setup
    myStack = (struct  customStack_t*)malloc(sizeof(struct customStack_t));
    myStack->max = 5000000;
    myStack->size = 0;
    lv = &myStack->element[0];
    prevLV = 0;
    programCounter = 0;
    activeMethods = 0;
    return m;
}

void push_constant(byte_t argument)
{
    byte_t arg2 = (word_t)text[programCounter + 2];
    short index = argument * 0x100u + arg2;
    push(get_constant(m, index));
    sp = &myStack->element[myStack->size];
}

void destroy_ijvm(ijvm* m)
{
    free(text);
    free(constant);
    free(m);
    free(local_variables);
    free(myStack);
}

byte_t *get_text(ijvm* m) 
{
    return (byte_t *)text;
}

unsigned int get_text_size(ijvm* m) 
{
    return text_size;
}

word_t get_constant(ijvm* m,int i) 
{
    return constant[i];
    return 0;
}

unsigned int get_program_counter(ijvm* m) 
{
    return programCounter;
}

word_t tos(ijvm* m) 
{
    if(myStack->size >= 0) return myStack->element[myStack->size];
    return -1;
}

bool finished(ijvm* m) 
{
  // TODO: implement me
    if(programCounter >= text_size) return true;
  return false;
}

word_t get_local_variable(ijvm* m, int i) 
{
    if (lv == &myStack->element[0] && activeMethods == 0) return local_variables[i];
    else
    {
        return *(lv + i);
    }
    return 0;
}

void step(ijvm* m) 
{
  // TODO: implement me
    byte_t instruction = text[programCounter];
    int8_t argument;
    char arg2;
    switch(instruction)
    {
        case OP_BIPUSH: argument = text[programCounter + 1];push(argument); programCounter += 2; break;
        case OP_DUP: push(tos(m)); programCounter++; break;
        case OP_IADD: iadd(); programCounter++; break;
        case OP_IAND: iand(); programCounter++; break;
        case OP_IOR: ior(); programCounter++; break;
        case OP_ISUB: isub(); programCounter++; break;
        case OP_NOP: programCounter++; break;
        case OP_POP: pop(); programCounter++; break;
        case OP_SWAP: swap(); programCounter++; break;
        case OP_ERR: fprintf(m->out, "Error"); programCounter= get_text_size(m) + 5; break;
        case OP_HALT: programCounter = get_text_size(m) + 5; break;
        case OP_IN: arg2 = fgetc(m->in);
            if(arg2 == EOF) push(0x0);
            else push(arg2);
            programCounter++; break;
        case OP_OUT: argument = pop(); fprintf(m->out, "%c", (char)argument); programCounter++; break;
        case OP_GOTO:
        case OP_IFEQ:
        case OP_IFLT:
        case OP_IF_ICMPEQ: argument = text[programCounter + 1];branch(instruction, argument); break;
        case OP_LDC_W: argument = text[programCounter + 1];push_constant(argument);programCounter += 3; break;
        case OP_ILOAD: argument = text[programCounter + 1];
            if(lv == &myStack->element[0] && activeMethods == 0) {push(local_variables[argument]);}
            else
            {
                push(*(lv + argument));
            }
            programCounter +=2; break;
        case OP_ISTORE: argument = text[programCounter + 1]; if(lv == &myStack->element[0] && activeMethods == 0) {local_variables[argument] = pop();}
            else
            {
                *(lv + argument) = pop();
            }
            programCounter +=2; break;
        case OP_IINC: argument = text[programCounter + 1];iirc(argument); programCounter += 3; break;
        case OP_WIDE: argument = text[programCounter + 1];wide_instructions(argument); programCounter += 4;break;
    }
}

byte_t get_instruction(ijvm* m) 
{ 
  return get_text(m)[get_program_counter(m)]; 
}

ijvm* init_ijvm_std(char *binary_path) 
{
  return init_ijvm(binary_path, stdin, stdout);
}

void run(ijvm* m) 
{
  while (!finished(m)) 
  {
    step(m);
  }
}


// Below: methods needed by bonus assignments, see ijvm.h
// You can leave these unimplemented if you are not doing these bonus 
// assignments.

int get_call_stack_size(ijvm* m) 
{
   // TODO: implement me if doing tail call bonus
   return 0;
}


// Checks if reference is a freed heap array. Note that this assumes that 
// 
bool is_heap_freed(ijvm* m, word_t reference) 
{
   // TODO: implement me if doing garbage collection bonus
   return 0;
}
