#ifndef LANDRU_BTNSLDR_H
#define LANDRU_BTNSLDR_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/input.h>
#include <landru/rect.h>

typedef struct {
	Input header;
	int16_t drag_offset;
	int16_t range_min;
	int16_t range_max;
	int16_t value;
	uint8_t is_vertical;
	uint8_t repeat_counter;
	uint8_t page_step;
	uint8_t action_mode;
} SliderButton;

void xbtnsldr_Create_Slider_Button_Module(void);
void xbtnsldr_Destroy_Slider_Button_Module(void);
SliderButton* xbtnsldr_Generate_Slider_Button(Input* parent, char* res_buffer, const char* name);
SliderButton* xbtnsldr_Alloc_Slider_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
										   uint8_t page_step, int16_t id);
void xbtnsldr_Get_Slider_Button_Range(SliderButton* btn, int16_t* out_min, int16_t* out_max);
void xbtnsldr_Set_Slider_Button_Range(SliderButton* btn, int16_t min, int16_t max);
int16_t xbtnsldr_Get_Slider_Button_Value(SliderButton* btn);
void xbtnsldr_Set_Slider_Button_Value(SliderButton* btn, int16_t value);
void xbtnsldr_Calc_Slider_Button_Rects(SliderButton* btn, Rect* frame, Rect* out_dec, Rect* out_inc,
									   Rect* out_track, Rect* out_thumb);
int16_t xbtnsldr_Calc_Slider_Thumb_Value(SliderButton* btn, Rect* track_rect, Rect* thumb_rect,
										 int16_t mouse_x, int16_t mouse_y);
void xbtnsldr_idraw_Slider_Button(Input* input, Rect* paint_rect, Rect* dirty_rect, int16_t refresh);
int16_t xbtnsldr_iupdate_Slider_Button(Input* input, Rect* frame, Rect* clip, int16_t phase, uint8_t mouse_l,
									   uint8_t mouse_r, int16_t x, int16_t y);

#endif
