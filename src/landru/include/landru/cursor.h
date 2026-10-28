#ifndef LANDRU_CURSOR_H
#define LANDRU_CURSOR_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef LANDRU_GAME_XW
#include <landru/actor.h>
#endif

void xcursor_Create_Cursor_Module(void);
void xcursor_Destroy_Cursor_Module(void);
void xcursor_Set_Cursor_HotSpot(int16_t hot_x, int16_t hot_y);
void xcursor_Get_Cursor_HotSpot(int16_t* out_x, int16_t* out_y);
#ifdef LANDRU_GAME_XW
enum {
	CURSOR_ACTOR_MAX_SIZE = 32,
	CURSOR_DEFAULT_FRAME = 0,
	CURSOR_DEFAULT_HOTSPOT = 1,
	CURSOR_SPECIAL_FRAME = 3,
	CURSOR_SPECIAL_HOT_X = 10,
	CURSOR_SPECIAL_HOT_Y = 5
};

void xcursor_Set_Default_Cursor_Actor(Actor* actor);
void xcursor_Set_Cursor(int16_t cursorId);
#else
void xcursor_Set_Cursor(uint8_t cursorId);
#endif
void xcursor_Push_Cursor_Canvas(void);
void xcursor_Pop_Cursor_Canvas(void);
void xcursor_Set_Cursor_Canvas(void);
void xcursor_Cursor_To_Screen(int16_t x, int16_t y);
void xcursor_Draw_Cursor(int16_t x, int16_t y, int16_t offscreen);
void xcursor_Erase_Cursor(int16_t offscreen);
void xcursor_Cursor_To_Back(void);
void xcursor_Cursor_To_Front(void);
void xcursor_Cursor_From_Fade(void);
bool xcursor_Is_Cursor(void);
LandruBoolResult xcursor_Is_Cursor_Visible(void);
void xcursor_Enable_Cursor(void);
void xcursor_Disable_Cursor(void);
void xcursor_Refresh_Cursor(void);
void xcursor_Show_Cursor(void);
void xcursor_Hide_Cursor(void);

/* PORT: selects a presentation-owned cursor without changing Landru's
 * visibility, input, hotspot or cursor-kind state. */
void xcursor_port_Set_External_Presentation(bool external);

extern int g_softwareCursorEnabled;

int xcursor_Get_Display_Count(void);
void xcursor_Draw_Software_Cursor_To_Surface(uint8_t* surface, int pitch, int height);
void xcursor_Select_Contrast_Colors(const uint8_t* rgb6_palette);

/* Publish the current cursor pose to the optional render sink. */
void xcursor_emit_render_state(void);

/* Read-only access to the cursor's current 8bpp bitmap. `*out_w / *out_h`
 * give the bitmap dimensions;
 * data is `out_w * out_h` bytes of palette indices (8bpp). Returns
 * NULL if no cursor is loaded. The returned pointer is valid until the
 * next xcursor_Set_Cursor or module shutdown. */
const uint8_t* xcursor_get_bitmap(int16_t* out_w, int16_t* out_h);

#ifdef LANDRU_MODERN
/* Read-only 16x16 software arrow: mask 0 is transparent, 1 dark, 2 bright.
 * The mask has static lifetime. Returns false while its module/cursor is hidden.
 * Colors follow the last Select_Contrast_Colors call; this is distinct from
 * the actor cursor returned by xcursor_get_bitmap. */
typedef struct LandruSoftwareCursor {
	const uint8_t* mask;
	int width, height;
	uint8_t dark_index, bright_index;
} LandruSoftwareCursor;

bool xcursor_Get_Software_Cursor(LandruSoftwareCursor* out);

/* Draw the shared cursor bitmap at a host-sampled pose without changing input state. */
void xcursor_Draw_Software_Cursor_At(uint8_t* surface, int pitch, int height, int x, int y);
#endif

#endif
