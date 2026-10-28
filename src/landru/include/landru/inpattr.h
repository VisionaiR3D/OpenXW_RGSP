#ifndef LANDRU_INPATTR_H
#define LANDRU_INPATTR_H

#include <landru/config.h>

#include <stdint.h>

#include <landru/input.h>
#include <landru/rect.h>

void xinpattr_Show_Input(Input* input);
void xinpattr_Hide_Input(Input* input);
int16_t xinpattr_Is_Input_Visible(Input* input);

void xinpattr_Activate_Input(Input* input);
void xinpattr_Deactivate_Input(Input* input);
int16_t xinpattr_Is_Input_Active(Input* input);

void xinpattr_Start_Input(Input* input);
void xinpattr_Stop_Input(Input* input);

#ifndef LANDRU_GAME_XW
void xinpattr_Dirty_Input(Input* input);
void xinpattr_Non_Dirty_Input(Input* input);
InputFlagResult xinpattr_Is_Input_Dirty(Input* input);

#endif
void xinpattr_Refresh_Input(Input* input);
void xinpattr_Non_Refresh_Input(Input* input);
InputFlagResult xinpattr_Is_Input_Refresh(Input* input);

void xinpattr_Refreshable_Input(Input* input);
void xinpattr_Non_Refreshable_Input(Input* input);
InputFlagResult xinpattr_Is_Input_Refreshable(Input* input);

void xinpattr_Discard_Input_Data(Input* input);
void xinpattr_Non_Discard_Input_Data(Input* input);
InputFlagResult xinpattr_Is_Discard_Input_Data(Input* input);

void xinpattr_Selected_Input(Input* input);
void xinpattr_Non_Selected_Input(Input* input);
InputFlagResult xinpattr_Is_Input_Selected(Input* input);
int16_t xinpattr_Get_Input_Selected(Input* input);

void xinpattr_Set_Input_Flag1(Input* input);
void xinpattr_Clear_Input_Flag1(Input* input);
InputFlagResult xinpattr_Is_Input_Flag1(Input* input);

void xinpattr_Set_Input_Flag2(Input* input);
void xinpattr_Clear_Input_Flag2(Input* input);
InputFlagResult xinpattr_Is_Input_Flag2(Input* input);

void xinpattr_Set_Input_Frame(Input* input, Rect* src);
void xinpattr_Get_Input_Frame(Input* input, Rect* dst);

void xinpattr_Set_Input_ZInput(Input* input, int16_t zinput);
int16_t xinpattr_Get_Input_ZInput(Input* input, uint16_t* out);

void xinpattr_Set_Input_Allign(Input* input, InputAlignment halign, InputAlignment valign);
void xinpattr_Get_Input_Allign(Input* input, InputAlignment* out_h, InputAlignment* out_v);

void xinpattr_Set_Input_Draw_Function(Input* input, InputDrawFunc fn);
void xinpattr_Get_Input_Draw_Function(Input* input, InputDrawFunc* out);
void xinpattr_Set_Input_Update_Function(Input* input, InputUpdateFunc fn);
void xinpattr_Get_Input_Update_Function(Input* input, InputUpdateFunc* out);
void xinpattr_Set_Input_User_Function(Input* input, InputUserFunc fn);
void xinpattr_Get_Input_User_Function(Input* input, InputUserFunc* out);

#ifndef LANDRU_GAME_XW
void xinpattr_Set_Input_Window(Input* input);
void xinpattr_Clear_Input_Window(Input* input);
InputFlagResult xinpattr_Is_Input_Window(Input* input);

void xinpattr_Set_Input_Window_Active(Input* input);
void xinpattr_Clear_Input_Window_Active(Input* input);
InputFlagResult xinpattr_Is_Input_Window_Active(Input* input);

void xinpattr_Activate_Window(Input* input);
#endif
void xinpattr_Move_Input(Input* input, int16_t x, int16_t y);

#ifndef LANDRU_GAME_XW
void xinpattr_Check_Input_ZInputs(void);
void xinpattr_Sort_Input_ZInputs(void);
#endif
Input* xinpattr_Sort_Input_ZInput_List(Input** head);

#endif
