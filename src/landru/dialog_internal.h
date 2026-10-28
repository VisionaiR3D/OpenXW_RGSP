#ifndef LANDRU_DIALOG_INTERNAL_H
#define LANDRU_DIALOG_INTERNAL_H

#include <landru/dialog.h>

#ifdef LANDRU_GAME_XW
/* Takes ownership even on failure. Cleanup runs after dismissal or cancellation;
 * result handlers run only when the parent resumes after the child. */
bool xdialog_Push_Owned_Dialog_Task(Input* list, DialogInputCleanup cleanup);
void xdialog_Cancel_Pending_Sub_Dialog(void);
#endif

#endif
