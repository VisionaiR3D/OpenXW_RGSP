#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "xw_remaster/cockpit_light_shape.inc"
#include "xw_remaster/cockpit_lighting.h"
#include "xw_remaster/hud_draw.h"
static XwRenderSource source;
static XwRenderAssetSetView asset_set;
const XwRenderAssetSetView* XwRenderAssets_Set(XwRenderAssetId id){return id==3?&asset_set:NULL;}
static bool fail_pref;
char* SDL_GetPrefPath(const char* a,const char* b){(void)a;(void)b;return strdup(fail_pref?"/missing-directory-cockpit-light-test/":"./");}
void SDL_free(void* p){free(p);}
const XwRenderSource* XwRenderAssets_Source(XwRenderAssetId id){return id?&source:NULL;}
int main(int argc,char**argv){
 if(argc>1&&!strcmp(argv[1],"failure")){fail_pref=true;assert(XwCockpitLighting_Mode()==1);assert(!XwCockpitLighting_SetMode(2));assert(XwCockpitLighting_Mode()==1);puts("PASS: failed save preserves current mode");return 0;}
 if(argc>1&&!strcmp(argv[1],"reload")){assert(XwCockpitLighting_Mode()==2);puts("PASS: mode persists across processes");return 0;}
 remove("cockpit-lighting.cfg");assert(XwCockpitLighting_Mode()==1);assert(XwCockpitLighting_SetMode(2));assert(!XwCockpitLighting_SetMode(3));
 XwRenderSnapshot* snapshot=calloc(1,sizeof *snapshot);assert(snapshot);snapshot->flight_version=94;snapshot->camera.rows[0]=snapshot->camera.rows[4]=snapshot->camera.rows[8]=1;snapshot->appearance.direction_q15[0]=snapshot->appearance.direction_q15[2]=23170;
 XwCockpitDefinition definition={.base=1,.width=320,.height=200};XwHudDraw draw={.snapshot=snapshot,.definition=&definition};strcpy(source.path,"/CP/XWING12.LFD");
 assert(XwCockpitLighting_Prepare(&draw));assert(XwCockpitLighting_Tint(0,0)[3]==1);
 strcpy(source.path,"/CP/XWING3.LFD");assert(!XwCockpitLighting_Prepare(&draw));strcpy(source.path,"/CP/AWING12.LFD");assert(!XwCockpitLighting_Prepare(&draw));strcpy(source.path,"/CP/XWING12.LFD");
 snapshot->camera.external=1;assert(!XwCockpitLighting_Prepare(&draw));snapshot->camera.external=0;
 snapshot->cockpit.suppressed=1;assert(!XwCockpitLighting_Prepare(&draw));snapshot->cockpit.suppressed=0;
 snapshot->cockpit.mirrored=1;assert(!XwCockpitLighting_Prepare(&draw));snapshot->cockpit.mirrored=0;
 snapshot->flight_version=98;assert(!XwCockpitLighting_Prepare(&draw));snapshot->flight_version=94;
 definition.width=640;assert(!XwCockpitLighting_Prepare(&draw));definition.width=320;
 assert(XwCockpitLighting_SetMode(0));assert(!XwCockpitLighting_Prepare(&draw));assert(XwCockpitLighting_SetMode(2));
 assert(XwCockpitLighting_Prepare(&draw));XwCockpitLighting_Reset();assert(XwCockpitLighting_Tint(140,140)[0]==1);assert(XwCockpitLighting_Prepare(&draw));
 float baseline[4000][4];for(int i=0;i<4000;i++)memcpy(baseline[i],XwCockpitLighting_Tint(i%80*4,i/80*4),sizeof baseline[i]);
 snapshot->flight_assets=3;asset_set.bindings.type_count=120;snapshot->key.mission_generation=1;snapshot->key.world_generation=1;snapshot->simulation_ticks=100;snapshot->object_count=1;
 snapshot->objects[0]=(XwSnapObject){.id={.kind=1,.slot=5,.generation=1},.genus=5,.type=18,.world_pos={0,0,100},.previous_world_pos={0,0,90}};
 assert(XwCockpitLighting_Prepare(&draw));unsigned colored=0,protected=0;
 for(int i=0;i<4000;i++){
  const float* tint=XwCockpitLighting_Tint(i%80*4,i/80*4);
  if(!cockpit_light_shape[i][3]){assert(memcmp(tint,baseline[i],sizeof baseline[i])==0);protected++;}
  else if(tint[0]>baseline[i][0]+.01f&&tint[0]-baseline[i][0]>tint[1]-baseline[i][1])colored++;
 }
 assert(colored>300&&protected>2000);
 printf("PASS: actual integrated red laser tints %u cells; %u protected cells bit-identical\n",colored,protected);
 snapshot->objects[0].type=20;snapshot->simulation_ticks++;assert(XwCockpitLighting_Prepare(&draw));colored=0;
 for(int i=0;i<4000;i++){const float* tint=XwCockpitLighting_Tint(i%80*4,i/80*4);if(tint[1]>baseline[i][1]+.01f&&tint[1]-baseline[i][1]>tint[0]-baseline[i][0])colored++;}assert(colored>300);
 puts("PASS: actual integrated green laser changes the correct channel");
 snapshot->object_count=4;
 for(int i=0;i<4;i++){snapshot->objects[i]=snapshot->objects[0];snapshot->objects[i].id.slot=5+i;snapshot->objects[i].type=18+i;}
 clock_t start=clock();for(int j=0;j<100;j++){snapshot->simulation_ticks++;for(int i=0;i<4;i++){snapshot->objects[i].world_pos[0]=(j%20)-10+i*12;snapshot->objects[i].previous_world_pos[0]=snapshot->objects[i].world_pos[0];}assert(XwCockpitLighting_Prepare(&draw));}
 printf("Integrated scene + tint recalculation: %.3f ms/grid (four moving lasers; excludes GPU)\n",1000.0*(clock()-start)/CLOCKS_PER_SEC/100);
 free(snapshot);
 puts("PASS: persistent selection, view/craft/resolution/version guards, OFF fallback and cache reset");return 0;
}
