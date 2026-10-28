#include <stddef.h>

#include <landru/actanim.h>
#include <landru/actdelt.h>
#include <landru/actor.h>
#include <landru/actraw.h>
#include <landru/font.h>
#include <landru/fourcc.h>
#include <landru/paint.h>
#include <landru/rect.h>
#include <landru/remap.h>
#include <landru/style.h>

// GLOBAL: TIE 0xD2F38
static Actor* style_actor_gbl;

int16_t xstyle_Get_Style_Base_Color(void) { return xremap_Get_Remap(REMAP_GRAY_3); }

int16_t xstyle_Get_Style_Up_Color(void) { return xremap_Get_Remap(REMAP_GRAY_2); }

int16_t xstyle_Get_Style_Down_Color(void) {
#ifdef LANDRU_GAME_XW
	return 38;
#else
	return xremap_Get_Remap(REMAP_GRAY_4);
#endif
}

/* X-Wing's style entry points use fixed palette indices (0x47A8F0–0x47AA6C).
 * TIE remaps its interface colors to the current scene palette.
 * REMAP_GRAY_N is a light-to-dark ramp where GRAY_1 is darkest (42/255) and
 * GRAY_5 is lightest (210/255). The Paint_* primitives below pass shadow=dark
 * and highlight=light so the underlying bevel builder produces the expected
 * raised-button look (light top, dark verticals+bottom when pressed=0).
 *
 * This matches the retail XSTYLE bindings verified at 0xaa194 (Paint_Base),
 * 0xaa210 (Paint_Alert_Base), 0xaa28c (Paint_Button), 0xaa344 (Paint_Lit_Button),
 * 0xaa3c0 (Paint_Border), 0xaa420 (Paint_TextField). */

int16_t xstyle_Style_Paint_Base(Rect* r, int16_t pressed) {
#ifdef LANDRU_GAME_XW
	return xpaint_Paint_Clipped_DBevel(r, 42, 38, 22, 26, 32, pressed);
#else
	int16_t fill = xremap_Get_Remap(REMAP_GRAY_3);
	int16_t inner_hi = xremap_Get_Remap(REMAP_GRAY_4);
	int16_t outer_hi = xremap_Get_Remap(REMAP_GRAY_5);
	int16_t inner_sh = xremap_Get_Remap(REMAP_GRAY_2);
	int16_t outer_sh = xremap_Get_Remap(REMAP_GRAY_1);
	return xpaint_Paint_Clipped_DBevel(r, outer_hi, inner_hi, outer_sh, inner_sh, fill, pressed);
#endif
}

int16_t xstyle_Style_Paint_Alert_Base(Rect* r, int16_t pressed) {
#ifdef LANDRU_GAME_XW
	return xpaint_Paint_Clipped_DBevel(r, 51, 50, 48, 49, 51, pressed);
#else
	int16_t fill = xremap_Get_Remap(REMAP_BLUE_3);
	int16_t inner_hi = xremap_Get_Remap(REMAP_BLUE_4);
	int16_t outer_hi = xremap_Get_Remap(REMAP_BLUE_5);
	int16_t inner_sh = xremap_Get_Remap(REMAP_BLUE_2);
	int16_t outer_sh = xremap_Get_Remap(REMAP_BLUE_1);
	return xpaint_Paint_Clipped_DBevel(r, outer_hi, inner_hi, outer_sh, inner_sh, fill, pressed);
#endif
}

void xstyle_Style_Paint_Button(Rect* r, int16_t pressed) {
#ifdef LANDRU_GAME_XW
	xpaint_Frame_Clipped_Rect(r, 16);
	xrect_Inset_Rect(r, 1, 1);
	xpaint_Paint_Clipped_DBevel(r, 42, 38, 22, 26, pressed ? 30 : 32, pressed);
	xrect_Inset_Rect(r, -1, -1);
#else
	int16_t border = xremap_Get_Remap(REMAP_GRAY_0);
	int16_t fill = xremap_Get_Remap(pressed ? REMAP_GRAY_2 : REMAP_GRAY_3);
	xpaint_Frame_Clipped_Rect(r, border);
	xrect_Inset_Rect(r, 1, 1);
	int16_t inner_hi = xremap_Get_Remap(REMAP_GRAY_4);
	int16_t outer_hi = xremap_Get_Remap(REMAP_GRAY_5);
	int16_t inner_sh = xremap_Get_Remap(REMAP_GRAY_2);
	int16_t outer_sh = xremap_Get_Remap(REMAP_GRAY_1);
	xpaint_Paint_Clipped_DBevel(r, outer_hi, inner_hi, outer_sh, inner_sh, fill, pressed);
	xrect_Inset_Rect(r, -1, -1);
#endif
}

int16_t xstyle_Style_Paint_Lit_Button(Rect* r, int16_t pressed) {
	int16_t fill = xremap_Get_Remap(REMAP_YELLOW_6);
	int16_t inner_hi = xremap_Get_Remap(REMAP_GRAY_4);
	int16_t outer_hi = xremap_Get_Remap(REMAP_GRAY_5);
	int16_t inner_sh = xremap_Get_Remap(REMAP_GRAY_2);
	int16_t outer_sh = xremap_Get_Remap(REMAP_GRAY_1);
	return xpaint_Paint_Clipped_DBevel(r, outer_hi, inner_hi, outer_sh, inner_sh, fill, pressed);
}

int16_t xstyle_Style_Paint_Border(Rect* r, int16_t pressed) {
#ifdef LANDRU_GAME_XW
	return xpaint_Paint_Clipped_Bevel(r, 38, 26, 32, pressed);
#else
	int16_t base = xremap_Get_Remap(REMAP_GRAY_3);
	int16_t shadow = xremap_Get_Remap(REMAP_GRAY_2);
	int16_t highlight = xremap_Get_Remap(REMAP_GRAY_4);
	return xpaint_Paint_Clipped_Bevel(r, highlight, shadow, base, pressed);
#endif
}

int16_t xstyle_Style_Paint_TextField(Rect* r) {
#ifdef LANDRU_GAME_XW
	return xpaint_Paint_Clipped_Bevel(r, 38, 26, 16, 1);
#else
	int16_t fill = xremap_Get_Remap(REMAP_GRAY_0);
	int16_t shadow = xremap_Get_Remap(REMAP_GRAY_2);
	int16_t highlight = xremap_Get_Remap(REMAP_GRAY_4);
	return xpaint_Paint_Clipped_Bevel(r, highlight, shadow, fill, 1);
#endif
}

void xstyle_Style_Button_Text(const char* str, Rect* r, int16_t pressed) {
	Rect ra = *r;
	int16_t color;
	if (pressed) {
#ifdef LANDRU_GAME_XW
		color = 38;
#else
		color = xremap_Get_Remap(REMAP_GRAY_4);
#endif
		ra.right -= 2;
		ra.bottom -= 2;
	} else {
#ifdef LANDRU_GAME_XW
		color = 26;
#else
		color = xremap_Get_Remap(REMAP_GRAY_1);
#endif
	}
#ifdef LANDRU_GAME_XW
	xfont_Print_Centered_Text(str, &ra, 0, color);
#else
	xfont_Print_Centered_Text(str, &ra, color, 0);
#endif
}

void xstyle_Style_Small_Button_Text(const char* str, Rect* r, int16_t pressed) {
	Rect ra = *r;
#ifdef LANDRU_GAME_XW
	int16_t color = pressed ? 38 : 26;
	xfont_Print_Centered_Text(str, &ra, 1, color);
#else
	int16_t color = xremap_Get_Remap(pressed ? REMAP_GRAY_4 : REMAP_GRAY_2);
	xfont_Print_Centered_Text(str, &ra, color, 1);
#endif
}

void xstyle_Style_Trim_Base(Rect* r) { xrect_Inset_Rect(r, 2, 2); }

void xstyle_Style_Trim_Button(Rect* r) { xrect_Inset_Rect(r, 2, 2); }

void xstyle_Style_Trim_Border(Rect* r) { xrect_Inset_Rect(r, 1, 1); }

void xstyle_Style_Trim_TextField(Rect* r) { xrect_Inset_Rect(r, 1, 1); }

void xstyle_Style_Set_Icon_Actor(Actor* actor) { style_actor_gbl = actor; }

void xstyle_Style_Clear_Icon_Actor(void) { style_actor_gbl = NULL; }

void xstyle_Style_Draw_Icon(uint8_t icon_id, Rect* clip, Rect* dest, int16_t xoff, int16_t yoff,
							int16_t lit) {
	if (!style_actor_gbl)
		return;
	xactor_Set_Actor_State(style_actor_gbl, 2 * icon_id + lit, 0);
	int16_t w, h;
	xactor_Get_Actor_Size(style_actor_gbl, &w, &h);
	if (style_actor_gbl->state < style_actor_gbl->arraySize)
		xactanim_Draw_Anim_Actor(style_actor_gbl, clip, dest, xoff, yoff, 1);
}

void xstyle_Style_Draw_Centered_Icon(StyleIconId icon_id, Rect* clip, Rect* dest, int16_t lit) {
	xstyle_Style_Draw_Centered_Actor(style_actor_gbl, clip, dest, 2 * icon_id + lit);
}

void xstyle_Style_Draw_Centered_Actor(Actor* actor, Rect* clip, Rect* dest, int16_t state) {
	if (!actor)
		return;

	int16_t saved_state = 0, saved_fract = 0;
	if (actor->res_type == FOURCC_ANIM) {
		xactor_Get_Actor_State(actor, &saved_state, &saved_fract);
		xactor_Set_Actor_State(actor, state, 0);
	}

	int16_t w, h, ox, oy;
	xactor_Get_Actor_Size(actor, &w, &h);
	xactor_Get_Actor_Offset(actor, &ox, &oy);

	int16_t cx = ((clip->right - clip->left - w) >> 1) + clip->left - ox;
	int16_t cy = ((clip->bottom - clip->top - h) >> 1) + clip->top - oy;

	switch (actor->res_type) {
		case FOURCC_DELT:
			xactdelt_Draw_Delta_Actor(actor, clip, dest, cx, cy, 1);
			break;
		case 'RAW ':
			xactraw_Draw_Raw_Actor(actor, clip, dest, cx, cy, 1);
			break;
		case FOURCC_ANIM:
			if (actor->arraySize > actor->state) {
				xactanim_Draw_Anim_Actor(actor, clip, dest, cx, cy, 1);
				xactor_Set_Actor_State(actor, saved_state, saved_fract);
			}
			break;
	}
}
