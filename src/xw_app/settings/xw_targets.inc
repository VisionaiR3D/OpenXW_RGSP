
#include "target_list.h"
#include "xw/flight/player/user.h"
#include "xw/flight/object/craft.h"
#include "xw/flight/object/create.h"
#include "xw/flight/object/laser.h"
#include "xw/flight/mission/mission.h"
#include "xw/flight/xw.h"
#include "xw_runtime/runtime/flight_types.h"
#include "xw_runtime/runtime/profile.h"
#include "xw_runtime/snapshot/render_objects.h"
static uint16_t TargetCurrent(void){return g_playerFlightState.currentTargetObjectIdx;}
static int TargetCapture(TargetEntry*out,int capacity){
 if(!g_playerFlightState.object||!g_playerFlightState.craft)return 0;
 const XwFlightProfile*p=XwProfile_ActiveFlight();
 if(!p)return 0;
 ObjectRecord*player=g_playerFlightState.object;
 CraftData*saved=g_curCraft;
 uint16_t ref=g_playerFlightState.objectIndex,first=UINT16_MAX;
 int n=0;
 for(int visits=0;visits<TARGET_CAP&&n<capacity;visits++){
  ref=user_picknexttarget(ref,1);
  if(ref==UINT16_MAX||ref==first)break;
  if(first==UINT16_MAX)first=ref;
  TargetEntry e={.ref=ref,.hull=-1,.shields=-1,.relation=2};
  XwSnapObjectId id=XwRenderObjects_Id(ref);
  if(!id.kind)continue;
  e.token=((uint64_t)id.kind<<56)|((uint64_t)id.generation<<16)|ref;
  int32_t x,y,z;
  if(ref<XW_OBJECT_COUNT){
   ObjectRecord*o=&g_objectTable[ref];x=o->worldX;y=o->worldY;z=o->worldZ;
   e.relation=o->iff==player->iff?1:o->iff==(player->iff^1)?0:2;
   if(ref<XW_CRAFT_OBJECT_COUNT&&o->familyId==XW_OBJECT_FAMILY_CRAFT&&o->instanceData){
    CraftData*c=o->instanceData;
    if(c->craftTypeIndex>=p->craft_definition_count)continue;
    snprintf(e.type,sizeof e.type,"%.35s",g_craftTypeDefs[c->craftTypeIndex].name);
    bool identified=user_CanRevealCraftIdentity(ref,c)!=0;
    if(identified&&c->flightGroupIndex<MISSION_FLIGHT_GROUP_COUNT)
     snprintf(e.name,sizeof e.name,"%.16s %u",g_missionFlightGroups[c->flightGroupIndex].name,c->craftIndexInFlightGroup+1);
    else snprintf(e.name,sizeof e.name,"Unknown #%u",ref);
    if(!XwFlightTypes_Dos()){e.systems_known=0xec; e.systems_damaged=e.systems_known&~c->workingSubsystems;}
    e.flags=TARGET_INSPECTABLE|(c->isInspected?TARGET_INSPECTED:0);
    if(c->hullDamage)e.flags|=TARGET_DAMAGED;
    if(e.relation==0&&c->aiTargetRef==g_playerFlightState.objectIndex&&(XwFlightTypes_Dos()||c->workingSubsystems))e.flags|=TARGET_ATTACKING;
    if(c->flightGroupIndex<MISSION_FLIGHT_GROUP_COUNT&&g_missionFlightGroups[c->flightGroupIndex].missionGoal)e.flags|=TARGET_OBJECTIVE;
    e.hull=TargetPercent((int)c->hullMax-c->hullDamage,c->hullMax);
    int max=g_craftTypeDefs[c->craftTypeIndex].nominalShieldEnergy[0]+g_craftTypeDefs[c->craftTypeIndex].nominalShieldEnergy[1];
    e.shields=TargetPercent((int)c->shieldEnergy[0]+c->shieldEnergy[1],max);
   }else if(XwFlightTypes_IsWarhead(o->objectType)){
    const WarheadGuidanceState*w=o->instanceData;
    snprintf(e.type,sizeof e.type,"%s",XwFlightTypes_ProjectileIndex(o->objectType)==6?"Proton torpedo":"Missile");
    snprintf(e.name,sizeof e.name,"%s #%u",e.type,ref);
    if(w&&w->targetObjIdx==g_playerFlightState.objectIndex)e.flags|=TARGET_INCOMING;
   }else{
    snprintf(e.type,sizeof e.type,"Object %u",o->objectType);snprintf(e.name,sizeof e.name,"Object #%u",ref);
   }
  }else{
   unsigned slot=ref-XW_MISSION_OBJECT_REF_BASE;if(slot>=MISSION_OBJECT_COUNT)continue;
   XwMissionObjectRecord*o=&g_missionObjects[slot];
   x=(int32_t)o->worldX*256;y=(int32_t)o->worldY*256;z=(int32_t)o->worldZ*256;
   snprintf(e.type,sizeof e.type,"%s",XwFlightTypes_IsMine(o->objectType)?"Mijn":"Ruimteobject");
   snprintf(e.name,sizeof e.name,"%s #%u",e.type,slot+1);
   if(o->goalFlags)e.flags|=TARGET_OBJECTIVE;
  }
  e.distance_km=TargetDistance(player->worldX,player->worldY,player->worldZ,x,y,z);
  out[n++]=e;
 }
 g_curCraft=saved;return n;
}
static bool TargetSelect(const TargetEntry*entry){
 static TargetEntry live[TARGET_CAP];int n=TargetCapture(live,TARGET_CAP);
 for(int i=0;i<n;i++)if(live[i].ref==entry->ref&&live[i].token==entry->token){
  user_setnewtarget(entry->ref);return TargetCurrent()==entry->ref;
 }
 return false;
}
