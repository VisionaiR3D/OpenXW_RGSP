#ifndef LANDRU_ACTDELT_H
#define LANDRU_ACTDELT_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

#include <landru/actor.h>
#include <landru/res.h>

void xactdelt_Create_Delta_Actor_Module(void);
void xactdelt_Destroy_Delta_Actor_Module(void);
void xactdelt_Get_Delta_Actor_Frame(Actor* actor, Rect* outFrame);
Actor* xactdelt_Alloc_Delta_Actor(LandruHandle data, Rect* rect, int16_t x, int16_t y, int16_t z);
#ifdef LANDRU_GAME_XW
Actor* xactdelt_Res_Delta_Actor(ResFile* rf, const char* resName, Rect* rect, int16_t x, int16_t y,
								int16_t z);
#else
Actor* xactdelt_Res_Delta_Actor(const char* resName, Rect* rect, int16_t x, int16_t y, int16_t z);
#endif
void xactdelt_Init_Delta_Actor(Actor* actor, LandruHandle data, Rect* rect, int16_t x, int16_t y, int16_t z);
int16_t xactdelt_Draw_Delta_Actor(Actor* actor, Rect* clip, Rect* dest, int16_t xoff, int16_t yoff,
								  int16_t refresh);
void xactdelt_Update_Delta_Actor(Actor* actor);
int xactdelt_Draw_Clipped_Delta(LandruHandle data, int16_t x, int16_t y, int dirty);
int xactdelt_Draw_Flipped_Clipped_Delta(Actor* actor, LandruHandle data, int16_t xoff, int16_t yoff,
										int dirty);
int xactdelt_Draw_Scaled_Clipped_Delta(Actor* actor, LandruHandle data, int16_t xoff, int16_t yoff,
									   int dirty);
int xactdelt_Draw_Color_Clipped_Delta(LandruHandle data, int16_t xoff, int16_t color, int16_t yoff,
									  int dirty);

#ifdef LANDRU_GAME_XW
void xactdelt_Get_Delta_Actor_Offset(Actor* actor, int16_t* x, int16_t* y);
#endif

#endif
