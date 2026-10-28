#include <stddef.h>

#include <landru/canvas.h>
#include <landru/inpattr.h>
#include <landru/inpcall.h>
#include <landru/io.h>
#include <landru/rect.h>
#include <landru/timer.h>

/* Globals */
// GLOBAL: TIE 0xD3010
static InputActiveState input_active_gbl;
// GLOBAL: TIE 0xD300C
static Input* input_user_gbl;
// GLOBAL: TIE 0xFC0FC
static Rect input_frame_gbl;
static Rect input_clip_gbl;
#ifndef LANDRU_GAME_XW
static Input* draw_input_gbl[256];
// GLOBAL: TIE 0xFC528
static int16_t draw_input_index_gbl;
#endif

/* --- Draw --- */

void xinpcall_Draw_Inputs(Input* list, Rect* parent_frame, Rect* parent_clip, int16_t refresh) {
#ifndef LANDRU_GAME_XW
	draw_input_index_gbl = 0;
#endif
	xinpcall_Draw_Input_Layer(list, parent_frame, parent_clip, refresh);
}

void xinpcall_Draw_Input_Layer(Input* list, Rect* parent_frame, Rect* parent_clip, int16_t refresh) {
#ifdef LANDRU_GAME_XW
	for (Input* input = list; input; input = input->next) {
		xtimer_Often();
		if (xinpattr_Is_Input_Visible(input)) {
			Rect frame, clip;
			xinpcall_Get_Input_Parent_Frame(parent_frame, parent_clip, &frame, &clip);
			xinpcall_Clip_Input_To_Frame(input, &frame, &clip);
			if (!xrect_Empty_Rect(&clip)) {
				int16_t effective =
					refresh | xinpattr_Is_Input_Refresh(input) | xinpattr_Is_Input_Refreshable(input);
				if (input->draw) {
					xcanvas_Set_Drawing_Canvas_Clip(&clip);
					input->draw(input, &frame, &clip, effective);
				}
				if (input->child)
					xinpcall_Draw_Input_Layer(input->child, &frame, &clip, effective);
			}
		}
		xinpattr_Non_Refresh_Input(input);
	}
#else
	/* Count inputs at this level */
	int16_t count = 0;
	for (Input* cur = list; cur; cur = cur->next)
		count++;

	if (count + draw_input_index_gbl > 256)
		return;

	/* Copy pointers into the draw stack */
	Input* cur = list;
	for (int16_t i = 0; i < count; i++) {
		draw_input_gbl[draw_input_index_gbl + i] = cur;
		cur = cur->next;
	}
	draw_input_index_gbl += count;

	/* Draw back-to-front */
	for (int16_t i = 1; i <= count; i++) {
		xtimer_Often();
		Input* input = draw_input_gbl[draw_input_index_gbl - i];

		if (!xinpattr_Is_Input_Visible(input))
			continue;

		Rect out_frame, out_clip;
		if (parent_frame && parent_clip) {
			out_frame = *parent_frame;
			out_clip = *parent_clip;
		} else {
			xinput_Get_System_Input_Frame(&out_frame);
#ifdef LANDRU_GAME_XW
			xinput_Get_System_Input_Clip(&out_clip);
#else
			xcanvas_Get_Drawing_Canvas_Bounds(&out_clip);
			xrect_Clip_Rect(&out_clip, &out_frame);
#endif
		}

		xinpcall_Clip_Input_To_Frame(input, &out_frame, &out_clip);
		if (xrect_Empty_Rect(&out_clip))
			continue;

		int16_t is_refresh = xinpattr_Is_Input_Refresh(input);
		int16_t effective = is_refresh | xinpattr_Is_Input_Refreshable(input) | refresh;

		if (input->draw) {
			xcanvas_Set_Drawing_Canvas_Clip(&out_clip);
			input->draw(input, &out_frame, &out_clip, effective);
		}

		if (input->child)
			xinpcall_Draw_Input_Layer(input->child, &out_frame, &out_clip, effective);

		xinpattr_Non_Refresh_Input(input);
	}

	draw_input_index_gbl -= count;
#endif
}

/* --- Frame computation --- */

void xinpcall_Get_Input_Parent_Frame(Rect* parent_frame, Rect* parent_clip, Rect* out_frame, Rect* out_clip) {
	if (parent_frame && parent_clip) {
		*out_frame = *parent_frame;
		*out_clip = *parent_clip;
	} else {
		xinput_Get_System_Input_Frame(out_frame);
#ifdef LANDRU_GAME_XW
		xinput_Get_System_Input_Clip(out_clip);
#else
		xcanvas_Get_Drawing_Canvas_Bounds(out_clip);
		xrect_Clip_Rect(out_clip, out_frame);
#endif
	}
}

int16_t xinpcall_Clip_Input_To_Frame(Input* input, Rect* frame, Rect* clip) {
	Rect r = input->frame;
	InputAlignment halign, valign;
	xinpattr_Get_Input_Allign(input, &halign, &valign);
	xrect_Allign_Rect(&r, frame, halign, valign);
	*frame = r;
	int16_t result = xrect_Clip_Rect(&r, clip);
	*clip = r;
	return result;
}

/* --- Mouse-grab state helpers --- */

void xinpcall_Set_InputActive_Down(Input* input, Rect* frame, Rect* clip, InputMouseEvent mouse_l,
								   InputMouseEvent mouse_r) {
	if (mouse_l == 1)
		input_active_gbl = INPUT_ACTIVE_LEFT_DOWN;
	if (mouse_r == 1)
		input_active_gbl = INPUT_ACTIVE_RIGHT_DOWN;
	input_user_gbl = input;
	input_frame_gbl = *frame;
	input_clip_gbl = *clip;
}

int16_t xinpcall_Replace_InputActive(Input* input) {
	Rect out_frame, out_clip;
	int16_t result = xinpcall_Find_View_Input_Frame(input, &out_frame, &out_clip);
	input_user_gbl = input;
	input_frame_gbl = out_frame;
	input_clip_gbl = out_clip;
	return result;
}

void xinpcall_Set_InputActive_Frame(Rect* frame, Rect* clip) {
	input_frame_gbl = *frame;
	input_clip_gbl = *clip;
}

void xinpcall_Set_InputActive_Ignore(InputMouseEvent mouse_l, InputMouseEvent mouse_r) {
	if (mouse_l == 1)
		input_active_gbl = INPUT_ACTIVE_LEFT_UP;
	if (mouse_r == 1)
		input_active_gbl = INPUT_ACTIVE_RIGHT_UP;
	input_user_gbl = NULL;
}

void xinpcall_Set_InputActive_Up(InputMouseEvent mouse_l, InputMouseEvent mouse_r) {
	if (mouse_l == 3 || mouse_r == 3)
		input_active_gbl = INPUT_ACTIVE_IDLE;
	input_user_gbl = NULL;
}

void xinpcall_Clear_Active_Input(void) {
	input_user_gbl = NULL;
	input_active_gbl = INPUT_ACTIVE_IDLE;
}

static int16_t update_mouse_down(Input* root, Rect* parent_frame, Rect* parent_clip, InputMouseEvent mouse_l,
								 InputMouseEvent mouse_r, int16_t mouse_x, int16_t mouse_y);

/* --- Update dispatcher --- */

void xinpcall_Update_Inputs(Input* root) {
	if (!root)
		return;

	int16_t mouse_x = xio_Mouse_X();
	int16_t mouse_y = xio_Mouse_Y();
	InputMouseEvent left_state = 0;
	InputMouseEvent right_state = 0;
	int16_t key = 0;

	if (xio_Any_Button() || xio_Any_Button_Release() || input_active_gbl) {
		/* Decode left button state */
		if (input_active_gbl != INPUT_ACTIVE_RIGHT_DOWN && input_active_gbl != INPUT_ACTIVE_RIGHT_UP) {
			if (xio_Left_Button()) {
				left_state = input_active_gbl ? 2 : 1;
			} else if (xio_Left_Button_Release()) {
				if (input_active_gbl == INPUT_ACTIVE_LEFT_DOWN) {
					left_state = 3;
				} else if (input_active_gbl == INPUT_ACTIVE_LEFT_UP) {
					if (xio_Right_Button())
						input_active_gbl = INPUT_ACTIVE_RIGHT_UP;
					else
						input_active_gbl = INPUT_ACTIVE_IDLE;
				}
			}
		}
		/* Decode right button state */
		if (input_active_gbl != INPUT_ACTIVE_LEFT_DOWN && input_active_gbl != INPUT_ACTIVE_LEFT_UP) {
			if (xio_Right_Button()) {
				right_state = input_active_gbl ? 2 : 1;
			} else if (xio_Right_Button_Release()) {
				if (input_active_gbl == INPUT_ACTIVE_RIGHT_DOWN) {
					right_state = 3;
				} else if (input_active_gbl == INPUT_ACTIVE_RIGHT_UP) {
					if (xio_Left_Button())
						input_active_gbl = INPUT_ACTIVE_LEFT_UP;
					else
						input_active_gbl = INPUT_ACTIVE_IDLE;
				}
			}
		}
	} else {
		key = xio_Get_Free_Key();
	}

	/* Dispatch key events */
	if (key)
		xinpcall_Update_Key_Inputs(root, NULL, NULL, key, mouse_x, mouse_y);

	/* Dispatch to grabbed input */
	if (input_user_gbl) {
		MouseUsage usage = input_user_gbl->mouseUsage;
		int dispatch = 0;
		if (usage == downUpInput && (left_state == 3 || right_state == 3))
			dispatch = 1;
		if (usage == downMoveUpInput || usage == allInput)
			dispatch = 1;

		if (dispatch) {
			InputUpdateFunc fn = input_user_gbl->update;
			if (fn) {
				fn(input_user_gbl, (&input_frame_gbl), (&input_clip_gbl), 0, left_state, right_state,
				   mouse_x - input_frame_gbl.left, mouse_y - input_frame_gbl.top);
			}
			if (left_state == 3 || right_state == 3)
				xinpcall_Set_InputActive_Up(left_state, right_state);
		}
		return;
	}

	/* Mouse down — hit test */
	if (left_state == 1 || right_state == 1) {
		int16_t updated = 0;
		for (Input* cur = root; cur && !updated; cur = cur->next) {
			if (!xinpattr_Is_Input_Visible(cur) || !xinpattr_Is_Input_Active(cur))
				continue;

			Rect out_frame, out_clip;
			xinpcall_Get_Input_Parent_Frame(NULL, NULL, &out_frame, &out_clip);
			xinpcall_Clip_Input_To_Frame(cur, &out_frame, &out_clip);

			if (!xrect_Point_In_Rect(&out_clip, mouse_x, mouse_y))
				continue;

#ifndef LANDRU_GAME_XW
			/* Window activation */
			if (xinpattr_Is_Input_Window(cur) && !xinpattr_Is_Input_Window_Active(cur)) {
				xinpattr_Activate_Window(cur);
				xinpattr_Selected_Input(cur);
			}
#endif

			/* Try children first */
			if (cur->child)
				updated = update_mouse_down(cur->child, &out_frame, &out_clip, left_state, right_state,
											mouse_x, mouse_y);

			/* Then try this input */
			if (!updated && cur->update && cur->mouseUsage) {
				InputUpdateFunc fn = cur->update;
				updated = fn(cur, &out_frame, &out_clip, 0, left_state, right_state, mouse_x - out_frame.left,
							 mouse_y - out_frame.top);
				if (updated) {
					if (cur->mouseUsage == downInput)
						xinpcall_Set_InputActive_Ignore(left_state, right_state);
					else
						xinpcall_Set_InputActive_Down(cur, &out_frame, &out_clip, left_state, right_state);
				}
			}

			/* If nothing handled it, still consume the click */
			if (!updated) {
				updated = 1;
				xinpcall_Set_InputActive_Ignore(left_state, right_state);
			}
		}

		if (!updated) {
			if (left_state == 1)
				input_active_gbl = INPUT_ACTIVE_LEFT_UP;
			if (right_state == 1)
				input_active_gbl = INPUT_ACTIVE_RIGHT_UP;
			input_user_gbl = NULL;
		}
		return;
	}

	/* Mouse move — deliver to hover inputs */
	for (Input* cur = root; cur; cur = cur->next) {
		int16_t handled = 0;
		if (!xinpattr_Is_Input_Visible(cur) || !xinpattr_Is_Input_Active(cur))
			continue;

		Rect frame, clip;
		xinpcall_Get_Input_Parent_Frame(NULL, NULL, &frame, &clip);
		xinpcall_Clip_Input_To_Frame(cur, &frame, &clip);

		if (!xrect_Point_In_Rect(&clip, mouse_x, mouse_y))
			continue;

		if (cur->child)
			handled = xinpcall_Update_Mouse_Move(cur->child, &frame, &clip, mouse_x, mouse_y);

		if (!handled && cur->update && cur->mouseUsage == allInput) {
			InputUpdateFunc fn = cur->update;
			fn(cur, &frame, &clip, 0, 0, 0, mouse_x - frame.left, mouse_y - frame.top);
		}
		break;
	}
}

/* --- Key dispatch --- */

int16_t xinpcall_Update_Key_Inputs(Input* root, Rect* parent_frame, Rect* parent_clip, int16_t key,
								   int16_t mouse_x, int16_t mouse_y) {
	int16_t updated = 0;
	int16_t focused_found = 0;

	for (Input* cur = root; cur && !updated; cur = cur->next) {
		if (!xinpattr_Is_Input_Visible(cur) || !xinpattr_Is_Input_Active(cur))
			continue;

		Rect out_frame, out_clip;
		if (parent_frame && parent_clip) {
			out_frame = *parent_frame;
			out_clip = *parent_clip;
		} else {
			xinput_Get_System_Input_Frame(&out_frame);
#ifdef LANDRU_GAME_XW
			xinput_Get_System_Input_Clip(&out_clip);
#else
			xcanvas_Get_Drawing_Canvas_Bounds(&out_clip);
			xrect_Clip_Rect(&out_clip, &out_frame);
#endif
		}
		xinpcall_Clip_Input_To_Frame(cur, &out_frame, &out_clip);

/* Deliver to key-focused input */
#ifdef LANDRU_GAME_XW
		if (cur->update && cur->keyEnabled) {
#else
		if (cur->update && (cur->flags & INPUT_KEY_FOCUSED)) {
#endif
			focused_found = 1;
			InputUpdateFunc fn = cur->update;
			updated =
				fn(cur, &out_frame, &out_clip, key, 0, 0, mouse_x - out_frame.left, mouse_y - out_frame.top);
		}

		/* Recurse into children */
		if (cur->child && !updated)
			updated = xinpcall_Update_Key_Inputs(cur->child, &out_frame, &out_clip, key, mouse_x, mouse_y);
	}

	if (updated || focused_found)
		xio_Get_Key();

	return updated;
}

/* --- Mouse down hit-test (recursive) --- */

static int16_t update_mouse_down(Input* root, Rect* parent_frame, Rect* parent_clip, InputMouseEvent mouse_l,
								 InputMouseEvent mouse_r, int16_t mouse_x, int16_t mouse_y) {
	int16_t result = 0;

	for (Input* cur = root; cur && !result; cur = cur->next) {
		if (!xinpattr_Is_Input_Visible(cur) || !xinpattr_Is_Input_Active(cur))
			continue;

		Rect frame, clip;
		if (parent_frame && parent_clip) {
			frame = *parent_frame;
			clip = *parent_clip;
		} else {
			xinput_Get_System_Input_Frame(&frame);
#ifdef LANDRU_GAME_XW
			xinput_Get_System_Input_Clip(&clip);
#else
			xcanvas_Get_Drawing_Canvas_Bounds(&clip);
			xrect_Clip_Rect(&clip, &frame);
#endif
		}
		xinpcall_Clip_Input_To_Frame(cur, &frame, &clip);

		if (!xrect_Point_In_Rect(&clip, mouse_x, mouse_y))
			continue;

#ifndef LANDRU_GAME_XW
		/* Window activation */
		if (xinpattr_Is_Input_Window(cur) && !xinpattr_Is_Input_Window_Active(cur)) {
			xinpattr_Activate_Window(cur);
			xinpattr_Selected_Input(cur);
		}
#endif

		/* Recurse into children */
		if (cur->child)
			result = update_mouse_down(cur->child, &frame, &clip, mouse_l, mouse_r, mouse_x, mouse_y);

		/* Try this input's update func */
		if (!result && cur->update && cur->mouseUsage) {
			InputUpdateFunc fn = cur->update;
			result = fn(cur, &frame, &clip, 0, mouse_l, mouse_r, mouse_x - frame.left, mouse_y - frame.top);
			if (result) {
				if (cur->mouseUsage == downInput)
					xinpcall_Set_InputActive_Ignore(mouse_l, mouse_r);
				else
					xinpcall_Set_InputActive_Down(cur, &frame, &clip, mouse_l, mouse_r);
			}
		}

		/* Consume click even if nothing handled it */
		if (!result) {
			xinpcall_Set_InputActive_Ignore(mouse_l, mouse_r);
			result = 1;
		}
	}

	return result;
}

#ifdef LANDRU_GAME_XW
int16_t xinpcall_Update_Mouse_Down(Input* root, Rect* parent_frame, Rect* parent_clip,
								   InputMouseEvent mouse_l, InputMouseEvent mouse_r, int16_t mouse_x,
								   int16_t mouse_y) {
#else
int16_t xinpcall_Update_Mouse_Down(Input* root, Rect* parent_frame, InputMouseEvent mouse_l,
								   Rect* parent_clip, InputMouseEvent mouse_r, int16_t mouse_x,
								   int16_t mouse_y) {
#endif
	return update_mouse_down(root, parent_frame, parent_clip, mouse_l, mouse_r, mouse_x, mouse_y);
}

/* --- Mouse move (hover) --- */

int16_t xinpcall_Update_Mouse_Move(Input* root, Rect* parent_frame, Rect* parent_clip, int16_t mouse_x,
								   int16_t mouse_y) {
	int16_t result = 0;

	for (Input* cur = root; cur && !result; cur = cur->next) {
		if (!xinpattr_Is_Input_Visible(cur) || !xinpattr_Is_Input_Active(cur))
			continue;

		Rect frame, clip;
		if (parent_frame && parent_clip) {
			frame = *parent_frame;
			clip = *parent_clip;
		} else {
			xinput_Get_System_Input_Frame(&frame);
#ifdef LANDRU_GAME_XW
			xinput_Get_System_Input_Clip(&clip);
#else
			xcanvas_Get_Drawing_Canvas_Bounds(&clip);
			xrect_Clip_Rect(&clip, &frame);
#endif
		}
		xinpcall_Clip_Input_To_Frame(cur, &frame, &clip);

		if (!xrect_Point_In_Rect(&clip, mouse_x, mouse_y))
			continue;

		if (cur->child)
			result = xinpcall_Update_Mouse_Move(cur->child, &frame, &clip, mouse_x, mouse_y);

		if (!result && cur->update && cur->mouseUsage == allInput) {
			InputUpdateFunc fn = cur->update;
			fn(cur, &frame, &clip, 0, 0, 0, mouse_x - frame.left, mouse_y - frame.top);
		}
		result = 1;
	}

	return result;
}

/* --- Mouse up/move on grabbed input --- */

#ifdef LANDRU_GAME_XW
void xinpcall_Update_Mouse_UpMove(Input* input, Rect* frame, Rect* clip, InputMouseEvent mouse_l,
								  InputMouseEvent mouse_r, int16_t mouse_x, int16_t mouse_y) {
#else
void xinpcall_Update_Mouse_UpMove(Input* input, Rect* frame, Rect* clip, InputMouseEvent mouse_l,
								  InputMouseEvent mouse_r) {
#endif
	if (input->update) {
		InputUpdateFunc fn = input->update;
#ifdef LANDRU_GAME_XW
		fn(input, frame, clip, 0, mouse_l, mouse_r, mouse_x - frame->left, mouse_y - frame->top);
#else
		fn(input, frame, clip, 0, mouse_l, mouse_r, 0, 0);
#endif
	}
	if (mouse_l == 3 || mouse_r == 3) {
		input_active_gbl = INPUT_ACTIVE_IDLE;
		input_user_gbl = NULL;
	}
}

/* --- User callback walker --- */

void xinpcall_User_Inputs(Input* root, int32_t context) {
	if (!root)
		return;

	for (Input* cur = root; cur; cur = cur->next) {
		if (!xinpattr_Is_Input_Visible(cur) || !xinpattr_Is_Input_Active(cur))
			continue;

		if (cur->user) {
			InputUserFunc fn = cur->user;
			fn(cur, context);
		}
		if (cur->child)
			xinpcall_User_Inputs(cur->child, context);
	}
}

/* --- Button state decoder --- */

int16_t xinpcall_Fetch_Input_Buttons(InputMouseEvent* out_left, InputMouseEvent* out_right) {
	InputMouseEvent left_state = 0;
	int16_t key = 0;

	if (!xio_Any_Button() && !xio_Any_Button_Release() && input_active_gbl == INPUT_ACTIVE_IDLE) {
		key = xio_Get_Free_Key();
		*out_left = 0;
		*out_right = 0;
		return key;
	}

	/* Left button */
	if (input_active_gbl != INPUT_ACTIVE_RIGHT_DOWN && input_active_gbl != INPUT_ACTIVE_RIGHT_UP) {
		if (xio_Left_Button()) {
			left_state = input_active_gbl ? 2 : 1;
		} else if (xio_Left_Button_Release()) {
			if (input_active_gbl == INPUT_ACTIVE_LEFT_DOWN) {
				left_state = 3;
			} else if (input_active_gbl == INPUT_ACTIVE_LEFT_UP) {
				if (xio_Right_Button())
					input_active_gbl = INPUT_ACTIVE_RIGHT_UP;
				else
					input_active_gbl = INPUT_ACTIVE_IDLE;
			}
		}
	}

	if (input_active_gbl == INPUT_ACTIVE_LEFT_DOWN || input_active_gbl == INPUT_ACTIVE_LEFT_UP) {
		*out_left = left_state;
		*out_right = 0;
		return 0;
	}

	/* Right button */
	if (xio_Right_Button()) {
		*out_left = left_state;
		*out_right = input_active_gbl ? 2 : 1;
		return 0;
	}

	if (xio_Right_Button_Release()) {
		if (input_active_gbl == INPUT_ACTIVE_RIGHT_DOWN) {
			*out_left = left_state;
			*out_right = 3;
			return 0;
		}
		if (input_active_gbl == INPUT_ACTIVE_RIGHT_UP) {
			if (xio_Left_Button())
				input_active_gbl = INPUT_ACTIVE_LEFT_UP;
			else
				input_active_gbl = INPUT_ACTIVE_IDLE;
		}
	}

	*out_left = left_state;
	*out_right = 0;
	return 0;
}

/* --- Tree search utilities --- */

int16_t xinpcall_Find_View_Input_Frame(Input* target, Rect* out_frame, Rect* out_clip) {
	xinput_Get_System_Input_Frame(out_frame);
#ifdef LANDRU_GAME_XW
	xinput_Get_System_Input_Clip(out_clip);
#else
	xcanvas_Get_Drawing_Canvas_Bounds(out_clip);
	xrect_Clip_Rect(out_clip, out_frame);
#endif

	Input* list = xinput_Get_Active_Input_List();
	return xinpcall_Find_Input_Frame(list, target, out_frame, out_clip);
}

int16_t xinpcall_Find_Input_Frame(Input* root, Input* target, Rect* frame, Rect* clip) {
	while (root) {
		Rect frame_save = *frame;
		Rect clip_save = *clip;

		if (xinpattr_Is_Input_Visible(root))
			xinpcall_Clip_Input_To_Frame(root, frame, clip);
		else {
			clip->right = clip->left;
			clip->bottom = clip->top;
		}

		if (root == target)
			return 1;

		if (xinpcall_Find_Input_Frame(root->child, target, frame, clip))
			return 1;

		*frame = frame_save;
		*clip = clip_save;
		root = root->next;
	}
	return 0;
}

Input* xinpcall_Check_Parent_Children_For_Input(Input* parent, Input* target) {
	Input* found = NULL;
	for (Input* cur = parent->child; cur && !found; cur = cur->next) {
		if (cur == target)
			found = parent;
		else if (cur->child)
			found = xinpcall_Check_Parent_Children_For_Input(cur, target);
	}
	return found;
}

Input* xinpcall_Find_View_Parent_For_Input(Input* target) {
	Input* list = xinput_Get_Active_Input_List();
	return xinpcall_Find_Parent_For_Input(list, target);
}

Input* xinpcall_Find_Parent_For_Input(Input* root, Input* target) {
	Input* found = NULL;
	for (Input* cur = root; cur && !found; cur = cur->next) {
		if (cur == target)
			return NULL;
		if (cur->child)
			found = xinpcall_Check_Parent_Children_For_Input(cur, target);
	}
	return found;
}

Input* xinpcall_Find_View_Input_With_ID(int16_t id) {
	Input* list = xinput_Get_Active_Input_List();
	return xinpcall_Find_Input_With_ID(list, id);
}

Input* xinpcall_Find_Input_With_ID(Input* root, int16_t id) {
	Input* found = NULL;
	for (Input* cur = root; cur && !found; cur = cur->next) {
		if (cur->id == id)
			found = cur;
		else if (cur->child)
			found = xinpcall_Find_Input_With_ID(cur->child, id);
	}
	return found;
}

#ifndef LANDRU_GAME_XW
/* --- Shared key focus (radio-button style) --- */

void xinpcall_Select_View_Shared_Key_Input(Input* target) {
	Input* list = xinput_Get_Active_Input_List();
	xinpcall_Select_Shared_Key_Input(list, target);
}

void xinpcall_Select_Shared_Key_Input(Input* root, Input* target) {
	for (Input* cur = root; cur; cur = cur->next) {
		if (cur == target) {
			cur->flags |= INPUT_KEY_FOCUSED;
			xinpattr_Refresh_Input(cur);
		} else {
			if ((cur->flags & INPUT_KEY_GROUP) && (cur->flags & INPUT_KEY_FOCUSED)) {
				cur->flags ^= INPUT_KEY_FOCUSED;
				xinpattr_Refresh_Input(cur);
			}
			if (cur->child)
				xinpcall_Select_Shared_Key_Input(cur->child, target);
		}
	}
}

#endif
