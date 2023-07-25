#include "pcb.h"
#include "ready_queue.h"

void LRU_queue_add_to_tail(QueueNode *node);
void print_LRU_queue();
QueueNode *LRU_queue_pop_head();