#ifndef LANDRU_ACTANIM_H
#define LANDRU_ACTANIM_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

#include <landru/actor.h>
#include <landru/res.h>

void xactanim_Create_Anim_Actor_Module(void);
void xactanim_Destroy_Anim_Actor_Module(void);
void xactanim_Get_Anim_Actor_Frame(Actor* actor, Rect* outFrame);
void xactanim_Set_Anim_Actor_State(Actor* actor, int16_t state, int16_t stateFract);
Actor* xactanim_Alloc_Anim_Actor(LandruHandle frames, Rect* rect, int16_t x, int16_t y, int16_t z);
#ifdef LANDRU_GAME_XW
Actor* xactanim_Res_Anim_Actor(ResFile* resFile, const char* resName, Rect* rect, int16_t x, int16_t y,
							   int16_t z);
#else
Actor* xactanim_Res_Anim_Actor(const char* resName, Rect* rect, int16_t x, int16_t y, int16_t z);
#endif
void xactanim_Init_Anim_Actor(Actor* actor, LandruHandle frames, Rect* rect, int16_t x, int16_t y, int16_t z);
void xactanim_Get_Anim_Actor_Bounds(Actor* actor, Rect* outRect);
int16_t xactanim_Draw_Anim_Actor(Actor* actor, Rect* clip, Rect* dest, int16_t xoff, int16_t yoff,
								 int16_t refresh);
void xactanim_Update_Anim_Actor(Actor* actor);

#ifdef LANDRU_GAME_XW
void xactanim_Get_Anim_Actor_Offset(Actor* actor, int16_t* x, int16_t* y);
#endif

#endif
