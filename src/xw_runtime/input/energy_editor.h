#ifndef XW_ENERGY_EDITOR_H
#define XW_ENERGY_EDITOR_H
#include "energy_distribution.h"
typedef struct {int lasers,shields,row;bool has_shields;} EnergyEditor;
static inline void EnergyEditor_Begin(EnergyEditor* e,int lasers,int shields,bool has_shields) {
 *e=(EnergyEditor){EnergyLevel(lasers),has_shields?EnergyLevel(shields):0,0,has_shields};
}
static inline int EnergyEditor_MotorMax(const EnergyEditor* e){(void)e;return 8;}
static inline int EnergyEditor_Motors(const EnergyEditor* e){return EnergyEditor_MotorMax(e)-e->lasers-e->shields;}
static inline void EnergyEditor_SetMotor(EnergyEditor* e,int value){
 int max=EnergyEditor_MotorMax(e),min=e->has_shields?0:4;if(value<min)value=min;if(value>max)value=max;
 while(EnergyEditor_Motors(e)<value){
  if(e->lasers>=e->shields&&e->lasers>0)e->lasers--;
  else if(e->has_shields&&e->shields>0)e->shields--;else break;
 }
 while(EnergyEditor_Motors(e)>value){
  if(e->lasers<=e->shields&&e->lasers<4)e->lasers++;
  else if(e->has_shields&&e->shields<4)e->shields++;
  else if(e->lasers<4)e->lasers++;else break;
 }
}
static inline void EnergyEditor_Adjust(EnergyEditor* e,int delta){
 if(e->row==0)e->lasers=EnergyLevel(e->lasers+delta);
 else if(e->row==1){if(e->has_shields)e->shields=EnergyLevel(e->shields+delta);}
 else EnergyEditor_SetMotor(e,EnergyEditor_Motors(e)+delta);
}
#endif
