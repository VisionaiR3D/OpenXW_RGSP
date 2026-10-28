#define _POSIX_C_SOURCE 200809L
#include "xw_runtime/storage/save_slots.h"
#include "xw_runtime/storage/replay_format.h"
#include "xw_runtime/runtime/flight_sim.h"
#include "xw/flight/fediskio.h"
#include "xw/flight/mission/mission.h"
#include "xw/flight/replay/replay.h"
#include "xw/frontend/register.h"
#include "xw/frontend/shell.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

/* Single atomic container: native pointer-independent mission checkpoint,
 * pilot context and labels. Never replace a slot until the complete image exists. */
typedef struct SlotHeader {
 char magic[8];uint32_t schema,header_size,payload_size,checksum,flight,scene;
 uint64_t saved_at;char mission[128];
 REGISTER_FastPilotRecord pilot;
 REGISTER_PilotFileRecord pilot_data;
} SlotHeader;
static char error[300],directory[1024];
static bool resuming;
static int resumed_scene;
static bool Fail(const char* why){snprintf(error,sizeof error,"%s",why);return false;}
const char* XwSaveSlots_Error(void){return error;}
static bool Root(void){
 if(directory[0])return true;
 char* p=SDL_GetPrefPath("TotallyOpen","OpenXW-HD");
 if(!p)return Fail("Opslagmap niet beschikbaar.");
 int n=snprintf(directory,sizeof directory,"%s",p);SDL_free(p);
 return n>0&&n<(int)sizeof directory;
}
static bool Path(char* out,size_t cap,const char* name){return Root()&&snprintf(out,cap,"%s%s",directory,name)<(int)cap;}
static uint32_t Hash(const void* data,size_t size){const uint8_t*p=data;uint32_t h=2166136261u;for(size_t i=0;i<size;i++)h=(h^p[i])*16777619u;return h;}
static bool Write(const char* name,const void* data,size_t size){
 char path[1200],tmp[1210];if(!Path(path,sizeof path,name))return false;
 snprintf(tmp,sizeof tmp,"%s.tmp",path);
 FILE*f=fopen(tmp,"wb");if(!f)return Fail("Opslaan mislukt: bestand kan niet worden aangemaakt.");
 chmod(tmp,0600);bool ok=fwrite(data,1,size,f)==size;
 if(fflush(f)||fsync(fileno(f)))ok=false;
 if(fclose(f))ok=false;
 if(!ok||rename(tmp,path)){unlink(tmp);return Fail("Opslaan mislukt; het vorige slot is behouden.");}
 int fd=-1; /* File data is durable before the atomic replacement. */
 (void)fd;return true;
}
static uint8_t* Read(const char* name,size_t* size){
 char path[1200];if(!Path(path,sizeof path,name))return NULL;
 FILE*f=fopen(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
 long n=ftell(f);if(n<=0||n>8*1024*1024||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 uint8_t* data=malloc((size_t)n);bool ok=data&&fread(data,1,(size_t)n,f)==(size_t)n;
 fclose(f);if(!ok){free(data);return NULL;}*size=(size_t)n;return data;
}
static uint8_t* Slot(int slot,SlotHeader* h,size_t* size){
 if(slot<1||slot>XW_SAVE_SLOT_COUNT){Fail("Ongeldig slot.");return NULL;}
 char name[40];snprintf(name,sizeof name,"mission-slot-%d.sav",slot);
 uint8_t* data=Read(name,size);
 if(!data){Fail("Dit slot is leeg of niet leesbaar.");return NULL;}
 if(*size<sizeof *h){free(data);Fail("Slot is beschadigd.");return NULL;}
 memcpy(h,data,sizeof *h);uint32_t stored=h->checksum;
 ((SlotHeader*)data)->checksum=0;
 bool ok=!memcmp(h->magic,"OXWSLOT1",8)&&h->schema==1&&h->header_size==sizeof *h&&
   h->payload_size==*size-sizeof *h&&h->payload_size>=24&&stored==Hash(data,*size)&&
   memchr(h->mission,0,sizeof h->mission)&&memchr(h->pilot.name,0,sizeof h->pilot.name)&&
   (h->scene==XW_SCENE_FLIGHT_PROVING_GROUNDS||h->scene==XW_SCENE_FLIGHT_COMBAT||h->scene==XW_SCENE_FLIGHT_TOUR);
 ((SlotHeader*)data)->checksum=stored;
 if(!ok){free(data);Fail("Slot is beschadigd of gebruikt een ander opslagformaat.");return NULL;}
 return data;
}
bool XwSaveSlots_Info(int slot,char* text,size_t size){
 SlotHeader h;size_t n;uint8_t* data=Slot(slot,&h,&n);
 if(!data){snprintf(text,size,"Slot %d - leeg",slot);return false;}
 free(data);time_t stamp=(time_t)h.saved_at;struct tm time;char date[40]="";
 if(localtime_r(&stamp,&time))strftime(date,sizeof date,"%d-%m %H:%M",&time);
 snprintf(text,size,"Slot %d - %.16s - %s",slot,h.pilot.name,date);return true;
}
bool XwSaveSlots_Save(int slot){
 if(slot<1||slot>XW_SAVE_SLOT_COUNT||!XwFlightSim_IsPlayerControl())return Fail("Opslaan kan alleen tijdens een actieve missie.");
 if(!XwReplayFormat_SaveCheckpoint("slot-draft.rpy"))return Fail(XwReplayFormat_Error());
 size_t n;uint8_t* snapshot=Read("slot-draft.rpy",&n);
 if(!snapshot)return Fail("Checkpoint kan niet worden gelezen.");
 SlotHeader h={0};memcpy(h.magic,"OXWSLOT1",8);h.schema=1;h.header_size=sizeof h;h.payload_size=n;
 h.flight=XwProfile_ActiveFlight()->version;h.saved_at=(uint64_t)time(NULL);
 h.scene=g_missionRuntimeState.mode==0?XW_SCENE_FLIGHT_PROVING_GROUNDS:g_missionRuntimeState.mode==FEDISKIO_MISSION_MODE_TOUR?XW_SCENE_FLIGHT_TOUR:XW_SCENE_FLIGHT_COMBAT;
 snprintf(h.mission,sizeof h.mission,"%s",g_currentMissionFile);h.pilot=g_RegisterShellPilot;h.pilot_data=g_RegisterPilotData;
 uint8_t* data=malloc(sizeof h+n);if(!data){free(snapshot);return Fail("Onvoldoende geheugen om op te slaan.");}
 memcpy(data,&h,sizeof h);memcpy(data+sizeof h,snapshot,n);free(snapshot);
 ((SlotHeader*)data)->checksum=Hash(data,sizeof h+n);
 char name[40];snprintf(name,sizeof name,"mission-slot-%d.sav",slot);
 bool ok=Write(name,data,sizeof h+n);free(data);
 char draft[1200];if(Path(draft,sizeof draft,"slot-draft.rpy"))unlink(draft);
 if(ok)error[0]=0;return ok;
}
bool XwSaveSlots_RequestLoad(int slot){
 SlotHeader h;size_t n;uint8_t* data=Slot(slot,&h,&n);if(!data)return false;
 if(h.flight!=(unsigned)XwProfile_RequestedFlight()->version){free(data);return Fail("Dit slot gebruikt een andere X-Wing-versie.");}
 bool ok=Write("slot-check.rpy",data+sizeof h,h.payload_size);free(data);
 if(ok)ok=XwReplayFormat_CheckFile("slot-check.rpy",XW_REPLAY_CHECKPOINT,(XwGameVersion)h.flight,NULL);
 char path[1200];if(Path(path,sizeof path,"slot-check.rpy"))unlink(path);
 if(!ok)return Fail("Slot kan niet veilig worden geladen.");
 char request[12];int len=snprintf(request,sizeof request,"%d\n",slot);
 if(!Write("slot-resume.request",request,(size_t)len))return false;
 error[0]=0;return true;
}
bool XwSaveSlots_BootResume(void){
 size_t len;uint8_t* request=Read("slot-resume.request",&len);if(!request)return false;
 char path[1200];if(Path(path,sizeof path,"slot-resume.request"))unlink(path);
 int slot=len==2&&request[1]=='\n'?request[0]-'0':0;free(request);
 SlotHeader h;size_t n;uint8_t* data=Slot(slot,&h,&n);if(!data)return false;
 bool ok=h.flight==(unsigned)XwProfile_RequestedFlight()->version&&Write("+savegame.rpy",data+sizeof h,h.payload_size);
 free(data);
 if(ok)ok=XwReplayFormat_CheckFile("+savegame.rpy",XW_REPLAY_CHECKPOINT,(XwGameVersion)h.flight,NULL);
 if(!ok)return Fail("Hervatten van het slot is mislukt.");
 g_RegisterShellPilot=h.pilot;g_RegisterPilotData=h.pilot_data;
 resumed_scene=h.scene;resuming=true;return true;
}
int XwSaveSlots_MissionScene(void){return resumed_scene;}
bool XwSaveSlots_IsResuming(void){return resuming;}
void XwSaveSlots_FinishResume(void){resuming=false;}
