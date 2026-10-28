#ifndef LANDRU_INPUT_H
#define LANDRU_INPUT_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <landru/rect.h>
#include <stdint.h>

typedef struct InputType InputType;
typedef struct Input Input;

#ifdef LANDRU_GAME_XW
typedef int32_t InputMouseEvent;
typedef int32_t InputAlignment;
typedef int32_t InputFlagResult;
#else
typedef uint8_t InputMouseEvent;
typedef uint8_t InputAlignment;
typedef int16_t InputFlagResult;
#endif

/* Callback signatures for Input function pointers */
typedef void (*InputDrawFunc)(Input* input, Rect* paint_rect, Rect* clip, int16_t refresh);
typedef int16_t (*InputUpdateFunc)(Input* input, Rect* frame, Rect* clip, int16_t phase,
								   InputMouseEvent mouse_l, InputMouseEvent mouse_r, int16_t dx, int16_t dy);
typedef void (*InputUserFunc)(Input* input, int32_t context);

typedef enum MouseUsage {
	noInput = 0,
	downInput = 1,
	downUpInput = 2,
	downMoveUpInput = 3,
	allInput = 4
} MouseUsage;

struct InputType {
	uint32_t type;
	InputType* next;
	void* func1;
	void* func2;
};

enum InputWindowFlags {
	INPUT_HALIGN_1 = 0x0001,
	INPUT_HALIGN_2 = 0x0002,
	INPUT_VALIGN_1 = 0x0010,
	INPUT_VALIGN_2 = 0x0020,
	INPUT_WINDOW = 0x0100,
	INPUT_WIN_ACTIVE = 0x0200,
};

enum InputStateFlags {
	INPUT_VISIBLE = 0x0001,
	INPUT_ACTIVE = 0x0002,
	INPUT_DISCARD_DATA = 0x0004,
#ifdef LANDRU_GAME_XW
	INPUT_REFRESHABLE = 0x0008,
	INPUT_REFRESH = 0x0010,
	INPUT_SELECTED = 0x0020,
#else
	INPUT_DIRTY = 0x0008,
	INPUT_REFRESHABLE = 0x0010,
	INPUT_REFRESH = 0x0020,
	INPUT_SELECTED = 0x0100,
#endif
	INPUT_KEY_FOCUSED = 0x0200,
	INPUT_KEY_GROUP = 0x0400,
	INPUT_FLAG1 = 0x4000,
	INPUT_FLAG2 = 0x8000,
};

#ifdef LANDRU_GAME_XW
struct Input {
	Input* next;
	Input* child;
	Rect frame;
	int32_t keyEnabled;
	int32_t mouseUsage;
	int16_t zinput;
	uint16_t flags;
	int16_t id;
	int32_t horizontalAlign;
	int32_t verticalAlign;
	int16_t var1;
	int16_t var2;
	void* varptr;
	LandruHandle varhdl;
	InputDrawFunc draw;
	InputUpdateFunc update;
	InputUserFunc user;
};
#else
struct Input {
	uint32_t type;
	Input* next;
	Input* child;
	Rect frame;
	uint16_t alignment;
	uint16_t flags;
	int16_t zinput;
	int16_t id;
	MouseUsage mouseUsage;
	int16_t var1;
	int16_t var2;
	void* varptr;
	LandruHandle varhdl;
	InputDrawFunc draw;
	InputUpdateFunc update;
	InputUserFunc user;
};
#endif

/* 25-byte resource header from LFD input records.
 *
 * The runtime struct is naturally aligned (the trailing exit_pending /
 * exit_code int16s land at odd offsets +0x15 / +0x17 in the on-disk
 * image, which forces 1-byte padding before exit_pending and 2-byte
 * trailing padding under natural alignment, growing sizeof from 25 to
 * 28). On-disk layout is the canonical 25-byte little-endian record
 * produced by the original DOS binary; loaders go through
 * InputResourceHeader_decode so BE / strict-align hosts get the same
 * in-memory values. Read-only at runtime, hence no encoder. */
typedef struct {
	uint32_t res_type;
	Rect frame;
	int16_t zinput;
	uint16_t window_flags;
	uint16_t state_flags;
	int16_t id;
	uint8_t pad;
	int16_t exit_pending;
	int16_t exit_code;
} InputResourceHeader;

#define INPUTRESOURCEHEADER_DISK_SIZE 25u

void InputResourceHeader_decode(InputResourceHeader* dst, const uint8_t* src);

#ifndef LANDRU_GAME_XW
extern Input* input_list_gbl;
#endif
#ifndef LANDRU_GAME_XW
extern int16_t zinput_build_gbl;
#endif

void xinput_Create_Input_Module(void);
void xinput_Destroy_Input_Module(void);
#ifndef LANDRU_GAME_XW
bool xinput_Create_Input_Type(uint32_t type, void* create_handler, void* free_handler);
void xinput_Destroy_Input_Type(uint32_t type);
InputType* xinput_Find_Input_Type(uint32_t type);
void xinput_Set_Active_Input_List(Input* input);
#endif
Input* xinput_Get_Active_Input_List(void);
#ifndef LANDRU_GAME_XW
LandruHandle xinput_Res_Input_Handle(const char* name);
Input* xinput_Generate_Input_Handle(LandruHandle handle, int16_t tree_index);
Input* xinput_Generate_Callback_Input_Handle(LandruHandle handle, int16_t tree_index, void* callback);
Input* xinput_Generate_Input_Branch(Input* parent, LandruHandle handle, int* offset, void* callback,
									int16_t generate);
Input* xinput_Generate_Input(Input* parent, char* buffer);
#endif
void xinput_Init_Input(Input* input, Rect* frame, int16_t z);
Input* xinput_Alloc_Input(Input* parent, Rect* frame, int16_t z, int16_t extend);
Input* xinput_Alloc_Dialog_Input(Input* parent, Rect* frame, int16_t z, int16_t extend);
void xinput_Free_Input_Lists(void);
void xinput_Free_Inputs(Input* input);
void xinput_Free_Input(Input* input);
void xinput_Free_Input_Data(Input* input);
#ifndef LANDRU_GAME_XW
void xinput_idraw_Input(Input* input, Rect* frame, Rect* rect, int16_t refresh);
#endif
#ifdef LANDRU_GAME_XW
void xinput_Add_Input_To_System(Input** head, Input* input);
#else
void xinput_Add_Input_To_System(Input* input);
#endif
void xinput_Add_Input_To_Parent(Input* parent, Input* child);
void xinput_Remove_Input_From_System(Input* input);
void xinput_Remove_Input_From_Parent(Input* parent, Input* input);
void xinput_Update_System_Inputs(void);
void xinput_User_System_Inputs(int32_t time);
#ifdef LANDRU_GAME_XW
void xinput_Draw_System_Inputs(void);
void xinput_Get_System_Input_Clip(Rect* outClip);
#else
void xinput_Draw_System_Inputs(int16_t refresh);
#endif
void xinput_Refresh_System_Inputs(void);
void xinput_Set_System_Input_Frame(Rect* frame);
void xinput_Get_System_Input_Frame(Rect* outFrame);

#endif
