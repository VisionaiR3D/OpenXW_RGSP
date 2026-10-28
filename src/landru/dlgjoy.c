#include <landru/dlgjoy.h>

#include <stddef.h>

#include <landru/canvas.h>
#include <landru/cursor.h>
#include <landru/font.h>
#include <landru/inpattr.h>
#include <landru/io.h>
#include <landru/sound.h>
#include <landru/style.h>

#include "dialog_internal.h"

enum {
	CALIBRATE_CENTER,
	CALIBRATE_UPPER_LEFT,
	CALIBRATE_LOWER_RIGHT,
	CALIBRATE_RECENTER,
	CALIBRATE_FINISHED,
};

enum {
	CALIBRATE_WIDTH = 300,
	CALIBRATE_HEIGHT = 40,
	CALIBRATE_ALIGN_CENTER = 1,
	CALIBRATE_FONT = 0,
	CALIBRATE_TEXT_COLOR = 15,
	CALIBRATE_SHADOW_COLOR = 16,
};

static bool calibration_active;

static void calibration_draw(Input* input, Rect* frame, Rect* clip, int16_t refresh) {
	(void)clip;
	if (!refresh)
		return;
	const char* prompt = "Center Joystick and Press Button";
	if (input->var1 == CALIBRATE_UPPER_LEFT)
		prompt = "Move Joystick to Top/Left and Press Button";
	else if (input->var1 == CALIBRATE_LOWER_RIGHT)
		prompt = "Move Joystick to Bottom/Right and Press Button";
	xstyle_Style_Paint_Base(frame, 0);
	Rect shadow = *frame;
	xrect_Offset_Rect(&shadow, 1, 1);
	xfont_Print_Centered_Text(prompt, &shadow, CALIBRATE_FONT, CALIBRATE_SHADOW_COLOR);
	xfont_Print_Centered_Text(prompt, frame, CALIBRATE_FONT, CALIBRATE_TEXT_COLOR);
}

static void calibration_user(Input* input, int32_t context) {
	if (!context)
		xcursor_Hide_Cursor();
	if (xio_Any_Button() && !input->var2 && input->var1 < CALIBRATE_FINISHED) {
		/* Windows input is already normalized; the legacy sampled ranges
		 * have no consumer in X-Wing's joystick driver. */
		input->var1++;
		input->var2 = 1;
		xinpattr_Refresh_Input(input);
	}
	if (!xio_Any_Button() && xio_Any_Button_Release()) {
		input->var2 = 0;
		if (input->var1 == CALIBRATE_FINISHED) {
			Rect bounds;
			xcanvas_Get_Drawing_Canvas_Bounds(&bounds);
			xio_Set_Mouse_Position(bounds.right / 2, bounds.bottom / 2);
			xcursor_Show_Cursor();
			xdialog_Set_Dialog_Exit(LANDRU_JOYSTICK_CALIBRATE_DONE);
		}
	}
}

static Input* calibration_create(void) {
	if (calibration_active)
		return NULL;
	Rect frame;
	xrect_Set_Rect(&frame, 0, 0, CALIBRATE_WIDTH, CALIBRATE_HEIGHT);
	Input* input = xinput_Alloc_Dialog_Input(NULL, &frame, 0, 0);
	if (!input)
		return NULL;
	xinpattr_Set_Input_User_Function(input, calibration_user);
	xinpattr_Set_Input_Draw_Function(input, calibration_draw);
	xinpattr_Set_Input_Allign(input, CALIBRATE_ALIGN_CENTER, CALIBRATE_ALIGN_CENTER);
	xinpattr_Start_Input(input);
	input->var1 = CALIBRATE_CENTER;
	input->var2 = 0;
	calibration_active = true;
	xsound_Pause_Sounds();
	return input;
}

static void calibration_free(Input* input) {
	xinput_Free_Inputs(input);
	calibration_active = false;
	xsound_Resume_Sounds();
}

bool xdlgjoy_Push_Joystick_Callibrate_Task(void) {
	Input* input = calibration_create();
	return input && xdialog_Push_Owned_Dialog_Task(input, calibration_free);
}

bool xdlgjoy_Schedule_Joystick_Callibrate(DialogSubResultHandler handler, void* ctx) {
	Input* input = calibration_create();
	return input && xdialog_Schedule_Owned_Sub_Dialog(input, calibration_free, handler, ctx);
}
