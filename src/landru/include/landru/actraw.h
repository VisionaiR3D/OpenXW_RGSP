#ifndef LANDRU_ACTRAW_H
#define LANDRU_ACTRAW_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdint.h>

#include <landru/actor.h>

void xactraw_Create_Raw_Actor_Module(void);
void xactraw_Destroy_Raw_Actor_Module(void);
void xactraw_Get_Raw_Actor_Frame(Actor* actor, Rect* outFrame);
Actor* xactraw_Alloc_Raw_Actor(LandruHandle data, Rect* rect, int16_t x, int16_t y, int16_t z);
Actor* xactraw_Res_Raw_Actor(const char* resName, Rect* rect, int16_t x, int16_t y, int16_t z);
void xactraw_Init_Raw_Actor(Actor* actor, LandruHandle data, Rect* rect, int16_t x, int16_t y, int16_t z);
int16_t xactraw_Draw_Raw_Actor(Actor* actor, Rect* clip, Rect* dest, int16_t xoff, int16_t yoff,
							   int16_t refresh);
void xactraw_Update_Raw_Actor(Actor* actor);
int xactraw_Draw_Clipped_Raw(LandruHandle data, int16_t xoff, int16_t yoff, int dirty);
LandruHandle xactraw_Uncompress_Delta_Data(LandruHandle src);

#endif
