#ifndef KERNEL
#define KERNEL
#include "pcb.h"
int process_initialize(char *filename);
int schedule_by_policy(char *policy, bool mt);
int shell_process_initialize();
int store_initialize(char *filename);
void ready_queue_destory();
void threads_terminate();
int create_backing_store();
int delete_backing_store();
int copy_file(char *source_path, char *target_path);
int line_count(char *filename);
#endif