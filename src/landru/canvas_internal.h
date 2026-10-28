#ifndef LANDRU_CANVAS_INTERNAL_H
#define LANDRU_CANVAS_INTERNAL_H

#include <landru/canvas.h>

typedef struct CanvasBinding {
	BitmapStruct* canvas;
#ifdef LANDRU_GAME_XW
	BitmapStruct drawing;
	Rect bounds;
	Rect stored_clip;
#else
	BitmapStruct* drawing;
#endif
	Rect raster_clip;
} CanvasBinding;

void xcanvas_Save_Binding(CanvasBinding* binding);
void xcanvas_Restore_Binding(const CanvasBinding* binding);
bool xcanvas_Push_Bitmap_Canvas(BitmapStruct* bitmap);
void xcanvas_Pop_Bitmap_Canvas(void);

#endif
