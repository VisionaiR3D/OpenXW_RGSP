#ifndef XW_COCKPIT_LIGHTING_H
#define XW_COCKPIT_LIGHTING_H
#include <stdbool.h>
struct XwHudDraw;
int XwCockpitLighting_Mode(void);
const char* XwCockpitLighting_Name(void);
bool XwCockpitLighting_SetMode(int mode);
bool XwCockpitLighting_Prepare(const struct XwHudDraw* draw);
const float* XwCockpitLighting_Tint(int source_x,int source_y);
void XwCockpitLighting_Reset(void);
#endif
