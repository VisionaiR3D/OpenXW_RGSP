#ifndef RGSP_CHORD_ENGINE_H
#define RGSP_CHORD_ENGINE_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

enum { CHORD_GROUPS=4, CHORD_DIRECTIONS=4, CHORD_EVENTS=24 };
typedef struct { int tap, direction[4]; } ChordBinding;
typedef struct { int action; bool down; } ChordEvent;
typedef struct {
 bool armed, active, gate, wait_modifiers, used;
 unsigned previous_modifiers, previous_dpad;
 int group, held[4], tap_release;
} ChordState;
typedef struct {
 ChordEvent events[CHORD_EVENTS];
 int count;
 bool consume_dpad;
} ChordOutput;
static inline void ChordEmit(ChordOutput *o,int action,bool down) {
 if(action>0 && o->count<CHORD_EVENTS) o->events[o->count++]=(ChordEvent){action,down};
}
static inline void ChordRelease(ChordState *s,ChordOutput *o) {
 for(int i=0;i<4;i++) { ChordEmit(o,s->held[i],false); s->held[i]=0; }
}
static inline ChordOutput ChordStep(ChordState *s,const ChordBinding binding[4],
                                    bool active,unsigned modifiers,unsigned dpad) {
 ChordOutput o={0};
 ChordEmit(&o,s->tap_release,false); s->tap_release=0;
 if(!active) {
  ChordRelease(s,&o); memset(s,0,sizeof(*s)); s->group=-1;
  return o;
 }
 if(!s->active) { s->active=true; s->group=-1; }
 if(!s->armed) {
  if(!modifiers && !dpad) s->armed=true;
  o.consume_dpad=modifiers || dpad;
  s->previous_modifiers=modifiers; s->previous_dpad=dpad;
  return o;
 }
 if(s->gate && !dpad) s->gate=false;
 if(s->wait_modifiers && !modifiers) s->wait_modifiers=false;
 bool started=false;
 if(s->group>=0 && !(modifiers & (1u<<s->group))) {
  ChordRelease(s,&o);
  if(!s->used && !dpad && !modifiers) {
   ChordEmit(&o,binding[s->group].tap,true);
   s->tap_release=binding[s->group].tap;
  }
  s->group=-1; s->gate=dpad!=0; s->wait_modifiers=modifiers!=0;
 }
 unsigned fresh=modifiers & ~s->previous_modifiers;
 if(s->group<0 && !s->wait_modifiers && fresh) {
  for(int i=0;i<4;i++) if(fresh & (1u<<i)) { s->group=i; break; }
  s->used=false; started=true;
 }
 if(s->group>=0) {
  if(dpad) s->used=true;
  bool single=dpad && !(dpad & (dpad-1));
  for(int i=0;i<4;i++) {
   bool down=single && (dpad & (1u<<i));
   if(s->held[i] && !down) { ChordEmit(&o,s->held[i],false); s->held[i]=0; }
   if(down && (started || !(s->previous_dpad & (1u<<i)))) {
    s->held[i]=binding[s->group].direction[i];
    ChordEmit(&o,s->held[i],true);
   }
  }
 }
 o.consume_dpad=s->group>=0 || s->gate || s->wait_modifiers;
 s->previous_modifiers=modifiers; s->previous_dpad=dpad;
 return o;
}
#endif
