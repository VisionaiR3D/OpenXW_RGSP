#ifndef LANDRU_BTNCHK_H
#define LANDRU_BTNCHK_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/input.h>
#include <landru/rect.h>

typedef struct {
	Input header;
	char* name;
	int16_t text_color;
	int16_t state; /* 0 = unchecked, 1 = checked */
	int16_t pressed;
} CheckButton;

void xbtnchk_Create_Check_Button_Module(void);
void xbtnchk_Destroy_Check_Button_Module(void);
CheckButton* xbtnchk_Generate_Check_Button(Input* parent, char* res_buffer, const char* name);
CheckButton* xbtnchk_Alloc_Check_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
										const char* name, int16_t id);
int16_t xbtnchk_Free_Check_Button(CheckButton* btn);
void xbtnchk_Init_Check_Button(CheckButton* btn, InputUserFunc callback, const char* name, int16_t id);
void xbtnchk_idraw_Check_Button(Input* input, Rect* paint_rect, Rect* dirty_rect, int16_t refresh);
int16_t xbtnchk_iupdate_Check_Button(Input* input, Rect* paint_rect, Rect* clip, int16_t phase,
									 uint8_t mouse_l, uint8_t mouse_r, int16_t x, int16_t y);
void xbtnchk_Set_Check_Button_Name(CheckButton* btn, const char* name);
void xbtnchk_Get_Check_Button_Name(CheckButton* btn, char* out);
void xbtnchk_Set_Check_Button_State(CheckButton* btn, int16_t state);
int16_t xbtnchk_Get_Check_Button_State(CheckButton* btn);

#endif
