#ifndef LANDRU_VIEWADD_H
#define LANDRU_VIEWADD_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/rect.h>

bool xviewadd_Clip_Object_To_View(int16_t viewId, int16_t zPlane, Rect* actFrame, Rect* outFull,
								  Rect* outClipped);

/* Push the view modal-loop task onto the tie_core task stack. Caller
 * must be a task itself; the runner will step the view task next
 * tick and dispatch back to the caller when the view exits. The
 * caller reads xerror_Get_Landru_Exit() for the next-scene id once
 * the view task pops. */
void xviewadd_Push_Handle_View_Task(void);
void xviewadd_Clear_View(void);
#ifdef LANDRU_GAME_XW
void xviewadd_Draw_View(void);
void xviewadd_Redraw_View(void);
#else
void xviewadd_Draw_View(int16_t refresh);
#endif
void xviewadd_Draw_View_Under_Dialog(void);
void xviewadd_Draw_View_Debug(int32_t time);
void xviewadd_Update_View(int32_t time);
void xviewadd_User_View(int32_t time);
void xviewadd_Update_End_View(void);

#endif
