#ifndef XW_COCKPIT_SCENE_MATH_H
#define XW_COCKPIT_SCENE_MATH_H
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
static inline float CockpitScene_Dot(const float a[3],const float b[3]) {
 return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
}
static inline bool CockpitScene_Normalize(float v[3]) {
 float n=sqrtf(CockpitScene_Dot(v,v));
 if(!isfinite(n)||n<1e-6f)return false;
 for(int i=0;i<3;i++)v[i]/=n;
 return true;
}
static inline bool CockpitScene_RayBox(const float origin[3],const float direction[3],
                                      const float minimum[3],const float maximum[3],float limit) {
 float near=.5f,far=limit;
 for(int a=0;a<3;a++) {
  if(fabsf(direction[a])<1e-12f){if(origin[a]<minimum[a]||origin[a]>maximum[a])return false;continue;}
  float x=(minimum[a]-origin[a])/direction[a],y=(maximum[a]-origin[a])/direction[a];
  near=fmaxf(near,fminf(x,y));far=fminf(far,fmaxf(x,y));
  if(near>far)return false;
 }
 return far>=near;
}
/* Plane intersection + even/odd polygon containment handles native concave
 * faces without filling their empty corners with a triangle fan. */
static inline bool CockpitScene_RayPolygon(const float origin[3],const float direction[3],
                                          const float vertices[][3],unsigned count,float limit) {
 if(count<3||count>127)return false;
 float normal[3]={0};
 for(unsigned i=2;i<count;i++) {
  float a[3],b[3];for(int k=0;k<3;k++){a[k]=vertices[i-1][k]-vertices[0][k];b[k]=vertices[i][k]-vertices[0][k];}
  normal[0]=a[1]*b[2]-a[2]*b[1];normal[1]=a[2]*b[0]-a[0]*b[2];normal[2]=a[0]*b[1]-a[1]*b[0];
  if(CockpitScene_Dot(normal,normal)>1e-10f)break;
 }
 float denominator=CockpitScene_Dot(normal,direction);
 if(fabsf(denominator)<1e-10f)return false;
 float delta[3];for(int k=0;k<3;k++)delta[k]=vertices[0][k]-origin[k];
 float distance=CockpitScene_Dot(normal,delta)/denominator;
 if(distance<.5f||distance>limit)return false;
 unsigned drop=0;for(unsigned k=1;k<3;k++)if(fabsf(normal[k])>fabsf(normal[drop]))drop=k;
 unsigned u=(drop+1)%3,v=(drop+2)%3;float x=origin[u]+direction[u]*distance,y=origin[v]+direction[v]*distance;
 bool inside=false;
 for(unsigned i=0,j=count-1;i<count;j=i++) {
  float ax=vertices[j][u],ay=vertices[j][v],bx=vertices[i][u],by=vertices[i][v];
  if((ay>y)!=(by>y))if(x<(bx-ax)*(y-ay)/(by-ay)+ax)inside=!inside;
 }
 return inside;
}
#endif
