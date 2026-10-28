#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <landru/canvas.h>
#include <landru/cursor.h>
#include <landru/dirty.h>
#include <landru/fade.h>
#include <landru/font.h>
#include <landru/memptr.h>
#include <landru/pal.h>
#include <landru/rect.h>
#include <landru/render.h>
#include <landru/task.h>
#include <landru/vesa.h>
#include <landru/view.h>

#include "render_internal.h"

// GLOBAL: TIE 0xD2F54
static Fade* fade_gbl;
// GLOBAL: TIE 0xD2F58
static int16_t fade_module_gbl;

#ifdef LANDRU_GAME_XW
typedef struct TimedText {
	const char* text;
	int16_t start, end, font, x, y, color;
} TimedText;

static TimedText timed_text[TIMED_TEXT_CAPACITY] = { 0 };
static int16_t timed_text_count = 0;
static int16_t speech_text_enabled = 0;

static struct {
	Rect bounds;
	int16_t x, y, height;
	bool pending;
} timed_text_background = { 0 };

void xfade_ClearTimedText(void) { timed_text_count = 0; }

void xfade_SetSpeechTextEnabled(int16_t enabled) { speech_text_enabled = enabled; }

void xfade_AddTimedText(const char* text, int16_t start, int16_t end, int16_t fontId, int16_t x, int16_t y,
						int16_t color) {
	if (timed_text_count == TIMED_TEXT_CAPACITY)
		return;
	if (color == TIMED_TEXT_TITLE_COLOR)
		x = LANDRU_VGA_WIDTH / 2 - (xfont_Get_String_Width_0(fontId, text) >> 1);
	TimedText entry = { text, start, end, fontId, x, y, color };
	timed_text[timed_text_count++] = entry;
}

static void draw_timed_text(int32_t time, Rect* text_frame) {
	xrect_Clear_Rect(text_frame);
	if (!timed_text_count)
		return;
	xcanvas_Max_Drawing_Canvas_Clip();
	int16_t capacity;
	uint8_t* pixels = xcanvas_Ask_Buffer(&capacity);
	if (!pixels)
		return;
	for (int i = 0; i < timed_text_count; ++i) {
		const TimedText* first = &timed_text[i];
		const TimedText* second = NULL;
		if (i + 1 < timed_text_count && timed_text[i + 1].start == first->start)
			second = &timed_text[++i];
		if (time < first->start || time > first->end)
			continue;
		bool shadow = xfont_Is_FontID_Shadow(first->font) != 0;
		if (!shadow)
			xfont_Enable_FontID_Shadow(first->font);
		int height = xfont_Get_FontID_Height(first->font) + 1;
		int16_t width = xfont_Get_String_Width_0(first->font, first->text);
		if (second) {
			int16_t secondWidth = xfont_Get_String_Width_0(first->font, second->text);
			if (secondWidth > width)
				width = secondWidth;
			height += second->y - first->y;
		}
		if (width > 0 && width <= LANDRU_SCREEN_WIDTH && height > 0 &&
			height <= capacity / LANDRU_SCREEN_WIDTH) {
			Rect current_frame;
			xrect_Set_Rect(&current_frame, first->x, first->y, first->x + width, first->y + height);
			xrect_Enclose_Rect(text_frame, &current_frame);
			xrect_Set_Rect(&timed_text_background.bounds, 0, 0, width, height);
			timed_text_background.x = first->x;
			timed_text_background.y = first->y;
			timed_text_background.height = height;
			stub_Copy_To_Clipped_Buffer(pixels, &timed_text_background.bounds, first->x, first->y,
										LANDRU_SCREEN_WIDTH, height);
			timed_text_background.pending = true;
			int color = first->color;
			if (time != first->end && (speech_text_enabled || color == TIMED_TEXT_TITLE_COLOR)) {
				if (color == TIMED_TEXT_TITLE_COLOR) {
					color += TIMED_TEXT_FADE_IN_RATE * (time - first->start);
					if (color > TIMED_TEXT_TITLE_MAX_COLOR)
						color = TIMED_TEXT_TITLE_MAX_COLOR;
					if (first->end - time < TIMED_TEXT_FADE_OUT_TICKS) {
						color = TIMED_TEXT_TITLE_COLOR + TIMED_TEXT_FADE_OUT_RATE * (first->end - time);
						if (color < TIMED_TEXT_TITLE_COLOR)
							color = TIMED_TEXT_TITLE_COLOR;
					}
				}
				xfont_Print_Clipped_Text(first->text, first->x, first->y, first->font, color);
				if (second)
					xfont_Print_Clipped_Text(second->text, first->x, second->y, first->font, color);
			}
		}
		if (!shadow)
			xfont_Disable_FontID_Shadow(first->font);
	}
}

void xfade_RestoreTimedTextBackground(void) {
	if (!timed_text_background.pending)
		return;
	int16_t capacity;
	const uint8_t* pixels = xcanvas_Ask_Buffer(&capacity);
	if (pixels && timed_text_background.height <= capacity / LANDRU_SCREEN_WIDTH)
		stub_Copy_From_Clipped_Buffer(pixels, &timed_text_background.bounds, timed_text_background.x,
									  timed_text_background.y, LANDRU_SCREEN_WIDTH,
									  timed_text_background.height);
}
#endif

/* Compute the sub-rectangle for a wipe transition.
   Modifies r, out_left, out_top in place based on mode, step, total.
   Returns 1 if a special mode (no pixel copy), 0 otherwise. */
static int compute_wipe_rect(Rect* r, Rect* bounds, FadeWipeMode mode, int16_t step, int16_t total,
							 int16_t is_not_last, int16_t* out_left, int16_t* out_top) {
	int16_t left = bounds->left;
	int16_t top = bounds->top;
	int16_t right = bounds->right;
	int16_t bottom = bounds->bottom;
	int16_t w = right - left;
	int16_t h = bottom - top;
	int16_t remaining = total - step;

	xrect_Copy_Rect(r, bounds);
	*out_left = left;
	*out_top = top;

	switch (mode) {
		case FADE_WIPE_SNAP_ON:
			if (step != total / 2 && is_not_last) {
				xrect_Set_Rect(r, 0, 0, 0, 0);
			} else {
				/*fprintf(stderr, "[SNAP_ON] step=%d total=%d is_not_last=%d rect=(%d,%d,%d,%d)\n",
					step, total, is_not_last, r->left, r->top, r->right, r->bottom);*/
			}
			break;
		case FADE_WIPE_SNAP_OFF:
			if (step != total / 2 && is_not_last)
				xrect_Set_Rect(r, 0, 0, 0, 0);
			break;
		case FADE_WIPE_RIGHT:
			r->left = right - step * w / total;
			break;
		case FADE_WIPE_LEFT:
			r->right = left + step * w / total;
			*out_left = right - (r->right - r->left);
			break;
		case FADE_WIPE_DOWN:
			r->bottom = top + h * step / total;
			*out_top = bottom - (r->bottom - r->top);
			break;
		case FADE_WIPE_UP:
			r->top = bottom - h * step / total;
			break;
		case FADE_WIPE_DIAG_TL_BR:
			r->top = bottom - step * h / total;
			r->right = left + step * w / total;
			*out_left = right - (r->right - r->left);
			break;
		case FADE_WIPE_DIAG_TR_BL:
			r->top = bottom - h * step / total;
			r->left = right - w * step / total;
			break;
		case FADE_WIPE_DIAG_BL_TR:
			r->bottom = top + step * h / total;
			r->right = left + step * w / total;
			*out_left = right - (r->right - r->left);
			*out_top = bottom - (r->bottom - r->top);
			break;
		case FADE_WIPE_DIAG_BR_TL:
			r->bottom = top + step * h / total;
			r->left = right - w * step / total;
			*out_top = bottom - (r->bottom - r->top);
			break;
		case FADE_WIPE_EXPAND_RIGHT:
			r->right = left + w * step / total;
			break;
		case FADE_WIPE_EXPAND_LEFT:
			r->left = right - w * step / total;
			*out_left = r->left;
			break;
		case FADE_WIPE_EXPAND_UP:
			r->top = bottom - step * h / total;
			*out_top = r->top;
			break;
		case FADE_WIPE_EXPAND_DOWN:
			r->bottom = top + step * h / total;
			break;
		case FADE_WIPE_CORNER_TL:
			r->top = bottom - step * h / total;
			*out_top = r->top;
			r->right = left + step * w / total;
			break;
		case FADE_WIPE_CORNER_TR:
			r->top = bottom - step * h / total;
			r->left = right - step * w / total;
			*out_left = r->left;
			*out_top = r->top;
			break;
		case FADE_WIPE_CORNER_BL:
			r->bottom = top + step * h / total;
			r->right = left + step * w / total;
			break;
		case FADE_WIPE_CORNER_BR:
			r->bottom = top + step * h / total;
			r->left = right - step * w / total;
			*out_left = r->left;
			break;
		case FADE_WIPE_IRIS_H: {
			int16_t inset = (w / 2) * remaining / total;
			r->left = left + inset;
			r->right = right - inset;
			*out_left = r->left;
			break;
		}
		case FADE_WIPE_IRIS_V: {
			int16_t inset = (h / 2) * remaining / total;
			r->top = top + inset;
			r->bottom = bottom - inset;
			*out_top = r->top;
			break;
		}
		case FADE_WIPE_IRIS_BOX: {
			int16_t v_inset = remaining * (h / 2) / total;
			int16_t h_inset = remaining * (w / 2) / total;
			r->top = top + v_inset;
			r->bottom = bottom - v_inset;
			r->left = left + h_inset;
			r->right = right - h_inset;
			*out_left = r->left;
			*out_top = r->top;
			break;
		}
		case FADE_WIPE_SPECIAL_1:
		case FADE_WIPE_SPECIAL_2:
		case FADE_WIPE_SPECIAL_3:
			return 1;
		default:
			break;
	}
	return 0;
}

void xfade_Create_Fade_Module(void) {
#ifdef LANDRU_GAME_XW
	timed_text_count = 0;
	speech_text_enabled = 1;
	timed_text_background.pending = false;
#endif
	fade_gbl = xmemptr_Alloc_System_Pointer(sizeof(Fade));
	memset(fade_gbl, 0, sizeof(Fade));
	for (int i = 0; i < 4; i++)
		fade_gbl->chan_mode[i] = FADE_WIPE_NONE;
	fade_gbl->full_mode = FADE_WIPE_NONE;
	fade_gbl->color_mode = FADE_COLOR_NONE;
	fade_module_gbl = 1;
}

void xfade_Destroy_Fade_Module(void) {
#ifdef LANDRU_GAME_XW
	timed_text_count = 0;
	timed_text_background.pending = false;
#endif
	if (fade_module_gbl) {
		if (fade_gbl) {
			xfade_Free_Fade(fade_gbl);
			fade_gbl = NULL;
		}
		fade_module_gbl = 0;
	}
}

Fade* xfade_Alloc_Fade(void) { return xmemptr_Alloc_System_Pointer(sizeof(Fade)); }

void xfade_Free_Fade(Fade* fade) {
	if (fade)
		xmemptr_Free_System_Pointer(fade);
}

void xfade_Start_Fade(int16_t channel, FadeWipeMode mode, FadeColorMode color_mode, int16_t capture_dest,
					  int16_t delay, int16_t sustain) {
	fade_gbl->chan_mode[channel] = mode;
	fade_gbl->chan_delay[channel] = delay;
	fade_gbl->chan_sustain[channel] = sustain;
	fade_gbl->chan_total[channel] = xfade_Calc_Fade_Length(mode);
	fade_gbl->chan_step[channel] = 0;
	xfade_Start_Color_Fade(color_mode, capture_dest, fade_gbl->chan_total[channel], delay, sustain);
}

void xfade_Start_Full_Fade(FadeWipeMode mode, FadeColorMode color_mode, int16_t capture_dest, int16_t delay,
						   int16_t sustain) {
	fade_gbl->full_mode = mode;
	fade_gbl->full_delay = delay;
	fade_gbl->full_sustain = sustain;
	fade_gbl->full_total = xfade_Calc_Fade_Length(mode);
	fade_gbl->full_step = 0;
	xfade_Start_Color_Fade(color_mode, capture_dest, fade_gbl->full_total, delay, sustain);
}

/* The optional sink classifies a scene transition when a fade begins. Latch
 * the result because the consumer's transition marker may be transient. */
static bool s_fade_is_scene_transition;

void xfade_Start_Color_Fade(FadeColorMode color_mode, int16_t capture_dest, int16_t total_steps,
							int16_t delay, int16_t sustain) {
	if (!color_mode)
		return;
	fade_gbl->color_mode = color_mode;
	xpal_Screen_To_Src_Palette(0, 0, 255);
	if (!capture_dest)
		xpal_Screen_To_Dest_Palette(0, 0, 255);
	fade_gbl->color_delay = delay;
	fade_gbl->color_sustain = sustain;
	fade_gbl->color_total = total_steps;
	fade_gbl->color_step = 0;

	s_fade_is_scene_transition = landru_render_is_scene_transition();
}

void xfade_Set_Fade_Color(uint8_t red, uint8_t green, uint8_t blue) {
	fade_gbl->fade_red = red;
	fade_gbl->fade_green = green;
	fade_gbl->fade_blue = blue;
}

int16_t xfade_Calc_Fade_Length(FadeWipeMode mode) {
	if (mode == FADE_WIPE_NONE)
		return 32;
	if (mode <= FADE_WIPE_INSTANT)
		return 1;
	if (mode == FADE_WIPE_SNAP_ON)
		return 16;
	return 32;
}

/* MODERN ADAPTATION: the task wrapper watches this serial so it can yield
 * immediately after an original TIE98 presentation call. */
static uint32_t fade_present_serial_gbl;

// FUNCTION: TIE98 0x4B82F0
void xfade_Fade_To_Video_Palette(int32_t current_time, int32_t end_time) {
	if (!fade_gbl->color_mode)
		return;

	/* Delay/sustain gating */
	bool should_fade = false;
	if (fade_gbl->color_sustain) {
		if (current_time == end_time) {
			if (fade_gbl->color_delay)
				fade_gbl->color_delay--;
			else
				should_fade = true;
		} else if (!fade_gbl->color_delay) {
			should_fade = true;
		}
	} else if (current_time == end_time) {
		if (fade_gbl->color_delay)
			fade_gbl->color_delay--;
		else
			should_fade = true;
	}

	if (!should_fade)
		return;

	int16_t step = fade_gbl->color_step;
	int16_t total = fade_gbl->color_total;
	int16_t half = total / 2;
	int16_t frac;
	uint8_t r = fade_gbl->fade_red;
	uint8_t g = fade_gbl->fade_green;
	uint8_t b = fade_gbl->fade_blue;

	switch (fade_gbl->color_mode) {
		case FADE_COLOR_CROSSFADE:
			if (step == half) {
				xpal_Dest_To_Screen_Palette(0, 0, 255);
				xpal_Put_Screen_Palette();
			}
			break;
		case FADE_COLOR_TWO_PHASE:
			if (step < half) {
				frac = (step << 8) / half;
				if (!frac)
					frac = 1;
				xpal_Build_Fade_Palette(2, 0, 255, frac, r, g, b);
			} else {
				if (step == half && landru_port_Present_Platform_Video()) {
					fade_present_serial_gbl++;
				}
				frac = ((step - half) << 8) / (total - half);
				if (!frac)
					frac = 1;
				xpal_Build_Fade_Palette(1, 0, 255, frac, r, g, b);
			}
			xpal_Put_Screen_Palette();
			break;
		case FADE_COLOR_TO_COLOR:
			frac = (step << 8) / total;
			if (!frac)
				frac = 1;
			xpal_Build_Fade_Palette(2, 0, 255, frac, r, g, b);
			xpal_Put_Screen_Palette();
			break;
		case FADE_COLOR_FROM_COLOR:
			frac = (step << 8) / total;
			if (!frac)
				frac = 1;
			xpal_Build_Fade_Palette(1, 0, 255, frac, r, g, b);
			xpal_Put_Screen_Palette();
			break;
		case FADE_COLOR_PAL_TO_PAL:
			frac = (step << 8) / total;
			if (!frac)
				frac = 1;
			/* Build_Palette_To_Palette ignores the target color — it
			 * lerps src_pal → dst_pal directly. The (0,0,0) trailing
			 * args are placeholder. Caller that wants a fade-to-black
			 * stages dst_pal = all-black before triggering. */
			xpal_Build_Fade_Palette(0, 0, 255, frac, 0, 0, 0);
			xpal_Put_Screen_Palette();
			break;
		default:
			break;
	}

	if (++fade_gbl->color_step > fade_gbl->color_total)
		fade_gbl->color_mode = FADE_COLOR_NONE;
}

// FUNCTION: TIE98 0x4B8810
void xfade_Fade_Copy_To_Video(Rect* bounds, FadeWipeMode mode, int16_t step, int16_t total,
							  int16_t is_not_last) {
	Rect r;
	int16_t left, top;

	if (xrect_Empty_Rect(bounds))
		return;

	int is_special = compute_wipe_rect(&r, bounds, mode, step, total, is_not_last, &left, &top);

	if (!xrect_Empty_Rect(&r)) {
		if (mode >= FADE_WIPE_RIGHT) {
			if (!is_special) {
#ifdef LANDRU_GAME_XW
				xcanvas_Copy_Screen_Portion_To_Video(left, top, r.left, r.top, r.right - r.left,
													 r.bottom - r.top);
#else
				xcanvas_Copy_Screen_Portion_To_Video(&r, left, top);
#endif
			}
			xdirty_Swap_Dirty_List(1);
		} else {
			xcanvas_Copy_Dirty_Screen_To_Video(&r);
			if (landru_port_Present_Platform_Video()) {
				fade_present_serial_gbl++;
			}
		}
	}
}

/* ------------------------------------------------------------------
 * Fade engine — fully task-driven.
 *
 * TIE98's explicit presentation calls remain yield points. Sustained fades
 * in both frontend modes additionally use the palette module's emulated VGA
 * retrace cadence.
 * Cursor restoration and Refresh_View run from the task's `end` callback;
 * normal frame presentation remains owned by the calling view task.
 *
 * Pushed via xfade_Push_Fade_To_Video_Screen_Task (or the
 * higher-level xcanvas_Push_Fade_Screen_To_Video_Task helper). The
 * caller-task yields after the push and resumes once the task
 * pops.
 * ------------------------------------------------------------------ */

typedef enum {
	FADE_PHASE_PALETTE = 0,
	FADE_PHASE_COPY,
	FADE_PHASE_CHANNELS,
	FADE_PHASE_ADVANCE,
	FADE_PHASE_VGA_WAIT,
} FadePhase;

typedef struct FadeTask {
	/* Clip rect stored by value so caller stack locals are safe to
	 * pass at push time (caller-task yields immediately; the Rect
	 * pointer would otherwise dangle). */
	Rect clip;
#ifdef LANDRU_GAME_XW
	Rect text_frame;
#endif
	int32_t start_time;
	int32_t end_time;
	int32_t k;
	int16_t is_dialog;
	int16_t channel;
	bool sustained;
	/* End-of-task work flags. The task's `end` callback runs these
	 * synchronously when the task pops, so the caller-task can yield
	 * after pushing without interleaving any post-fade work itself. */
	uint8_t end_cursor;    /* FadeEndCursorAction */
	bool end_refresh_view; /* xview_Refresh_View (sustained fades) */
	FadePhase phase;
} FadeTask;

static LandruTaskStepResult fade_task_step(void* self) {
	FadeTask* t = (FadeTask*)self;

	for (;;) {
		if (t->phase == FADE_PHASE_VGA_WAIT) {
			if (xpal_Next_VGA_Delay_Us() != UINT64_MAX)
				return LANDRU_TASK_STEP_YIELD;
			t->phase = FADE_PHASE_PALETTE;
		}

		if (t->k > t->end_time)
			return LANDRU_TASK_STEP_DONE;

		if (t->phase == FADE_PHASE_PALETTE) {
			uint32_t present_serial = fade_present_serial_gbl;
			xfade_Fade_To_Video_Palette(t->k, t->start_time);
			t->phase = FADE_PHASE_COPY;
			if (fade_present_serial_gbl != present_serial)
				return LANDRU_TASK_STEP_YIELD;
		}

		if (t->phase == FADE_PHASE_COPY) {
			bool full_active = false;
			if (fade_gbl->full_mode != FADE_WIPE_NONE) {
				if (t->k == t->start_time)
					full_active = fade_gbl->full_delay == 0;
				else if (!fade_gbl->full_delay && fade_gbl->full_sustain)
					full_active = true;
			}

			if (full_active) {
				uint32_t present_serial = fade_present_serial_gbl;
				xfade_Fade_Copy_To_Video(&t->clip, fade_gbl->full_mode, fade_gbl->full_step,
										 fade_gbl->full_total, t->k != t->end_time);
				if (++fade_gbl->full_step > fade_gbl->full_total)
					fade_gbl->full_mode = FADE_WIPE_NONE;
				t->phase = FADE_PHASE_ADVANCE;
				if (fade_present_serial_gbl != present_serial)
					return LANDRU_TASK_STEP_YIELD;
				continue;
			}

			if (fade_gbl->full_delay && fade_gbl->full_mode && t->k == t->start_time)
				fade_gbl->full_delay--;

			if (t->is_dialog) {
				if (!xrect_Empty_Rect(&t->clip))
					xcanvas_Copy_Dirty_Screen_To_Video(&t->clip);
				t->phase = FADE_PHASE_ADVANCE;
				continue;
			}

#ifdef LANDRU_GAME_XW
			draw_timed_text(t->start_time, &t->text_frame);
#endif
			t->channel = 0;
			t->phase = FADE_PHASE_CHANNELS;
		}

		if (t->phase == FADE_PHASE_CHANNELS) {
			while (t->channel < 4) {
				int16_t ch = t->channel++;
				Rect view_frame;
				bool chan_active = false;
				uint32_t present_serial;

				if (!xview_Is_View_Copy(ch))
					continue;
				xview_Get_View_Clip_Frame(ch, &view_frame);
				xrect_Clip_Rect(&view_frame, &t->clip);

				if (fade_gbl->chan_mode[ch]) {
					if (t->k == t->start_time)
						chan_active = fade_gbl->chan_delay[ch] == 0;
					else if (!fade_gbl->chan_delay[ch] && fade_gbl->chan_sustain[ch])
						chan_active = true;
				}

				if (chan_active) {
					present_serial = fade_present_serial_gbl;
					xfade_Fade_Copy_To_Video(&view_frame, fade_gbl->chan_mode[ch], fade_gbl->chan_step[ch],
											 fade_gbl->chan_total[ch], t->k != t->end_time);
					if (++fade_gbl->chan_step[ch] > fade_gbl->chan_total[ch])
						fade_gbl->chan_mode[ch] = FADE_WIPE_NONE;
					if (fade_present_serial_gbl != present_serial)
						return LANDRU_TASK_STEP_YIELD;
				} else {
					if (fade_gbl->chan_delay[ch] && fade_gbl->chan_mode[ch] && t->k == t->start_time)
						fade_gbl->chan_delay[ch]--;
					if (fade_gbl->chan_mode[ch] == FADE_WIPE_NONE && !xrect_Empty_Rect(&view_frame))
						xcanvas_Copy_Dirty_Screen_To_Video(&view_frame);
				}
			}
#ifdef LANDRU_GAME_XW
			/* Captions can lie outside the animated view rectangle. */
			Rect text_frame = t->text_frame;
			bool text_visible = xrect_Clip_Rect(&text_frame, &t->clip);
			if (text_visible)
				xcanvas_Copy_Screen_To_Video(&text_frame);
#endif
			t->phase = FADE_PHASE_ADVANCE;
#ifdef LANDRU_GAME_XW
			if (text_visible && t->sustained && landru_port_Present_Platform_Video()) {
				fade_present_serial_gbl++;
				return LANDRU_TASK_STEP_YIELD;
			}
#endif
		}

		if (t->phase == FADE_PHASE_ADVANCE) {
			t->k++;
			if (t->sustained && t->k <= t->end_time) {
				t->phase = FADE_PHASE_VGA_WAIT;
				return LANDRU_TASK_STEP_CONTINUE;
			}
			t->phase = FADE_PHASE_PALETTE;
		}
	}
}

static uint64_t fade_task_next_wake_delay_us(const void* self) {
	const FadeTask* t = (const FadeTask*)self;
	if (t->phase != FADE_PHASE_VGA_WAIT)
		return UINT64_MAX;
	return xpal_Next_VGA_Delay_Us();
}

static void fade_task_end(void* self) {
	FadeTask* t = (FadeTask*)self;

	/* Cursor restoration. The pre-fade cursor management was hidden
	 * inside xcanvas_Fade_Screen_To_Video; that helper is gone now,
	 * so the post-fade cursor-restore logic lives here and runs as
	 * the task pops. Callers that don't need cursor work pass
	 * FADE_END_CURSOR_NONE. */
	switch (t->end_cursor) {
		case FADE_END_CURSOR_TO_FRONT:
			xcursor_Cursor_To_Front();
			break;
		case FADE_END_CURSOR_FROM_FADE:
			xcursor_Cursor_From_Fade();
			break;
		default:
			break;
	}

	if (t->end_refresh_view)
		xview_Refresh_View();
}

static const LandruTaskVtable fade_task_vt = {
	.step = fade_task_step,
	.end = fade_task_end,
	.next_wake_delay_us = fade_task_next_wake_delay_us,
};

/* Push a FadeTask onto the tie_core task stack. Caller-task yields
 * (CONTINUE); when the task pops, its `end` callback runs the
 * post-fade housekeeping (cursor restoration, Refresh_View for
 * sustained fades). The caller-task picks back up cleanly on its
 * next step with no extra phases needed.
 *
 *   end_cursor     : FadeEndCursorAction (NONE / TO_FRONT / FROM_FADE).
 *                    Pre-fade cursor work (Cursor_To_Back) is the
 *                    caller's responsibility — runs synchronously
 *                    before the push.
 *   force_refresh_view : true → refresh the view on pop even when the
 *                    fade is not sustained.
 *
 * Returns 1 on success, 0 on task-stack overflow. */
int xfade_Push_Fade_To_Video_Screen_Task(const Rect* clip, int16_t is_dialog, FadeEndCursorAction end_cursor,
										 bool force_refresh_view) {
	int32_t now = xview_Get_View_Time();
	int32_t start_time = now;
	int32_t end_time = now;

	if (xfade_Is_Fade_Step(fade_gbl->full_mode, fade_gbl->full_sustain, fade_gbl->full_delay, now, now)) {
		if (fade_gbl->full_sustain)
			end_time = now + fade_gbl->full_total;
		for (int i = 0; i < 4; i++)
			fade_gbl->chan_mode[i] = FADE_WIPE_NONE;
	} else {
		for (int i = 0; i < 4; i++) {
			if (xfade_Is_Fade_Step(fade_gbl->chan_mode[i], fade_gbl->chan_sustain[i], fade_gbl->chan_delay[i],
								   start_time, start_time) &&
				fade_gbl->chan_sustain[i]) {
				if (start_time + fade_gbl->chan_total[i] > end_time)
					end_time = start_time + fade_gbl->full_total;
			}
		}
	}

	bool sustained = (end_time > start_time);

	FadeTask* t = (FadeTask*)landru_task_push(&fade_task_vt);
	if (!t)
		return 0;
#ifdef LANDRU_GAME_XW
	timed_text_background.pending = false;
#endif
	t->clip = *clip;
	t->start_time = start_time;
	t->end_time = end_time;
	t->k = start_time;
	t->is_dialog = is_dialog;
	t->sustained = sustained;
	t->end_cursor = (uint8_t)end_cursor;
	t->end_refresh_view = sustained || force_refresh_view;
	t->channel = 0;
	t->phase = FADE_PHASE_PALETTE;
	return 1;
}

void xfade_Calc_Fade_Lock_Time(int32_t* lock_end, int32_t* lock_start) {
	int32_t now = xview_Get_View_Time();
	int32_t latest_end = now;

	if (fade_gbl->full_mode && !fade_gbl->full_delay) {
		if (fade_gbl->full_sustain)
			latest_end = now + fade_gbl->full_total;
		for (int i = 0; i < 4; i++)
			fade_gbl->chan_mode[i] = FADE_WIPE_NONE;
	} else {
		for (int i = 0; i < 4; i++) {
			if (fade_gbl->chan_mode[i] && !fade_gbl->chan_delay[i] && fade_gbl->chan_sustain[i]) {
				if (now + fade_gbl->chan_total[i] > latest_end)
					latest_end = now + fade_gbl->full_total;
			}
		}
	}

	*lock_start = now;
	*lock_end = latest_end;
}

bool xfade_Is_Fade_Step(FadeWipeMode mode, int16_t sustain, int16_t delay, int32_t cur, int32_t end) {
	if (!mode)
		return false;
	if (cur != end)
		return !delay && sustain;
	return delay == 0;
}

bool xfade_Is_Fade_Delay_Step(FadeWipeMode mode, int16_t sustain, int32_t cur, int32_t end) {
	return sustain && mode && cur == end;
}

LandruBoolResult xfade_Fade_Active(void) {
	if (fade_gbl->full_mode && !fade_gbl->full_delay)
		return true;
	for (int i = 0; i < 4; i++)
		if (fade_gbl->chan_mode[i] && !fade_gbl->chan_delay[i])
			return true;
	return false;
}

bool xfade_Trans_Fade_Active(void) {
	if (fade_gbl->full_mode && !fade_gbl->full_delay) {
		if (fade_gbl->full_mode > FADE_WIPE_SNAP_OFF || fade_gbl->color_mode != FADE_COLOR_PAL_TO_PAL ||
			fade_gbl->full_sustain)
			return true;
	}
	for (int i = 0; i < 4; i++) {
		if (fade_gbl->chan_mode[i] && !fade_gbl->chan_delay[i]) {
			if (fade_gbl->chan_mode[i] > FADE_WIPE_SNAP_OFF ||
				fade_gbl->color_mode != FADE_COLOR_PAL_TO_PAL || fade_gbl->chan_sustain[i])
				return true;
		}
	}
	return false;
}

/* Publish a renderer-neutral fade factor and target color. Fade-out legs
 * preserve the prior frame while their source contribution falls; fade-in
 * legs render the current frame while their source contribution rises.
 * Generic palette interpolation cannot be represented by one color, so it
 * produces no fade event unless the target is black or a scene transition
 * requires a two-phase black bridge. */
void xfade_emit_render_state(void) {
	LandruFadeRenderState state = { 0 };
	LandruFadeRenderState* out = &state;
	out->kind = LANDRU_FADE_NONE;
	out->source_factor = 255;
	out->r = out->g = out->b = 0;
	out->freeze_frame = 0;

	if (!fade_module_gbl || !fade_gbl)
		goto publish;
	uint8_t cm = fade_gbl->color_mode;
	int16_t step = fade_gbl->color_step;
	int16_t total = fade_gbl->color_total;
	if (cm == FADE_COLOR_NONE || total <= 0)
		goto publish;
	if (step < 0)
		step = 0;
	if (step > total)
		step = total;

	/* Fraction of the source frame that remains, expressed 0..256 to match
	 * the engine's fractional
	 * arithmetic before scaling to 0..255. */
	int source_remaining_q8;
	bool freeze;
	uint8_t target_r = fade_gbl->fade_red;
	uint8_t target_g = fade_gbl->fade_green;
	uint8_t target_b = fade_gbl->fade_blue;

	switch (cm) {
		case FADE_COLOR_PAL_TO_PAL:
			if (xpal_Compare_Dest_Pal_Color(0, 255, 0, 0, 0)) {
				/* The destination palette identifies a fade to black. */
				source_remaining_q8 = ((int)(total - step) << 8) / total;
				freeze = true;
				/* Engine ignores fade_red/g/b for this mode; the actual
				 * fade target is dst_pal which we just verified is all-
				 * black. */
				target_r = target_g = target_b = 0;
				break;
			}
			if (s_fade_is_scene_transition) {
				/* Bridge a scene transition through black, preserving the old
				 * frame during the first half and using the new frame afterward. */
				int16_t half = total / 2;
				if (half <= 0) {
					source_remaining_q8 = 256;
					freeze = false;
					target_r = target_g = target_b = 0;
					break;
				}
				if (step < half) {
					source_remaining_q8 = ((int)(half - step) << 8) / half;
					freeze = true;
				} else {
					int16_t denom = total - half;
					source_remaining_q8 = (denom <= 0) ? 256 : ((int)(step - half) << 8) / denom;
					freeze = false;
				}
				target_r = target_g = target_b = 0;
				break;
			}
			/* A generic palette swap has no single-color representation. */
			goto publish;

		case FADE_COLOR_TO_COLOR:
			source_remaining_q8 = ((int)(total - step) << 8) / total;
			freeze = true;
			break;

		case FADE_COLOR_FROM_COLOR:
			source_remaining_q8 = ((int)step << 8) / total;
			freeze = false;
			break;

		case FADE_COLOR_TWO_PHASE: {
			int16_t half = total / 2;
			if (half <= 0) {
				source_remaining_q8 = 256;
				freeze = false;
				break;
			}
			/* Equal source and destination palettes identify the one-way variant
			 * of the two-phase fade. */
			if (xpal_Compare_Src_Dest_Palette()) {
				source_remaining_q8 = 0;
				freeze = true;
				break;
			}
			if (step < half) {
				source_remaining_q8 = ((int)(half - step) << 8) / half;
				freeze = true;
			} else {
				int16_t denom = total - half;
				source_remaining_q8 = (denom <= 0) ? 256 : ((int)(step - half) << 8) / denom;
				freeze = false;
			}
			break;
		}

		case FADE_COLOR_CROSSFADE:
			goto publish;

		default:
			goto publish;
	}

	if (source_remaining_q8 < 0)
		source_remaining_q8 = 0;
	if (source_remaining_q8 > 256)
		source_remaining_q8 = 256;

	out->kind = LANDRU_FADE_ACTIVE;
	out->source_factor = (uint8_t)((source_remaining_q8 * 255 + 128) / 256);
	out->r = target_r;
	out->g = target_g;
	out->b = target_b;
	out->freeze_frame = freeze ? 1 : 0;

publish:
	landru_render_fade(out);
}
