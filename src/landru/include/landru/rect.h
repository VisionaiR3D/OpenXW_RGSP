#ifndef LANDRU_RECT_H
#define LANDRU_RECT_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef LANDRU_GAME_XW
typedef int32_t RectAlignment;
#else
typedef uint8_t RectAlignment;
#endif

typedef struct Rect Rect;
typedef struct Poly Poly;

struct Rect {
	int16_t top;
	int16_t left;
	int16_t bottom;
	int16_t right;
};

struct Poly {
	int16_t x[4];
	int16_t y[4];
};

void xrect_Clear_Rect(Rect* rect);
void xrect_Set_Rect(Rect* rect, int16_t left, int16_t top, int16_t right, int16_t bottom);
void xrect_Max_Rect(Rect* rect);
void xrect_Copy_Rect(Rect* dst, Rect* src);
bool xrect_Equal_Rect(Rect* rect1, Rect* rect2);
void xrect_Offset_Rect(Rect* rect, int16_t xOffset, int16_t yOffset);
void xrect_Origin_Rect(Rect* rect);
void xrect_Allign_Rect(Rect* rect, Rect* parent, RectAlignment xAlign, RectAlignment yAlign);
void xrect_Flip_Rect(Rect* rect, Rect* frame, int16_t hflip, int16_t vflip);
void xrect_Inset_Rect(Rect* rect, int16_t offsetX, int16_t offsetY);
LandruBoolResult xrect_Empty_Rect(Rect* rect);
bool xrect_Sect_Rect(Rect* a, Rect* b);
void xrect_Enclose_Rect(Rect* dstRect, Rect* srcRect);
bool xrect_Clip_Rect(Rect* dstRect, const Rect* clipRect);
void xrect_Clip_Point_To_Rect(Rect* clipRect, int16_t* pointX, int16_t* pointY);
LandruBoolResult xrect_Point_In_Rect(Rect* rect, int16_t pointX, int16_t pointY);
void xrect_Set_Poly(Poly* p, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2,
					int16_t x3, int16_t y3);
void xrect_Copy_Poly(Poly* dst, Poly* src);

#ifdef LANDRU_GAME_XW
void xrect_Scale_Rect(Rect* rect, int16_t xScale, int16_t yScale);
#endif

#endif
