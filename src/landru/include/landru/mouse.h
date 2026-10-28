#ifndef LANDRU_MOUSE_H
#define LANDRU_MOUSE_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

bool xmouse_MS_Initialize_Mouse(void);
void xmouse_MS_Show_Mouse(void);
void xmouse_MS_Hide_Mouse(void);
void xmouse_MS_Get_Mouse_Pos(int16_t* buttons, int16_t* x, int16_t* y);
void xmouse_MS_Mouse_Movement(int16_t* x, int16_t* y);
void xmouse_MS_Set_Mouse_Pos(int16_t x, int16_t y);

#endif
