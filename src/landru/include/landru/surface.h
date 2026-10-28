#ifndef LANDRU_SURFACE_H
#define LANDRU_SURFACE_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/bitmap.h>
#include <landru/rect.h>

typedef enum LandruSurfaceSet {
	LANDRU_SURFACE_VGA = 0,
	LANDRU_SURFACE_SVGA = 1,
} LandruSurfaceSet;

typedef struct LandruVideoTarget {
	uint8_t* pixels;
	int16_t width;
	int16_t height;
	int16_t stride;
	Rect bounds;
	uint32_t generation;
	bool dirty;
} LandruVideoTarget;

/* Creates the session-lifetime surface selector after canvas and VESA setup. */
bool xsurface_Create_Surface_Module(bool create_secondary_vga);
void xsurface_Destroy_Surface_Module(void);

bool xsurface_Select_Surface_Set(LandruSurfaceSet set);
LandruSurfaceSet xsurface_Get_Surface_Set(void);
bool xsurface_Has_Surface_Set(LandruSurfaceSet set);

BitmapStruct* xsurface_Get_Active_Render_Bitmap(void);
BitmapStruct* xsurface_Get_Active_Diff_Bitmap(void);
bool xsurface_Get_Active_Video_Target(LandruVideoTarget* out);
void xsurface_Get_Logical_Bounds(Rect* out);

void xsurface_Invalidate_Presentation(void);
void xsurface_Mark_Active_Video_Dirty(void);
bool xsurface_Take_Active_Video_Dirty(void);
uint32_t xsurface_Get_Presentation_Generation(void);

#ifdef LANDRU_MODERN
/* Opt in before surface creation. All views are borrowed until publication,
 * invalidation, palette changes or module destruction; use on the owner thread. */
typedef struct LandruPresentedFrame {
	const uint8_t* pixels;
	const uint8_t* palette;
	int width, height, stride;
	int aspect_numerator, aspect_denominator;
	uint64_t generation;
} LandruPresentedFrame;

bool landru_port_Set_Native_Vga_Presentation(bool enabled);
bool xsurface_Uses_Native_Vga_Presentation(void);
void xsurface_Commit_Presentation(void);
bool xsurface_Get_Presented_Frame(LandruPresentedFrame* out);
const uint8_t* xsurface_Get_Effective_Palette(void);
/* The host boundary retains palette ranges even before surfaces are created. */
void xsurface_Reset_Effective_Palette(void);
void xsurface_Set_Effective_Palette(const uint8_t* rgb, int start, int count);
#endif

#endif
