#include <stdlib.h>
#include <string.h>

#include <landru/bitmap.h>
#include <landru/canvas.h>
#include <landru/cursor.h>
#include <landru/dirty.h>
#include <landru/fade.h>
#include <landru/paint.h>
#include <landru/rect.h>
#include <landru/remap.h>
#include <landru/surface.h>
#include <landru/vesa.h>

#include "canvas_internal.h"
#include "vesa_internal.h"

bool canvas_module_gbl;
// GLOBAL: TIE 0xFBBF4
bool diff_exists_gbl;
// GLOBAL: TIE 0xFBBFA
bool diff_used_gbl;
bool diff_valid_gbl;
BitmapStruct screen_bm_gbl;
BitmapStruct diff_bm_gbl;
BitmapStruct buff_bm_gbl;
BitmapStruct* canvas_bm_gbl;
// GLOBAL: TIE 0xFBBC8
BitmapStruct* draw_bm_gbl;

// GLOBAL: TIE 0xFBB9C
void* buff_gbl;
// GLOBAL: TIE 0xFBBE4
void* draw_buff_gbl;
// GLOBAL: TIE 0xFBBE8
int16_t buff_size_gbl;
int16_t canvas_stack_size_gbl;
// GLOBAL: TIE 0xFBBF6
int16_t draw_w_gbl;
int16_t draw_h_gbl;
// GLOBAL: TIE 0xFBBF2
int16_t clip_left_gbl;
// GLOBAL: TIE 0xFBBEA
int16_t clip_top_gbl;
// GLOBAL: TIE 0xFBBEC
int16_t clip_right_gbl;
// GLOBAL: TIE 0xFBBEE
int16_t clip_bottom_gbl;
BitmapStruct* canvas_stack_bm_gbl[LANDRU_CANVAS_STACK_CAPACITY];

#ifdef LANDRU_GAME_XW
static BitmapStruct drawing_bitmap = { 0 };
static Rect canvas_bounds = { 0 };
static Rect stored_clip = { 0 };
static CanvasBinding canvas_stack[LANDRU_CANVAS_STACK_CAPACITY] = { 0 };

static void clear_canvas_rect(const Rect* rect) {
	int width = rect->right - rect->left;
	if (!draw_buff_gbl || width <= 0)
		return;
	for (int row = rect->top; row < rect->bottom; ++row)
		memset((uint8_t*)draw_buff_gbl + row * draw_w_gbl + rect->left, 0, (size_t)width);
}

static void bind_pixels(uint8_t* pixels, int16_t width, int16_t height, const Rect* raster_clip) {
	xbitmap_Init_Bitmap(&drawing_bitmap);
	drawing_bitmap.data = pixels;
	drawing_bitmap.w = width;
	drawing_bitmap.h = height;
	draw_bm_gbl = &drawing_bitmap;
	canvas_bm_gbl = &drawing_bitmap;
	draw_buff_gbl = pixels;
	draw_w_gbl = width;
	draw_h_gbl = height;
	xrect_Set_Rect(&canvas_bounds, 0, 0, width, height);
	stored_clip = canvas_bounds;
	Rect clip = *raster_clip;
	xcanvas_Set_Drawing_Canvas_Clip(&clip);
}
#endif

#ifdef LANDRU_GAME_XW
void xcanvas_Create_Canvas_Module(void) {
	const int16_t width = LANDRU_SCREEN_WIDTH;
	const int16_t height = LANDRU_SCREEN_HEIGHT;
	const int16_t buffSize = LANDRU_CANVAS_SCRATCH_SIZE;
	const bool diff = diff_exists_gbl;
#else
void xcanvas_Create_Canvas_Module(int16_t buffSize, int16_t width, int16_t height, bool diff) {
#endif
	xbitmap_Init_Bitmap(&screen_bm_gbl);
	xbitmap_Alloc_System_Bitmap(&screen_bm_gbl, width, height);
#ifdef LANDRU_GAME_XW
	xbitmap_Erase_Bitmap(&screen_bm_gbl);
#endif

	if (diff) {
		xbitmap_Init_Bitmap(&diff_bm_gbl);
		diff_exists_gbl = xbitmap_Alloc_System_Bitmap(&diff_bm_gbl, width, height) != 0;
		diff_used_gbl = diff_exists_gbl;
	} else {
		diff_exists_gbl = false;
		diff_used_gbl = false;
	}

	if (buffSize) {
		buff_gbl = malloc(buffSize);
		buff_size_gbl = buffSize;
	} else {
		buff_gbl = NULL;
		buff_size_gbl = 0;
	}

	xbitmap_Init_Bitmap(&buff_bm_gbl);
	buff_bm_gbl.data = buff_gbl;
	diff_valid_gbl = false;
	canvas_bm_gbl = &screen_bm_gbl;
	xcanvas_Set_Drawing_Canvas();
	canvas_stack_size_gbl = 0;
	canvas_module_gbl = true;
}

void xcanvas_Destroy_Canvas_Module(void) {
	if (!canvas_module_gbl)
		return;

	free(buff_gbl);
	xbitmap_Free_Bitmap(&screen_bm_gbl);
	if (diff_exists_gbl) {
		xbitmap_Free_Bitmap(&diff_bm_gbl);
	}
	canvas_module_gbl = false;
	buff_gbl = NULL;
	draw_buff_gbl = NULL;
	draw_bm_gbl = NULL;
	canvas_bm_gbl = NULL;
	canvas_stack_size_gbl = 0;
	diff_exists_gbl = false;
	diff_used_gbl = false;
	diff_valid_gbl = false;
}

#ifdef LANDRU_GAME_XW
void xcanvas_InitClippingScratch(void) {
	xcanvas_Max_Drawing_Canvas_Clip();
	/* Shared text rendering owns temporary clipping bitmaps per draw call. */
}

void xcanvas_FreeClippingScratch(void) {
	/* X-Wing retained a text-clipping handle until shutdown. The shared font
	 * renderer instead frees each temporary bitmap before returning, so no
	 * clipping allocation remains for this lifecycle entry point to release. */
}
#endif

BitmapStruct* xcanvas_Get_Current_Canvas_Bitmap(void) {
#ifdef LANDRU_GAME_XW
	return draw_bm_gbl;
#else
	return canvas_bm_gbl;
#endif
}

void* xcanvas_Ask_Buffer(int16_t* outBufSize) {
	*outBufSize = buff_size_gbl;
	return buff_gbl;
}

void xcanvas_Save_Binding(CanvasBinding* binding) {
	binding->canvas = canvas_bm_gbl;
#ifdef LANDRU_GAME_XW
	binding->drawing = *draw_bm_gbl;
	binding->bounds = canvas_bounds;
	binding->stored_clip = stored_clip;
#else
	binding->drawing = draw_bm_gbl;
#endif
	xcanvas_Get_Raster_Clip(&binding->raster_clip);
}

void xcanvas_Restore_Binding(const CanvasBinding* binding) {
#ifdef LANDRU_GAME_XW
	drawing_bitmap = binding->drawing;
	draw_bm_gbl = &drawing_bitmap;
	draw_buff_gbl = drawing_bitmap.data;
	draw_w_gbl = drawing_bitmap.w;
	draw_h_gbl = drawing_bitmap.h;
	canvas_bounds = binding->bounds;
	stored_clip = binding->stored_clip;
#else
	xcanvas_Set_Drawing_Canvas_Bitmap(binding->drawing);
#endif
	canvas_bm_gbl = binding->canvas;
	Rect clip = binding->raster_clip;
	xcanvas_Set_Drawing_Canvas_Clip(&clip);
}

bool xcanvas_Push_Bitmap_Canvas(BitmapStruct* bitmap) {
	if (canvas_stack_size_gbl >= LANDRU_CANVAS_STACK_CAPACITY)
		return false;
#ifdef LANDRU_GAME_XW
	xcanvas_Save_Binding(&canvas_stack[canvas_stack_size_gbl]);
#else
	canvas_stack_bm_gbl[canvas_stack_size_gbl] = canvas_bm_gbl;
#endif
	canvas_bm_gbl = bitmap;
	canvas_stack_size_gbl++;
	xcanvas_Set_Drawing_Canvas();
	return true;
}

void xcanvas_Pop_Bitmap_Canvas(void) {
	if (canvas_stack_size_gbl) {
		canvas_stack_size_gbl--;
#ifdef LANDRU_GAME_XW
		xcanvas_Restore_Binding(&canvas_stack[canvas_stack_size_gbl]);
#else
		canvas_bm_gbl = canvas_stack_bm_gbl[canvas_stack_size_gbl];
		xcanvas_Set_Drawing_Canvas();
#endif
	}
}

#ifdef LANDRU_GAME_XW
void xcanvas_Push_Canvas(uint8_t** previous_pixels, uint8_t* pixels, Rect* previous_raster_clip,
						 int16_t* previous_width, int16_t* previous_height, int16_t width, int16_t height,
						 int16_t clear) {
	*previous_pixels = draw_buff_gbl;
	*previous_width = stored_clip.right - stored_clip.left;
	*previous_height = stored_clip.bottom - stored_clip.top;
	xcanvas_Get_Raster_Clip(previous_raster_clip);
	Rect clip;
	xrect_Set_Rect(&clip, 0, 0, width, height);
	bind_pixels(pixels, width, height, &clip);
	if (clear)
		clear_canvas_rect(&clip);
}

void xcanvas_Pop_Canvas(uint8_t* previous_pixels, Rect* previous_raster_clip, int16_t previous_width,
						int16_t previous_height) {
	bind_pixels(previous_pixels, previous_width, previous_height, previous_raster_clip);
}

uint8_t* xcanvas_Get_Screen_Buffer(void) { return xbitmap_Lock_Bitmap(&screen_bm_gbl); }

uint8_t* xcanvas_Get_Draw_Buffer(void) { return draw_buff_gbl; }

void xcanvas_Set_Draw_Buffer(uint8_t* pixels) {
	drawing_bitmap.data = pixels;
	drawing_bitmap.offset = 0;
	draw_buff_gbl = pixels;
}

void xcanvas_Get_Screen_Bounds(Rect* rect) {
	xrect_Set_Rect(rect, 0, 0, LANDRU_SCREEN_WIDTH, LANDRU_SCREEN_HEIGHT);
}
#else
bool xcanvas_Push_Canvas(BitmapStruct* bitmap) { return xcanvas_Push_Bitmap_Canvas(bitmap); }

void xcanvas_Pop_Canvas(void) { xcanvas_Pop_Bitmap_Canvas(); }
#endif

bool xcanvas_Push_Buffer_Canvas(int16_t width, int16_t height) {
	if (!buff_bm_gbl.data || width <= 0 || height <= 0)
		return false;
	if (width * height > buff_size_gbl)
		return false;
	buff_bm_gbl.w = width;
	buff_bm_gbl.h = height;
	xbitmap_Max_Bitmap_Clipping(&buff_bm_gbl);
	return xcanvas_Push_Bitmap_Canvas(&buff_bm_gbl);
}

void xcanvas_Pop_Buffer_Canvas(void) { xcanvas_Pop_Bitmap_Canvas(); }

bool xcanvas_Activate_Screen_Bitmap(BitmapStruct* old_bitmap, BitmapStruct* new_bitmap) {
	if (!old_bitmap || !new_bitmap || canvas_stack_size_gbl != 0 || canvas_bm_gbl != old_bitmap)
		return false;
	canvas_bm_gbl = new_bitmap;
	xcanvas_Set_Drawing_Canvas();
	return true;
}

bool xcanvas_Is_Screen_Drawing_Target(void) {
#ifdef LANDRU_GAME_XW
	return draw_buff_gbl && draw_buff_gbl == xcanvas_Get_Screen_Buffer();
#else
	return draw_bm_gbl == xsurface_Get_Active_Render_Bitmap();
#endif
}

/* Non-screen coordinates are normally excluded from render publication.
 * A caller may opt in when the consumer understands the auxiliary target. */
static bool s_render_allow_non_screen;

void xcanvas_Set_Render_Allow_Non_Screen(bool on) { s_render_allow_non_screen = on; }

bool xcanvas_Render_Emit_Allowed(void) {
	return xcanvas_Is_Screen_Drawing_Target() || s_render_allow_non_screen;
}

/* Route explicitly captured scratch-canvas work to the auxiliary target. */
LandruRenderTarget xcanvas_Render_Emit_Target(void) {
	return s_render_allow_non_screen ? LANDRU_RENDER_TARGET_AUXILIARY : LANDRU_RENDER_TARGET_SCREEN;
}

/* Suppress flag for the lfont render-capture path — see canvas.h.
 * textext's draw callback owns this. The actor system's dirty-rect
 * machinery fires the callback on a non-deterministic cadence, so we
 * can't rely on it for per-tick render coverage; textext re-emits
 * its own subtitle records every tick and uses this flag to keep the
 * lfont hook from double-emitting on the redraw frames. */
static bool s_suppress_text_render;

void xcanvas_Set_Suppress_Text_Render(bool on) { s_suppress_text_render = on; }

bool xcanvas_Render_Text_Emit_Allowed(void) {
	return !s_suppress_text_render && xcanvas_Render_Emit_Allowed();
}

void xcanvas_Set_Drawing_Canvas(void) { xcanvas_Set_Drawing_Canvas_Bitmap(canvas_bm_gbl); }

void xcanvas_Set_Drawing_Canvas_Bitmap(BitmapStruct* bitmap) {
	int16_t width = bitmap->w;
	int16_t height = bitmap->h;
	void* ptr = xbitmap_Lock_Bitmap(bitmap);
	if (ptr) {
		draw_buff_gbl = ptr;
		draw_w_gbl = width;
		draw_h_gbl = height;
	}
	xbitmap_Unlock_Bitmap(bitmap);
#ifdef LANDRU_GAME_XW
	drawing_bitmap = *bitmap;
	drawing_bitmap.data = ptr;
	drawing_bitmap.offset = 0;
	draw_bm_gbl = &drawing_bitmap;
	draw_buff_gbl = ptr;
	draw_w_gbl = width;
	draw_h_gbl = height;
	xrect_Set_Rect(&canvas_bounds, 0, 0, width, height);
	stored_clip = bitmap->clip;
#else
	draw_bm_gbl = bitmap;
#endif
	clip_left_gbl = bitmap->clip.left;
	clip_top_gbl = bitmap->clip.top;
	clip_right_gbl = bitmap->clip.right;
	clip_bottom_gbl = bitmap->clip.bottom;
}

void xcanvas_Get_Drawing_Canvas_Bounds(Rect* rect) {
#ifdef LANDRU_GAME_XW
	*rect = canvas_bounds;
#else
	if (xcanvas_Is_Screen_Drawing_Target())
		xsurface_Get_Logical_Bounds(rect);
	else
		xrect_Set_Rect(rect, 0, 0, draw_bm_gbl->w, draw_bm_gbl->h);
#endif
}

void xcanvas_Set_Drawing_Canvas_Clip(Rect* rect) {
	draw_bm_gbl->clip = *rect;
	clip_left_gbl = rect->left;
	clip_top_gbl = rect->top;
	clip_right_gbl = rect->right;
	clip_bottom_gbl = rect->bottom;
}

void xcanvas_Get_Drawing_Canvas_Clip(Rect* rect) {
#ifdef LANDRU_GAME_XW
	*rect = stored_clip;
#else
	*rect = draw_bm_gbl->clip;
#endif
}

void xcanvas_Get_Raster_Clip(Rect* rect) {
#ifdef LANDRU_GAME_XW
	xrect_Set_Rect(rect, clip_left_gbl, clip_top_gbl, clip_right_gbl, clip_bottom_gbl);
#else
	*rect = draw_bm_gbl->clip;
#endif
}

bool xcanvas_Clip_Rect_To_Raster(Rect* rect) {
	Rect clip;
	xcanvas_Get_Raster_Clip(&clip);
	return xrect_Clip_Rect(rect, &clip);
}

void xcanvas_Max_Drawing_Canvas_Clip(void) {
	Rect clip;
#ifdef LANDRU_GAME_XW
	xcanvas_Get_Screen_Bounds(&clip);
#else
	xrect_Set_Rect(&clip, 0, 0, draw_bm_gbl->w, draw_bm_gbl->h);
#endif
	xcanvas_Set_Drawing_Canvas_Clip(&clip);
}

bool xcanvas_Clip_Rect_To_Canvas(Rect* rect) {
#ifdef LANDRU_GAME_XW
	return xrect_Clip_Rect(rect, &canvas_bounds);
#else
	return xbitmap_Clip_Rect_To_Bitmap(draw_bm_gbl, rect);
#endif
}

void xcanvas_Enable_Screen_Diff(void) {
	diff_used_gbl = diff_exists_gbl;
	diff_valid_gbl = false;
}

void xcanvas_Disable_Screen_Diff(void) { diff_used_gbl = false; }

void xcanvas_Invalid_Screen_Diff(void) { diff_valid_gbl = false; }

void xcanvas_Valid_Screen_Diff(void) { diff_valid_gbl = diff_used_gbl; }

bool xcanvas_Is_Screen_Diff(void) { return diff_exists_gbl; }

bool xcanvas_Is_Screen_Diff_Used(void) { return diff_used_gbl && diff_exists_gbl; }

bool xcanvas_Is_Screen_Diff_Valid(void) { return diff_valid_gbl && diff_used_gbl && diff_exists_gbl; }

#ifdef LANDRU_GAME_XW
static void copy_from_buffer(const uint8_t* buffer, int16_t screen_x, int16_t screen_y, int16_t buffer_x,
							 int16_t buffer_y, int16_t width, int16_t height, int16_t stride) {
	Raster_BlitIndexed8Clipped(draw_buff_gbl, draw_w_gbl, LANDRU_SCREEN_HEIGHT,
							   buffer + buffer_y * stride + buffer_x, width, height, stride, screen_x,
							   screen_y, LANDRU_RASTER_COPY);
}

static void copy_to_buffer(uint8_t* buffer, int16_t screen_x, int16_t screen_y, int16_t buffer_x,
						   int16_t buffer_y, int16_t width, int16_t height, int16_t stride) {
	Raster_BlitIndexed8Clipped(buffer, stride, LANDRU_SCREEN_HEIGHT,
							   (const uint8_t*)draw_buff_gbl + screen_y * draw_w_gbl + screen_x, width,
							   height, draw_w_gbl, buffer_x, buffer_y, LANDRU_RASTER_COPY);
}

typedef struct CanvasCopyRegion {
	int16_t screen_x, screen_y;
	int16_t buffer_x, buffer_y;
	int16_t width, height;
} CanvasCopyRegion;

static bool clip_buffer_region(const Rect* rect, int16_t x, int16_t y, CanvasCopyRegion* region) {
	region->screen_x = x;
	region->screen_y = y;
	region->buffer_x = rect->left;
	region->buffer_y = rect->top;
	region->width = rect->right - rect->left;
	region->height = rect->bottom - rect->top;
	if (x < clip_left_gbl) {
		region->screen_x = clip_left_gbl;
		region->width -= clip_left_gbl - x;
		region->buffer_x += clip_left_gbl - x;
	}
	if (region->width + region->screen_x > clip_right_gbl)
		region->width = clip_right_gbl - region->screen_x;
	if (y < clip_top_gbl) {
		region->screen_y = clip_top_gbl;
		region->height -= clip_top_gbl - y;
		region->buffer_y += clip_top_gbl - y;
	}
	if (region->height + region->screen_y > clip_bottom_gbl)
		region->height = clip_bottom_gbl - region->screen_y;
	return region->width > 0 && region->height > 0;
}

int16_t stub_Copy_From_Clipped_Buffer(const uint8_t* buffer, const Rect* rect, int16_t x, int16_t y,
									  int16_t buffer_width, int16_t buffer_height) {
	CanvasCopyRegion region;
	(void)buffer_height;
	if (!clip_buffer_region(rect, x, y, &region))
		return 0;
	copy_from_buffer(buffer, region.screen_x, region.screen_y, region.buffer_x, region.buffer_y, region.width,
					 region.height, buffer_width);
	return 1;
}

int16_t stub_Copy_To_Clipped_Buffer(uint8_t* buffer, const Rect* rect, int16_t x, int16_t y,
									int16_t buffer_width, int16_t buffer_height) {
	CanvasCopyRegion region;
	(void)buffer_height;
	if (!clip_buffer_region(rect, x, y, &region))
		return 0;
	copy_to_buffer(buffer, region.screen_x, region.screen_y, region.buffer_x, region.buffer_y, region.width,
				   region.height, buffer_width);
	return 1;
}

int16_t xcanvas_Scroll_Clipped_Buffer(uint8_t* pixels, const Rect* clip, int16_t dx, int16_t dy,
									  int16_t width, int16_t height) {
	Rect source;
	Rect destination;
	xrect_Set_Rect(&source, 0, 0, width, height);
	xrect_Clip_Rect(&source, clip);
	destination = source;
	xrect_Offset_Rect(&destination, dx, dy);
	if (!xrect_Clip_Rect(&destination, &source))
		return 0;
	if (!dx && !dy)
		return 1;
	source = destination;
	xrect_Offset_Rect(&source, -dx, -dy);
	int rows = destination.bottom - destination.top;
	int columns = destination.right - destination.left;
	/* The original reverse loop shifts both spans left by a byte. Use the
	 * actual rectangles here, avoiding that out-of-bounds read. */
	for (int row = 0; row < rows; ++row) {
		int index = dy > 0 ? rows - 1 - row : row;
		memmove(pixels + (destination.top + index) * width + destination.left,
				pixels + (source.top + index) * width + source.left, (size_t)columns);
	}
	return 1;
}

static void copy_diff_region(int16_t x, int16_t y, int16_t width, int16_t height, bool restore) {
	if (!diff_exists_gbl)
		return;
	uint8_t* pixels = xbitmap_Lock_Bitmap(&diff_bm_gbl);
	int16_t stride = canvas_bounds.right - canvas_bounds.left;
	int16_t row = y;
	int16_t rows = height;
	if (y < LANDRU_DIFF_SPLIT_ROWS) {
		if (y + height > LANDRU_DIFF_SPLIT_ROWS)
			rows = LANDRU_DIFF_SPLIT_ROWS - y;
		if (restore)
			copy_from_buffer(pixels, x, y, x, y, width, rows, stride);
		else
			copy_to_buffer(pixels, x, y, x, y, width, rows, stride);
		rows = height + y - LANDRU_DIFF_SPLIT_ROWS;
		row = LANDRU_DIFF_SPLIT_ROWS;
	}
	if (row >= LANDRU_DIFF_SPLIT_ROWS && rows > 0) {
		uint8_t* lower = pixels + LANDRU_DIFF_SPLIT_ROWS * LANDRU_SCREEN_WIDTH;
		if (restore)
			copy_from_buffer(lower, x, row, x, row - LANDRU_DIFF_SPLIT_ROWS, width, rows, stride);
		else
			copy_to_buffer(lower, x, row, x, row - LANDRU_DIFF_SPLIT_ROWS, width, rows, stride);
	}
	xbitmap_Unlock_Bitmap(&diff_bm_gbl);
}

void xcanvas_Copy_Screen_Portion_To_Diff(int16_t x, int16_t y, int16_t width, int16_t height) {
	copy_diff_region(x, y, width, height, false);
}

void xcanvas_Copy_Diff_Portion_To_Screen(int16_t x, int16_t y, int16_t width, int16_t height) {
	copy_diff_region(x, y, width, height, true);
}

void xcanvas_Copy_Screen_To_Diff(void) {
	xcanvas_Copy_Screen_Portion_To_Diff(0, 0, canvas_bounds.right, canvas_bounds.bottom);
}

void xcanvas_Copy_Diff_To_Screen(void) {
	xcanvas_Copy_Diff_Portion_To_Screen(0, 0, canvas_bounds.right, canvas_bounds.bottom);
}
#else
void xcanvas_Copy_Screen_To_Diff(void) {
	BitmapStruct* screen = xsurface_Get_Active_Render_Bitmap();
	BitmapStruct* diff = xsurface_Get_Active_Diff_Bitmap();
	if (screen && diff)
		xbitmap_Copy_Bitmap(diff, screen, 0, 0);
}

void xcanvas_Copy_Screen_Portion_To_Diff(Rect* rect) {
	BitmapStruct* screen = xsurface_Get_Active_Render_Bitmap();
	BitmapStruct* diff = xsurface_Get_Active_Diff_Bitmap();
	if (screen && diff)
		xbitmap_Copy_Bitmap_Portion(diff, screen, rect, rect->left, rect->top);
}

void xcanvas_Copy_Diff_To_Screen(void) {
	BitmapStruct* screen = xsurface_Get_Active_Render_Bitmap();
	BitmapStruct* diff = xsurface_Get_Active_Diff_Bitmap();
	if (screen && diff)
		xbitmap_Copy_Bitmap(screen, diff, 0, 0);
}

void xcanvas_Copy_Diff_Portion_To_Screen(Rect* rect) {
	BitmapStruct* screen = xsurface_Get_Active_Render_Bitmap();
	BitmapStruct* diff = xsurface_Get_Active_Diff_Bitmap();
	if (screen && diff)
		xbitmap_Copy_Bitmap_Portion(screen, diff, rect, rect->left, rect->top);
}

#endif

/* Push a full-screen fade task with appropriate cursor management and
 * end-of-task work. Caller-task yields after this call; on the next
 * step the FadeTask has popped and the screen is in its post-fade
 * state with the cursor restored. */
void xcanvas_Push_Fade_Screen_To_Video_Task(int16_t fade_style) {
	Rect bounds;
	xsurface_Get_Logical_Bounds(&bounds);

	bool cursor_was_visible = xcursor_Is_Cursor_Visible();
	if (cursor_was_visible)
		xcursor_Cursor_To_Back();

	(void)xfade_Push_Fade_To_Video_Screen_Task(
		&bounds, fade_style, cursor_was_visible ? FADE_END_CURSOR_TO_FRONT : FADE_END_CURSOR_FROM_FADE,
		/*force_refresh_view=*/false);
}

static void copy_screen_region_to_video(Rect* rect, bool use_diff) {
#ifdef LANDRU_GAME_XW
	int16_t width = rect->right - rect->left;
	int16_t height = rect->bottom - rect->top;
	xcanvas_Copy_Screen_Portion_To_Video(rect->left, rect->top, rect->left, rect->top, width, height);
	if (use_diff && diff_used_gbl)
		xcanvas_Copy_Screen_Portion_To_Diff(rect->left, rect->top, width, height);
#else
	BitmapStruct* screen = xsurface_Get_Active_Render_Bitmap();
	BitmapStruct* diff = xsurface_Get_Active_Diff_Bitmap();
	if (!screen)
		return;
	if (use_diff && diff_valid_gbl && diff_used_gbl && diff) {
		xvesa_Copy_Diff_Bitmap_Portion_To_Video(screen, diff, rect, rect->left, rect->top);
	} else {
		xvesa_Copy_Bitmap_Portion_To_Video(screen, rect, rect->left, rect->top);
		if (use_diff && diff_used_gbl && diff)
			xbitmap_Copy_Bitmap_Portion(diff, screen, rect, rect->left, rect->top);
	}
#endif
}

void xcanvas_Copy_Screen_To_Video(Rect* rect) {
	Rect clipped = *rect;
#ifdef LANDRU_GAME_XW
	if (!xcanvas_Clip_Rect_To_Canvas(&clipped))
		return;
#else
	BitmapStruct* screen = xsurface_Get_Active_Render_Bitmap();
	if (!screen)
		return;
	xbitmap_Max_Bitmap_Clipping(screen);
	xbitmap_Clip_Rect_To_Bitmap(screen, &clipped);
#endif
	copy_screen_region_to_video(&clipped, true);
}

#ifdef LANDRU_GAME_XW
void xcanvas_Copy_Screen_Portion_To_Video(int16_t destination_x, int16_t destination_y, int16_t source_x,
										  int16_t source_y, int16_t width, int16_t height) {
	LandruVideoTarget target;
	if (!draw_buff_gbl)
		return;
	bool direct_surface = xvesa_Begin_Video_Access(&target);
	if (!target.pixels)
		return;
	int32_t mode =
		(landru_video_flags_gbl & LANDRU_VIDEO_VGA_COMPAT) ? LANDRU_RASTER_DOUBLE : LANDRU_RASTER_COPY;
#ifdef LANDRU_MODERN
	if (xsurface_Uses_Native_Vga_Presentation())
		mode = LANDRU_RASTER_COPY;
#endif
	Raster_BlitIndexed8Clipped(target.pixels, target.stride, target.height,
							   (const uint8_t*)draw_buff_gbl + source_y * LANDRU_SCREEN_WIDTH + source_x,
							   width, height, LANDRU_SCREEN_WIDTH, destination_x, destination_y, mode);
	xsurface_Mark_Active_Video_Dirty();
	xvesa_End_Video_Access(direct_surface);
}
#else
void xcanvas_Copy_Screen_Portion_To_Video(Rect* rect, int16_t x, int16_t y) {
	BitmapStruct* screen = xsurface_Get_Active_Render_Bitmap();
	BitmapStruct* diff = xsurface_Get_Active_Diff_Bitmap();
	if (!screen)
		return;
	xvesa_Copy_Bitmap_Portion_To_Video(screen, rect, x, y);
	if (diff)
		xbitmap_Copy_Bitmap_Portion(diff, screen, rect, rect->left, rect->top);
}

#endif

void xcanvas_Copy_Dirty_Screen_To_Video(Rect* clip) {
	if (xfade_Trans_Fade_Active()) {
		diff_valid_gbl = false;
		xdirty_Swap_Dirty_List(1);
		copy_screen_region_to_video(clip, false);
	} else {
		Rect rect;
		xdirty_Prepare_Dirty_List();
		while (xdirty_Next_Dirty_Rect(&rect)) {
			if (xrect_Clip_Rect(&rect, clip))
				copy_screen_region_to_video(&rect, true);
		}
		if (diff_used_gbl && diff_exists_gbl)
			diff_valid_gbl = true;
	}
}

void xcanvas_Erase_Canvas(void) {
#ifdef LANDRU_GAME_XW
	clear_canvas_rect(&stored_clip);
#else
	xbitmap_Erase_Bitmap(draw_bm_gbl);
#endif
}

void xcanvas_Erase_Canvas_Rect(Rect* rect) {
#ifdef LANDRU_GAME_XW
	if (xrect_Clip_Rect(rect, &stored_clip))
		clear_canvas_rect(rect);
#else
	Rect saved_clip = draw_bm_gbl->clip;
	xbitmap_Max_Bitmap_Clipping(draw_bm_gbl);
	xcanvas_Set_Drawing_Canvas_Clip(&draw_bm_gbl->clip);
	int16_t color = xremap_Get_Remap(REMAP_GRAY_0);
	xpaint_Paint_Clipped_Rect(rect, color);
	draw_bm_gbl->clip = saved_clip;
	clip_left_gbl = saved_clip.left;
	clip_top_gbl = saved_clip.top;
	clip_right_gbl = saved_clip.right;
	clip_bottom_gbl = saved_clip.bottom;
#endif
}

void xcanvas_Copy_Bitmap_To_Canvas(BitmapStruct* bitmap, int16_t x, int16_t y) {
	xbitmap_Copy_Bitmap(draw_bm_gbl, bitmap, x, y);
}

void xcanvas_Copy_Bitmap_Clip_To_Canvas(BitmapStruct* bitmap, int16_t x, int16_t y) {
	xbitmap_Copy_Bitmap_Clip(draw_bm_gbl, bitmap, x, y);
}

void xcanvas_Copy_Bitmap_Portion_To_Canvas(BitmapStruct* bitmap, Rect* rect, int16_t x, int16_t y) {
	xbitmap_Copy_Bitmap_Portion(draw_bm_gbl, bitmap, rect, x, y);
}

void xcanvas_Copy_Canvas_Portion_To_Bitmap(BitmapStruct* bitmap, Rect* rect, int16_t x, int16_t y) {
	xbitmap_Copy_Bitmap_Portion(bitmap, draw_bm_gbl, rect, x, y);
}
