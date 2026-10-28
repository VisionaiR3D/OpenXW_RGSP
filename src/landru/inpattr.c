#include <stddef.h>

#include <landru/inpattr.h>
#include <landru/inpcall.h>
#include <landru/rect.h>
#include <landru/view.h>

/* --- Visibility --- */

void xinpattr_Show_Input(Input* input) { input->flags |= INPUT_VISIBLE; }

void xinpattr_Hide_Input(Input* input) { input->flags &= ~INPUT_VISIBLE; }

int16_t xinpattr_Is_Input_Visible(Input* input) { return input->flags & INPUT_VISIBLE; }

/* --- Active --- */

void xinpattr_Activate_Input(Input* input) { input->flags |= INPUT_ACTIVE; }

void xinpattr_Deactivate_Input(Input* input) { input->flags &= ~INPUT_ACTIVE; }

int16_t xinpattr_Is_Input_Active(Input* input) { return input->flags & INPUT_ACTIVE; }

/* --- Start / Stop (visible + active combined) --- */

void xinpattr_Start_Input(Input* input) { input->flags |= (INPUT_VISIBLE | INPUT_ACTIVE); }

void xinpattr_Stop_Input(Input* input) { input->flags &= ~(INPUT_VISIBLE | INPUT_ACTIVE); }

/* --- Dirty --- */

#ifndef LANDRU_GAME_XW
void xinpattr_Dirty_Input(Input* input) { input->flags |= INPUT_DIRTY; }

void xinpattr_Non_Dirty_Input(Input* input) { input->flags &= ~INPUT_DIRTY; }

InputFlagResult xinpattr_Is_Input_Dirty(Input* input) { return input->flags & INPUT_DIRTY; }

#endif

/* --- Refresh --- */

void xinpattr_Refresh_Input(Input* input) { input->flags |= INPUT_REFRESH; }

void xinpattr_Non_Refresh_Input(Input* input) { input->flags &= ~INPUT_REFRESH; }

InputFlagResult xinpattr_Is_Input_Refresh(Input* input) { return input->flags & INPUT_REFRESH; }

/* --- Refreshable --- */

void xinpattr_Refreshable_Input(Input* input) { input->flags |= INPUT_REFRESHABLE; }

void xinpattr_Non_Refreshable_Input(Input* input) { input->flags &= ~INPUT_REFRESHABLE; }

InputFlagResult xinpattr_Is_Input_Refreshable(Input* input) { return input->flags & INPUT_REFRESHABLE; }

/* --- Discard data --- */

void xinpattr_Discard_Input_Data(Input* input) { input->flags |= INPUT_DISCARD_DATA; }

void xinpattr_Non_Discard_Input_Data(Input* input) { input->flags &= ~INPUT_DISCARD_DATA; }

InputFlagResult xinpattr_Is_Discard_Input_Data(Input* input) { return input->flags & INPUT_DISCARD_DATA; }

/* --- Selected (read-and-clear via Get_Input_Selected) --- */

void xinpattr_Selected_Input(Input* input) { input->flags |= INPUT_SELECTED; }

void xinpattr_Non_Selected_Input(Input* input) { input->flags &= ~INPUT_SELECTED; }

InputFlagResult xinpattr_Is_Input_Selected(Input* input) { return input->flags & INPUT_SELECTED; }

int16_t xinpattr_Get_Input_Selected(Input* input) {
	if (xinpattr_Is_Input_Selected(input)) {
		xinpattr_Non_Selected_Input(input);
		return 1;
	}
	return 0;
}

/* --- User flags 1 & 2 (return boolean 0/1, not raw mask) --- */

void xinpattr_Set_Input_Flag1(Input* input) { input->flags |= INPUT_FLAG1; }

void xinpattr_Clear_Input_Flag1(Input* input) { input->flags &= ~INPUT_FLAG1; }

InputFlagResult xinpattr_Is_Input_Flag1(Input* input) { return (input->flags & INPUT_FLAG1) != 0; }

void xinpattr_Set_Input_Flag2(Input* input) { input->flags |= INPUT_FLAG2; }

void xinpattr_Clear_Input_Flag2(Input* input) { input->flags &= ~INPUT_FLAG2; }

InputFlagResult xinpattr_Is_Input_Flag2(Input* input) { return (input->flags & INPUT_FLAG2) != 0; }

/* --- Frame --- */

void xinpattr_Set_Input_Frame(Input* input, Rect* src) { xrect_Copy_Rect(&input->frame, src); }

void xinpattr_Get_Input_Frame(Input* input, Rect* dst) { xrect_Copy_Rect(dst, &input->frame); }

/* --- ZInput --- */

void xinpattr_Set_Input_ZInput(Input* input, int16_t zinput) {
	input->zinput = zinput;
#ifndef LANDRU_GAME_XW
	zinput_build_gbl = 1;
#endif
}

int16_t xinpattr_Get_Input_ZInput(Input* input, uint16_t* out) {
	*out = input->zinput;
	return input->zinput;
}

/* --- Alignment (encoded in window_flags low byte) --- */

void xinpattr_Set_Input_Allign(Input* input, InputAlignment halign, InputAlignment valign) {
#ifdef LANDRU_GAME_XW
	input->horizontalAlign = halign;
	input->verticalAlign = valign;
#else
	uint16_t keep = input->alignment & ~(INPUT_HALIGN_1 | INPUT_HALIGN_2 | INPUT_VALIGN_1 | INPUT_VALIGN_2);
	input->alignment = keep;

	if (halign == 1)
		input->alignment |= INPUT_HALIGN_1;
	else if (halign == 2)
		input->alignment |= INPUT_HALIGN_2;

	if (valign == 1)
		input->alignment |= INPUT_VALIGN_1;
	else if (valign == 2)
		input->alignment |= INPUT_VALIGN_2;
#endif
}

void xinpattr_Get_Input_Allign(Input* input, InputAlignment* out_h, InputAlignment* out_v) {
#ifdef LANDRU_GAME_XW
	*out_h = input->horizontalAlign;
	*out_v = input->verticalAlign;
#else
	uint16_t h_bits = input->alignment & (INPUT_HALIGN_1 | INPUT_HALIGN_2);
	if (h_bits == INPUT_HALIGN_1)
		*out_h = 1;
	else if (h_bits == INPUT_HALIGN_2)
		*out_h = 2;
	else
		*out_h = 0;

	uint16_t v_bits = input->alignment & (INPUT_VALIGN_1 | INPUT_VALIGN_2);
	if (v_bits == INPUT_VALIGN_1)
		*out_v = 1;
	else if (v_bits == INPUT_VALIGN_2)
		*out_v = 2;
	else
		*out_v = 0;
#endif
}

/* --- Function pointers --- */

void xinpattr_Set_Input_Draw_Function(Input* input, InputDrawFunc fn) { input->draw = fn; }

void xinpattr_Get_Input_Draw_Function(Input* input, InputDrawFunc* out) { *out = input->draw; }

void xinpattr_Set_Input_Update_Function(Input* input, InputUpdateFunc fn) { input->update = fn; }

void xinpattr_Get_Input_Update_Function(Input* input, InputUpdateFunc* out) { *out = input->update; }

void xinpattr_Set_Input_User_Function(Input* input, InputUserFunc fn) { input->user = fn; }

void xinpattr_Get_Input_User_Function(Input* input, InputUserFunc* out) { *out = input->user; }

/* --- Window flags --- */

#ifndef LANDRU_GAME_XW
void xinpattr_Set_Input_Window(Input* input) { input->alignment |= INPUT_WINDOW; }

void xinpattr_Clear_Input_Window(Input* input) { input->alignment &= ~INPUT_WINDOW; }

InputFlagResult xinpattr_Is_Input_Window(Input* input) { return (input->alignment & INPUT_WINDOW) != 0; }

void xinpattr_Set_Input_Window_Active(Input* input) { input->alignment |= INPUT_WIN_ACTIVE; }

void xinpattr_Clear_Input_Window_Active(Input* input) { input->alignment &= ~INPUT_WIN_ACTIVE; }

InputFlagResult xinpattr_Is_Input_Window_Active(Input* input) {
	return (input->alignment & INPUT_WIN_ACTIVE) != 0;
}

/* --- Activate_Window: move input to front of parent view's child list --- */

void xinpattr_Activate_Window(Input* input) {
	Input* parent = xinpcall_Find_View_Parent_For_Input(input);
	if (!parent)
		return;

	/* Unlink input from its current position in the child list */
	if (input != parent->child) {
		Input* prev = parent->child;
		while (prev->next != input)
			prev = prev->next;
		prev->next = input->next;
		input->next = parent->child;
		parent->child = input;
	}

	/* Renumber zinput from 100 upward, clear window_active on all */
	int16_t z = 100;
	for (Input* cur = input; cur; cur = cur->next) {
		xinpattr_Set_Input_ZInput(cur, z++);
		xinpattr_Clear_Input_Window_Active(cur);
	}
	xinpattr_Set_Input_Window_Active(input);
	xinpattr_Refresh_Input(input);
}

#endif

/* --- Move --- */

void xinpattr_Move_Input(Input* input, int16_t x, int16_t y) {
	xrect_Offset_Rect(&input->frame, x - input->frame.left, y - input->frame.top);
}

/* --- Z-order sorting --- */

#ifndef LANDRU_GAME_XW
void xinpattr_Check_Input_ZInputs(void) {
	if (zinput_build_gbl) {
		zinput_build_gbl = 0;
		xview_Get_Current_View();
		Input* head = input_list_gbl;
		xinpattr_Sort_Input_ZInput_List(&head);
	}
}

void xinpattr_Sort_Input_ZInputs(void) {
	xview_Get_Current_View();
	Input* head = input_list_gbl;
	xinpattr_Sort_Input_ZInput_List(&head);
}

#endif

Input* xinpattr_Sort_Input_ZInput_List(Input** head) {
	/* Recurse into children first.
	   The binary does NOT write the sorted child head back to cur->child —
	   the local is discarded. This matches the original behavior. */
	for (Input* iter = *head; iter; iter = iter->next) {
		if (iter->child) {
			Input* child_head = iter->child;
			xinpattr_Sort_Input_ZInput_List(&child_head);
		}
	}

	/* Insertion sort by descending zinput */
	Input* sorted = NULL;
	Input* unsorted = *head;
	while (unsorted) {
		Input* cur = unsorted;
		unsorted = unsorted->next;
		cur->next = NULL;

		Input* scan = sorted;
		Input* prev = NULL;
		while (scan && scan->zinput >= cur->zinput) {
			prev = scan;
			scan = scan->next;
		}

		if (prev) {
			cur->next = prev->next;
			prev->next = cur;
		} else {
			cur->next = sorted;
			sorted = cur;
		}
	}

	*head = sorted;
	return sorted;
}
