#ifndef LANDRU_DELTA_H
#define LANDRU_DELTA_H

#include <landru/config.h>

#include <stdint.h>

void xdelta_Delta_Image(int16_t x_off, int16_t y_off, uint8_t* data);
void xdelta_Delta_Clip(int16_t x_base, int16_t y_base, uint8_t* data);

#endif
