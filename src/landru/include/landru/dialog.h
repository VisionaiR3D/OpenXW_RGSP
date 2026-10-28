#ifndef LANDRU_DIALOG_H
#define LANDRU_DIALOG_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/input.h>
#include <landru/rect.h>
#include <landru/view.h>

typedef void (*DialogUpdateFunc)(int32_t frame);

void xdialog_Create_Dialog_Module(void);
void xdialog_Destroy_Dialog_Module(void);
#ifdef LANDRU_GAME_XW
void xdialog_Add_Dialog_To_System(ViewStruct* view, Input* list);
void xdialog_Remove_Dialog_From_System(ViewStruct* view, Input* ptr);
void xdialog_Draw_System_Dialogs(void);
#else
void xdialog_Add_Dialog_To_System(Input* list);
void xdialog_Remove_Dialog_From_System(Input* ptr);
void xdialog_Draw_System_Dialogs(int16_t refresh);
#endif

/* Push the dialog task on the tie_core stack and run the prologue
 * setup; caller-task should yield (CONTINUE) and read
 * xdialog_Get_Dialog_Exit() on the next step after our task pops.
 * From inside an input callback, use xdialog_Schedule_Sub_Dialog
 * instead. */
void xdialog_Push_Dialog_View_Task(Input* list);
void xdialog_Update_Dialog_End_View(int32_t frame);
void xdialog_Save_Dialog_Background(void);

/* --- Deferred sub-dialog channel ---------------------------------
 *
 * Input callbacks (iuser/iupdate) run synchronously inside the owning
 * DialogTask::step and cannot push tasks themselves: returning from
 * the callback returns to xinpcall_User_Inputs, then to the
 * DialogTask step, with no provision for resuming after a yielded
 * sub-task.
 *
 * To open a modal sub-dialog from inside a callback, the callback
 * stages a request via xdialog_Schedule_Sub_Dialog. After the
 * callback batch returns, the DialogTask picks up the request:
 * pushes the sub-dialog as a nested DialogTask and yields. When the
 * sub-dialog pops, `handler(result, ctx)` runs synchronously to
 * execute the post-result logic that used to follow the inline
 * synchronous xdialog_Handle_Dialog_View call.
 *
 * The channel is single-slot. A new request replaces an unstarted request;
 * in X-Wing, any owned inputs from the replaced request are released. */
typedef void (*DialogSubResultHandler)(int16_t result, void* ctx);

#ifdef LANDRU_GAME_XW
typedef void (*DialogInputCleanup)(Input* list);

/* Takes ownership even on failure. Cleanup runs after dismissal or cancellation;
 * result handlers run only when the parent resumes after the child. */
bool xdialog_Schedule_Owned_Sub_Dialog(Input* list, DialogInputCleanup cleanup,
									   DialogSubResultHandler handler, void* ctx);
#endif

void xdialog_Schedule_Sub_Dialog(Input* sub_dlg, DialogSubResultHandler handler, void* ctx);

/* Drain primitives for owning task steps (DialogTask, ViewAddTask).
 * After processing a callback batch, the owning step calls
 * xdialog_Try_Push_Pending_Sub_Dialog: if a callback staged a
 * request, the sub-dialog is pushed onto the tie_core task stack and
 * the function returns 1. The caller retains the returned handler and
 * context while suspended, then invokes them after the sub-dialog pops. */
int xdialog_Try_Push_Pending_Sub_Dialog(DialogSubResultHandler* out_handler, void** out_ctx);
void xdialog_Invoke_Sub_Handler(DialogSubResultHandler handler, void* ctx);
void xdialog_Clear_Dialog_Exit(void);
int16_t xdialog_Get_Dialog_Exit(void);
void xdialog_Set_Dialog_Exit(int16_t code);
bool xdialog_Is_Dialog_Exit(void);
bool xdialog_Is_Dialog_Running(void);
LandruBoolResult xdialog_Is_Active_Dialog(void);
void xdialog_Set_Dialog_Update_Function(DialogUpdateFunc fn);
DialogUpdateFunc xdialog_Get_Dialog_Update_Function(void);
void xdialog_Clear_Dialog_Update_Function(void);
void xdialog_Refresh_Active_Dialog(void);

#endif
