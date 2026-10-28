#ifndef LANDRU_BTNTEXT_H
#define LANDRU_BTNTEXT_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/input.h>
#include <landru/rect.h>

typedef struct {
	Input header;
	char* name;
	int16_t text_color;
} TextButton;

void xbtntext_Create_Text_Button_Module(void);
void xbtntext_Destroy_Text_Button_Module(void);
TextButton* xbtntext_Generate_Text_Button(Input* parent, char* res_buffer, const char* name);
TextButton* xbtntext_Alloc_Text_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
									   const char* name, int16_t id);
TextButton* xbtntext_Alloc_Frame_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
										const char* name, int16_t id);
int16_t xbtntext_Free_Text_Button(TextButton* btn);
void xbtntext_Init_Text_Button(TextButton* btn, InputUserFunc callback, const char* name, int16_t id);
void xbtntext_idraw_Text_Button(TextButton* btn, Rect* paint_rect, Rect* dirty_rect, int16_t refresh);
void xbtntext_Set_Text_Button_Name(TextButton* btn, const char* name);
void xbtntext_Get_Text_Button_Name(TextButton* btn, char* out);
void xbtntext_Set_Text_Button_Color(TextButton* btn, int16_t color);
int16_t xbtntext_Get_Text_Button_Color(TextButton* btn);

#endif
