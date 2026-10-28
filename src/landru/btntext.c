#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <landru/btntext.h>
#include <landru/dirty.h>
#include <landru/font.h>
#include <landru/inpattr.h>
#include <landru/memptr.h>
#include <landru/paint.h>
#include <landru/rect.h>
#include <landru/style.h>

#include "binio.h"
#include <landru/fourcc.h>

// GLOBAL: TIE 0xD3024
static bool text_button_module_gbl;

void xbtntext_Create_Text_Button_Module(void) {
	xinput_Create_Input_Type(FOURCC_TEXT, (void*)xbtntext_Generate_Text_Button,
							 (void*)xbtntext_Free_Text_Button);
	xinput_Create_Input_Type(FOURCC_FRAM, (void*)xbtntext_Generate_Text_Button,
							 (void*)xbtntext_Free_Text_Button);
	text_button_module_gbl = true;
}

void xbtntext_Destroy_Text_Button_Module(void) {
	if (text_button_module_gbl) {
		xinput_Destroy_Input_Type(FOURCC_FRAM);
		xinput_Destroy_Input_Type(FOURCC_TEXT);
		text_button_module_gbl = false;
	}
}

TextButton* xbtntext_Generate_Text_Button(Input* parent, char* res_buffer, const char* name) {
	InputResourceHeader hdr;
	InputResourceHeader_decode(&hdr, (const uint8_t*)res_buffer);

	TextButton* btn = xbtntext_Alloc_Text_Button(parent, &hdr.frame, hdr.zinput, NULL, name, hdr.id);
	if (!btn)
		return NULL;

	btn->header.alignment = hdr.window_flags;
	btn->header.flags = hdr.state_flags | INPUT_REFRESH;
	btn->header.var1 = hdr.exit_pending;
	btn->header.var2 = hdr.exit_code;
	btn->header.type = hdr.res_type;

	const uint8_t* ext = (const uint8_t*)res_buffer + INPUTRESOURCEHEADER_DISK_SIZE;
	if (br_i16le(ext + 0))
		btn->text_color = br_i16le(ext + 2);

	return btn;
}

TextButton* xbtntext_Alloc_Text_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
									   const char* name, int16_t id) {
	TextButton* btn =
		(TextButton*)xinput_Alloc_Dialog_Input(parent, frame, zinput, sizeof(TextButton) - sizeof(Input));
	if (btn) {
		xbtntext_Init_Text_Button(btn, callback, name, id);
		btn->header.type = FOURCC_TEXT;
	}
	return btn;
}

TextButton* xbtntext_Alloc_Frame_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
										const char* name, int16_t id) {
	TextButton* btn =
		(TextButton*)xinput_Alloc_Dialog_Input(parent, frame, zinput, sizeof(TextButton) - sizeof(Input));
	if (btn) {
		xbtntext_Init_Text_Button(btn, callback, name, id);
		btn->header.type = FOURCC_FRAM;
	}
	return btn;
}

int16_t xbtntext_Free_Text_Button(TextButton* btn) {
	if (btn->name) {
		free(btn->name);
		btn->name = NULL;
	}
	return 1;
}

void xbtntext_Init_Text_Button(TextButton* btn, InputUserFunc callback, const char* name, int16_t id) {
	xinpattr_Set_Input_Draw_Function(&btn->header, (InputDrawFunc)xbtntext_idraw_Text_Button);
	/* No updateFunc — text buttons are display-only, not interactive */
	xinpattr_Set_Input_User_Function(&btn->header, callback);
	btn->header.mouseUsage = downMoveUpInput;
	btn->header.id = id;

	if (name)
		btn->name = xmemptr_Duplicate_String(name);
	else
		btn->name = NULL;

	btn->text_color = 26;
}

void xbtntext_idraw_Text_Button(TextButton* btn, Rect* paint_rect, Rect* dirty_rect, int16_t refresh) {
	if (!refresh)
		return;

	/* Clear background */
	int16_t base_color = xstyle_Get_Style_Base_Color();
	xpaint_Paint_Clipped_Rect(paint_rect, base_color);

	/* Save and set font 0 */
	uint16_t saved_font = xfont_Get_Font();
	xfont_Set_Font(0);

	int16_t font_h = xfont_Get_FontID_Height(0);

	/* FRAM mode: draw a 1px frame, offset top by half font height */
	if (btn->header.type == FOURCC_FRAM) {
		if (btn->name) {
			int16_t top_offset = font_h >> 1;
			paint_rect->top += top_offset;
			xpaint_Frame_Clipped_Rect(paint_rect, btn->text_color);
			paint_rect->top -= top_offset;
		} else {
			xpaint_Frame_Clipped_Rect(paint_rect, btn->text_color);
		}
	}

	/* Draw name text centered */
	Rect dst;
	xrect_Copy_Rect(&dst, paint_rect);

	if (btn->name) {
		if (btn->header.type == FOURCC_FRAM) {
			/* Clear a gap behind the centered text label */
			int16_t str_w = xfont_Get_String_Width(btn->name);
			dst.bottom = dst.top + font_h;
			dst.left = dst.left + (dst.right - dst.left - str_w) / 2 - 2;
			dst.right = dst.left + str_w + 4;
			int16_t gap_color = xstyle_Get_Style_Base_Color();
			xpaint_Paint_Clipped_Rect(&dst, gap_color);
		}

		int16_t saved_color = xfont_Get_Font_Color();
		xfont_Set_Font_Color(btn->text_color);
		xfont_Draw_Alligned_Text(btn->name, &dst, 1, 1);
		xfont_Set_Font_Color(saved_color);
	}

	xfont_Set_Font(saved_font);

	if (xinpattr_Is_Input_Dirty(&btn->header))
		xdirty_Dirty_Rect(dirty_rect);
}

void xbtntext_Set_Text_Button_Name(TextButton* btn, const char* name) {
	free(btn->name);
	btn->name = xmemptr_Duplicate_String(name);
	xinpattr_Refresh_Input(&btn->header);
}

void xbtntext_Get_Text_Button_Name(TextButton* btn, char* out) {
	if (btn->name)
		strcpy(out, btn->name);
	else
		*out = '\0';
}

void xbtntext_Set_Text_Button_Color(TextButton* btn, int16_t color) {
	btn->text_color = color;
	/* No refresh — caller must trigger redraw manually */
}

int16_t xbtntext_Get_Text_Button_Color(TextButton* btn) { return btn->text_color; }
