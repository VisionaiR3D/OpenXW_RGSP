#ifndef HANDHELD_TARGET_LIST_H
#define HANDHELD_TARGET_LIST_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
enum { TARGET_CAP=256, TARGET_PAGE=4 };
enum { TARGET_ATTACKING=1, TARGET_OBJECTIVE=2, TARGET_INSPECTED=4, TARGET_INSPECTABLE=8,
       TARGET_DAMAGED=16, TARGET_INCOMING=32 };
typedef struct TargetEntry {
 uint16_t ref;uint64_t token;
 char name[48],type[40];
 double distance_km;
 int hull,shields,relation; /* percentages, -1 unavailable; relation 0 enemy, 1 friend, 2 neutral */
 unsigned flags;
 unsigned systems_known,systems_damaged;
} TargetEntry;
static const char*const target_filters[]={"ALL","HOSTILE","FRIENDLY","NEUTRAL","ATTACKING YOU","OBJECTIVES","INSPECTED","NOT INSPECTED","DAMAGED","INCOMING"};
static const char*const target_sorts[]={"DISTANCE","NAME","TYPE","HULL DAMAGE"};
static const char*const target_relations[]={"HOSTILE","FRIENDLY","NEUTRAL"};
static bool TargetMatches(const TargetEntry* e,int filter){
 switch(filter){
 case 1:return e->relation==0;case 2:return e->relation==1;case 3:return e->relation==2;
 case 4:return (e->flags&TARGET_ATTACKING)!=0;case 5:return (e->flags&TARGET_OBJECTIVE)!=0;
 case 6:return (e->flags&(TARGET_INSPECTABLE|TARGET_INSPECTED))==(TARGET_INSPECTABLE|TARGET_INSPECTED);
 case 7:return (e->flags&(TARGET_INSPECTABLE|TARGET_INSPECTED))==TARGET_INSPECTABLE;
 case 8:return (e->flags&TARGET_DAMAGED)!=0;case 9:return (e->flags&TARGET_INCOMING)!=0;
 default:return true;
 }
}
static int TargetCompare(const TargetEntry*a,const TargetEntry*b,int sort){
 int r=0;
 if(sort==1)r=strcmp(a->name,b->name);
 if(sort==2)r=strcmp(a->type,b->type);
 if(sort==3){int ah=a->hull<0?101:a->hull,bh=b->hull<0?101:b->hull;r=(ah>bh)-(ah<bh);}
 if(!r)r=(a->distance_km>b->distance_km)-(a->distance_km<b->distance_km);
 if(!r)r=(a->ref>b->ref)-(a->ref<b->ref);
 return r;
}
static int TargetView(const TargetEntry*all,int count,int filter,int sort,int*indices){
 int n=0;
 for(int i=0;i<count;i++)if(TargetMatches(all+i,filter)){
  int at=n;while(at>0&&TargetCompare(all+i,all+indices[at-1],sort)<0){indices[at]=indices[at-1];--at;}
  indices[at]=i;++n;
 }
 return n;
}
static double TargetDistance(int32_t px,int32_t py,int32_t pz,int32_t x,int32_t y,int32_t z){
 /* Wrapped integer coordinates match the native flight world before promotion. */
 double dx=(int32_t)((uint32_t)x-(uint32_t)px),dy=(int32_t)((uint32_t)y-(uint32_t)py),dz=(int32_t)((uint32_t)z-(uint32_t)pz);
 return sqrt(dx*dx+dy*dy+dz*dz)*161.0/6553600.0;
}
static int TargetPercent(int remaining,int max){if(max<=0)return -1;return remaining<=0?0:(int)((int64_t)remaining*100/max);}
#endif
