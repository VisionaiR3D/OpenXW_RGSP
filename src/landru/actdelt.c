#include <stdlib.h>
#include <string.h>

#include <landru/actdelt.h>
#include <landru/actor.h>
#include <landru/canvas.h>
#include <landru/color.h>
#include <landru/delta.h>
#include <landru/dirty.h>
#include <landru/flip.h>
#include <landru/fourcc.h>
#include <landru/rect.h>
#include <landru/res.h>
#include <landru/scale.h>

#include "host_internal.h"

// GLOBAL: TIE 0xD2F70
static bool delta_actor_module_gbl = false;
#ifndef LANDRU_MODERN
static Actor* scaled_svga_warnings[32];

static void warn_scaled_svga_once(Actor* actor) {
	for (size_t i = 0; i < sizeof scaled_svga_warnings / sizeof scaled_svga_warnings[0]; i++) {
		if (scaled_svga_warnings[i] == actor)
			return;
		if (!scaled_svga_warnings[i]) {
			scaled_svga_warnings[i] = actor;
			landru_host_log(LANDRU_LOG_WARN, "scaled DELT '%.*s' is unsupported on a %dx%d SVGA canvas\n", 8,
							actor->res_name, draw_w_gbl, draw_h_gbl);
			return;
		}
	}
}
#endif

void xactdelt_Create_Delta_Actor_Module(void) {
#ifndef LANDRU_MODERN
	memset(scaled_svga_warnings, 0, sizeof scaled_svga_warnings);
#endif
	xactor_Create_Actor_Type(FOURCC_DELT, xactdelt_Get_Delta_Actor_Frame, NULL);
	delta_actor_module_gbl = true;
}

void xactdelt_Destroy_Delta_Actor_Module(void) {
	if (delta_actor_module_gbl) {
		xactor_Destroy_Actor_Type(FOURCC_DELT);
		delta_actor_module_gbl = false;
	}
}

void xactdelt_Get_Delta_Actor_Frame(Actor* actor, Rect* outFrame) {
	if (actor->data) {
		int16_t* hdr = xmemhdl_Lock_Handle(actor->data);
		xrect_Set_Rect(outFrame, hdr[0], hdr[1], hdr[2] + 1, hdr[3] + 1);
	} else {
		xrect_Set_Rect(outFrame, 0, 0, 0, 0);
	}
}

Actor* xactdelt_Alloc_Delta_Actor(LandruHandle data, Rect* rect, int16_t x, int16_t y, int16_t z) {
	Actor* actor = xactor_Alloc_Actor(0);
	if (!actor)
		return NULL;

	xactdelt_Init_Delta_Actor(actor, data, rect, x, y, z);
	xactor_Set_Actor_Name(actor, FOURCC_DELT, "");
	xactor_Non_Discard_Actor_Data(actor);
	return actor;
}

#ifdef LANDRU_GAME_XW
Actor* xactdelt_Res_Delta_Actor(ResFile* rf, const char* resName, Rect* rect, int16_t x, int16_t y,
								int16_t z) {
#else
Actor* xactdelt_Res_Delta_Actor(const char* resName, Rect* rect, int16_t x, int16_t y, int16_t z) {
	ResFile* rf = xres_Find_File_With_Resource(FOURCC_DELT, resName);
#endif
	int offset;
	uint32_t size;
	if (!rf || !xres_Get_Resource_Offset(rf, FOURCC_DELT, resName, &offset, &size))
		return NULL;
	LandruHandle data = xres_Load_Resource_Data(rf, offset, size, LANDRU_MEMORY_DEFAULT);
	if (!data)
		return NULL;

	Actor* actor = xactor_Alloc_Actor(0);
	if (!actor) {
		xmemhdl_Free_Handle(data);
		return NULL;
	}

	xactdelt_Init_Delta_Actor(actor, data, rect, x, y, z);
	xactor_Set_Actor_Name(actor, FOURCC_DELT, resName);
	return actor;
}

void xactdelt_Init_Delta_Actor(Actor* actor, LandruHandle data, Rect* rect, int16_t x, int16_t y, int16_t z) {
	xrect_Set_Rect(&actor->frame, rect->left, rect->top, rect->right, rect->bottom);
	actor->x = x;
	actor->y = y;
	actor->zplane = z;
	xactor_Discard_Actor_Data(actor);
	actor->draw = xactdelt_Draw_Delta_Actor;
	actor->data = data;
	actor->update = (xactorUpdateFunc)xactdelt_Update_Delta_Actor;
	xactor_Add_Actor_To_System(actor);

	Rect frame;
	xactdelt_Get_Delta_Actor_Frame(actor, &frame);
	actor->w = frame.right - frame.left;
	actor->h = frame.bottom - frame.top;
	xrect_Copy_Rect(&actor->bounds, &frame);
}

int16_t xactdelt_Draw_Delta_Actor(Actor* actor, Rect* clip, Rect* dest, int16_t xoff, int16_t yoff,
								  int16_t refresh) {
	(void)clip;
	(void)dest;
	if (!refresh)
		return 0;
	if (!actor->data)
		return 0;

#ifdef LANDRU_MODERN
	xactor_emit_draw(actor, xoff, yoff);
#endif

	int dirty = xactor_Is_Actor_Dirty(actor);

	/* Scaled path */
	if (actor->xscale != 256 || actor->yscale != 256) {
		return xactdelt_Draw_Scaled_Clipped_Delta(actor, actor->data, xoff, yoff, dirty);
	}

	/* Flipped path */
	if ((actor->flags & AF_HFLIP) || (actor->flags & AF_VFLIP)) {
		return xactdelt_Draw_Flipped_Clipped_Delta(actor, actor->data, xoff, yoff, dirty);
	}

	/* Color-substitution path */
	if (actor->flags & AF_REMAP_COLOR) {
		return xactdelt_Draw_Color_Clipped_Delta(actor->data, xoff, actor->foreColor, yoff, dirty);
	}

	/* Default: plain clipped delta */
	return xactdelt_Draw_Clipped_Delta(actor->data, xoff, yoff, dirty);
}

void xactdelt_Update_Delta_Actor(Actor* actor) {
	xactor_Move_Actor(actor);
	xactor_Move_Actor_Frame(actor);
}

int xactdelt_Draw_Clipped_Delta(LandruHandle handle, int16_t x, int16_t y, int dirty) {
	void* data = xmemhdl_Lock_Handle(handle);
	if (!data)
		return 0;
	int16_t* hdr = (int16_t*)data;
	Rect r, clipped;
	xrect_Set_Rect(&r, hdr[0] + x, hdr[1] + y, hdr[2] + x + 1, hdr[3] + y + 1);
	xrect_Copy_Rect(&clipped, &r);

	int ret = 0;
	if (xcanvas_Clip_Rect_To_Raster(&clipped)) {
		uint8_t* pixels = (uint8_t*)(hdr + 4);
		if (xrect_Equal_Rect(&clipped, &r)) {
			xdelta_Delta_Image(x, y, pixels);
		} else {
			xdelta_Delta_Clip(x, y, pixels);
		}
		if (dirty)
			xdirty_Dirty_Rect(&clipped);
		ret = 1;
	}
	return ret;
}

int xactdelt_Draw_Flipped_Clipped_Delta(Actor* actor, LandruHandle handle, int16_t xoff, int16_t yoff,
										int dirty) {
	void* data = xmemhdl_Lock_Handle(handle);
	if (!data)
		return 0;

	int16_t* hdr = (int16_t*)data;
	bool hFlip = (actor->flags & AF_HFLIP) != 0;
	bool vFlip = (actor->flags & AF_VFLIP) != 0;

	int16_t br_x = actor->bounds.right + actor->bounds.left - 1;
	int16_t br_y = actor->bounds.bottom + actor->bounds.top - 1;

	int16_t left, right, top, bottom;
	if (hFlip) {
		/* mirror around (br_x+1)/2: hdr[0] (smaller) maps to the right edge */
		left = (br_x - hdr[2]) + xoff;
		right = (br_x - hdr[0]) + xoff;
	} else {
		left = hdr[0] + xoff;
		right = hdr[2] + xoff;
	}
	if (vFlip) {
		top = (br_y - hdr[3]) + yoff;
		bottom = (br_y - hdr[1]) + yoff;
	} else {
		top = hdr[1] + yoff;
		bottom = hdr[3] + yoff;
	}

	uint8_t* pixels = (uint8_t*)(hdr + 4);
	int16_t cl = clip_left_gbl;
	int16_t ct = clip_top_gbl;
	int16_t cr = clip_right_gbl;
	int16_t cb = clip_bottom_gbl;

	int ret = 1;
	if (left >= cl && top >= ct && right < cr && bottom < cb) {
		xflip_Delta_Flip(pixels, xoff, yoff, br_x, br_y, hFlip, vFlip);
	} else if (left < cr && top < cb && right >= cl && bottom >= ct) {
		xflip_Delta_Clip_Flip(pixels, xoff, yoff, br_x, br_y, hFlip, vFlip, cl, ct, cr - 1, cb - 1);
	} else {
		ret = 0;
	}

	if (ret && dirty) {
		Rect dirtyRect;
		xrect_Set_Rect(&dirtyRect, left, top, right + 1, bottom + 1);
		Rect clipRect;
		xrect_Set_Rect(&clipRect, cl, ct, cr, cb);
		xrect_Clip_Rect(&dirtyRect, &clipRect);
		xdirty_Dirty_Rect(&dirtyRect);
	}

	return ret;
}

int xactdelt_Draw_Scaled_Clipped_Delta(Actor* actor, LandruHandle handle, int16_t xoff, int16_t yoff,
									   int dirty) {
	void* data = xmemhdl_Lock_Handle(handle);
	if (!data)
		return 0;
	int16_t w, h, ox, oy;
	xactor_Get_Actor_Size(actor, &w, &h);
	if (actor->res_type == FOURCC_DELT)
		xactor_Get_Actor_Offset(actor, &ox, &oy);
	else {
		ox = 0;
		oy = 0;
	}

	int16_t scaled_w = (int16_t)((int32_t)w * actor->xscale / 256);
	int16_t scaled_h = (int16_t)((int32_t)h * actor->yscale / 256);

	int16_t left_x = xoff - (scaled_w - w) / 2;
	int16_t top_y = yoff - (scaled_h - h) / 2;
	int16_t right_x = left_x + scaled_w;
	int16_t bottom_y = top_y + scaled_h;

	int16_t cw = clip_right_gbl - clip_left_gbl;
	int16_t ch = clip_bottom_gbl - clip_top_gbl;
#ifndef LANDRU_MODERN
	if (draw_w_gbl > 320 || draw_h_gbl > 200) {
		warn_scaled_svga_once(actor);
		return 0;
	}
#endif

	if (left_x >= clip_left_gbl + cw)
		return 0;
	if (top_y >= clip_top_gbl + ch)
		return 0;
	if (right_x < clip_left_gbl)
		return 0;
	if (bottom_y < clip_top_gbl)
		return 0;

	uint8_t* pixels = (uint8_t*)data + 8;
	if (!xscale_Scale_Delta_Clip(pixels, left_x, top_y, w, h, ox, oy, actor->xscale, actor->yscale,
								 clip_left_gbl, clip_top_gbl, clip_left_gbl + cw - 1, clip_top_gbl + ch - 1))
		return 0;

	if (dirty) {
		Rect r, clip;
		xrect_Set_Rect(&r, left_x, top_y, right_x + 1, bottom_y + 1);
		xrect_Set_Rect(&clip, clip_left_gbl, clip_top_gbl, clip_left_gbl + cw, clip_top_gbl + ch);
		xrect_Clip_Rect(&r, &clip);
		xdirty_Dirty_Rect(&r);
	}

	return 1;
}

int xactdelt_Draw_Color_Clipped_Delta(LandruHandle handle, int16_t xoff, int16_t color, int16_t yoff,
									  int dirty) {
	void* data = xmemhdl_Lock_Handle(handle);
	if (!data)
		return 0;
	int16_t* hdr = (int16_t*)data;
	Rect r, clipped;
	xrect_Set_Rect(&r, hdr[0] + xoff, hdr[1] + yoff, hdr[2] + xoff + 1, hdr[3] + yoff + 1);
	xcanvas_Get_Raster_Clip(&clipped);
	xrect_Clip_Rect(&clipped, &r);

	if (xrect_Empty_Rect(&clipped))
		return 0;

	uint8_t* pixels = (uint8_t*)(hdr + 4);
	if (xrect_Equal_Rect(&clipped, &r)) {
		xcolor_Delta_Color_Image(pixels, xoff, yoff, (uint8_t)color);
	} else {
		xcolor_Delta_Color_Clip(pixels, xoff, yoff, clipped.left, clipped.top, clipped.right - 1,
								clipped.bottom - 1, (uint8_t)color);
	}
	if (dirty)
		xdirty_Dirty_Rect(&clipped);
	return 1;
}

#ifdef LANDRU_GAME_XW
void xactdelt_Get_Delta_Actor_Offset(Actor* actor, int16_t* x, int16_t* y) {
	Rect frame;
	xactdelt_Get_Delta_Actor_Frame(actor, &frame);
	*x = frame.left;
	*y = frame.top;
}
#endif
