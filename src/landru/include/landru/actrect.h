#ifndef LANDRU_ACTRECT_H
#define LANDRU_ACTRECT_H

#include <landru/actor.h>

#ifdef LANDRU_GAME_XW
Actor* xactrect_Alloc_Blank_Actor(int32_t unusedData, const Rect* frame, int16_t x, int16_t y, int16_t z);
#endif

#endif
