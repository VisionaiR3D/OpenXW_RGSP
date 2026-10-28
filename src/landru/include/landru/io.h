#ifndef LANDRU_IO_H
#define LANDRU_IO_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/rect.h>

#ifdef LANDRU_GAME_XW
typedef int32_t LandruSystemSpeed;

enum { LANDRU_SYSTEM_SPEED_DEFAULT = 2 };
#else
typedef uint8_t LandruSystemSpeed;

enum { LANDRU_SYSTEM_SPEED_DEFAULT = 4 };
#endif

extern Rect mouse_limits_gbl;
extern int16_t joy_port_gbl;
extern int16_t mouse_x_gbl;
extern int16_t mouse_y_gbl;
extern int16_t mouse_cursor_x_gbl;
extern int16_t mouse_cursor_y_gbl;
extern int16_t fast_mouse_x_gbl;
extern int16_t fast_mouse_y_gbl;
extern int16_t last_mouse_x_gbl;
extern int16_t last_mouse_y_gbl;
extern int16_t joy_move_gbl;
extern int16_t joy_xf_gbl;
extern int16_t joy_yf_gbl;
extern int16_t down_button_gbl;
extern int16_t fast_button_gbl;
extern bool fast_mouse_release_gbl;
extern bool fast_joy_release_gbl;
extern int16_t last_button_gbl;
extern int16_t release_button_gbl;
extern int16_t key_gbl;
extern bool ask_key_gbl;
extern int16_t special_key_gbl;
extern bool key_btn_gbl;
extern int32_t cursor_time_gbl;
extern int16_t cursor_rate_gbl;
extern LandruSystemSpeed system_speed_gbl;
extern bool io_module_gbl;
extern bool mouse_exists_gbl;
extern int16_t joy_exists_gbl;
#ifdef LANDRU_GAME_XW
extern int16_t joy_callibrate_gbl;
int16_t xio_Is_Joystick_Callibrate(void);
int16_t xio_Test_Joystick_Callibrate(void);
#endif

void xio_Create_IO_Module(void);
void xio_Destroy_IO_Module(void);
LandruBoolResult xio_Is_Mouse_Input(void);
LandruBoolResult xio_Is_Joystick_Input(void);

void xio_Flush_Input(void);
int xio_Poll_Input(void);
void xio_Poll_Fast_Input(void);
void xio_Poll_Fast_Mouse_Input(int16_t* outX, int16_t* outY, int16_t* outBtn);
void xio_Poll_Fast_Joystick_Input(int16_t* outX, int16_t* outY, int16_t* outBtn);
void xio_Calc_Mouse_Pos(int16_t* outX, int16_t* outY, int16_t* outBtn);
void xio_Set_Mouse_Position(int16_t x, int16_t y);
void xio_Set_Mouse_Limits(Rect* rect);
void xio_Get_Mouse_Limits(Rect* out);
int16_t xio_Any_Button(void);
int16_t xio_Left_Button(void);
int16_t xio_Right_Button(void);
int16_t xio_Any_Button_Release(void);
int16_t xio_Left_Button_Release(void);
int16_t xio_Right_Button_Release(void);
int16_t xio_Button_Changed(void);
int16_t xio_Mouse_X(void);
int16_t xio_Mouse_Y(void);
int16_t xio_Mouse_Moved(void);
int16_t xio_Input_Changed(void);
int16_t xio_Get_Key(void);
int16_t xio_Get_Free_Key(void);
int16_t xio_Wait_For_Key(void);
void xio_Clear_Key(void);
void xio_Set_Key_Buttons(void);
void xio_Clear_Key_Buttons(void);
LandruBoolResult xio_Is_Key_Buttons(void);
bool xio_Is_Insert_Key_Down(void);
bool xio_Is_Caps_Lock_Down(void);
bool xio_Is_Num_Lock_Down(void);
bool xio_Is_Scroll_Lock_Down(void);
bool xio_Is_Alt_Key_Down(void);
bool xio_Is_Ctrl_Key_Down(void);
bool xio_Is_Shift_Key_Down(void);
bool xio_Is_Left_Shift_Key_Down(void);
bool xio_Is_Right_Shift_Key_Down(void);
LandruBoolResult xio_Blink(void);
bool xio_Fast_Blink(void);
void xio_Set_System_Speed(LandruSystemSpeed speed);
bool xio_Is_System_Fast_Enough(uint8_t level);
LandruBoolResult xio_Is_System_Slower_Than(LandruSystemSpeed speed);
int xio_Special_Keys(void);
int xio_Wait_Key(void);
int xio_Test_Key(void);
void xio_Set_Screenshot_Hook(void (*hook)(void));

#endif
