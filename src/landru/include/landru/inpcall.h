#ifndef LANDRU_INPCALL_H
#define LANDRU_INPCALL_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/input.h>
#include <landru/rect.h>

typedef enum {
	INPUT_ACTIVE_IDLE = 0,
	INPUT_ACTIVE_LEFT_DOWN = 1,
	INPUT_ACTIVE_RIGHT_DOWN = 2,
	INPUT_ACTIVE_LEFT_UP = 3,
	INPUT_ACTIVE_RIGHT_UP = 4,
} InputActiveState;

void xinpcall_Draw_Inputs(Input* list, Rect* parent_frame, Rect* parent_clip, int16_t refresh);
void xinpcall_Draw_Input_Layer(Input* list, Rect* parent_frame, Rect* parent_clip, int16_t refresh);
void xinpcall_Get_Input_Parent_Frame(Rect* parent_frame, Rect* parent_clip, Rect* out_frame, Rect* out_clip);
int16_t xinpcall_Clip_Input_To_Frame(Input* input, Rect* frame, Rect* clip);
void xinpcall_Update_Inputs(Input* root);
int16_t xinpcall_Update_Key_Inputs(Input* root, Rect* parent_frame, Rect* parent_clip, int16_t key,
								   int16_t mouse_x, int16_t mouse_y);
#ifdef LANDRU_GAME_XW
int16_t xinpcall_Update_Mouse_Down(Input* root, Rect* parent_frame, Rect* parent_clip,
								   InputMouseEvent mouse_l, InputMouseEvent mouse_r, int16_t mouse_x,
								   int16_t mouse_y);
#else
int16_t xinpcall_Update_Mouse_Down(Input* root, Rect* parent_frame, InputMouseEvent mouse_l,
								   Rect* parent_clip, InputMouseEvent mouse_r, int16_t mouse_x,
								   int16_t mouse_y);
#endif
int16_t xinpcall_Update_Mouse_Move(Input* root, Rect* parent_frame, Rect* parent_clip, int16_t mouse_x,
								   int16_t mouse_y);
#ifdef LANDRU_GAME_XW
void xinpcall_Update_Mouse_UpMove(Input* input, Rect* frame, Rect* clip, InputMouseEvent mouse_l,
								  InputMouseEvent mouse_r, int16_t mouse_x, int16_t mouse_y);
#else
void xinpcall_Update_Mouse_UpMove(Input* input, Rect* frame, Rect* clip, InputMouseEvent mouse_l,
								  InputMouseEvent mouse_r);
#endif
void xinpcall_User_Inputs(Input* root, int32_t context);
int16_t xinpcall_Fetch_Input_Buttons(InputMouseEvent* out_left, InputMouseEvent* out_right);
void xinpcall_Set_InputActive_Down(Input* input, Rect* frame, Rect* clip, InputMouseEvent mouse_l,
								   InputMouseEvent mouse_r);
int16_t xinpcall_Replace_InputActive(Input* input);
void xinpcall_Set_InputActive_Frame(Rect* frame, Rect* clip);
void xinpcall_Set_InputActive_Ignore(InputMouseEvent mouse_l, InputMouseEvent mouse_r);
void xinpcall_Set_InputActive_Up(InputMouseEvent mouse_l, InputMouseEvent mouse_r);
void xinpcall_Clear_Active_Input(void);
int16_t xinpcall_Find_View_Input_Frame(Input* target, Rect* out_frame, Rect* out_clip);
int16_t xinpcall_Find_Input_Frame(Input* root, Input* target, Rect* frame, Rect* clip);
Input* xinpcall_Check_Parent_Children_For_Input(Input* parent, Input* target);
Input* xinpcall_Find_View_Parent_For_Input(Input* target);
Input* xinpcall_Find_Parent_For_Input(Input* root, Input* target);
Input* xinpcall_Find_View_Input_With_ID(int16_t id);
Input* xinpcall_Find_Input_With_ID(Input* root, int16_t id);
#ifndef LANDRU_GAME_XW
void xinpcall_Select_View_Shared_Key_Input(Input* target);
void xinpcall_Select_Shared_Key_Input(Input* root, Input* target);

#endif

#endif
