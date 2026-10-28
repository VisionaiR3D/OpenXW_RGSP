#include <stdlib.h>
#include <string.h>

#include <landru/btnpush.h>
#include <landru/dirty.h>
#include <landru/fourcc.h>
#include <landru/inpattr.h>
#include <landru/memptr.h>
#include <landru/rect.h>
#include <landru/style.h>

// GLOBAL: TIE 0xD3018
static bool push_button_module_gbl;

void xbtnpush_Create_Button_Module(void) {
#ifndef LANDRU_GAME_XW
	xinput_Create_Input_Type(FOURCC_BUTN, (void*)xbtnpush_Generate_Button, (void*)xbtnpush_Free_Button);
	xinput_Create_Input_Type(FOURCC_SBTN, (void*)xbtnpush_Generate_Button, (void*)xbtnpush_Free_Button);
#endif
	push_button_module_gbl = true;
}

void xbtnpush_Destroy_Button_Module(void) {
	if (push_button_module_gbl) {
#ifndef LANDRU_GAME_XW
		xinput_Destroy_Input_Type(FOURCC_SBTN);
		xinput_Destroy_Input_Type(FOURCC_BUTN);
#endif
		push_button_module_gbl = false;
	}
}

#ifndef LANDRU_GAME_XW
PushButton* xbtnpush_Generate_Button(Input* parent, char* res_buffer, const char* name) {
	InputResourceHeader hdr;
	InputResourceHeader_decode(&hdr, (const uint8_t*)res_buffer);

	PushButton* btn;
	if (hdr.res_type == FOURCC_BUTN)
		btn = xbtnpush_Alloc_Button(parent, &hdr.frame, hdr.zinput, 0, name, hdr.id);
	else
		btn = xbtnpush_Alloc_Small_Button(parent, &hdr.frame, hdr.zinput, 0, name, hdr.id);

	if (!btn)
		return NULL;

	btn->header.alignment = hdr.window_flags;
	btn->header.flags = hdr.state_flags | INPUT_REFRESH;
	btn->header.type = hdr.res_type;
	btn->header.var1 = hdr.exit_pending;
	btn->header.var2 = hdr.exit_code;

	return btn;
}

#endif

PushButton* xbtnpush_Alloc_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
								  const char* name, int16_t id) {
	PushButton* btn =
		(PushButton*)xinput_Alloc_Dialog_Input(parent, frame, zinput, sizeof(PushButton) - sizeof(Input));
	if (btn) {
		xbtnpush_Init_Button(btn, callback, name, id);
#ifndef LANDRU_GAME_XW
		btn->header.type = FOURCC_BUTN;
#endif
	}
	return btn;
}

PushButton* xbtnpush_Alloc_Small_Button(Input* parent, Rect* frame, int16_t zinput, InputUserFunc callback,
										const char* name, int16_t id) {
	PushButton* btn = xbtnpush_Alloc_Button(parent, frame, zinput, callback, name, id);
	if (btn) {
#ifdef LANDRU_GAME_XW
		btn->header.draw = xbtnpush_idraw_SmallButton;
#else
		btn->header.type = FOURCC_SBTN;
#endif
	}
	return btn;
}

#ifndef LANDRU_GAME_XW
int16_t xbtnpush_Free_Button(PushButton* btn) {
	if (btn->name) {
		free(btn->name);
		btn->name = NULL;
	}
	return 1;
}

#endif

void xbtnpush_Init_Button(PushButton* btn, InputUserFunc callback, const char* name, int16_t id) {
	xinpattr_Set_Input_Draw_Function(&btn->header, xbtnpush_idraw_Button);
	xinpattr_Set_Input_Update_Function(&btn->header, xbtnpush_iupdate_Button);
	xinpattr_Set_Input_User_Function(&btn->header, callback);
	btn->header.mouseUsage = downMoveUpInput;
	btn->header.id = id;

#ifdef LANDRU_GAME_XW
	btn->labels = name;
	btn->labelIndex = 0;
#else
	if (name)
		btn->name = xmemptr_Duplicate_String(name);
	else
		btn->name = NULL;

#endif

	btn->pressed = 0;
}

#ifdef LANDRU_GAME_XW
static const char* button_label(const PushButton* btn) {
	size_t offset = 0;
	for (int16_t index = 0; index != btn->labelIndex && btn->labels[offset]; index++)
		offset += strlen(&btn->labels[offset]) + 1;
	return &btn->labels[offset];
}

void xbtnpush_idraw_Button(Input* input, Rect* frame, Rect* clip, int16_t refresh) {
	PushButton* btn = (PushButton*)input;
	(void)clip;
	if (refresh) {
		xstyle_Style_Paint_Button(frame, btn->pressed);
		xstyle_Style_Button_Text(button_label(btn), frame, btn->pressed);
	}
}

void xbtnpush_idraw_SmallButton(Input* input, Rect* frame, Rect* clip, int16_t refresh) {
	PushButton* btn = (PushButton*)input;
	(void)clip;
	if (refresh) {
		xstyle_Style_Paint_Border(frame, btn->pressed);
		xstyle_Style_Small_Button_Text(button_label(btn), frame, btn->pressed);
	}
}
#else
void xbtnpush_idraw_Button(Input* input, Rect* paint_rect, Rect* dirty_rect, int16_t do_draw) {
	PushButton* btn = (PushButton*)input;
	if (!do_draw)
		return;

	/* Paint button background — large vs small dispatch */
	if (btn->header.type == FOURCC_BUTN)
		xstyle_Style_Paint_Button(paint_rect, btn->pressed);
	else
		xstyle_Style_Paint_Border(paint_rect, btn->pressed);

	/* Draw centered label text */
	if (btn->name) {
		if (btn->header.type == FOURCC_BUTN)
			xstyle_Style_Button_Text(btn->name, paint_rect, btn->pressed);
		else
			xstyle_Style_Small_Button_Text(btn->name, paint_rect, btn->pressed);
	}

	if (xinpattr_Is_Input_Dirty(&btn->header))
		xdirty_Dirty_Rect(dirty_rect);
}

#endif

int16_t xbtnpush_iupdate_Button(Input* input, Rect* paint_rect, Rect* clip, int16_t phase,
								InputMouseEvent mouse_l, InputMouseEvent mouse_r, int16_t x, int16_t y) {
	PushButton* btn = (PushButton*)input;
	(void)clip;
	if (phase)
		return 0;

	InputMouseEvent mouse_state = 0;
	if (mouse_l)
		mouse_state = mouse_l;
	if (mouse_r)
		mouse_state = mouse_r;

	if (mouse_state == 1) {
		btn->pressed = 1;
	} else if (mouse_state == 2) {
		btn->pressed = xrect_Point_In_Rect(paint_rect, paint_rect->left + x, paint_rect->top + y);
	} else if (mouse_state == 3) {
		if (btn->pressed) {
			xinpattr_Selected_Input(&btn->header);
			btn->pressed = 0;
		}
	}

	xinpattr_Refresh_Input(&btn->header);
	/* Binary XBTNPUSH_iupdate_Button always returns 1 (0xad3e5: mov eax,1).
	 * XINPCALL_Update_Mouse_Down reads this as "update handled" — non-zero
	 * arms Set_InputActive_Down, which grabs the input so mouse_state=3
	 * (release) dispatches back here. Returning 0 made the dispatcher take
	 * the "nobody handled it" path and Set_InputActive_Ignore, silently
	 * eating the release event. */
	return 1;
}

void xbtnpush_Set_Button_Name(PushButton* btn, const char* name) {
#ifdef LANDRU_GAME_XW
	btn->labels = name;
#else
	free(btn->name);
	btn->name = xmemptr_Duplicate_String(name);
#endif
	xinpattr_Refresh_Input(&btn->header);
}

#ifdef LANDRU_GAME_XW
void xbtnpush_Set_Button_LabelIndex(PushButton* btn, int16_t index) {
	btn->labelIndex = index;
	xinpattr_Refresh_Input(&btn->header);
}
#else
void xbtnpush_Get_Button_Name(PushButton* btn, char* out) {
	if (btn->name)
		strcpy(out, btn->name);
	else
		*out = '\0';
}

#endif
