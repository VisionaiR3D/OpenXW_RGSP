#ifndef LANDRU_DLG_H
#define LANDRU_DLG_H

#include <landru/dialog.h>

#ifdef LANDRU_GAME_XW
enum {
	CHOICES_FONT = 0,
	CHOICES_ROW_PADDING = 8,
	CHOICES_ROW_GAP = 2,
	CHOICES_WIDTH_PADDING = 64,
	CHOICES_MARGIN = 8,
	CHOICES_ALIGN_CENTER = 1
};

/* Borrows the double-NUL-terminated labels until dismissal. The handler
 * receives dialog exit (choice ID + 1); subtract one for the choice ID. */
bool xdlg_Schedule_Choices(const char* labels, DialogSubResultHandler handler, void* ctx);
#endif

#endif
