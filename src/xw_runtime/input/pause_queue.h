#ifndef TIE_PAUSE_QUEUE_H
#define TIE_PAUSE_QUEUE_H
#include <stdbool.h>
#include <string.h>
enum { PAUSE_COMMAND_CAP=16 };
typedef struct { int actions[PAUSE_COMMAND_CAP]; int count; } PauseQueue;
static inline bool PauseQueue_Push(PauseQueue* q,int action) {
 if(action<=0 || q->count>=PAUSE_COMMAND_CAP) return false;
 q->actions[q->count++]=action; return true;
}
static inline void PauseQueue_Undo(PauseQueue* q) { if(q->count>0) --q->count; }
static inline void PauseQueue_Clear(PauseQueue* q) { q->count=0; }
static inline int PauseQueue_Drain(PauseQueue* q,int out[PAUSE_COMMAND_CAP]) {
 int count=q->count; memcpy(out,q->actions,(size_t)count*sizeof(int)); q->count=0; return count;
}
#endif
