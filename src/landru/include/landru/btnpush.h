#ifndef LANDRU_BTNPUSH_H
#define LANDRU_BTNPUSH_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/input.h>
#include <landru/rect.h>

typedef struct {
	Input header;
#ifdef LANDRU_GAME_XW
	const char* labels;
	int16_t labelIndex;
#else
	char* name;
#endif
	int16_t pressed;
} PushButton;

void xbtnpush_Create_Button_Module(void);
void xbtnpush_Destroy_Button_Module(void);
#ifndef LANDRU_GAME_XW
PushButton* xbtnpush_Generate_Button(Input* parent, char* res_buffer, const char* name);
#endif
PushButton* xbtnpush_Alloc_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
								  const char* name, int16_t id);
PushButton* xbtnpush_Alloc_Small_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
										const char* name, int16_t id);
#ifndef LANDRU_GAME_XW
int16_t xbtnpush_Free_Button(PushButton* btn);
#endif
void xbtnpush_Init_Button(PushButton* btn, InputUserFunc callback, const char* name, int16_t id);
void xbtnpush_idraw_Button(Input* input, Rect* paint_rect, Rect* dirty_rect, int16_t do_draw);
int16_t xbtnpush_iupdate_Button(Input* input, Rect* paint_rect, Rect* clip, int16_t phase,
								InputMouseEvent mouse_l, InputMouseEvent mouse_r, int16_t x, int16_t y);
void xbtnpush_Set_Button_Name(PushButton* btn, const char* name);
#ifndef LANDRU_GAME_XW
void xbtnpush_Get_Button_Name(PushButton* btn, char* out);
#endif

#ifdef LANDRU_GAME_XW
void xbtnpush_idraw_SmallButton(Input* input, Rect* frame, Rect* clip, int16_t refresh);
void xbtnpush_Set_Button_LabelIndex(PushButton* btn, int16_t index);
#endif

#endif
