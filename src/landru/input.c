#include <stddef.h>
#include <stdlib.h>

#include <landru/canvas.h>
#include <landru/dirty.h>
#include <landru/inpattr.h>
#include <landru/inpcall.h>
#include <landru/input.h>
#include <landru/memptr.h>
#include <landru/rect.h>
#include <landru/style.h>
#include <landru/vesa.h>
#include <landru/view.h>

#include "binio.h"
#include <landru/file.h>
#include <landru/fourcc.h>
#include <landru/res.h>

#ifndef LANDRU_GAME_XW
/* Static buffers for resource deserialization */
static int32_t input_res_buffer[64];
static uint8_t input_str_buffer[256];

/* Module globals */
// GLOBAL: TIE 0xFC10C
InputType* input_type_gbl = NULL;
// GLOBAL: TIE 0xFC104
Input* input_list_gbl = NULL;
#endif
#ifndef LANDRU_GAME_XW
// GLOBAL: TIE 0xFC108
Input* active_input_list_gbl = NULL;
#endif
// GLOBAL: TIE 0xD3008
bool input_module_gbl = false;
#ifndef LANDRU_GAME_XW
// GLOBAL: TIE 0xFC0FC
Rect input_frame_gbl;
// GLOBAL: TIE 0xFC110
bool refresh_inputs_gbl = false;
// GLOBAL: TIE 0xFC114
int16_t zinput_build_gbl = 0;
#endif

static Input** system_input_head(void) {
#ifdef LANDRU_GAME_XW
	return &view_gbl->input_list;
#else
	return &input_list_gbl;
#endif
}

/* --- Module lifecycle --- */

#ifndef LANDRU_GAME_XW
void xinput_Create_Input_Module(void) {
	input_type_gbl = NULL;
	input_list_gbl = NULL;
	active_input_list_gbl = NULL;
	input_module_gbl = true;
	xcanvas_Get_Drawing_Canvas_Bounds(&input_frame_gbl);
	if (xinput_Create_Input_Type(FOURCC_INPT, NULL, NULL)) {
		if (!xinput_Create_Input_Type(FOURCC_BACK, NULL, NULL)) {
			xinput_Destroy_Input_Type(FOURCC_INPT);
			input_module_gbl = false;
		}
	}
}

void xinput_Destroy_Input_Module(void) {
	if (input_module_gbl) {
		xinput_Destroy_Input_Type(FOURCC_INPT);
		xinput_Destroy_Input_Type(FOURCC_BACK);
		input_module_gbl = false;
	}
}

/* --- Input type registry --- */

bool xinput_Create_Input_Type(uint32_t type, void* create_handler, void* free_handler) {
	InputType* t = xmemptr_Alloc_Clear_System_Pointer(sizeof(InputType));
	if (!t)
		return false;
	t->type = type;
	t->func1 = create_handler;
	t->func2 = free_handler;
	t->next = input_type_gbl;
	input_type_gbl = t;
	return true;
}

void xinput_Destroy_Input_Type(uint32_t type) {
	InputType* cur = input_type_gbl;
	InputType* prev = NULL;
	while (cur) {
		if (cur->type == type)
			break;
		prev = cur;
		cur = cur->next;
	}
	if (cur) {
		if (prev)
			prev->next = cur->next;
		else
			input_type_gbl = input_type_gbl->next;
		xmemptr_Free_System_Pointer(cur);
	}
}

InputType* xinput_Find_Input_Type(uint32_t type) {
	for (InputType* t = input_type_gbl; t; t = t->next) {
		if (t->type == type)
			return t;
	}
	return NULL;
}

#else
void xinput_Create_Input_Module(void) { input_module_gbl = true; }

void xinput_Destroy_Input_Module(void) { input_module_gbl = false; }
#endif

/* --- Active input list --- */

#ifdef LANDRU_GAME_XW
Input* xinput_Get_Active_Input_List(void) {
	Input* input = view_gbl->dialog_list;
	if (!input)
		return view_gbl->input_list;
	while (input->next)
		input = input->next;
	return input;
}
#else
void xinput_Set_Active_Input_List(Input* input) { active_input_list_gbl = input; }

Input* xinput_Get_Active_Input_List(void) {
	if (active_input_list_gbl)
		return active_input_list_gbl;
	return *system_input_head();
}

#endif

#ifndef LANDRU_GAME_XW
/* --- Resource loading --- */

LandruHandle xinput_Res_Input_Handle(const char* name) {
	ResFile* rf = xres_Find_File_With_Resource(FOURCC_INPT, name);
	int offset;
	uint32_t size;
	if (!rf || !xres_Get_Resource_Offset(rf, FOURCC_INPT, name, &offset, &size))
		return LANDRU_NULL_HANDLE;
	return xres_Load_Resource_Data(rf, offset, size, LANDRU_MEMORY_DEFAULT);
}

Input* xinput_Generate_Input_Handle(LandruHandle handle, int16_t tree_index) {
	return xinput_Generate_Callback_Input_Handle(handle, tree_index, NULL);
}

typedef Input* (*InputCallbackFunc)(Input* parent, void* res_buffer, uint8_t* str_buffer);

Input* xinput_Generate_Callback_Input_Handle(LandruHandle handle, int16_t tree_index, void* callback) {
	InputCallbackFunc cb = (InputCallbackFunc)callback;
	uint8_t* buf = xmemhdl_Lock_Handle(handle);
	int offset = 0;
	Input* last_input = NULL;

	xfile_Read_Word_From_Buffer(buf, &offset); /* skip version */
	int16_t tree_count = xfile_Read_Word_From_Buffer(buf, &offset);

	for (int16_t t = 0; t < tree_count; t++) {
		if (t == tree_index) {
			Input* cur_input = NULL;
			int16_t rec_count = xfile_Read_Word_From_Buffer(buf, &offset);

			for (int16_t r = 0; r < rec_count; r++) {
				uint8_t name_data[12];
				xfile_Read_Data_From_Buffer(buf, name_data, &offset, 8);
				name_data[9] = 0;

				int16_t res_size = xfile_Read_Word_From_Buffer(buf, &offset);
				xfile_Read_Data_From_Buffer(buf, input_res_buffer, &offset, res_size);

				int16_t str_size = xfile_Read_Word_From_Buffer(buf, &offset);
				if (str_size)
					xfile_Read_Data_From_Buffer(buf, input_str_buffer, &offset, str_size);
				else
					input_str_buffer[0] = 0;

				cur_input = NULL;
				if (cb)
					cur_input = cb(NULL, input_res_buffer, input_str_buffer);

				if (!cur_input) {
					uint32_t res_type = *(uint32_t*)input_res_buffer;
					InputType* type = xinput_Find_Input_Type(res_type);
					if (type && type->func1) {
						typedef Input* (*CreateFunc)(Input*, void*, uint8_t*);
						cur_input = ((CreateFunc)type->func1)(NULL, input_res_buffer, input_str_buffer);
					} else {
						cur_input = xinput_Generate_Input(NULL, (char*)input_res_buffer);
					}
				}

				if (cur_input) {
					if (cb)
						cb(cur_input, input_res_buffer, input_str_buffer);
					xinput_Generate_Input_Branch(cur_input, handle, &offset, callback, 1);
				}
			}
			last_input = cur_input;
		} else {
			/* Skip this tree */
			int16_t rec_count = xfile_Read_Word_From_Buffer(buf, &offset);
			for (int16_t r = 0; r < rec_count; r++) {
				uint8_t name_data[12];
				xfile_Read_Data_From_Buffer(buf, name_data, &offset, 8);
				name_data[9] = 0;

				int16_t res_size = xfile_Read_Word_From_Buffer(buf, &offset);
				xfile_Read_Data_From_Buffer(buf, input_res_buffer, &offset, res_size);

				int16_t str_size = xfile_Read_Word_From_Buffer(buf, &offset);
				if (str_size)
					xfile_Read_Data_From_Buffer(buf, input_str_buffer, &offset, str_size);
				else
					input_str_buffer[0] = 0;

				xinput_Generate_Input_Branch(NULL, handle, &offset, callback, 0);
			}
		}
	}
	return last_input;
}

Input* xinput_Generate_Input_Branch(Input* parent, LandruHandle handle, int* offset, void* callback,
									int16_t generate) {
	InputCallbackFunc cb = (InputCallbackFunc)callback;
	uint8_t* buf = xmemhdl_Lock_Handle(handle);
	Input* cur_input = NULL;

	int16_t rec_count = xfile_Read_Word_From_Buffer(buf, offset);

	for (int16_t i = 0; i < rec_count; i++) {
		uint8_t name_data[12];
		xfile_Read_Data_From_Buffer(buf, name_data, offset, 8);
		name_data[9] = 0;

		int16_t res_size = xfile_Read_Word_From_Buffer(buf, offset);
		xfile_Read_Data_From_Buffer(buf, input_res_buffer, offset, res_size);

		int16_t str_size = xfile_Read_Word_From_Buffer(buf, offset);
		if (str_size)
			xfile_Read_Data_From_Buffer(buf, input_str_buffer, offset, str_size);
		else
			input_str_buffer[0] = 0;

		if (generate) {
			cur_input = NULL;
			if (cb)
				cur_input = cb(NULL, input_res_buffer, input_str_buffer);

			if (!cur_input) {
				uint32_t res_type = *(uint32_t*)input_res_buffer;
				InputType* type = xinput_Find_Input_Type(res_type);
				if (type && type->func1) {
					typedef Input* (*CreateFunc)(Input*, void*, uint8_t*);
					cur_input = ((CreateFunc)type->func1)(parent, input_res_buffer, input_str_buffer);
				} else {
					cur_input = xinput_Generate_Input(parent, (char*)input_res_buffer);
				}
			}

			if (cur_input) {
				if (cb)
					cb(cur_input, input_res_buffer, input_str_buffer);
				xinput_Generate_Input_Branch(cur_input, handle, offset, callback, 1);
			}
		} else {
			xinput_Generate_Input_Branch(NULL, handle, offset, callback, 0);
		}
	}
	return cur_input;
}

/* On-disk codec for the 25-byte LFD input-resource header. Layout:
 *   +0x00 res_type (u32)
 *   +0x04 frame.top, .left, .bottom, .right (4 x i16)
 *   +0x0C zinput (i16)
 *   +0x0E window_flags (u16)
 *   +0x10 state_flags (u16)
 *   +0x12 id (i16)
 *   +0x14 pad (u8)
 *   +0x15 exit_pending (i16, ODD offset)
 *   +0x17 exit_code    (i16, ODD offset)
 * Read-only at runtime, hence no encoder. */
void InputResourceHeader_decode(InputResourceHeader* dst, const uint8_t* src) {
	dst->res_type = br_u32le(src + 0x00);
	dst->frame.top = br_i16le(src + 0x04);
	dst->frame.left = br_i16le(src + 0x06);
	dst->frame.bottom = br_i16le(src + 0x08);
	dst->frame.right = br_i16le(src + 0x0A);
	dst->zinput = br_i16le(src + 0x0C);
	dst->window_flags = br_u16le(src + 0x0E);
	dst->state_flags = br_u16le(src + 0x10);
	dst->id = br_i16le(src + 0x12);
	dst->pad = src[0x14];
	dst->exit_pending = br_i16le(src + 0x15);
	dst->exit_code = br_i16le(src + 0x17);
}

Input* xinput_Generate_Input(Input* parent, char* buffer) {
	InputResourceHeader hdr;
	InputResourceHeader_decode(&hdr, (const uint8_t*)buffer);
	Input* input = xinput_Alloc_Dialog_Input(parent, &hdr.frame, hdr.zinput, 0);
	if (!input)
		return NULL;

	input->type = hdr.res_type;
	input->alignment = hdr.window_flags;
	input->flags = hdr.state_flags | INPUT_REFRESH;
	/* The id at offset 18 (= 0x12) and mouseUsage at offset 20 (= 0x14)
	 * are read directly from the on-disk image; the binary uses these
	 * raw byte reads as a side channel. The id matches hdr.id; the
	 * mouseUsage byte is the 'pad' field in the header struct. */
	input->id = hdr.id;
	input->mouseUsage = (MouseUsage)hdr.pad;
	input->var1 = hdr.exit_pending;
	input->var2 = hdr.exit_code;
	xinpattr_Set_Input_Draw_Function(input, xinput_idraw_Input);
	return input;
}

#endif

/* --- Init / Alloc --- */

void xinput_Init_Input(Input* input, Rect* frame, int16_t z) {
	input->child = NULL;
#ifndef LANDRU_GAME_XW
	input->type = FOURCC_INPT;
#endif
	input->mouseUsage = downUpInput;
	input->next = NULL;
	input->frame = *frame;
#ifdef LANDRU_GAME_XW
	input->keyEnabled = 1;
	input->horizontalAlign = 0;
	input->verticalAlign = 0;
	input->flags = INPUT_VISIBLE | INPUT_ACTIVE | INPUT_REFRESH;
#else
	input->flags = INPUT_VISIBLE | INPUT_ACTIVE | INPUT_DIRTY | INPUT_REFRESH | INPUT_KEY_FOCUSED;
	input->alignment = 0;
#endif
	input->zinput = z;
#ifndef LANDRU_GAME_XW
	input->id = 0;
#endif
}

Input* xinput_Alloc_Input(Input* parent, Rect* frame, int16_t z, int16_t extend) {
	Input* input = xmemptr_Alloc_Clear_System_Pointer(sizeof(Input) + extend);
	if (!input)
		return NULL;
	xinput_Init_Input(input, frame, z);
	if (parent)
		xinput_Add_Input_To_Parent(parent, input);
	else {
#ifdef LANDRU_GAME_XW
		xinput_Add_Input_To_System(system_input_head(), input);
#else
		xinput_Add_Input_To_System(input);
#endif
	}
	return input;
}

Input* xinput_Alloc_Dialog_Input(Input* parent, Rect* frame, int16_t z, int16_t extend) {
	Input* input = xmemptr_Alloc_Clear_System_Pointer(sizeof(Input) + extend);
	if (!input)
		return NULL;
	xinput_Init_Input(input, frame, z);
	if (parent)
		xinput_Add_Input_To_Parent(parent, input);
	return input;
}

/* --- Free --- */

void xinput_Free_Input_Lists(void) {
	Input* cur = (*system_input_head());
	while (cur) {
		Input* next = cur->next;
		if (cur->child) {
			xinput_Free_Inputs(cur->child);
			cur->child = NULL;
		}
		xinput_Free_Input(cur);
		cur = next;
	}
	(*system_input_head()) = NULL;
}

void xinput_Free_Inputs(Input* input) {
	while (input) {
		Input* next = input->next;
		if (input->child) {
			xinput_Free_Inputs(input->child);
			input->child = NULL;
		}
		xinput_Free_Input(input);
		input = next;
	}
}

void xinput_Free_Input(Input* input) {
#ifdef LANDRU_GAME_XW
	xinput_Free_Input_Data(input);
	xmemptr_Free_System_Pointer(input);
#else
	/* Find the InputType for this input's res_type */
	InputType* type = NULL;
	for (InputType* t = input_type_gbl; t; t = t->next) {
		if (t->type == input->type) {
			type = t;
			break;
		}
	}

	/* If no type, no free_handler, or free_handler returns non-zero: do default cleanup */
	typedef int16_t (*FreeFunc)(Input*);
	if (!type || !type->func2 || ((FreeFunc)type->func2)(input)) {
		xinput_Free_Input_Data(input);
		xmemptr_Free_System_Pointer(input);
	}

#endif
}

void xinput_Free_Input_Data(Input* input) {
	if (xinpattr_Is_Discard_Input_Data(input)) {
		if (input->varptr) {
			xmemptr_Free_System_Pointer(input->varptr);
		}
		if (input->varhdl) {
			xmemhdl_Free_Handle(input->varhdl);
		}
	}
	input->varhdl = LANDRU_NULL_HANDLE;
	input->varptr = NULL;
}

/* --- Draw --- */

#ifndef LANDRU_GAME_XW
void xinput_idraw_Input(Input* input, Rect* frame, Rect* rect, int16_t refresh) {
	if (!refresh)
		return;

	if (input->type == FOURCC_BACK)
		xstyle_Style_Paint_TextField(frame);
	else if (input->type == FOURCC_INPT)
		xstyle_Style_Paint_Border(frame, 0);

	if (xinpattr_Is_Input_Dirty(input))
		xdirty_Dirty_Rect(rect);
}

#endif

/* --- Add/Remove (sorted by descending zinput) --- */

#ifdef LANDRU_GAME_XW
void xinput_Add_Input_To_System(Input** head, Input* input) {
#else
void xinput_Add_Input_To_System(Input* input) {
	Input** head = system_input_head();
#endif
	Input* cur = *head;
	Input* prev = NULL;
	while (cur) {
		if (input->zinput >= cur->zinput)
			break;
		prev = cur;
		cur = cur->next;
	}
	input->next = cur;
	if (prev)
		prev->next = input;
	else
		*head = input;
#ifdef LANDRU_GAME_XW
	input->id = 0;
#endif
}

void xinput_Add_Input_To_Parent(Input* parent, Input* child) {
	Input* cur = parent->child;
	Input* prev = NULL;
	while (cur) {
		if (child->zinput >= cur->zinput)
			break;
		prev = cur;
		cur = cur->next;
	}
	child->next = cur;
	if (prev)
		prev->next = child;
	else
		parent->child = child;
#ifdef LANDRU_GAME_XW
	child->id = 0;
#endif
}

void xinput_Remove_Input_From_System(Input* input) {
	Input* cur = (*system_input_head());
	Input* prev = NULL;
	while (cur) {
		if (cur == input)
			break;
		prev = cur;
		cur = cur->next;
	}
	if (cur == input) {
		if (prev)
			prev->next = input->next;
		else
			(*system_input_head()) = input->next;
		input->next = NULL;
	}
}

void xinput_Remove_Input_From_Parent(Input* parent, Input* input) {
	Input* cur = parent->child;
	Input* prev = NULL;
	while (cur) {
		if (cur == input)
			break;
		prev = cur;
		cur = cur->next;
	}
	if (cur == input) {
		if (prev)
			prev->next = input->next;
		else
			parent->child = input->next;
		input->next = NULL;
	}
}

/* --- System dispatchers --- */

void xinput_Update_System_Inputs(void) { xinpcall_Update_Inputs((*system_input_head())); }

void xinput_User_System_Inputs(int32_t time) { xinpcall_User_Inputs((*system_input_head()), time); }

#ifdef LANDRU_GAME_XW
void xinput_Draw_System_Inputs(void) {
	xinpcall_Draw_Input_Layer(view_gbl->input_list, NULL, NULL, view_gbl->refresh_inputs);
	view_gbl->refresh_inputs = 0;
}

void xinput_Refresh_System_Inputs(void) { view_gbl->refresh_inputs = 1; }

void xinput_Set_System_Input_Frame(Rect* frame) { view_gbl->input_frame = *frame; }

void xinput_Get_System_Input_Frame(Rect* outFrame) {
	if ((landru_video_flags_gbl & LANDRU_VIDEO_VGA_COMPAT))
		xrect_Set_Rect(outFrame, 0, 0, VIEW_INPUT_WIDTH, VIEW_INPUT_HEIGHT);
	else
		*outFrame = view_gbl->input_frame;
}

void xinput_Get_System_Input_Clip(Rect* outClip) {
	xinput_Get_System_Input_Frame(outClip);
	if (!(landru_video_flags_gbl & LANDRU_VIDEO_VGA_COMPAT))
		xrect_Clip_Rect(outClip, &view_gbl->clip_frame);
}
#else
void xinput_Draw_System_Inputs(int16_t refresh) {
	int16_t flags = refresh | refresh_inputs_gbl;
	refresh_inputs_gbl = false;
	xinpcall_Draw_Inputs((*system_input_head()), NULL, NULL, flags);
}

void xinput_Refresh_System_Inputs(void) { refresh_inputs_gbl = true; }

void xinput_Set_System_Input_Frame(Rect* frame) { xrect_Copy_Rect(&input_frame_gbl, frame); }

void xinput_Get_System_Input_Frame(Rect* outFrame) { xrect_Copy_Rect(outFrame, &input_frame_gbl); }

#endif
