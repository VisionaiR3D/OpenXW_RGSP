#ifndef HANDHELD_STEERING_ASSIST_H
#define HANDHELD_STEERING_ASSIST_H
#include <math.h>
#include <stdbool.h>
#include <string.h>
typedef struct { float age[2],gain; int sign[2]; } SteeringAssist;
static inline float AssistClamp(float x,float low,float high){return x<low?low:x>high?high:x;}
/* Angle only: no engine scratch globals, target selection or orientation is changed. */
static inline float AssistTargetGain(double dx,double dy,double dz,double fx,double fy,double fz){
 double dot=dx*fx+dy*fy+dz*fz, norm=(dx*dx+dy*dy+dz*dz)*(fx*fx+fy*fy+fz*fz);
 if(!(dot>0) || !(norm>0) || !isfinite(norm)) return 1.0f;
 double cosine=dot/sqrt(norm);
 /* Full precision inside 3 degrees; smoothly blend back to normal at 8. */
 return 0.48f+0.52f*AssistClamp((float)((0.998629535-cosine)/(0.998629535-0.990268069)),0,1);
}
static inline void AssistReset(SteeringAssist* s){memset(s,0,sizeof *s);s->gain=1;}
static inline void AssistStep(SteeringAssist* s,int* x,int* y,float dt,float target_gain,bool enabled){
 if(!enabled){AssistReset(s);return;}
 dt=AssistClamp(dt,0,0.05f);target_gain=AssistClamp(target_gain,0.48f,1);
 if(!(s->gain>=0.48f && s->gain<=1))s->gain=1;
 float change=dt*4.0f;s->gain+=AssistClamp(target_gain-s->gain,-change,change);
 int raw[2]={*x,*y},result[2];
 for(int i=0;i<2;i++){
  int sign=(raw[i]>0)-(raw[i]<0);
  if(!sign){s->sign[i]=0;s->age[i]=0;result[i]=0;continue;}
  if(sign!=s->sign[i]){s->sign[i]=sign;s->age[i]=0;}
  float ramp=0.40f+0.60f*AssistClamp(s->age[i]/0.18f,0,1);
  s->age[i]+=dt;result[i]=(int)(raw[i]*ramp*s->gain);
 }
 if(raw[0] && raw[1]){result[0]=(int)(result[0]*0.70710678f);result[1]=(int)(result[1]*0.70710678f);}
 *x=result[0];*y=result[1];
}
#endif

