#include <stdio.h>  // for getc, printf
#include <stdlib.h> // malloc, free
#include "ijvm.h"
#include "util.h" // read this file for debug prints, endianness helper functions


// see ijvm.h for descriptions of the below functions

uint32_t text_size;
uint32_t constant_size;
word_t *constant;
byte_t *text;

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
    return m;
}

void destroy_ijvm(ijvm* m) 
{
    free(text);
    free(constant);
    free(m);
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
  // TODO: implement me
  return 0;
}

word_t tos(ijvm* m) 
{
  // this operation should NOT pop (remove top element from stack)
  // TODO: implement me
  return -1;
}

bool finished(ijvm* m) 
{
  // TODO: implement me
  return false;
}

word_t get_local_variable(ijvm* m, int i) 
{
  // TODO: implement me
  return 0;
}

void step(ijvm* m) 
{
  // TODO: implement me

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
