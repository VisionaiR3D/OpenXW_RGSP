#ifndef LANDRU_DLGJOY_H
#define LANDRU_DLGJOY_H

#include <landru/config.h>

#ifdef LANDRU_GAME_XW
#include <landru/dialog.h>

enum { LANDRU_JOYSTICK_CALIBRATE_KEY = 0x2E00, LANDRU_JOYSTICK_CALIBRATE_DONE = 1 };

/* Returns whether the task was pushed. Yield and read xdialog_Get_Dialog_Exit
 * after it finishes. Input callbacks must use the deferred entry point. */
bool xdlgjoy_Push_Joystick_Callibrate_Task(void);

/* Returns whether the request was queued. The handler receives the result
 * after dismissal; cancellation frees the dialog without invoking it. */
bool xdlgjoy_Schedule_Joystick_Callibrate(DialogSubResultHandler handler, void* ctx);
#endif

#endif
