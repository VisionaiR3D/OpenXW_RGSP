#ifndef COMMAND_LEAD_SOLUTION_H
#define COMMAND_LEAD_SOLUTION_H
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
/* Engine bolts inherit shooter SPEED along their firing direction, not the
 * shooter's velocity vector. All speeds below use native units. Both engines
 * convert those to world units/second with 4660/256 (MATH2_mphconvert).
 * Straight-line, constant-speed prediction; does not steer or fire for player. */
static inline bool LeadSolution(const int32_t origin[3],const int32_t target[3],
 const int16_t direction[3],double target_speed,double bolt_speed,double lifetime,int32_t out[3]){
 if(!(bolt_speed>0)||!(lifetime>0)||target_speed<0)return false;
 double r[3],v[3],a=0,b=0,c=0,s=bolt_speed*(4660.0/256.0);
 for(int i=0;i<3;i++){r[i]=(double)target[i]-origin[i];v[i]=direction[i]*(target_speed*(4660.0/256.0)/32768.0);a+=v[i]*v[i];b+=2*r[i]*v[i];c+=r[i]*r[i];}
 a-=s*s;if(c<1)return false;
 double t=-1;
 if(fabs(a)<1e-10*s*s){if(fabs(b)>1e-12)t=-c/b;}
 else{
  double disc=b*b-4*a*c;if(disc<0)return false;
  double q=-.5*(b+copysign(sqrt(disc),b));
  double t0=q/a,t1=q!=0?c/q:-1;
  if(t0>0)t=t0;
  if(t1>0&&(t<0||t1<t))t=t1;
 }
 if(!isfinite(t)||t<=0||t>lifetime)return false;
 for(int i=0;i<3;i++){double p=target[i]+v[i]*t;if(!isfinite(p)||p<INT32_MIN||p>INT32_MAX)return false;out[i]=(int32_t)llround(p);}
 return true;
}
#endif
