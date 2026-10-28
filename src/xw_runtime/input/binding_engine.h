#ifndef HANDHELD_BINDING_ENGINE_H
#define HANDHELD_BINDING_ENGINE_H
#include <stdbool.h>
#include <string.h>
enum { BINDING_CAP=256, BINDING_BITS=12 };
typedef struct { unsigned mask; int action; } FlightBinding;
typedef struct {
 bool armed;
 unsigned previous, used;
 int next[BINDING_CAP], held[BINDING_CAP];
 bool pending[BINDING_CAP], pulse[BINDING_CAP];
} BindingState;
typedef struct { int action; bool down; } BindingEvent;
typedef struct { BindingEvent events[BINDING_CAP*2]; int count; unsigned consume; } BindingOutput;
static inline void BindingEmit(BindingOutput* o,int action,bool down) {
 if(action && o->count<BINDING_CAP*2) o->events[o->count++]=(BindingEvent){action,down};
}
static inline int BindingPick(BindingState* s,const FlightBinding* b,int n,int first) {
 int count=0,which=s->next[first],chosen=0;
 for(int j=first;j<n;j++) if(b[j].mask==b[first].mask && b[j].action) count++;
 if(!count) return 0;
 which%=count; int k=0;
 for(int j=first;j<n;j++) if(b[j].mask==b[first].mask && b[j].action && k++==which) { chosen=b[j].action; break; }
 s->next[first]=(which+1)%count; return chosen;
}
static inline BindingOutput BindingStep(BindingState* s,const FlightBinding* b,int n,bool active,unsigned raw) {
 BindingOutput o={0};
 if(!active) {
  for(int i=0;i<n;i++) { BindingEmit(&o,s->held[i],false); s->held[i]=0; s->pending[i]=false; s->pulse[i]=false; }
  s->armed=false; s->previous=0; s->used=0; return o;
 }
 for(int i=0;i<n;i++) if(b[i].action && b[i].mask) {
  o.consume|=b[i].mask&255u;
  if((raw & b[i].mask & 255u) || ((raw&b[i].mask)==b[i].mask)) o.consume|=b[i].mask&3840u;
  if(b[i].mask!=1u && (b[i].mask&raw&255u)) o.consume|=3840u;
 }
 if(o.consume & raw & 255u & ~1u) o.consume|=3840u;
 if(!s->armed) { if(!raw) s->armed=true; return o; }
 unsigned fresh=raw & ~s->previous;
 for(int i=0;i<n;i++) {
  unsigned m=b[i].mask; if(!m || !b[i].action) continue;
  bool first=true; for(int j=0;j<i;j++) if(b[j].mask==m && b[j].action) { first=false; break; }
  if(!first) continue;
  bool down=(raw&m)==m,was=(s->previous&m)==m;
  if(s->held[i] && (!down || s->pulse[i])) { BindingEmit(&o,s->held[i],false); s->held[i]=0; s->pulse[i]=false; }
  bool prefix=false,shadow=false;
  for(int j=0;j<n;j++) if(b[j].action && b[j].mask!=m && (b[j].mask&m)==m) {
   prefix=true; if((raw&b[j].mask)==b[j].mask) shadow=true;
  }
  if(s->pending[i] && !down) {
   if(!(s->used&m)) { s->held[i]=BindingPick(s,b,n,i); s->pulse[i]=true; BindingEmit(&o,s->held[i],true); }
   s->pending[i]=false;
   /* A deferred tap is released next frame even though the physical key is up. */
  }
  if(down && !was && (fresh&m) && !(s->used&m) && prefix) s->pending[i]=true;
  if(shadow) s->pending[i]=false;
  if(down && (fresh&m) && !shadow && !prefix) {
   s->held[i]=BindingPick(s,b,n,i); BindingEmit(&o,s->held[i],true); s->used|=m;
  }
 }
 s->used &= raw; s->previous=raw; return o;
}

/* Continuous controls must remain held even when their button is a prefix
 * of a system chord. They pass steering through only while used alone (A
 * firing may coexist). A system chord latches them off until its button
 * is released, and the original prefix engine owns all tap commands. */
static inline BindingOutput BindingStepHeld(BindingState* s,const FlightBinding* b,int n,bool active,unsigned raw,int held_action) {
 FlightBinding commands[BINDING_CAP];
 memcpy(commands,b,n*sizeof *b);
 for(int i=0;i<n;i++) if(b[i].action==held_action) commands[i].action=0;
 BindingOutput o=BindingStep(s,commands,n,active,raw);
 if(!active) return o;
 bool steering=false;
 for(int i=0;i<n;i++) if(b[i].action==held_action && b[i].mask) {
  unsigned m=b[i].mask;
  bool down=s->armed && (raw&m)==m && !(s->used&m) &&
    (raw&254u)==(m&254u);
  /* Explicit system-direction bindings take priority over the hold. */
  for(int j=0;j<n;j++) if(commands[j].action && (commands[j].mask&3840u) &&
      (raw&commands[j].mask)==commands[j].mask) down=false;
  if(s->held[i] && !down) { BindingEmit(&o,s->held[i],false);s->held[i]=0; }
  if(down && !s->held[i]) { s->held[i]=held_action;BindingEmit(&o,held_action,true); }
  o.consume|=m&255u;
  if(down && !(m&3840u)) steering=true;
 }
 if(steering) o.consume&=~3840u;
 return o;
}
/* Extend the existing binding engine with continuous flight controls. */
static inline bool RollBinding_IsHeld(int action,int modifier,int left,int right) {
 return action==modifier || action==left || action==right;
}
static inline BindingOutput BindingStepFlight(BindingState* s,const FlightBinding* b,int n,
 bool active,unsigned raw,int modifier,int left,int right) {
 FlightBinding commands[BINDING_CAP];
 memcpy(commands,b,n*sizeof *b);
 /* A held flight control owns its exact mask. Longer commands remain intact. */
 for(int i=0;i<n;i++) if(RollBinding_IsHeld(b[i].action,modifier,left,right))
  for(int j=0;j<n;j++) if(b[j].mask==b[i].mask) commands[j].action=0;
 BindingOutput out=BindingStep(s,commands,n,active,raw);
 if(!active) return out;
 bool steering=false;
 for(int i=0;i<n;i++) if(b[i].mask && RollBinding_IsHeld(b[i].action,modifier,left,right)) {
  unsigned mask=b[i].mask;
  bool down=s->armed && (raw&mask)==mask && !(s->used&mask) && (raw&254u)==(mask&254u);
  /* An explicit direction assignment overrides an ordinary held control. */
  for(int j=0;j<n;j++) if(commands[j].action && (commands[j].mask&3840u) &&
   (raw&commands[j].mask)==commands[j].mask) down=false;
  /* A direct roll assignment replaces a roll modifier sharing the same mask. */
  if(b[i].action==modifier)
   for(int j=0;j<n;j++) if(b[j].mask==mask && (b[j].action==left || b[j].action==right)) down=false;
  if(s->held[i] && !down) { BindingEmit(&out,s->held[i],false);s->held[i]=0; }
  if(down && !s->held[i]) { s->held[i]=b[i].action;BindingEmit(&out,b[i].action,true); }
  out.consume |= mask&255u;
  if((raw&mask&255u) || ((raw&mask)==mask)) out.consume |= mask&3840u;
  if(down && !(mask&3840u)) steering=true;
 }
 if(steering) out.consume &= ~3840u;
 return out;
}

#endif
