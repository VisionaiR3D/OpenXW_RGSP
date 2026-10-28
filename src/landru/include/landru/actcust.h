#ifndef LANDRU_ACTCUST_H
#define LANDRU_ACTCUST_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

#include <landru/actor.h>
#include <landru/rect.h>

void xactcust_Create_Custom_Actor_Module(void);
void xactcust_Destroy_Custom_Actor_Module(void);
Actor* xactcust_Alloc_Custom_Actor(LandruHandle data, Rect* frame, int16_t x, int16_t y, int16_t z);
void xactcust_Init_Custom_Actor(Actor* actor, Rect* frame, int16_t x, int16_t y, int16_t z);
int16_t xactcust_Draw_Custom_Actor(Actor* actor, Rect* rect, Rect* clipRect, int16_t x, int16_t y,
								   int16_t refresh);
void xactcust_Update_Custom_Actor(Actor* actor);

#endif
