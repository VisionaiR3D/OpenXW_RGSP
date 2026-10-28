#include "xw_remaster/cockpit_lighting.h"
#include "xw_remaster/cockpit_light_math.h"
#include "xw_remaster/cockpit_scene_lighting.h"
#include "xw_remaster/hud_draw.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cockpit_light_shape.inc"

static bool initialized,cache_valid;
static int mode=1,cached_mode;
static char preference_path[1024];
static float cached_light[3],tints[4000][4];
static XwCockpitEnvironment cached_environment;
static const float neutral[4]={1,1,1,1};
static void Init(void) {
 if(initialized) return;
 initialized=true;
 char* pref=SDL_GetPrefPath("TotallyOpen","OpenXW-HD");
 if(!pref) return;
 int n=snprintf(preference_path,sizeof preference_path,"%scockpit-lighting.cfg",pref);
 SDL_free(pref);
 if(n<0||n>=(int)sizeof preference_path){preference_path[0]=0;return;}
 FILE* f=fopen(preference_path,"r");
 if(f){int value;char extra;if(fscanf(f,"%d %c",&value,&extra)==1&&value>=0&&value<=2)mode=value;fclose(f);}
}
int XwCockpitLighting_Mode(void) {Init();return mode;}
const char* XwCockpitLighting_Name(void) {
 static const char* const names[]={"OFF","SUBTLE","STRONG"};
 return names[XwCockpitLighting_Mode()];
}
bool XwCockpitLighting_SetMode(int next) {
 Init();if(next<0||next>2||!preference_path[0])return false;
 char tmp[1040];snprintf(tmp,sizeof tmp,"%s.tmp",preference_path);
 FILE* f=fopen(tmp,"w");if(!f)return false;
 bool ok=fprintf(f,"%d\n",next)>0;
 if(fflush(f)!=0)ok=false;
 if(ok&&fsync(fileno(f))!=0)ok=false;
 if(fclose(f)!=0)ok=false;
 if(!ok||rename(tmp,preference_path)!=0){remove(tmp);return false;}
 mode=next;cache_valid=false;return true;
}
static void EnvironmentTint(int x,int y,const XwCockpitEnvironment* environment,float tint[4]) {
 const unsigned char* p=cockpit_light_shape[y*80+x];
 if(!p[3]||(environment->sun_visibility==1&&environment->flash_count==0))return;
 float mask=p[3]/255.0f,amount=mask*(mode==1?.5f:1.0f),ambient=mode==1?.62f:.28f;
 float floor_tint=1+amount*(ambient-1);
 if(environment->sun_visibility<1)for(int c=0;c<3;c++)tint[c]=floor_tint+(tint[c]-floor_tint)*environment->sun_visibility;
 if(!environment->flash_count)return;
 float normal[3],length=0;
 for(int c=0;c<3;c++){normal[c]=p[c]*(2.0f/255)-1;length+=normal[c]*normal[c];}
 length=sqrtf(length);if(length<1e-6f)return;
 float additive[3]={0};
 for(unsigned i=0;i<environment->flash_count;i++) {
  const XwCockpitFlash* flash=&environment->flashes[i];
  float facing=0;for(int c=0;c<3;c++)facing+=normal[c]/length*flash->direction[c];
  float front=CockpitLight_Clamp((flash->direction[2]+.2f)/.5f,0,1);
  float contribution=flash->strength*fmaxf(0,.3f+.7f*facing)*front*(mode==1?.4f:.85f);
  for(int c=0;c<3;c++)additive[c]+=flash->color[c]*contribution;
 }
 for(int c=0;c<3;c++)tint[c]+=mask*fminf(additive[c],mode==1?.35f:.65f);
}
bool XwCockpitLighting_Prepare(const XwHudDraw* draw) {
 if(!XwCockpitLighting_Mode()||!draw||!draw->definition||!draw->snapshot)return false;
 const XwRenderSnapshot* s=draw->snapshot;
 const XwRenderSource* source=XwRenderAssets_Source(draw->definition->base);
 /* The authored shape must never be applied to another craft, side/rear view,
  * a different resolution, external view, or suppressed cockpit. */
 if(s->flight_version!=94||s->camera.external||s->cockpit.suppressed||s->cockpit.mirrored||
    draw->definition->width!=320||draw->definition->height!=200||!source)return false;
 const char* name=strrchr(source->path,'/');name=name?name+1:source->path;
 const char* backslash=strrchr(name,'\\');if(backslash)name=backslash+1;
 if(strcmp(name,"XWING12.LFD")&&strcmp(name,"xwing12.lfd"))return false;
 float light[3];
 if(!CockpitLight_LocalDirection(s->camera.rows,s->appearance.direction_q15,light))return false;
 XwCockpitEnvironment environment;XwCockpitScene_Prepare(s,&environment);
 bool same=cache_valid&&cached_mode==mode&&!memcmp(&cached_environment,&environment,sizeof environment);
 for(int c=0;c<3;c++)same=same&&fabsf(light[c]-cached_light[c])<.0001f;
 if(!same) {
  for(int y=0;y<50;y++)for(int x=0;x<80;x++) {
   CockpitLight_TintCell(cockpit_light_shape,x,y,light,mode,tints[y*80+x]);
   EnvironmentTint(x,y,&environment,tints[y*80+x]);
  }
  cached_environment=environment;
  memcpy(cached_light,light,sizeof light);cached_mode=mode;cache_valid=true;
 }
 return true;
}
const float* XwCockpitLighting_Tint(int x,int y) {
 if(x<0||x>=320||y<0||y>=200||!cache_valid)return neutral;
 return tints[(y/4)*80+x/4];
}
void XwCockpitLighting_Reset(void){cache_valid=false;memset(&cached_environment,0,sizeof cached_environment);XwCockpitScene_Reset();}
