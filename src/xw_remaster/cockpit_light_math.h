#ifndef XW_COCKPIT_LIGHT_MATH_H
#define XW_COCKPIT_LIGHT_MATH_H
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
enum { COCKPIT_LIGHT_WIDTH=80, COCKPIT_LIGHT_HEIGHT=50, COCKPIT_LIGHT_CELL=4 };
static inline float CockpitLight_Clamp(float x,float a,float b) { return fminf(b,fmaxf(a,x)); }
/* Source camera rows are right, eye-down, forward. Authored shape is right,
 * eye-up, aperture-facing: positive Z admits light from the front window. */
static inline bool CockpitLight_LocalDirection(const float rows[9],const int32_t world[3],float out[3]) {
 for(int r=0;r<3;r++) {
  out[r]=0;
  for(int a=0;a<3;a++) out[r]+=rows[r*3+a]*(world[a]/32768.0f);
 }
 out[1]=-out[1];
 float length=sqrtf(out[0]*out[0]+out[1]*out[1]+out[2]*out[2]);
 if(!isfinite(length)||length<1e-6f) return false;
 for(int a=0;a<3;a++) out[a]/=length;
 return true;
}
static inline float CockpitLight_Depth(const unsigned char shape[][5],float x,float y) {
 if(x<0||x>79||y<0||y>49) return 0;
 int x0=(int)x,y0=(int)y,x1=x0<79?x0+1:x0,y1=y0<49?y0+1:y0;
 float tx=x-x0,ty=y-y0;
 float top=shape[y0*80+x0][4]*(1-tx)+shape[y0*80+x1][4]*tx;
 float bottom=shape[y1*80+x0][4]*(1-tx)+shape[y1*80+x1][4]*tx;
 return (top*(1-ty)+bottom*ty)*(3.0f/255);
}
static inline void CockpitLight_TintCell(const unsigned char shape[][5],int x,int y,
                                        const float light[3],int mode,float tint[4]) {
 for(int c=0;c<4;c++) tint[c]=1;
 if(mode<=0||x<0||x>=80||y<0||y>=50) return;
 const unsigned char* p=shape[y*80+x];
 if(!p[3]) return;
 float n[3],len=0;
 for(int c=0;c<3;c++){n[c]=p[c]*(2.0f/255)-1;len+=n[c]*n[c];}
 len=sqrtf(len);
 float diffuse=0;
 if(len>1e-6f) for(int c=0;c<3;c++) diffuse+=n[c]/len*light[c];
 /* A source behind the pilot cannot shine through the solid aft cockpit. */
 float admission=CockpitLight_Clamp((light[2]+.15f)/.45f,0,1);
 admission=admission*admission*(3-2*admission);
 diffuse=fmaxf(0,diffuse)*admission;
 float shadow=0,h=p[4]*(3.0f/255);
 if(light[2]>0) for(int distance=2;distance<=22;distance+=2) {
  float xx=x+light[0]*distance/4.0f,yy=y-light[1]*distance/4.0f;
  float blocker=CockpitLight_Depth(shape,xx,yy);
  float delta=blocker-h-distance*light[2]*.10f;
  shadow=fmaxf(shadow,CockpitLight_Clamp(delta*.85f,0,.55f));
 }
 /* SUBTLE preserves the original gentle exposure. STRONG deliberately has
  * a wider linear-light range: the earlier ~0.72..1.12 gains were difficult
  * to distinguish on the RGSP after sRGB encoding, especially head-on. */
 const float strength=mode==1?.5f:1.0f;
 const float ambient=mode==1?.62f:.28f;
 const float sun_strength=mode==1?.5f:1.10f;
 float amount=(p[3]/255.0f)*strength;
 float direct=sun_strength*diffuse*(1-shadow);
 const float warmth[3]={1.04f,1.0f,.96f};
 for(int c=0;c<3;c++) tint[c]=1+amount*(ambient+direct*warmth[c]-1);
}
#endif
