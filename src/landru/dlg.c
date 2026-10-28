#include <landru/dlg.h>

#include <stddef.h>
#include <string.h>

#include <landru/btnpush.h>
#include <landru/font.h>
#include <landru/inpattr.h>
#include <landru/style.h>

#include "dialog_internal.h"

static void choices_draw(Input* input, Rect* frame, Rect* clip, int16_t refresh) {
	(void)input;
	(void)clip;
	if (refresh)
		xstyle_Style_Paint_Alert_Base(frame, 0);
}

static void choice_user(Input* input, int32_t context) {
	(void)context;
	if (xinpattr_Get_Input_Selected(input))
		xdialog_Set_Dialog_Exit(input->id + 1);
}

bool xdlg_Schedule_Choices(const char* labels, DialogSubResultHandler handler, void* ctx) {
	int16_t width = 0;
	int16_t height = 0;
	int16_t rowHeight = xfont_Get_FontID_Height(CHOICES_FONT) + CHOICES_ROW_PADDING;
	for (size_t offset = 0; labels[offset]; offset += strlen(labels + offset) + 1) {
		int16_t labelWidth = xfont_Get_String_Width_0(CHOICES_FONT, labels + offset);
		if (labelWidth > width)
			width = labelWidth;
		height += rowHeight + CHOICES_ROW_GAP;
	}
	Rect frame;
	xrect_Set_Rect(&frame, 0, 0, width + CHOICES_WIDTH_PADDING, height + rowHeight + CHOICES_MARGIN * 2);
	Input* dialog = xinput_Alloc_Dialog_Input(NULL, &frame, 0, 0);
	if (!dialog)
		return false;
	xinpattr_Set_Input_Draw_Function(dialog, choices_draw);
	xinpattr_Set_Input_Allign(dialog, CHOICES_ALIGN_CENTER, CHOICES_ALIGN_CENTER);
	xrect_Inset_Rect(&frame, CHOICES_MARGIN, CHOICES_MARGIN);
	frame.bottom = frame.top + rowHeight;
	int16_t id = 1;
	for (size_t offset = 0; labels[offset]; offset += strlen(labels + offset) + 1, ++id) {
		if (!xbtnpush_Alloc_Button(dialog, &frame, 0, choice_user, labels + offset, id)) {
			xinput_Free_Inputs(dialog);
			return false;
		}
		xrect_Offset_Rect(&frame, 0, rowHeight + CHOICES_ROW_GAP);
	}
	if (!xbtnpush_Alloc_Button(dialog, &frame, 0, choice_user, "Exit", 0)) {
		xinput_Free_Inputs(dialog);
		return false;
	}
	return xdialog_Schedule_Owned_Sub_Dialog(dialog, xinput_Free_Inputs, handler, ctx);
}
