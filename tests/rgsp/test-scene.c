#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "xw_remaster/cockpit_scene_lighting.h"
#include "xw_remaster/cockpit_scene_math.h"
#include "xw_runtime/snapshot/render_dos_assets.h"
static XwRenderAssetSetView set;
static XwRenderDosMesh mesh;
static XwRenderDosFace faces[6];
static XwSnapRange component={0,1};
static XwRenderDosModel model;
static XwRenderSource geometry;
const XwRenderSource* XwRenderAssets_Source(XwRenderAssetId id){return id==2?&geometry:NULL;}
const XwRenderAssetSetView* XwRenderAssets_Set(XwRenderAssetId id){return id==7?&set:NULL;}
static void Setup(void){
 const int16_t v[8][3]={{-50,-50,-50},{50,-50,-50},{50,50,-50},{-50,50,-50},{-50,-50,50},{50,-50,50},{50,50,50},{-50,50,50}};
 const uint8_t indices[6][4]={{0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}};
 memcpy(mesh.vertices,v,sizeof v);mesh.vertex_count=8;mesh.face_count=6;mesh.format=1;mesh.faces=faces;
 for(int i=0;i<6;i++){faces[i].vertex_count=4;faces[i].color=14;memcpy(faces[i].indices,indices[i],4);}
 model=(XwRenderDosModel){.component_count=1,.lod_count=1,.components=&component,.lods=&mesh};
 geometry=(XwRenderSource){.id=2,.kind=XW_SOURCE_DOS_MODEL,.data=&model,.size=sizeof model};
 set.bindings.type_count=120;set.bindings.types[15].geometry=2;set.bindings.types[1].max_extent=64;
}
static XwRenderSnapshot* Snapshot(void){
 XwRenderSnapshot* s=calloc(1,sizeof *s);assert(s);s->flight_version=94;s->flight_assets=7;s->key.mission_generation=1;s->key.world_generation=1;s->simulation_ticks=100;s->appearance.direction_q15[2]=32767;s->camera.rows[0]=s->camera.rows[4]=s->camera.rows[8]=1;s->camera.player=(XwSnapObjectId){.kind=1,.slot=100,.generation=1};return s;
}
static void Ship(XwSnapObject* o,int x,int z){*o=(XwSnapObject){.id={.kind=1,.slot=1,.generation=1},.type=15,.genus=4,.world_pos={x,0,z},.orientation_dirty=1,.pitch=0x4000};}
static void Shot(XwSnapObject* o,unsigned type,int z){*o=(XwSnapObject){.id={.kind=1,.slot=2,.generation=1},.type=type,.genus=5,.world_pos={0,0,z},.previous_world_pos={0,0,z-10}};}
static XwCockpitEnvironment Fresh(XwRenderSnapshot* s){XwCockpitScene_Reset();XwCockpitEnvironment e;XwCockpitScene_Prepare(s,&e);return e;}
int main(void){
 Setup();float origin[3]={-1,-1,0},ray[3]={0,0,1};float concave[6][3]={{-2,-2,5},{2,-2,5},{2,0,5},{0,0,5},{0,2,5},{-2,2,5}};
 assert(CockpitScene_RayPolygon(origin,ray,(const float(*)[3])concave,6,10));float notch[3]={1,1,0};assert(!CockpitScene_RayPolygon(notch,ray,(const float(*)[3])concave,6,10));assert(!CockpitScene_RayPolygon(origin,ray,(const float(*)[3])concave,6,4));
 XwRenderSnapshot* s=Snapshot();s->object_count=1;Ship(&s->objects[0],0,250);XwCockpitEnvironment e=Fresh(s);assert(e.sun_visibility==0);
 Ship(&s->objects[0],25,250);e=Fresh(s);assert(e.sun_visibility>0&&e.sun_visibility<1);
 Ship(&s->objects[0],100,250);e=Fresh(s);assert(e.sun_visibility==1);
 Ship(&s->objects[0],0,-250);e=Fresh(s);assert(e.sun_visibility==1);
 Ship(&s->objects[0],0,250);s->objects[0].id=s->camera.player;e=Fresh(s);assert(e.sun_visibility==1);
 Ship(&s->objects[0],0,250);s->objects[0].genus=0;e=Fresh(s);assert(e.sun_visibility==1);s->objects[0].genus=9;e=Fresh(s);assert(e.sun_visibility==0);
 s->object_count=1;Shot(&s->objects[0],18,100);e=Fresh(s);assert(e.flash_count==1&&e.flashes[0].color[0]>e.flashes[0].color[1]);float first=e.flashes[0].strength;
 s->object_count=0;s->simulation_ticks+=12;XwCockpitScene_Prepare(s,&e);assert(e.flash_count==1&&e.flashes[0].strength<first);float decayed=e.flashes[0].strength;
 s->paused=1;XwCockpitScene_Prepare(s,&e);assert(e.flash_count==1&&e.flashes[0].strength==decayed);s->paused=0;s->simulation_ticks+=236;XwCockpitScene_Prepare(s,&e);assert(e.flash_count==0);
 s->object_count=1;Shot(&s->objects[0],20,100);e=Fresh(s);assert(e.flash_count==1&&e.flashes[0].color[1]>e.flashes[0].color[0]);Shot(&s->objects[0],22,100);e=Fresh(s);assert(e.flash_count==1&&e.flashes[0].color[2]>e.flashes[0].color[0]);
 Shot(&s->objects[0],18,1000);s->objects[0].previous_world_pos[2]=-1000;e=Fresh(s);assert(e.flash_count==1);Shot(&s->objects[0],18,10000);e=Fresh(s);assert(e.flash_count==0);
 Shot(&s->objects[0],18,-100);e=Fresh(s);assert(e.flash_count==0);
 s->objects[0]=(XwSnapObject){.id={.kind=1,.slot=5,.generation=1},.type=64,.genus=13,.world_pos={0,0,400},.animation_state=2};e=Fresh(s);assert(e.flash_count==1&&e.flashes[0].color[0]>e.flashes[0].color[2]);
 s->object_count=2;Ship(&s->objects[0],0,75);Shot(&s->objects[1],18,200);e=Fresh(s);assert(e.sun_visibility==0&&e.flash_count==0);Shot(&s->objects[1],18,30);e=Fresh(s);assert(e.sun_visibility==0&&e.flash_count==1);
 s->object_count=16;for(unsigned i=0;i<16;i++){Shot(&s->objects[i],18+(i%6),100);s->objects[i].id.slot=i+10;s->objects[i].world_pos[0]=i*3;s->objects[i].previous_world_pos[0]=i*3;}e=Fresh(s);assert(e.flash_count==4);
 s->object_count=0;s->key.world_generation++;XwCockpitScene_Prepare(s,&e);assert(e.flash_count==0);
 puts("PASS: concave blocker rays, finite source distance, full/partial sun occlusion, own/fighter/behind rejection, red/green/ion colors, swept fast shots, near range, explosions, source occlusion, source in shadow, 4-light cap, pause/fade/world reset");
 s->object_count=8;for(unsigned i=0;i<8;i++){Ship(&s->objects[i],(int)i*35,250+(int)i*30);s->objects[i].id.slot=i+1;}e=Fresh(s);clock_t start=clock();for(int i=0;i<1000;i++){s->simulation_ticks++;XwCockpitScene_Prepare(s,&e);}printf("Scene derivation: %.3f ms/frame (8 cube blockers, 5 solar rays; excludes tint grid/GPU)\n",1000.0*(clock()-start)/CLOCKS_PER_SEC/1000);free(s);return 0;
}

