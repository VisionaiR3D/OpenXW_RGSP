#ifndef LANDRU_VESA_INTERNAL_H
#define LANDRU_VESA_INTERNAL_H

#include <landru/surface.h>

bool xvesa_Begin_Video_Access(LandruVideoTarget* target);
void xvesa_End_Video_Access(bool direct_surface);

#endif
