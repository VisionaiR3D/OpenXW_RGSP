#include "xw_remaster/cockpit_scene_lighting.h"
#include "xw_remaster/cockpit_scene_math.h"
#include "xw_remaster/render_math.h"
#include "xw_runtime/snapshot/render_dos_assets.h"
#include <float.h>
#include <string.h>

enum { MODEL_CACHE_CAP=32, COMPONENT_CAP=64, BLOCKER_CAP=8 };
typedef struct ModelBounds {
 XwRenderAssetId id;
 unsigned count;
 uint16_t lod[COMPONENT_CAP];
 float minimum[COMPONENT_CAP][3],maximum[COMPONENT_CAP][3],radius;
} ModelBounds;
typedef struct Blocker {
 const XwSnapObject* object;
 const XwRenderDosModel* model;
 const ModelBounds* bounds;
 float pose[16],score;
} Blocker;
typedef struct RetainedFlash {
 XwSnapObjectId id;
 double position[3];
 float color[3],strength;
} RetainedFlash;
static ModelBounds model_cache[MODEL_CACHE_CAP];
static unsigned model_count;
static RetainedFlash retained[COCKPIT_FLASH_CAP];
static uint64_t mission,world,last_tick;
static bool scene_valid;
static float sun_visibility=1;

void XwCockpitScene_Reset(void) {
 memset(model_cache,0,sizeof model_cache);model_count=0;
 memset(retained,0,sizeof retained);mission=world=last_tick=0;scene_valid=false;sun_visibility=1;
}
static bool Same(XwSnapObjectId a,XwSnapObjectId b) {
 return a.kind&&a.kind==b.kind&&a.slot==b.slot&&a.generation==b.generation;
}
static const ModelBounds* Bounds(XwRenderAssetId id,const XwRenderDosModel* model) {
 for(unsigned i=0;i<model_count;i++)if(model_cache[i].id==id)return &model_cache[i];
 if(model_count==MODEL_CACHE_CAP||!model||model->component_count>COMPONENT_CAP)return NULL;
 ModelBounds* b=&model_cache[model_count];memset(b,0,sizeof *b);b->id=id;
 const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 for(unsigned i=0;i<model->component_count;i++) {
  XwSnapRange r=model->components[i];
  if(!r.count||r.first>=model->lod_count||r.count>model->lod_count-r.first)continue;
  unsigned lod=r.first+r.count-1;const XwRenderDosMesh* m=&model->lods[lod];
  if(!m->vertex_count||!m->faces)continue;
  unsigned c=b->count++;b->lod[c]=(uint16_t)lod;
  for(int a=0;a<3;a++){b->minimum[c][a]=FLT_MAX;b->maximum[c][a]=-FLT_MAX;}
  float matrix[16];XwRenderMath_DosMeshMatrix(identity,m->format,m->coordinate_shift,false,matrix);
  for(unsigned v=0;v<m->vertex_count;v++) {
   float p[3]={0};
   for(int a=0;a<3;a++) {
    b->minimum[c][a]=fminf(b->minimum[c][a],m->vertices[v][a]);
    b->maximum[c][a]=fmaxf(b->maximum[c][a],m->vertices[v][a]);
    for(int k=0;k<3;k++)p[a]+=matrix[4*a+k]*m->vertices[v][k];
   }
   b->radius=fmaxf(b->radius,sqrtf(CockpitScene_Dot(p,p)));
  }
 }
 ++model_count;return b;
}
static unsigned Blockers(const XwRenderSnapshot* s,const XwRenderAssetSet* set,Blocker out[BLOCKER_CAP]) {
 unsigned count=0;
 if(!set)return 0;
 for(unsigned i=0;i<s->object_count&&i<XW_SNAP_OBJECTS;i++) {
  const XwSnapObject* o=&s->objects[i];
  /* Large freighters/starships and solid asteroids. Ignore the player's own
   * ship, fighters, projectiles and all transparent billboard scenery. */
  if((o->genus!=3&&o->genus!=4&&o->genus!=9)||Same(o->id,s->camera.player)||o->type>=set->type_count)continue;
  const XwRenderSource* source=XwRenderAssets_Source(set->types[o->type].geometry);
  if(!source||source->kind!=XW_SOURCE_DOS_MODEL||source->size!=sizeof(XwRenderDosModel))continue;
  const XwRenderDosModel* model=source->data;
  const ModelBounds* bounds=Bounds(source->id,model);
  if(!bounds||!bounds->count||bounds->radius<=0)continue;
  double distance2=0;for(int k=0;k<3;k++){double v=(double)o->world_pos[k]-s->camera.world_pos[k];distance2+=v*v;}
  float score=bounds->radius*bounds->radius/(float)fmax(1,distance2);
  unsigned slot=count;
  if(count==BLOCKER_CAP){slot=0;for(unsigned j=1;j<count;j++)if(out[j].score<out[slot].score)slot=j;if(score<=out[slot].score)continue;}
  else ++count;
  out[slot]=(Blocker){.object=o,.model=model,.bounds=bounds,.score=score};
  XwRenderMath_ObjectMatrix(o,s->flight_version,s->camera.world_pos,out[slot].pose);
 }
 return count;
}
static bool Blocked(const Blocker* blockers,unsigned count,const float direction[3],float limit) {
 for(unsigned i=0;i<count;i++) {
  const Blocker* b=&blockers[i];
  float center[3]={b->pose[3],b->pose[7],b->pose[11]},radius=b->bounds->radius;
  float along=CockpitScene_Dot(center,direction);
  if(along+radius<0||along-radius>limit||CockpitScene_Dot(center,center)-along*along>radius*radius)continue;
  for(unsigned c=0;c<b->bounds->count;c++) {
   const XwRenderDosMesh* mesh=&b->model->lods[b->bounds->lod[c]];
   float matrix[16],origin[3]={0},ray[3]={0};
   XwRenderMath_DosMeshMatrix(b->pose,mesh->format,mesh->coordinate_shift,false,matrix);
   /* The native mesh matrix has orthogonal, uniformly scaled axes. Transform
    * the eye and ray into raw model coordinates; distance remains world units. */
   bool valid=true;
   for(int a=0;a<3;a++) {
    float scale2=0;for(int r=0;r<3;r++)scale2+=matrix[r*4+a]*matrix[r*4+a];
    if(scale2<1e-12f){valid=false;break;}
    for(int r=0;r<3;r++){origin[a]-=matrix[r*4+a]*matrix[r*4+3]/scale2;ray[a]+=matrix[r*4+a]*direction[r]/scale2;}
   }
   if(!valid||!CockpitScene_RayBox(origin,ray,b->bounds->minimum[c],b->bounds->maximum[c],limit))continue;
   for(unsigned f=0;f<mesh->face_count;f++) {
    const XwRenderDosFace* face=&mesh->faces[f];
    if(face->vertex_count<3||face->vertex_count>127||!(face->color&63))continue;
    float polygon[127][3];bool good=true;
    for(unsigned v=0;v<face->vertex_count;v++) {
     unsigned index=face->indices[v];if(index>=mesh->vertex_count){good=false;break;}
     for(int a=0;a<3;a++)polygon[v][a]=mesh->vertices[index][a];
    }
    if(good&&CockpitScene_RayPolygon(origin,ray,(const float(*)[3])polygon,face->vertex_count,limit))return true;
   }
  }
 }
 return false;
}
static float Sun(const XwRenderSnapshot* s,const Blocker* blockers,unsigned count) {
 float direction[3];for(int a=0;a<3;a++)direction[a]=s->appearance.direction_q15[a]/32768.0f;
 if(!count||!CockpitScene_Normalize(direction))return 1;
 float axis[3]={0,0,1};if(fabsf(direction[2])>.9f){axis[1]=1;axis[2]=0;}
 float right[3]={direction[1]*axis[2]-direction[2]*axis[1],direction[2]*axis[0]-direction[0]*axis[2],direction[0]*axis[1]-direction[1]*axis[0]};
 CockpitScene_Normalize(right);
 float up[3]={direction[1]*right[2]-direction[2]*right[1],direction[2]*right[0]-direction[0]*right[2],direction[0]*right[1]-direction[1]*right[0]};
 unsigned hits=0;
 for(int sample=0;sample<5;sample++) {
  float ray[3];for(int a=0;a<3;a++)ray[a]=direction[a]+.007f*(sample==1?right[a]:sample==2?-right[a]:sample==3?up[a]:sample==4?-up[a]:0);
  CockpitScene_Normalize(ray);hits+=Blocked(blockers,count,ray,1e9f);
 }
 return (5u-hits)/5.0f;
}
static void KeepFlash(const XwSnapObject* o,const double position[3],const float color[3],float strength) {
 unsigned slot=COCKPIT_FLASH_CAP;
 for(unsigned i=0;i<COCKPIT_FLASH_CAP;i++)if(Same(retained[i].id,o->id)){slot=i;break;}
 if(slot==COCKPIT_FLASH_CAP){slot=0;for(unsigned i=1;i<COCKPIT_FLASH_CAP;i++)if(retained[i].strength<retained[slot].strength)slot=i;if(strength<=retained[slot].strength)return;retained[slot].strength=0;}
 retained[slot].id=o->id;memcpy(retained[slot].position,position,sizeof retained[slot].position);memcpy(retained[slot].color,color,sizeof retained[slot].color);
 retained[slot].strength=fmaxf(retained[slot].strength,strength);
}
static void CaptureFlashes(const XwRenderSnapshot* s,float scale) {
 for(unsigned i=0;i<s->object_count&&i<XW_SNAP_OBJECTS;i++) {
  const XwSnapObject* o=&s->objects[i];
  bool shot=o->genus==5||o->genus==6,explosion=o->genus==13;
  if(!shot&&!explosion)continue;
  float color[3]={1,.24f,.035f},power=1,range=scale*32;
  if(shot) {
   if(o->type<18||o->type>25)continue;
   range=scale*7;power=.62f;
   if(o->type<=19){color[0]=1;color[1]=.025f;color[2]=.008f;}
   else if(o->type<=21){color[0]=.015f;color[1]=1;color[2]=.045f;}
   else if(o->type<=23){color[0]=.08f;color[1]=.17f;color[2]=1;}
   else {color[0]=.38f;color[1]=.09f;color[2]=1;power=.24f;range=scale*10;}
  } else {
   if(o->type<64||o->type>69)continue;
   static const float envelope[]={.15f,.40f,1,.95f,.8f,.65f,.5f,.36f,.24f,.15f,.08f,.03f};
   power=o->animation_state<12?envelope[o->animation_state]:.015f;
   range*=1+fminf(o->billboard_scale,8)*.15f;
  }
  double position[3],distance2=0,t=1,delta[3],start[3];
  if(shot) {
   double length2=0,numerator=0;
   for(int a=0;a<3;a++){start[a]=(double)o->previous_world_pos[a]-s->camera.world_pos[a];delta[a]=(double)o->world_pos[a]-o->previous_world_pos[a];length2+=delta[a]*delta[a];numerator-=start[a]*delta[a];}
   if(length2>1e-6)t=fmin(1,fmax(0,numerator/length2));
  }
  for(int a=0;a<3;a++){position[a]=shot?o->previous_world_pos[a]+t*delta[a]:o->world_pos[a];double d=position[a]-s->camera.world_pos[a];distance2+=d*d;}
  if(distance2>=range*range)continue;
  float fade=1-(float)distance2/(range*range);float strength=power*fade*fade;
  if(strength<.008f)continue;
  /* A swept laser may cross the exact eye between snapshots. Retain its
   * closest front-facing endpoint rather than an undefined zero direction. */
  if(distance2<1)for(int a=0;a<3;a++)position[a]=o->world_pos[a];
  KeepFlash(o,position,color,strength);
 }
}
void XwCockpitScene_Prepare(const XwRenderSnapshot* s,XwCockpitEnvironment* out) {
 memset(out,0,sizeof *out);out->sun_visibility=1;
 if(!s||s->flight_version!=94)return;
 bool reset=!scene_valid||mission!=s->key.mission_generation||world!=s->key.world_generation||s->simulation_ticks<last_tick;
 if(reset){XwCockpitScene_Reset();mission=s->key.mission_generation;world=s->key.world_generation;}
 bool advance=reset||last_tick!=s->simulation_ticks;
 float dt=reset?0:(float)(s->simulation_ticks-last_tick)/236.0f;
 if(dt>.5f){memset(retained,0,sizeof retained);reset=true;dt=0;}
 const XwRenderAssetSetView* view=XwRenderAssets_Set(s->flight_assets);
 const XwRenderAssetSet* set=view?&view->bindings:NULL;
 Blocker blockers[BLOCKER_CAP];unsigned count=Blockers(s,set,blockers);
 float target=Sun(s,blockers,count);
 if(reset)sun_visibility=target;
 else if(dt>0)sun_visibility+=(target-sun_visibility)*(1-expf(-dt/.12f));
 if(fabsf(sun_visibility-target)<.002f)sun_visibility=target;
 out->sun_visibility=sun_visibility;
 if(advance&&!s->paused) {
  float decay=expf(-dt/.10f);
  for(unsigned i=0;i<COCKPIT_FLASH_CAP;i++){retained[i].strength*=decay;if(retained[i].strength<.005f)memset(&retained[i],0,sizeof retained[i]);}
  float scale=64;
  if(set)for(unsigned i=0;i<s->object_count&&i<XW_SNAP_OBJECTS;i++)if(Same(s->objects[i].id,s->camera.player)&&s->objects[i].type<set->type_count){scale=fmaxf(32,set->types[s->objects[i].type].max_extent);break;}
  CaptureFlashes(s,scale);
 }
 for(unsigned i=0;i<COCKPIT_FLASH_CAP;i++) {
  RetainedFlash* f=&retained[i];if(f->strength<.005f)continue;
  float ray[3],distance=0;
  for(int a=0;a<3;a++){ray[a]=(float)(f->position[a]-s->camera.world_pos[a]);distance+=ray[a]*ray[a];}
  distance=sqrtf(distance);if(distance<1||!CockpitScene_Normalize(ray))continue;
  XwCockpitFlash* light=&out->flashes[out->flash_count];
  for(int row=0;row<3;row++)for(int a=0;a<3;a++)light->direction[row]+=s->camera.rows[row*3+a]*ray[a];
  light->direction[1]=-light->direction[1];
  if(light->direction[2]<-.2f||Blocked(blockers,count,ray,fmaxf(0,distance-2))){memset(light,0,sizeof *light);continue;}
  memcpy(light->color,f->color,sizeof light->color);light->strength=f->strength;++out->flash_count;
 }
 last_tick=s->simulation_ticks;scene_valid=true;
}
