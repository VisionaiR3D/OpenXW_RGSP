#define _POSIX_C_SOURCE 200809L
#include "xw_runtime/input/combos.h"
#include "xw_runtime/input/input_bridge.h"
#include "xw_runtime/input/pause_queue.h"
#include "aeron/aeron.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "steering_assist.h"
#include "xw/flight/player/user.h"
#include "xw/flight/object/object.h"
#include "xw/flight/object/create.h"
static PauseQueue commands;
static bool commands_ready;
static int command_pulses[PAUSE_COMMAND_CAP],command_pulse_count;
bool XwCombos_Queue(int action) { return PauseQueue_Push(&commands,action); }
int XwCombos_QueueCount(void) { return commands.count; }
int XwCombos_QueueLast(void) { return commands.count ? commands.actions[commands.count-1] : 0; }
void XwCombos_Undo(void) { PauseQueue_Undo(&commands); }
void XwCombos_Clear(void) { PauseQueue_Clear(&commands); commands_ready=false; }
void XwCombos_ResumeQueued(void) { commands_ready=true; }
static void ApplyQueued(bool active) {
 if(!active) return;
 for(int i=0;i<command_pulse_count;i++) XwControllerMapping_InjectAction(command_pulses[i],false);
 command_pulse_count=0;
 if(!commands_ready) return;
 commands_ready=false;
 command_pulse_count=PauseQueue_Drain(&commands,command_pulses);
 for(int i=0;i<command_pulse_count;i++) {
  XwControllerMapping_InjectAction(command_pulses[i],true);
  Aeron_LogInfo("xw.quick","execute: %s",XwInputActions_ToName(command_pulses[i]));
 }
}


static XwComboGroup groups[6];
static void Defaults(XwComboGroup* g) {
 g[0]=(XwComboGroup){4,{XW_INPUT_ACTION_MATCH_SPEED,{XW_INPUT_ACTION_THROTTLE_FULL,XW_INPUT_ACTION_THROTTLE_ZERO,XW_INPUT_ACTION_THROTTLE_ONE_THIRD,XW_INPUT_ACTION_THROTTLE_TWO_THIRDS}}};
 g[1]=(XwComboGroup){0,{XW_INPUT_ACTION_ENERGY_NORMALIZE,{XW_INPUT_ACTION_CANNON_RECHARGE_INCREASE,XW_INPUT_ACTION_CANNON_RECHARGE_DECREASE,XW_INPUT_ACTION_CYCLE_SHIELD_RECHARGE_RATE,XW_INPUT_ACTION_NONE}}};
 g[2]=(XwComboGroup){1,{XW_INPUT_ACTION_AUTO_TARGET,{XW_INPUT_ACTION_TARGET_PREV,XW_INPUT_ACTION_TARGET_NEXT,XW_INPUT_ACTION_TARGET_NEAREST_FIGHTER,XW_INPUT_ACTION_TARGET_MY_ATTACKER}}};
 g[3]=(XwComboGroup){5,{XW_INPUT_ACTION_CYCLE_WEAPON_GROUP,{XW_INPUT_ACTION_CYCLE_WEAPON_GROUP,XW_INPUT_ACTION_CYCLE_WEAPON_FIRING_MODE,XW_INPUT_ACTION_WEAPON_GROUP_PREVIOUS,XW_INPUT_ACTION_CYCLE_WEAPON_GROUP}}};
 g[4]=(XwComboGroup){6,{XW_INPUT_ACTION_CYCLE_SHIELD_MODE,{XW_INPUT_ACTION_SHIELDS_FORWARD,XW_INPUT_ACTION_SHIELDS_REAR,XW_INPUT_ACTION_SHIELDS_BALANCED,XW_INPUT_ACTION_XFER_SHIELDS_TO_CANNON}}};
 g[5]=(XwComboGroup){2,{XW_INPUT_ACTION_INFO_MAP,{XW_INPUT_ACTION_NONE,XW_INPUT_ACTION_NONE,XW_INPUT_ACTION_NONE,XW_INPUT_ACTION_HYPERSPACE}}};
}

static FlightBinding bindings[BINDING_CAP];
static int count;
static BindingState states[AERON_CONTROLLER_MAX];
static uint32_t ids[AERON_CONTROLLER_MAX];
static bool initialized,suspended;
static bool lead_enabled,target_list_requested;
static char lead_path[1024];
static char path[1024],error[256];
static bool assist_enabled=true,assist_active;
static char assist_path[1024];
static SteeringAssist assist_state[AERON_CONTROLLER_MAX];
static double assist_last_time;

static const char* keys[]={"A","B","X","Y","L1","R1","L2","R2","Omhoog","Omlaag","Links","Rechts"};
static const int buttons[]={AERON_GAMEPAD_BUTTON_SOUTH,AERON_GAMEPAD_BUTTON_EAST,AERON_GAMEPAD_BUTTON_WEST,AERON_GAMEPAD_BUTTON_NORTH,AERON_GAMEPAD_BUTTON_LEFT_SHOULDER,AERON_GAMEPAD_BUTTON_RIGHT_SHOULDER,-1,-1,AERON_GAMEPAD_BUTTON_DPAD_UP,AERON_GAMEPAD_BUTTON_DPAD_DOWN,AERON_GAMEPAD_BUTTON_DPAD_LEFT,AERON_GAMEPAD_BUTTON_DPAD_RIGHT};
static void Dispatch(BindingOutput o) { for(int i=0;i<o.count;i++) ((o.events[i].action==XW_INPUT_ACTION_TARGET_LIST_OPEN)?(void)(target_list_requested|=o.events[i].down):XwControllerMapping_InjectAction((XwInputAction)o.events[i].action,o.events[i].down)); }
static void Reset(bool cycles) { for(int i=0;i<AERON_CONTROLLER_MAX;i++) { Dispatch(BindingStep(&states[i],bindings,count,false,0)); if(cycles) memset(&states[i],0,sizeof states[i]); } }
static const char* ActionName(int a) { return a ? XwInputActions_ToName((XwInputAction)a) : "none"; }
static bool Save(void) {
 char tmp[1100]; snprintf(tmp,sizeof tmp,"%s.tmp",path);
 FILE* f=path[0] ? fopen(tmp,"w") : NULL;
 if(!f) { snprintf(error,sizeof error,"Opslaan mislukt: %s",strerror(errno)); return false; }
 bool ok=fprintf(f,"OpenXW-bindings 3\n")>0;
 for(int i=0;i<count;i++) if(fprintf(f,"%u %s\n",bindings[i].mask,ActionName(bindings[i].action))<0) ok=false;
 if(fflush(f)!=0) ok=false;
 if(ok && fsync(fileno(f))!=0) ok=false;
 if(fclose(f)!=0) ok=false;
 if(!ok || rename(tmp,path)!=0) { snprintf(error,sizeof error,"Opslaan mislukt: %s",strerror(errno)); remove(tmp); return false; }
 error[0]=0; return true;
}
static void FromGroups(void) {
 const int map[]={2,3,6,7,1,4,5,-1};
 count=0; bindings[count++]=(FlightBinding){1,XW_INPUT_ACTION_FIRE_WEAPON};
 for(int g=0;g<6;g++) if(groups[g].button>=0 && groups[g].button<7) {
  unsigned m=1u<<map[groups[g].button];
  if(groups[g].binding.tap) bindings[count++]=(FlightBinding){m,groups[g].binding.tap};
  for(int d=0;d<4;d++) if(groups[g].binding.direction[d]) bindings[count++]=(FlightBinding){m|(256u<<d),groups[g].binding.direction[d]};
 }
}
static void GroupedDefaults(void) {
 count=0;
 bindings[count++]=(FlightBinding){384,XW_INPUT_ACTION_TARGET_LIST_OPEN};
 bindings[count++]=(FlightBinding){1,XW_INPUT_ACTION_FIRE_WEAPON};
 bindings[count++]=(FlightBinding){2,XW_INPUT_ACTION_MATCH_SPEED};
 bindings[count++]=(FlightBinding){258,XW_INPUT_ACTION_THROTTLE_FULL};
 bindings[count++]=(FlightBinding){514,XW_INPUT_ACTION_THROTTLE_ZERO};
 bindings[count++]=(FlightBinding){1026,XW_INPUT_ACTION_THROTTLE_ONE_THIRD};
 bindings[count++]=(FlightBinding){2050,XW_INPUT_ACTION_THROTTLE_TWO_THIRDS};
 bindings[count++]=(FlightBinding){4,XW_INPUT_ACTION_ENERGY_NORMALIZE};
 bindings[count++]=(FlightBinding){1028,XW_INPUT_ACTION_CYCLE_CANNON_RECHARGE_RATE};
 bindings[count++]=(FlightBinding){2052,XW_INPUT_ACTION_CYCLE_SHIELD_RECHARGE_RATE};
 bindings[count++]=(FlightBinding){260,XW_INPUT_ACTION_ENGINE_MORE};
 bindings[count++]=(FlightBinding){516,XW_INPUT_ACTION_ENGINE_LESS};
 bindings[count++]=(FlightBinding){32,XW_INPUT_ACTION_CYCLE_SHIELD_MODE};
 bindings[count++]=(FlightBinding){1056,XW_INPUT_ACTION_XFER_SHIELDS_TO_CANNON};
 bindings[count++]=(FlightBinding){2080,XW_INPUT_ACTION_XFER_CANNON_TO_SHIELDS};
 bindings[count++]=(FlightBinding){16,XW_INPUT_ACTION_CYCLE_WEAPON_GROUP};
 bindings[count++]=(FlightBinding){1040,XW_INPUT_ACTION_CYCLE_WEAPON_FIRING_MODE};
 bindings[count++]=(FlightBinding){8,XW_INPUT_ACTION_AUTO_TARGET};
 bindings[count++]=(FlightBinding){264,XW_INPUT_ACTION_TARGET_PREV};
 bindings[count++]=(FlightBinding){520,XW_INPUT_ACTION_TARGET_NEXT};
 bindings[count++]=(FlightBinding){1032,XW_INPUT_ACTION_TARGET_NEAREST_FIGHTER};
 bindings[count++]=(FlightBinding){2056,XW_INPUT_ACTION_TARGET_MY_ATTACKER};
 bindings[count++]=(FlightBinding){64,XW_INPUT_ACTION_COMM_REPORT_ORDERS};
 bindings[count++]=(FlightBinding){320,XW_INPUT_ACTION_COMM_ASSIGN_TARGET};
 bindings[count++]=(FlightBinding){576,XW_INPUT_ACTION_COMM_COVER_ME};
 bindings[count++]=(FlightBinding){128,XW_INPUT_ACTION_TARGET_ROLL_MODIFIER};
 bindings[count++]=(FlightBinding){192,XW_INPUT_ACTION_TOGGLE_S_FOILS};
 bindings[count++]=(FlightBinding){240,XW_INPUT_ACTION_HYPERSPACE};
}
static void Init(void) {
 if(initialized) return; initialized=true; Defaults(groups); GroupedDefaults();
 char* pref=SDL_GetPrefPath("TotallyOpen","OpenXW-HD");
 if(!pref) { snprintf(error,sizeof error,"Instellingenmap niet gevonden"); return; }
 snprintf(path,sizeof path,"%sbindings-v3.cfg",pref);
 FILE* f=fopen(path,"r");
 if(f) {
  FlightBinding candidate[BINDING_CAP]; int n=0; char line[256],name[100],extra; unsigned mask; bool ok=true;
  if(!fgets(line,sizeof line,f) || strcmp(line,"OpenXW-bindings 3\n")) ok=false;
  while(ok && fgets(line,sizeof line,f)) {
   if(n>=BINDING_CAP || sscanf(line,"%u %99s %c",&mask,name,&extra)!=2 || mask>4095u || ((mask&768u)==768u) || ((mask&3072u)==3072u)) { ok=false; break; }
   int a=XwInputActions_FromName(name);
   if(!a && strcmp(name,"none")) { ok=false; break; }
   candidate[n++]=(FlightBinding){mask,a};
  }
  if(ferror(f)) ok=false; fclose(f);
  if(ok) { memcpy(bindings,candidate,n*sizeof *bindings); count=n; }
  else snprintf(error,sizeof error,"Ongeldig bindings-v3.cfg; standaard actief, bestand behouden");
 } else {
  char legacy[1024]; snprintf(legacy,sizeof legacy,"%scombos-v2.cfg",pref); f=fopen(legacy,"r");
  if(f) {
   XwComboGroup candidate[6]; Defaults(candidate); char line[512],names[5][80]; int index,button,version; unsigned seen=0; bool ok=true;
   if(!fgets(line,sizeof line,f) || sscanf(line,"OpenXW-combos %d",&version)!=1 || version!=2) ok=false;
   while(ok && fgets(line,sizeof line,f)) {
    if(sscanf(line,"%d %d %79s %79s %79s %79s %79s",&index,&button,names[0],names[1],names[2],names[3],names[4])!=7 || index<0 || index>=6 || button<0 || button>7 || (seen&(1u<<index))) { ok=false; break; }
    seen|=1u<<index; candidate[index].button=button;
    for(int n=0;n<5;n++) { int a=XwInputActions_FromName(names[n]); if(!a && strcmp(names[n],"none")) { ok=false; break; } if(n) candidate[index].binding.direction[n-1]=a; else candidate[index].binding.tap=a; }
   }
   if(ferror(f)) ok=false; fclose(f);
   if(ok && seen==63) { memcpy(groups,candidate,sizeof groups); FromGroups(); }
   else snprintf(error,sizeof error,"Oude combinaties ongeldig; standaard actief");
  }
 }
 snprintf(assist_path,sizeof assist_path,"%ssteering-assist.cfg",pref);
 FILE* af=fopen(assist_path,"r");if(af){int enabled;if(fscanf(af,"%d",&enabled)==1 && (enabled==0 || enabled==1))assist_enabled=enabled!=0;fclose(af);}
 snprintf(lead_path,sizeof lead_path,"%slead-indicator.cfg",pref);
 FILE* lf=fopen(lead_path,"r");if(lf){int enabled;if(fscanf(lf,"%d",&enabled)==1&&(enabled==0||enabled==1))lead_enabled=enabled!=0;fclose(lf);}
 SDL_free(pref);
}
int XwCombos_Count(void) { Init(); return count; }
const FlightBinding* XwCombos_Bindings(void) { Init(); return bindings; }
const char* XwCombos_Error(void) { Init(); return error; }
const char* XwCombos_KeyName(int i) { return i>=0 && i<12 ? keys[i] : ""; }
void XwCombos_FormatMask(unsigned mask,char* out,int size) {
 if(size<=0) return; out[0]=0;
 for(int i=0;i<12;i++) if(mask&(1u<<i)) { int len=strlen(out); snprintf(out+len,size-len,"%s%s",len?" + ":"",keys[i]); }
 if(!mask) snprintf(out,size,"Niet toegewezen");
}
bool XwCombos_SetBinding(int i,unsigned mask,int action) {
 Init(); if(i<0 || i>=count || mask>4095u || ((mask&768u)==768u) || ((mask&3072u)==3072u) || action<0 || action>=XW_INPUT_ACTION_COUNT) return false;
 FlightBinding old=bindings[i]; Reset(true); bindings[i]=(FlightBinding){mask,action}; if(Save()) return true; bindings[i]=old; return false;
}
bool XwCombos_SetActionBinding(int action,unsigned mask) {
 Init();
 if(action<=0 || action>=XW_INPUT_ACTION_COUNT || mask>4095u || ((mask&768u)==768u) || ((mask&3072u)==3072u)) return false;
 FlightBinding old[BINDING_CAP],next[BINDING_CAP]; int old_count=count,n=0; bool placed=false;
 memcpy(old,bindings,sizeof old);
 for(int i=0;i<count;i++) {
  if(bindings[i].action==action) {
   if(mask && !placed) { next[n++]=(FlightBinding){mask,action}; placed=true; }
  } else next[n++]=bindings[i];
 }
 if(mask && !placed) {
  if(n>=BINDING_CAP) { snprintf(error,sizeof error,"Maximum 256 toewijzingen"); return false; }
  next[n++]=(FlightBinding){mask,action};
 }
 Reset(true); memcpy(bindings,next,n*sizeof *bindings); count=n;
 if(Save()) return true;
 memcpy(bindings,old,sizeof old); count=old_count; return false;
}
bool XwCombos_AddBinding(void) { Init(); if(count>=BINDING_CAP) { snprintf(error,sizeof error,"Maximum 256 toewijzingen"); return false; } Reset(true); bindings[count++]=(FlightBinding){0,0}; if(Save()) return true; count--; return false; }
bool XwCombos_RemoveBinding(int i) { Init(); if(i<0 || i>=count) return false; FlightBinding old[BINDING_CAP]; memcpy(old,bindings,sizeof old); Reset(true); memmove(bindings+i,bindings+i+1,(count-i-1)*sizeof *bindings); count--; if(Save()) return true; count++; memcpy(bindings,old,sizeof old); return false; }
bool XwCombos_MoveBinding(int i,int delta) { Init(); int j=i+delta; if(i<0 || i>=count || j<0 || j>=count) return false; Reset(true); FlightBinding old=bindings[i]; bindings[i]=bindings[j]; bindings[j]=old; if(Save()) return true; bindings[j]=bindings[i]; bindings[i]=old; return false; }
bool XwCombos_RestoreDefaults(void) { Init(); FlightBinding old[BINDING_CAP]; int n=count; memcpy(old,bindings,sizeof old); Reset(true); Defaults(groups); GroupedDefaults(); if(Save()) return true; memcpy(bindings,old,sizeof old); count=n; return false; }
void XwCombos_Suspend(bool value) { Init(); suspended=value; if(value) Reset(false); }

bool XwCombos_AssistEnabled(void){Init();return assist_enabled;}
bool XwCombos_AssistActive(void){return assist_active;}
bool XwCombos_SetAssist(bool enabled){
 Init();if(!assist_path[0])return false;
 char tmp[1100];snprintf(tmp,sizeof tmp,"%s.tmp",assist_path);
 FILE* f=fopen(tmp,"w");if(!f)return false;
 bool ok=fprintf(f,"%d\n",enabled?1:0)>0;
 if(fflush(f)!=0)ok=false;if(ok && fsync(fileno(f))!=0)ok=false;if(fclose(f)!=0)ok=false;
 if(!ok || rename(tmp,assist_path)!=0){remove(tmp);return false;}
 assist_enabled=enabled;assist_active=false;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++)AssistReset(&assist_state[i]);
 return true;
}
static float AssistFrameTime(void){
 struct timespec now;if(clock_gettime(CLOCK_MONOTONIC,&now)!=0)return 1.0f/60;
 double t=now.tv_sec+now.tv_nsec/1000000000.0,dt=assist_last_time?t-assist_last_time:1.0/60;
 assist_last_time=t;return (float)dt;
}

static float AssistNearTarget(void){
 if(!g_playerFlightState.object || g_playerFlightState.currentTargetObjectIdx>=XW_CRAFT_OBJECT_COUNT ||
    g_playerFlightState.currentTargetObjectIdx==g_playerFlightState.objectIndex)return 1;
 ObjectRecord* player=g_playerFlightState.object;ObjectRecord* target=&g_objectTable[g_playerFlightState.currentTargetObjectIdx];
 return AssistTargetGain((double)target->worldX-player->worldX,(double)target->worldY-player->worldY,
  (double)target->worldZ-player->worldZ,player->cachedForwardX,player->cachedForwardY,player->cachedForwardZ);
}

const AeronInputSnapshot* XwCombos_Filter(const AeronInputSnapshot* input) {
 Init(); assist_active=false; if(!input) { Reset(false); for(int i=0;i<AERON_CONTROLLER_MAX;i++)AssistReset(&assist_state[i]); return input; }
 float assist_dt=AssistFrameTime();
 bool active=!suspended && input->has_focus && XwInput_GameplayActive();
 ApplyQueued(active);
 static AeronInputSnapshot filtered; filtered=*input;
 for(int slot=0;slot<AERON_CONTROLLER_MAX;slot++) {
  AeronControllerSnapshot* d=&filtered.controllers[slot];
  if(ids[slot]!=d->instance_id) { Dispatch(BindingStep(&states[slot],bindings,count,false,0)); memset(&states[slot],0,sizeof states[slot]); ids[slot]=d->instance_id; }
  bool handheld=d->connected && d->kind==AERON_CONTROLLER_KIND_GAMEPAD && !strcmp(d->name,"H700 Gamepad") && d->axis_count==0;
  unsigned raw=0;
  if(handheld) for(int b=0;b<12;b++) {
   bool down=b==6 ? d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFT_TRIGGER]>16383 : b==7 ? d->gamepad_axes[AERON_GAMEPAD_AXIS_RIGHT_TRIGGER]>16383 : (d->gamepad_buttons&(1u<<buttons[b]))!=0;
   if(down) raw|=1u<<b;
  }
  BindingOutput out=BindingStepFlight(&states[slot],bindings,count,active&&handheld,raw,XW_INPUT_ACTION_TARGET_ROLL_MODIFIER,XW_INPUT_ACTION_ROLL_LEFT,XW_INPUT_ACTION_ROLL_RIGHT); Dispatch(out);
  if(!active || !handheld) {AssistReset(&assist_state[slot]);continue;}
  for(int b=0;b<12;b++) if(out.consume&(1u<<b)) {
   if(b==6 || b==7) d->gamepad_axes[b==6 ? AERON_GAMEPAD_AXIS_LEFT_TRIGGER : AERON_GAMEPAD_AXIS_RIGHT_TRIGGER]=0;
   else { unsigned mask=1u<<buttons[b]; d->gamepad_buttons&=~mask; d->gamepad_pressed_buttons&=~mask; d->gamepad_released_buttons&=~mask; }
  }
  if(out.consume&3840u) { d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFTX]=0; d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFTY]=0; }
  bool help=assist_enabled && !(raw&254u) && !(out.consume&3840u);
  int ax=d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFTX],ay=d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFTY];
  AssistStep(&assist_state[slot],&ax,&ay,assist_dt,help?AssistNearTarget():1,help);
  if(help){d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFTX]=(int16_t)ax;d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFTY]=(int16_t)ay;assist_active=true;}

 }
 return &filtered;
}

bool XwCombos_LeadEnabled(void){Init();return lead_enabled;}
bool XwCombos_TakeTargetListRequest(void){bool pending=target_list_requested;target_list_requested=false;return pending;}
bool XwCombos_SetLead(bool enabled){
 Init();if(!lead_path[0])return false;char tmp[1100];snprintf(tmp,sizeof tmp,"%s.tmp",lead_path);
 FILE*f=fopen(tmp,"w");if(!f)return false;bool ok=fprintf(f,"%d\n",enabled?1:0)>0;
 if(fflush(f)!=0)ok=false;
 if(ok&&fsync(fileno(f))!=0)ok=false;
 if(fclose(f)!=0)ok=false;
 if(!ok||rename(tmp,lead_path)!=0){remove(tmp);return false;}lead_enabled=enabled;return true;
}
