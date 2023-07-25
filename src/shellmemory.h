#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
#include "pcb.h"
void mem_init(int frame_size, int var_size);
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(FILE* fp, int* pStart, int* pEnd, char* fileID);
int load_page(FILE* fp, PCB* process);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
#endif