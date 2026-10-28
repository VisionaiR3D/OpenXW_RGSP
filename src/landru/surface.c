#include <stdlib.h>
#include <string.h>

#include <landru/canvas.h>
#include <landru/dirty.h>
#include <landru/io.h>
#include <landru/surface.h>
#include <landru/vesa.h>

#include "host_internal.h"

typedef struct LandruSurfaceState {
	BitmapStruct vga_render;
	BitmapStruct vga_diff;
	uint8_t* vga_video;
	LandruSurfaceSet native_set;
	LandruSurfaceSet active_set;
	uint32_t generation;
	bool secondary_vga;
	bool vga_video_dirty;
	bool initialized;
#ifdef LANDRU_MODERN
	uint8_t* presented_pixels;
	bool presented_valid;
#endif
} LandruSurfaceState;

static LandruSurfaceState surface_state;
#ifdef LANDRU_MODERN
static bool native_vga_presentation;
static uint8_t effective_palette[256 * 3];
static uint64_t presented_generation;
#endif

static bool uses_vga_video(void) {
	return surface_state.initialized && xsurface_Get_Surface_Set() == LANDRU_SURFACE_VGA &&
		   surface_state.vga_video != NULL;
}

static const char* surface_name(LandruSurfaceSet set) { return set == LANDRU_SURFACE_SVGA ? "SVGA" : "VGA"; }

static BitmapStruct* render_for_set(LandruSurfaceSet set) {
#ifdef LANDRU_GAME_XW
	(void)set;
	return &screen_bm_gbl;
#else
	if (set == surface_state.native_set)
		return &screen_bm_gbl;
	if (set == LANDRU_SURFACE_VGA && surface_state.secondary_vga)
		return &surface_state.vga_render;
	return NULL;
#endif
}

static BitmapStruct* diff_for_set(LandruSurfaceSet set) {
	if (!diff_exists_gbl)
		return NULL;
#ifdef LANDRU_GAME_XW
	(void)set;
	return &diff_bm_gbl;
#else
	if (set == surface_state.native_set)
		return &diff_bm_gbl;
	if (set == LANDRU_SURFACE_VGA && surface_state.secondary_vga)
		return &surface_state.vga_diff;
	return NULL;
#endif
}

bool xsurface_Create_Surface_Module(bool create_secondary_vga) {
	if (surface_state.initialized || !screen_bm_gbl.data || !vesa_buff_gbl)
		return false;

	memset(&surface_state, 0, sizeof surface_state);
	surface_state.native_set =
		(screen_bm_gbl.w == 640 && screen_bm_gbl.h == 480) ? LANDRU_SURFACE_SVGA : LANDRU_SURFACE_VGA;
	surface_state.active_set = surface_state.native_set;
	surface_state.generation = 1;

#ifdef LANDRU_GAME_XW
	(void)create_secondary_vga;
	surface_state.active_set = xsurface_Get_Surface_Set();
#else
	if (create_secondary_vga) {
		if (surface_state.native_set != LANDRU_SURFACE_SVGA ||
			!xbitmap_Alloc_System_Bitmap(&surface_state.vga_render, 320, 200) ||
			!xbitmap_Alloc_System_Bitmap(&surface_state.vga_diff, 320, 200) ||
			!(surface_state.vga_video = calloc(320u * 200u, 1))) {
			xbitmap_Free_Bitmap(&surface_state.vga_render);
			xbitmap_Free_Bitmap(&surface_state.vga_diff);
			free(surface_state.vga_video);
			surface_state.vga_video = NULL;
			return false;
		}
		/* Keep drawing and presentation separate so fades retain the previous
		 * frame until their copy step, as in the native VGA path. */
		vga_compat_buffer_gbl = surface_state.vga_video;
		surface_state.secondary_vga = true;
	}

#endif
#ifdef LANDRU_MODERN
	if (native_vga_presentation) {
		if (!surface_state.vga_video)
			surface_state.vga_video = calloc(320u * 200u, 1);
		surface_state.presented_pixels = malloc(320u * 200u);
		if (!surface_state.vga_video || !surface_state.presented_pixels) {
			free(surface_state.presented_pixels);
			free(surface_state.vga_video);
			xbitmap_Free_Bitmap(&surface_state.vga_render);
			xbitmap_Free_Bitmap(&surface_state.vga_diff);
			vga_compat_buffer_gbl = NULL;
			memset(&surface_state, 0, sizeof surface_state);
			return false;
		}
		vga_compat_buffer_gbl = surface_state.vga_video;
	}
#endif
	surface_state.initialized = true;
	return true;
}

void xsurface_Destroy_Surface_Module(void) {
	if (!surface_state.initialized)
		return;

	if (xsurface_Get_Surface_Set() != surface_state.native_set)
		(void)xsurface_Select_Surface_Set(surface_state.native_set);
	xbitmap_Free_Bitmap(&surface_state.vga_diff);
	xbitmap_Free_Bitmap(&surface_state.vga_render);
	free(surface_state.vga_video);
#ifdef LANDRU_MODERN
	free(surface_state.presented_pixels);
	++presented_generation;
#endif
	vga_compat_buffer_gbl = NULL;
	landru_video_flags_gbl &= (uint16_t)~LANDRU_VIDEO_VGA_COMPAT;
	memset(&surface_state, 0, sizeof surface_state);
}

bool xsurface_Has_Surface_Set(LandruSurfaceSet set) {
	if (!surface_state.initialized)
		return false;
#ifdef LANDRU_GAME_XW
	return set == LANDRU_SURFACE_VGA || set == LANDRU_SURFACE_SVGA;
#else
	return set == surface_state.native_set || (set == LANDRU_SURFACE_VGA && surface_state.secondary_vga);
#endif
}

bool xsurface_Select_Surface_Set(LandruSurfaceSet set) {
#ifdef LANDRU_GAME_XW
	if (!xsurface_Has_Surface_Set(set)) {
		landru_host_log(LANDRU_LOG_ERROR, "invalid Landru surface transition to %s\n", surface_name(set));
		return false;
	}
	if (set == xsurface_Get_Surface_Set())
		return true;
	bool low_resolution = set == LANDRU_SURFACE_VGA;
	xbitmap_Set_VGA_Compatibility_Mode(low_resolution,
									   low_resolution ? LANDRU_VGA_WIDTH : LANDRU_SCREEN_WIDTH,
									   low_resolution ? LANDRU_VGA_HEIGHT : LANDRU_SCREEN_HEIGHT);
	surface_state.active_set = set;
#ifdef LANDRU_MODERN
	if (uses_vga_video())
		memset(surface_state.vga_video, 0, 320u * 200u);
#endif
	Rect bounds;
	xsurface_Get_Logical_Bounds(&bounds);
	xio_Set_Mouse_Limits(&bounds);
	xcanvas_Invalid_Screen_Diff();
	xdirty_Max_Dirty_List();
	xsurface_Invalidate_Presentation();
	return true;
#else
	BitmapStruct* old_render;
	BitmapStruct* new_render;
	LandruSurfaceSet old_set;
	int16_t old_w;
	int16_t old_h;
	int16_t new_x;
	int16_t new_y;
	Rect bounds;

	if (!xsurface_Has_Surface_Set(set)) {
		landru_host_log(LANDRU_LOG_ERROR, "invalid Landru surface transition to %s\n", surface_name(set));
		return false;
	}
	if (set == surface_state.active_set) {
		BitmapStruct* active = render_for_set(set);
		if (active) {
			xrect_Set_Rect(&bounds, 0, 0, active->w, active->h);
			xio_Set_Mouse_Limits(&bounds);
		}
		return true;
	}

	old_set = surface_state.active_set;
	old_render = render_for_set(old_set);
	new_render = render_for_set(set);
	if (!old_render || !new_render || !xcanvas_Activate_Screen_Bitmap(old_render, new_render))
		return false;

	old_w = old_render->w;
	old_h = old_render->h;
	new_x = (int16_t)((int32_t)xio_Mouse_X() * new_render->w / old_w);
	new_y = (int16_t)((int32_t)xio_Mouse_Y() * new_render->h / old_h);
	if (new_x >= new_render->w)
		new_x = new_render->w - 1;
	if (new_y >= new_render->h)
		new_y = new_render->h - 1;

	surface_state.active_set = set;
	if (set == LANDRU_SURFACE_VGA && surface_state.secondary_vga)
		xbitmap_Set_VGA_Compatibility_Mode(1, 320, 200);
	else
		xbitmap_Set_VGA_Compatibility_Mode(0, new_render->w, new_render->h);
	xrect_Set_Rect(&bounds, 0, 0, new_render->w, new_render->h);
	xio_Set_Mouse_Limits(&bounds);
	xio_Set_Mouse_Position(new_x, new_y);
	xcanvas_Invalid_Screen_Diff();
	xdirty_Max_Dirty_List();

	if (set == LANDRU_SURFACE_VGA && surface_state.secondary_vga) {
		memset(surface_state.vga_video, 0, 320u * 200u);
		surface_state.vga_video_dirty = true;
	} else {
		vesa_dirty_gbl = true;
	}
	xsurface_Invalidate_Presentation();
	landru_host_log(LANDRU_LOG_INFO, "Landru surface %s %dx%d -> %s %dx%d\n", surface_name(old_set), old_w,
					old_h, surface_name(set), new_render->w, new_render->h);
	return true;
#endif
}

LandruSurfaceSet xsurface_Get_Surface_Set(void) {
#ifdef LANDRU_GAME_XW
	return (landru_video_flags_gbl & LANDRU_VIDEO_VGA_COMPAT) ? LANDRU_SURFACE_VGA : LANDRU_SURFACE_SVGA;
#else
	return surface_state.active_set;
#endif
}

BitmapStruct* xsurface_Get_Active_Render_Bitmap(void) { return render_for_set(surface_state.active_set); }

BitmapStruct* xsurface_Get_Active_Diff_Bitmap(void) { return diff_for_set(surface_state.active_set); }

bool xsurface_Get_Active_Video_Target(LandruVideoTarget* out) {
	if (!out)
		return false;
	memset(out, 0, sizeof *out);
	if (uses_vga_video()) {
		out->pixels = surface_state.vga_video;
		out->width = 320;
		out->height = 200;
		out->stride = 320;
		out->dirty = surface_state.vga_video_dirty;
	} else {
		out->pixels = vesa_buff_gbl;
		out->width = vesa_w_gbl;
		out->height = vesa_h_gbl;
		out->stride = vesa_bpsl_gbl;
		out->dirty = vesa_dirty_gbl;
	}
	xrect_Set_Rect(&out->bounds, 0, 0, out->width, out->height);
	out->generation = surface_state.generation;
	return out->pixels != NULL;
}

void xsurface_Get_Logical_Bounds(Rect* out) {
	if (!out)
		return;
#ifdef LANDRU_GAME_XW
	xrect_Set_Rect(out, 0, 0, landru_logical_width_gbl, landru_logical_height_gbl);
#else
	BitmapStruct* render = xsurface_Get_Active_Render_Bitmap();
	if (!render)
		xrect_Clear_Rect(out);
	else
		xrect_Set_Rect(out, 0, 0, render->w, render->h);
#endif
}

void xsurface_Invalidate_Presentation(void) {
	if (!surface_state.initialized)
		return;
	surface_state.generation++;
#ifdef LANDRU_MODERN
	surface_state.presented_valid = false;
	++presented_generation;
#endif
	xsurface_Mark_Active_Video_Dirty();
}

void xsurface_Mark_Active_Video_Dirty(void) {
	if (uses_vga_video())
		surface_state.vga_video_dirty = true;
	else
		vesa_dirty_gbl = true;
}

bool xsurface_Take_Active_Video_Dirty(void) {
	bool dirty;
	if (uses_vga_video()) {
		dirty = surface_state.vga_video_dirty;
		surface_state.vga_video_dirty = false;
	} else {
		dirty = vesa_dirty_gbl;
		vesa_dirty_gbl = false;
	}
	return dirty;
}

uint32_t xsurface_Get_Presentation_Generation(void) { return surface_state.generation; }

#ifdef LANDRU_MODERN
bool landru_port_Set_Native_Vga_Presentation(bool enabled) {
	if (surface_state.initialized)
		return false;
	native_vga_presentation = enabled;
	return true;
}

bool xsurface_Uses_Native_Vga_Presentation(void) { return native_vga_presentation && uses_vga_video(); }

void xsurface_Commit_Presentation(void) {
	if (!xsurface_Uses_Native_Vga_Presentation())
		return;
	memcpy(surface_state.presented_pixels, surface_state.vga_video, 320u * 200u);
	surface_state.presented_valid = true;
	++presented_generation;
}

bool xsurface_Get_Presented_Frame(LandruPresentedFrame* out) {
	if (!out)
		return false;
	memset(out, 0, sizeof *out);
	if (!xsurface_Uses_Native_Vga_Presentation() || !surface_state.presented_valid)
		return false;
	*out = (LandruPresentedFrame) {
		surface_state.presented_pixels, effective_palette, 320, 200, 320, 4, 3, presented_generation
	};
	return true;
}

const uint8_t* xsurface_Get_Effective_Palette(void) { return effective_palette; }

void xsurface_Reset_Effective_Palette(void) {
	memset(effective_palette, 0, sizeof effective_palette);
	surface_state.presented_valid = false;
	++presented_generation;
}

void xsurface_Set_Effective_Palette(const uint8_t* rgb, int start, int count) {
	if (!rgb || count <= 0 || start >= 256)
		return;
	if (start < 0) {
		if (start <= -count)
			return;
		rgb += (size_t)(-start) * 3;
		count += start;
		start = 0;
	}
	if (count > 256 - start)
		count = 256 - start;
	if (memcmp(effective_palette + start * 3, rgb, (size_t)count * 3) != 0) {
		memcpy(effective_palette + start * 3, rgb, (size_t)count * 3);
		if (surface_state.presented_valid)
			++presented_generation;
	}
}
#endif
