#include <stddef.h>

#include <landru/bitmap.h>
#include <landru/canvas.h>
#include <landru/cursor.h>
#include <landru/fade.h>
#include <landru/io.h>
#ifdef LANDRU_GAME_XW
#include <landru/fourcc.h>
#endif
#include <landru/mouse.h>
#include <landru/rect.h>
#include <landru/render.h>
#include <landru/vesa.h>

#include "canvas_internal.h"
#include "render_internal.h"

static BitmapStruct cursor_bm_gbl;
static BitmapStruct cursor_back_gbl;
static int16_t cursor_enable_gbl;
// GLOBAL: TIE 0xFBC84
static int16_t cursor_show_gbl;
static bool fade_cursor_gbl;
static bool cur_redraw_gbl;
// GLOBAL: TIE 0xFBC82
static uint8_t cursor_state_gbl;
static bool cursor_module_gbl;
static int16_t curs_hot_x, curs_hot_y;
static int16_t last_curs_x, last_curs_y;
static uint8_t cursor_id_gbl; /* current cursor kind for render capture */
#ifdef LANDRU_GAME_XW
static Actor* default_cursor_actor = NULL;
static Actor* selected_cursor_actor = NULL;
static int16_t selected_cursor_frame = 0;
static int16_t cursor_actor_width = 0;
static int16_t cursor_actor_height = 0;

typedef struct CursorFrameBounds {
	int16_t left, top, right, bottom;
} CursorFrameBounds;
#endif
/* PORT: frontends present the cursor outside the Landru surface. */
static bool cursor_external_presentation_gbl;
// GLOBAL: TIE98 0x58ABCC
int g_softwareCursorEnabled;
// GLOBAL: TIE98 0x58B320
static int g_cursorDisplayCount;
// GLOBAL: TIE98 0x58B324
static uint8_t g_softwareCursorDarkColorIndex;
// GLOBAL: TIE98 0x4F47A8
static uint8_t g_softwareCursorBrightColorIndex;

// GLOBAL: TIE98 0x4F46A8
static const uint8_t g_softwareCursorBitmap[16][16] = {
	{ 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0 }, { 1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
	{ 1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 2, 1, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 1, 1, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0 }, { 1, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
};

#ifndef LANDRU_GAME_XW
static uint8_t pointer_cursor[16][16] = {
	{ 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

static uint8_t wait_cursor[16][16] = {
	{ 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x01, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x01, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x02, 0x0f, 0x0f, 0x01, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x01, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x01, 0x01, 0x01, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x0f, 0x0f, 0x0f, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

#endif

void xcursor_Create_Cursor_Module(void) {
	xbitmap_Alloc_Trans_System_Bitmap(&cursor_bm_gbl, 32, 32);
	xbitmap_Alloc_System_Bitmap(&cursor_back_gbl, 32, 32);
	xbitmap_Erase_Bitmap(&cursor_bm_gbl);
	xbitmap_Erase_Bitmap(&cursor_back_gbl);
	cursor_enable_gbl = 1;
	cursor_show_gbl = 0;
#ifdef LANDRU_GAME_XW
	/* X-Wing's display count shares the visibility counter's module lifetime. */
	g_cursorDisplayCount = 0;
#endif
	fade_cursor_gbl = false;
	cur_redraw_gbl = true;
	cursor_state_gbl = 0;
	curs_hot_x = 0;
	curs_hot_y = 0;
	last_curs_x = xio_Mouse_X();
	last_curs_y = xio_Mouse_Y();
	Rect rect;
	xrect_Set_Rect(&rect, 0, 0, 16, 16);
	xbitmap_Set_Bitmap_Clipping(&cursor_bm_gbl, &rect);
	xbitmap_Set_Bitmap_Clipping(&cursor_back_gbl, &rect);
	cursor_module_gbl = true;
	xcursor_Set_Cursor(2);
}

void xcursor_Destroy_Cursor_Module(void) {
#ifdef LANDRU_GAME_XW
	default_cursor_actor = NULL;
	selected_cursor_actor = NULL;
	selected_cursor_frame = 0;
	cursor_actor_width = 0;
	cursor_actor_height = 0;
#endif
	if (cursor_module_gbl) {
		xbitmap_Free_Bitmap(&cursor_bm_gbl);
		xbitmap_Free_Bitmap(&cursor_back_gbl);
		cursor_module_gbl = false;
	}
}

void xcursor_Set_Cursor_HotSpot(int16_t hot_x, int16_t hot_y) {
	if (curs_hot_x != hot_x || curs_hot_y != hot_y) {
		curs_hot_x = hot_x;
		curs_hot_y = hot_y;
		cur_redraw_gbl = true;
	}
}

void xcursor_Get_Cursor_HotSpot(int16_t* out_x, int16_t* out_y) {
	*out_x = curs_hot_x;
	*out_y = curs_hot_y;
}

#ifdef LANDRU_GAME_XW
static void select_cursor_actor(Actor* actor, int16_t frame, int16_t hot_x, int16_t hot_y) {
	if (!xcursor_Is_Cursor() || !actor || (actor == selected_cursor_actor && frame == selected_cursor_frame))
		return;
	Rect old_rect;
	Rect clip;
	xrect_Set_Rect(&old_rect, last_curs_x - curs_hot_x, last_curs_y - curs_hot_y,
				   last_curs_x - curs_hot_x + cursor_actor_width,
				   last_curs_y - curs_hot_y + cursor_actor_height);
	xcanvas_Get_Drawing_Canvas_Clip(&clip);
	xrect_Clip_Rect(&old_rect, &clip);
	if (!xrect_Empty_Rect(&old_rect) && xcanvas_Is_Screen_Diff_Used())
		xcanvas_Copy_Screen_Portion_To_Diff(old_rect.left, old_rect.top, old_rect.right - old_rect.left,
											old_rect.bottom - old_rect.top);
	selected_cursor_actor = actor;
	selected_cursor_frame = frame;
	curs_hot_x = hot_x;
	curs_hot_y = hot_y;
	cur_redraw_gbl = true;
	cursor_actor_width = 0;
	cursor_actor_height = 0;
	LandruHandle data = LANDRU_NULL_HANDLE;
	if (actor->res_type == FOURCC_DELT)
		data = actor->data;
	else if (actor->res_type == FOURCC_ANIM)
		data = xactor_Get_Actor_Array_Data(actor, frame);
	if (data && xmemhdl_Get_Handle_Size(data) >= sizeof(CursorFrameBounds)) {
		const CursorFrameBounds* bounds = xmemhdl_Lock_Handle(data);
		if (bounds) {
			int16_t width = bounds->right - bounds->left + 1;
			int16_t height = bounds->bottom - bounds->top + 1;
			if (width <= CURSOR_ACTOR_MAX_SIZE && height <= CURSOR_ACTOR_MAX_SIZE) {
				cursor_actor_width = width;
				cursor_actor_height = height;
			}
		}
	}
	/* Actor pixels have no display consumer in XWING95.EXE. */
	last_curs_x = xio_Mouse_X();
	last_curs_y = xio_Mouse_Y();
}

void xcursor_Set_Default_Cursor_Actor(Actor* actor) {
	default_cursor_actor = actor;
	select_cursor_actor(actor, CURSOR_DEFAULT_FRAME, CURSOR_DEFAULT_HOTSPOT, CURSOR_DEFAULT_HOTSPOT);
}

void xcursor_Set_Cursor(int16_t cursorId) {
	int16_t hot_x = cursorId == CURSOR_SPECIAL_FRAME ? CURSOR_SPECIAL_HOT_X : 0;
	int16_t hot_y = cursorId == CURSOR_SPECIAL_FRAME ? CURSOR_SPECIAL_HOT_Y : 0;
	select_cursor_actor(default_cursor_actor, cursorId, hot_x, hot_y);
}
#else
void xcursor_Set_Cursor(uint8_t cursorId) {
	cursor_id_gbl = cursorId;
	uint8_t* cursor = NULL;
	if (cursorId == 0)
		cursor = (uint8_t*)pointer_cursor;
	else if (cursorId == 1)
		cursor = (uint8_t*)wait_cursor;

	uint8_t* dst = xbitmap_Lock_Bitmap(&cursor_bm_gbl);
	for (int y = 0; y < 32; y++) {
		for (int x = 0; x < 32; x++) {
			if (y < 16 && x < 16)
				dst[x + y * 32] = cursor ? cursor[x + y * 16] : 0;
		}
	}
	xbitmap_Unlock_Bitmap(&cursor_bm_gbl);
}

#endif

void xcursor_Push_Cursor_Canvas(void) { xcanvas_Push_Bitmap_Canvas(&cursor_bm_gbl); }

void xcursor_Pop_Cursor_Canvas(void) { xcanvas_Pop_Bitmap_Canvas(); }

void xcursor_Set_Cursor_Canvas(void) { xcanvas_Set_Drawing_Canvas_Bitmap(&cursor_bm_gbl); }

void xcursor_Cursor_To_Screen(int16_t x, int16_t y) {
	if (cursor_external_presentation_gbl) {
		last_curs_x = x;
		last_curs_y = y;
		cur_redraw_gbl = false;
		return;
	}
	if (last_curs_x != x || last_curs_y != y || cur_redraw_gbl) {
		xcursor_Erase_Cursor(0);
		xcursor_Draw_Cursor(x, y, 0);
		cur_redraw_gbl = false;
	}
}

void xcursor_Draw_Cursor(int16_t x, int16_t y, int16_t offscreen) {
	if (xrect_Empty_Rect(&cursor_bm_gbl.clip) || cursor_show_gbl)
		return;
	if (cursor_external_presentation_gbl) {
		last_curs_x = x;
		last_curs_y = y;
		return;
	}

	if (offscreen)
		cursor_state_gbl |= 2;
	else
		cursor_state_gbl |= 1;

	last_curs_x = x;
	last_curs_y = y;
	int16_t left = x - curs_hot_x;
	int16_t top = y - curs_hot_y;
	Rect rect;
	xrect_Set_Rect(&rect, left, top, cursor_bm_gbl.w + left, cursor_bm_gbl.h + top);
	if (offscreen) {
		xcanvas_Copy_Canvas_Portion_To_Bitmap(&cursor_back_gbl, &rect, 0, 0);
		xcanvas_Copy_Bitmap_Clip_To_Canvas(&cursor_bm_gbl, left, top);
	} else {
		xvesa_Copy_Video_Portion_To_Bitmap(&cursor_back_gbl, &rect, 0, 0);
		xvesa_Copy_Bitmap_Clip_To_Video(&cursor_bm_gbl, left, top);
	}
}

void xcursor_Erase_Cursor(int16_t offscreen) {
	if (cursor_external_presentation_gbl)
		return;
	if (offscreen) {
		if (!(cursor_state_gbl & 2))
			return;
		cursor_state_gbl ^= 2;
	} else {
		if (!(cursor_state_gbl & 1))
			return;
		cursor_state_gbl ^= 1;
	}

	int16_t x = last_curs_x - curs_hot_x;
	int16_t y = last_curs_y - curs_hot_y;

	if (offscreen) {
		xcanvas_Copy_Bitmap_Clip_To_Canvas(&cursor_back_gbl, x, y);
		if (xcanvas_Is_Screen_Diff_Used() && !xrect_Empty_Rect(&cursor_bm_gbl.clip)) {
			Rect diff;
			xrect_Copy_Rect(&diff, &cursor_bm_gbl.clip);
			xrect_Offset_Rect(&diff, x - diff.left, y - diff.top);
#ifdef LANDRU_GAME_XW
			xcanvas_Copy_Screen_Portion_To_Diff(diff.left, diff.top, diff.right - diff.left,
												diff.bottom - diff.top);
#else
			xcanvas_Copy_Screen_Portion_To_Diff(&diff);
#endif
		}
	} else {
		xvesa_Copy_Bitmap_Clip_To_Video(&cursor_back_gbl, x, y);
	}
}

void xcursor_Cursor_To_Back(void) {
	if (xfade_Trans_Fade_Active()) {
		if (!fade_cursor_gbl) {
			fade_cursor_gbl = true;
			xcursor_Hide_Cursor();
		}
	} else {
		xcursor_Draw_Cursor(last_curs_x, last_curs_y, 1);
	}
}

void xcursor_Cursor_To_Front(void) {
	if (!xfade_Trans_Fade_Active()) {
		if (fade_cursor_gbl) {
			fade_cursor_gbl = false;
			xcursor_Show_Cursor();
		} else {
			xcursor_Erase_Cursor(1);
		}
	}
}

void xcursor_Cursor_From_Fade(void) {
	if (!xfade_Trans_Fade_Active() && fade_cursor_gbl) {
		fade_cursor_gbl = false;
		xcursor_Show_Cursor();
	}
}

bool xcursor_Is_Cursor(void) { return cursor_enable_gbl == 1; }

LandruBoolResult xcursor_Is_Cursor_Visible(void) { return cursor_show_gbl == 0; }

void xcursor_Enable_Cursor(void) {
	cursor_enable_gbl = 1;
	xcursor_Show_Cursor();
}

void xcursor_Disable_Cursor(void) {
	xcursor_Hide_Cursor();
	cursor_enable_gbl = -1;
}

void xcursor_Refresh_Cursor(void) { cur_redraw_gbl = true; }

void xcursor_Show_Cursor(void) {
	if (xcursor_Is_Cursor()) {
		cursor_show_gbl--;
		if (cursor_show_gbl == 0) {
			cur_redraw_gbl = true;
			/* Notify the host when the engine resumes cursor rendering. */
			xmouse_MS_Show_Mouse();
		}
		++g_cursorDisplayCount;
		xcursor_Cursor_To_Screen(last_curs_x, last_curs_y);
	}
}

void xcursor_Hide_Cursor(void) {
	if (xcursor_Is_Cursor()) {
		if (cursor_show_gbl == 0) {
			cur_redraw_gbl = true;
			/* Notify the host when the engine stops cursor rendering. */
			xmouse_MS_Hide_Mouse();
		}
		cursor_show_gbl++;
		--g_cursorDisplayCount;
		xcursor_Cursor_To_Screen(last_curs_x, last_curs_y);
	}
}

void xcursor_port_Set_External_Presentation(bool external) {
	if (cursor_external_presentation_gbl == external)
		return;
	if (external && cursor_module_gbl) {
		xcursor_Erase_Cursor(0);
		xcursor_Erase_Cursor(1);
	}
	cursor_external_presentation_gbl = external;
	cur_redraw_gbl = true;
}

// FUNCTION: TIE98 0x4A54C0
int xcursor_Get_Display_Count(void) { return g_cursorDisplayCount; }

// FUNCTION: TIE98 0x4A5570
void xcursor_Draw_Software_Cursor_To_Surface(uint8_t* surface, int pitch, int height) {
#ifdef LANDRU_MODERN
	xcursor_Draw_Software_Cursor_At(surface, pitch, height, xio_Mouse_X(), xio_Mouse_Y());
#else
	int mouse_x = xio_Mouse_X();
	int mouse_y = xio_Mouse_Y();
	if (mouse_x > pitch - 1)
		mouse_x = pitch - 1;
	if (mouse_y > height - 1)
		mouse_y = height - 1;
	int width = pitch - mouse_x;
	if (width > 16)
		width = 16;
	if (width < 0)
		width = 0;
	int rows = height - mouse_y;
	if (rows > 16)
		rows = 16;
	if (rows < 0)
		rows = 0;

	for (int y = 0; y < rows; ++y) {
		uint8_t* output = surface + pitch * (mouse_y + y) + mouse_x;
		for (int x = 0; x < width; ++x) {
			const uint8_t cursor_pixel = g_softwareCursorBitmap[y][x];
			if (cursor_pixel == 1)
				output[x] = g_softwareCursorDarkColorIndex;
			else if (cursor_pixel == 2)
				output[x] = g_softwareCursorBrightColorIndex;
		}
	}
#endif
}

#ifdef LANDRU_MODERN
bool xcursor_Get_Software_Cursor(LandruSoftwareCursor* out) {
	if (!out || !cursor_module_gbl || cursor_enable_gbl != 1 || cursor_show_gbl != 0 ||
		g_cursorDisplayCount < 0)
		return false;
	*out = (LandruSoftwareCursor) { &g_softwareCursorBitmap[0][0], 16, 16, g_softwareCursorDarkColorIndex,
									g_softwareCursorBrightColorIndex };
	return true;
}

void xcursor_Draw_Software_Cursor_At(uint8_t* surface, int pitch, int height, int mouse_x, int mouse_y) {
	if (!surface || pitch <= 0 || height <= 0)
		return;
	if (mouse_x < 0)
		mouse_x = 0;
	if (mouse_y < 0)
		mouse_y = 0;
	if (mouse_x > pitch - 1)
		mouse_x = pitch - 1;
	if (mouse_y > height - 1)
		mouse_y = height - 1;
	int width = pitch - mouse_x;
	if (width > 16)
		width = 16;
	if (width < 0)
		width = 0;
	int rows = height - mouse_y;
	if (rows > 16)
		rows = 16;
	if (rows < 0)
		rows = 0;

	for (int y = 0; y < rows; ++y) {
		uint8_t* output = surface + pitch * (mouse_y + y) + mouse_x;
		for (int x = 0; x < width; ++x) {
			const uint8_t cursor_pixel = g_softwareCursorBitmap[y][x];
			if (cursor_pixel == 1)
				output[x] = g_softwareCursorDarkColorIndex;
			else if (cursor_pixel == 2)
				output[x] = g_softwareCursorBrightColorIndex;
		}
	}
}
#endif

// FUNCTION: TIE98 0x4A5630
void xcursor_Select_Contrast_Colors(const uint8_t* rgb6_palette) {
	int darkest_sum = 189;
	int brightest_sum = 0;
	uint8_t darkest = 0;
	uint8_t brightest = 0;
	for (int index = 0; index < 256; ++index) {
		const int sum = rgb6_palette[index * 3] + rgb6_palette[index * 3 + 1] + rgb6_palette[index * 3 + 2];
		if (sum > brightest_sum) {
			brightest_sum = sum;
			brightest = (uint8_t)index;
		}
		if (sum < darkest_sum) {
			darkest_sum = sum;
			darkest = (uint8_t)index;
		}
		if (brightest_sum == 189 && darkest_sum == 0)
			break;
	}
	g_softwareCursorDarkColorIndex = darkest;
	g_softwareCursorBrightColorIndex = brightest;
}

void xcursor_emit_render_state(void) {
	LandruCursorRenderState state = { 0 };
	LandruCursorRenderState* out = &state;
	/* Use the live cursor pose (mouse_cursor_x_gbl), not the
	 * rate-limited rendered pose (last_curs_x). last_curs_x is set
	 * inside xcursor_Draw_Cursor, which only fires when the
	 * cursor_rate_gbl=8 PIT-tick gate (~31 Hz) elapses — between
	 * fires it is stale by up to 32 ms. Consumers need the live pose to
	 * observe engine-initiated cursor warps immediately. */
	out->x = mouse_cursor_x_gbl;
	out->y = mouse_cursor_y_gbl;
	out->hot_x = curs_hot_x;
	out->hot_y = curs_hot_y;
	out->w = cursor_bm_gbl.w;
	out->h = cursor_bm_gbl.h;
	/* An externally presented cursor is intentionally absent from the video
	 * bitmap, so its visibility follows Landru's module and nesting state. */
	out->visible = cursor_external_presentation_gbl
					   ? (cursor_module_gbl && cursor_enable_gbl == 1 && cursor_show_gbl == 0)
					   : (cursor_module_gbl && (cursor_state_gbl & 1));
	out->kind = (LandruCursorKind)cursor_id_gbl;
	landru_render_cursor(out);
}

const uint8_t* xcursor_get_bitmap(int16_t* out_w, int16_t* out_h) {
	if (out_w)
		*out_w = cursor_bm_gbl.w;
	if (out_h)
		*out_h = cursor_bm_gbl.h;
	return cursor_bm_gbl.data;
}
