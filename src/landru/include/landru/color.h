#ifndef LANDRU_COLOR_H
#define LANDRU_COLOR_H

#include <landru/config.h>

#include <stdint.h>

void xcolor_Delta_Color_Image(uint8_t* data, int16_t x_off, int16_t y_off, uint8_t color);
void xcolor_Delta_Color_Clip(uint8_t* data, int16_t x_off, int16_t y_off, int16_t clip_left, int16_t clip_top,
							 int16_t clip_right, int16_t clip_bottom, uint8_t color);
void xcolor_Copy_Transparent(uint8_t* dst, const uint8_t* src, int count);

#endif
