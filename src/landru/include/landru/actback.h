#ifndef LANDRU_ACTBACK_H
#define LANDRU_ACTBACK_H

#include <landru/actor.h>

#ifdef LANDRU_GAME_XW
enum {
	BACK_IDLE = 0,
	BACK_CAPTURE = 1,
	BACK_RESTORE_CAPTURE = 2,
	BACK_RESTORE = 3,
	BACK_STATE_MASK = 0x7f,
	BACK_HOLD_STATE = 0x80
};

Actor* xactback_Alloc_Back_Actor(LandruHandle buffer, const Rect* frame, int16_t x, int16_t y, int16_t z);
void xactback_Alloc_Back_Actor_Buffer(Actor* actor, uint32_t pixelBytes);
void xactback_Set_Back_Actor_Rect(Actor* actor, const Rect* rect);
void xactback_Stop_Back_Actor(Actor* actor);
#endif

#endif
