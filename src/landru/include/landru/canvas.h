#ifndef LANDRU_CANVAS_H
#define LANDRU_CANVAS_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/bitmap.h>
#include <landru/render.h>

enum {
	LANDRU_SCREEN_WIDTH = 640,
	LANDRU_SCREEN_HEIGHT = 480,
	LANDRU_VGA_WIDTH = 320,
	LANDRU_VGA_HEIGHT = 200,
	LANDRU_DIFF_SPLIT_ROWS = 100,
	LANDRU_CANVAS_STACK_CAPACITY = 4,
	LANDRU_CANVAS_SCRATCH_ROWS = 44,
	LANDRU_CANVAS_SCRATCH_SIZE = LANDRU_SCREEN_WIDTH * LANDRU_CANVAS_SCRATCH_ROWS
};

extern bool canvas_module_gbl;
extern bool diff_exists_gbl;
extern bool diff_used_gbl;
extern bool diff_valid_gbl;
extern BitmapStruct screen_bm_gbl;
extern BitmapStruct diff_bm_gbl;
extern BitmapStruct buff_bm_gbl;
extern BitmapStruct* canvas_bm_gbl;
extern BitmapStruct* draw_bm_gbl;

extern void* buff_gbl;
extern void* draw_buff_gbl;
extern int16_t buff_size_gbl;
extern int16_t canvas_stack_size_gbl;
extern int16_t draw_w_gbl;
extern int16_t draw_h_gbl;
extern int16_t clip_left_gbl;
extern int16_t clip_top_gbl;
extern int16_t clip_right_gbl;
extern int16_t clip_bottom_gbl;
extern BitmapStruct* canvas_stack_bm_gbl[LANDRU_CANVAS_STACK_CAPACITY];

#ifdef LANDRU_GAME_XW
void xcanvas_Create_Canvas_Module(void);
void xcanvas_InitClippingScratch(void);
#else
void xcanvas_Create_Canvas_Module(int16_t buffSize, int16_t width, int16_t height, bool diff);
#endif
void xcanvas_Destroy_Canvas_Module(void);
BitmapStruct* xcanvas_Get_Current_Canvas_Bitmap(void);
void* xcanvas_Ask_Buffer(int16_t* outBufSize);
#ifdef LANDRU_GAME_XW
void xcanvas_Push_Canvas(uint8_t** previous_pixels, uint8_t* pixels, Rect* previous_raster_clip,
						 int16_t* previous_width, int16_t* previous_height, int16_t width, int16_t height,
						 int16_t clear);
void xcanvas_Pop_Canvas(uint8_t* previous_pixels, Rect* previous_raster_clip, int16_t previous_width,
						int16_t previous_height);
uint8_t* xcanvas_Get_Screen_Buffer(void);
uint8_t* xcanvas_Get_Draw_Buffer(void);
void xcanvas_Set_Draw_Buffer(uint8_t* pixels);
void xcanvas_Get_Screen_Bounds(Rect* rect);
int16_t stub_Copy_From_Clipped_Buffer(const uint8_t* buffer, const Rect* rect, int16_t x, int16_t y,
									  int16_t buffer_width, int16_t buffer_height);
int16_t stub_Copy_To_Clipped_Buffer(uint8_t* buffer, const Rect* rect, int16_t x, int16_t y,
									int16_t buffer_width, int16_t buffer_height);
int16_t xcanvas_Scroll_Clipped_Buffer(uint8_t* pixels, const Rect* clip, int16_t dx, int16_t dy,
									  int16_t width, int16_t height);
#else
bool xcanvas_Push_Canvas(BitmapStruct* bitmap);
void xcanvas_Pop_Canvas(void);
#endif
bool xcanvas_Push_Buffer_Canvas(int16_t width, int16_t height);
void xcanvas_Pop_Buffer_Canvas(void);

/* Changes the session screen canvas at a scene boundary. */
bool xcanvas_Activate_Screen_Bitmap(BitmapStruct* old_bitmap, BitmapStruct* new_bitmap);

/* True iff the active drawing target is the canonical screen bitmap.
 * Used by render capture to suppress paint/draw/text records
 * generated while a non-screen scratch canvas is active. */
bool xcanvas_Is_Screen_Drawing_Target(void);

/* Render-capture gate. Same as Is_Screen_Drawing_Target except it also
 * returns true when a caller has explicitly opted in via
 * xcanvas_Set_Render_Allow_Non_Screen. emit_paint / xactor_emit_draw /
 * xfont_Draw_Clipped_Text gate on this. */
bool xcanvas_Render_Emit_Allowed(void);

/* Routing tag for a render record. The auxiliary target is active while
 * non-screen canvas capture has been explicitly enabled. */
LandruRenderTarget xcanvas_Render_Emit_Target(void);

/* Override the canvas-leak gate while a non-screen canvas is bound.
 * Caller MUST set false before returning to general engine code,
 * otherwise scratch-canvas primitives leak into the render sink. */
void xcanvas_Set_Render_Allow_Non_Screen(bool on);

/* Suppress xfont_Draw_Clipped_Text render capture while set. Callers that
 * publish persistent text separately use this to prevent duplicate records.
 * Nested set/clear is not supported. */
void xcanvas_Set_Suppress_Text_Render(bool on);

/* xfont_Draw_Clipped_Text gates render capture on this combined
 * accessor. It is true iff the canvas gate allows it and no caller has set
 * the suppress flag. */
bool xcanvas_Render_Text_Emit_Allowed(void);
void xcanvas_Set_Drawing_Canvas(void);
void xcanvas_Set_Drawing_Canvas_Bitmap(BitmapStruct* bitmap);
void xcanvas_Get_Drawing_Canvas_Bounds(Rect* rect);
void xcanvas_Set_Drawing_Canvas_Clip(Rect* rect);
void xcanvas_Get_Drawing_Canvas_Clip(Rect* rect);
void xcanvas_Get_Raster_Clip(Rect* rect);
bool xcanvas_Clip_Rect_To_Raster(Rect* rect);
void xcanvas_Max_Drawing_Canvas_Clip(void);
bool xcanvas_Clip_Rect_To_Canvas(Rect* rect);
void xcanvas_Enable_Screen_Diff(void);
void xcanvas_Disable_Screen_Diff(void);
#ifdef LANDRU_GAME_XW
/* Shared text rendering releases temporary clipping bitmaps after each draw. */
void xcanvas_FreeClippingScratch(void);
#endif
void xcanvas_Invalid_Screen_Diff(void);
void xcanvas_Valid_Screen_Diff(void);
bool xcanvas_Is_Screen_Diff(void);
bool xcanvas_Is_Screen_Diff_Used(void);
bool xcanvas_Is_Screen_Diff_Valid(void);
void xcanvas_Copy_Screen_To_Diff(void);
#ifdef LANDRU_GAME_XW
void xcanvas_Copy_Screen_Portion_To_Diff(int16_t x, int16_t y, int16_t width, int16_t height);
void xcanvas_Copy_Diff_Portion_To_Screen(int16_t x, int16_t y, int16_t width, int16_t height);
#else
void xcanvas_Copy_Screen_Portion_To_Diff(Rect* rect);
void xcanvas_Copy_Diff_Portion_To_Screen(Rect* rect);
#endif
void xcanvas_Copy_Diff_To_Screen(void);
/* Push a fade-to-video task with cursor + present + refresh handling
 * as the task's end work. Caller-task yields (CONTINUE) after this
 * call; resumes on its next step with the fade complete. */
void xcanvas_Push_Fade_Screen_To_Video_Task(int16_t fade_style);
void xcanvas_Copy_Screen_To_Video(Rect* rect);
#ifdef LANDRU_GAME_XW
void xcanvas_Copy_Screen_Portion_To_Video(int16_t destination_x, int16_t destination_y, int16_t source_x,
										  int16_t source_y, int16_t width, int16_t height);
#else
void xcanvas_Copy_Screen_Portion_To_Video(Rect* rect, int16_t x, int16_t y);
#endif
void xcanvas_Copy_Dirty_Screen_To_Video(Rect* rect);
void xcanvas_Erase_Canvas(void);
void xcanvas_Erase_Canvas_Rect(Rect* rect);
void xcanvas_Copy_Bitmap_To_Canvas(BitmapStruct* bitmap, int16_t x, int16_t y);
void xcanvas_Copy_Bitmap_Clip_To_Canvas(BitmapStruct* bitmap, int16_t x, int16_t y);
void xcanvas_Copy_Bitmap_Portion_To_Canvas(BitmapStruct* bitmap, Rect* rect, int16_t x, int16_t y);
void xcanvas_Copy_Canvas_Portion_To_Bitmap(BitmapStruct* bitmap, Rect* rect, int16_t x, int16_t y);

#endif
