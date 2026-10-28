#include <landru/actor.h>
#include <landru/canvas.h>
#include <landru/film.h>
#include <landru/input.h>
#include <landru/memptr.h>
#include <landru/pal.h>
#include <landru/sound.h>
#include <landru/view.h>

// GLOBAL: TIE 0xD2F68
ViewStruct* view_default_gbl;
// GLOBAL: TIE 0xD2F64
ViewStruct* view_gbl;
// GLOBAL: TIE 0xD2F6C
bool view_module_gbl;

void xview_Create_View_Module(void) {
	view_default_gbl = xmemptr_Alloc_System_Pointer(sizeof(ViewStruct));
	if (!view_default_gbl)
		return;

	view_module_gbl = true;
	xview_Init_View(view_default_gbl);
	view_default_gbl->coords = 0;
	view_default_gbl->frameCount = 0;
	xview_Set_Current_View(view_default_gbl);
}

void xview_Destroy_View_Module(void) {
	if (view_default_gbl)
		xview_Free_View(view_default_gbl);
	view_module_gbl = false;
#ifdef LANDRU_MODERN
	view_default_gbl = view_gbl = NULL;
#endif
}

void xview_Init_View(ViewStruct* view) {
#ifdef LANDRU_GAME_XW
	view->input_list = NULL;
	view->dialog_list = NULL;
	view->refresh_films = 0;
	view->refresh_actors = 0;
	view->refresh_inputs = 0;
	view->refresh_dialogs = 0;
	view->dialog_update = NULL;
	xcanvas_Get_Drawing_Canvas_Clip(&view->input_frame);
#endif
	for (int i = 0; i < VIEW_LAYER_COUNT; i++) {
		if (i == 0) {
#ifdef LANDRU_GAME_XW
			xcanvas_Get_Drawing_Canvas_Clip(&view->frame[i]);
#else
			xcanvas_Get_Drawing_Canvas_Bounds(&view->frame[i]);
#endif
			view->zstart[i] = -32000;
			view->zstop[i] = 32000;
		} else {
			xrect_Set_Rect(&view->frame[i], 0, 0, 0, 0);
			view->zstart[i] = -32767;
			view->zstop[i] = -32767;
		}
		view->rel_x[i] = 0;
		view->rel_y[i] = 0;
		view->x_vel[i] = 0;
		view->y_vel[i] = 0;
		view->trackActor[i] = NULL;
		view->track_active[i] = 0;
		view->clear_view[i] = 3;
	}
#ifdef LANDRU_GAME_XW
	xcanvas_Get_Drawing_Canvas_Clip(&view->clip_frame);
#else
	xcanvas_Get_Drawing_Canvas_Bounds(&view->clip_frame);
#endif
	view->time = 0;
	view->refresh_world = 0;
	view->clear = 1;
	view->step = 0;
	view->stepCount = 0;
	view->update = NULL;
}

ViewStruct* xview_Alloc_View(void) { return xmemptr_Alloc_System_Pointer(sizeof(ViewStruct)); }

void xview_Free_View(ViewStruct* view) {
	xview_Free_All_From_View(view);
	xmemptr_Free_System_Pointer(view);
}

void xview_Free_All_From_View(ViewStruct* view) {
	(void)view;
	xfilm_Free_Films(xfilm_Ask_Film_List());
	xactor_Free_Actors(xactor_Ask_Actor_List());
	xpal_Free_Palettes(xpal_Ask_Palette_List());
#ifdef LANDRU_GAME_XW
	xinput_Free_Inputs(view->input_list);
	view->input_list = NULL;
	xinput_Free_Inputs(view->dialog_list);
	view->dialog_list = NULL;
#else
	xinput_Free_Input_Lists();
#endif
	xsound_Free_Sounds(xsound_Ask_Sound_List());
}

void xview_Set_Current_View(ViewStruct* view) { view_gbl = view; }

ViewStruct* xview_Get_Current_View(void) { return view_gbl; }

void xview_Move_View(void) {
	for (int i = 0; i < VIEW_LAYER_COUNT; i++) {
		if (view_gbl->track_active[i]) {
#ifndef LANDRU_GAME_XW
			xview_Track_View(i, 0);
#endif
		} else {
			view_gbl->rel_x[i] += view_gbl->x_vel[i];
			view_gbl->rel_y[i] += view_gbl->y_vel[i];
		}
	}
}

void xview_Origin_To_View(int16_t layer, int16_t* x, int16_t* y) {
	*x -= view_gbl->rel_x[layer];
	*y -= view_gbl->rel_y[layer];
}

void xview_View_To_Origin(int16_t layer, int16_t* x, int16_t* y) {
	*x += view_gbl->rel_x[layer];
	*y += view_gbl->rel_y[layer];
}

#ifndef LANDRU_GAME_XW
void xview_Track_View(int16_t viewId, int16_t snap) {
	if (!view_gbl->trackActor[viewId])
		return;

	int16_t x, y, xf, yf;
	Rect viewFrame;
	xactor_Get_Actor_Pos(view_gbl->trackActor[viewId], &x, &y, &xf, &yf);
	xview_Get_View_Frame(viewId, &viewFrame);

	x = x - ((viewFrame.right - viewFrame.left) >> 1) + view_gbl->track_off_x[viewId];
	y = y - ((viewFrame.bottom - viewFrame.top) >> 1) + view_gbl->track_off_y[viewId];
	xrect_Clip_Point_To_Rect(&view_gbl->trackFrame[viewId], &x, &y);

	int16_t dx = x - view_gbl->rel_x[viewId];
	int16_t dy = y - view_gbl->rel_y[viewId];

	if (!snap) {
		if (dx > view_gbl->max_track_xv[viewId])
			dx = view_gbl->max_track_xv[viewId];
		if (dy > view_gbl->max_track_yv[viewId])
			dy = view_gbl->max_track_yv[viewId];
		if (dx < -view_gbl->max_track_xv[viewId])
			dx = -view_gbl->max_track_xv[viewId];
		if (dy < -view_gbl->max_track_yv[viewId])
			dy = -view_gbl->max_track_yv[viewId];
	}

	view_gbl->rel_x[viewId] += dx;
	view_gbl->rel_y[viewId] += dy;
}

void xview_Track_Actor(int16_t layer, Actor* actor, Rect* bounds, int16_t max_vx, int16_t max_vy) {
	view_gbl->trackActor[layer] = actor;
	xview_Set_Tracking_Frame(layer, bounds);
	xview_Set_Tracking_Max_Vel(layer, max_vx, max_vy);
	int16_t size_x, size_y;
	xactor_Get_Actor_Size(actor, &size_x, &size_y);
	xview_Set_Tracking_Offset(layer, size_x >> 1, size_y >> 1);
	xview_Activate_Tracking(layer);
}

void xview_Activate_Tracking(int16_t viewId) { view_gbl->track_active[viewId] = 1; }

void xview_Deactivate_Tracking(int16_t viewId) { view_gbl->track_active[viewId] = 0; }

void xview_Set_Tracking_Frame(int16_t viewId, Rect* frame) {
	xrect_Copy_Rect(&view_gbl->trackFrame[viewId], frame);
}

void xview_Get_Tracking_Frame(int16_t viewId, Rect* outFrame) {
	xrect_Copy_Rect(outFrame, &view_gbl->trackFrame[viewId]);
}

void xview_Set_Tracking_Max_Vel(int16_t viewId, int16_t vx, int16_t vy) {
	view_gbl->max_track_xv[viewId] = vx;
	view_gbl->max_track_yv[viewId] = vy;
}

void xview_Get_Tracking_Max_Vel(int16_t viewId, int16_t* out_vx, int16_t* out_vy) {
	*out_vx = view_gbl->max_track_xv[viewId];
	*out_vy = view_gbl->max_track_yv[viewId];
}

void xview_Set_Tracking_Offset(int16_t viewId, int16_t ox, int16_t oy) {
	view_gbl->track_off_x[viewId] = ox;
	view_gbl->track_off_y[viewId] = oy;
}

void xview_Get_Tracking_Offset(int16_t viewId, int16_t* out_ox, int16_t* out_oy) {
	*out_ox = view_gbl->track_off_x[viewId];
	*out_oy = view_gbl->track_off_y[viewId];
}

#endif

void xview_Set_View_Frame(int16_t viewId, Rect* frame) { xrect_Copy_Rect(&view_gbl->frame[viewId], frame); }

void xview_Get_View_Frame(int16_t viewId, Rect* outFrame) {
	xrect_Copy_Rect(outFrame, &view_gbl->frame[viewId]);
}

int16_t xview_Get_View_Clip_Frame(int16_t viewId, Rect* outFrame) {
	xrect_Copy_Rect(outFrame, &view_gbl->frame[viewId]);
	return xrect_Clip_Rect(outFrame, &view_gbl->clip_frame);
}

void xview_Set_Full_View_Clip_Frame(Rect* frame) { xrect_Copy_Rect(&view_gbl->clip_frame, frame); }

void xview_Get_Full_View_Clip_Frame(Rect* outFrame) { xrect_Copy_Rect(outFrame, &view_gbl->clip_frame); }

void xview_Restore_Full_View_Clip_Frame(void) { xcanvas_Get_Drawing_Canvas_Bounds(&view_gbl->clip_frame); }

void xview_Set_View_ZPlane(int16_t viewId, int16_t z_near, int16_t z_far) {
	view_gbl->zstart[viewId] = z_near;
	view_gbl->zstop[viewId] = z_far;
}

void xview_Get_View_ZPlane(int16_t viewId, int16_t* out_near, int16_t* out_far) {
	*out_near = view_gbl->zstart[viewId];
	*out_far = view_gbl->zstop[viewId];
}

void xview_Set_View_Pos(int16_t viewId, int16_t x, int16_t y) {
	view_gbl->rel_x[viewId] = x;
	view_gbl->rel_y[viewId] = y;
}

void xview_Get_View_Pos(int16_t viewId, int16_t* out_x, int16_t* out_y) {
	*out_x = view_gbl->rel_x[viewId];
	*out_y = view_gbl->rel_y[viewId];
}

void xview_Set_View_Speed(int16_t viewId, int16_t vx, int16_t vy) {
	view_gbl->x_vel[viewId] = vx;
	view_gbl->y_vel[viewId] = vy;
}

void xview_Get_View_Speed(int16_t viewId, int16_t* out_vx, int16_t* out_vy) {
	*out_vx = view_gbl->x_vel[viewId];
	*out_vy = view_gbl->y_vel[viewId];
}

void xview_Set_View_Update_Function(void (*updateFunc)(int32_t)) { view_gbl->update = updateFunc; }

void xview_Get_View_Update_Function(void (**out)(int32_t)) { *out = view_gbl->update; }

void xview_Clear_View_Update_Function(void) { view_gbl->update = NULL; }

void xview_Refresh_View(void) { view_gbl->refresh_world = 1; }

bool xview_Is_View_Erase(int16_t viewId) { return view_gbl->clear_view[viewId] & 1; }

void xview_Enable_View_Erase(int16_t viewId) { view_gbl->clear_view[viewId] |= 1; }

void xview_Disable_View_Erase(int16_t viewId) { view_gbl->clear_view[viewId] &= ~1; }

void xview_Enable_Global_View_Erase(void) { view_gbl->clear = 1; }

void xview_Disable_Global_View_Erase(void) { view_gbl->clear = 0; }

void xview_Enable_All_View_Erase(void) {
	view_gbl->clear = 1;
	for (int16_t i = 0; i < VIEW_LAYER_COUNT; i++)
		xview_Enable_View_Erase(i);
}

void xview_Disable_All_View_Erase(void) {
	view_gbl->clear = 0;
	for (int16_t i = 0; i < VIEW_LAYER_COUNT; i++)
		xview_Disable_View_Erase(i);
}

bool xview_Is_View_Copy(int16_t viewId) { return view_gbl->clear_view[viewId] & 2; }

void xview_Enable_View_Copy(int16_t viewId) { view_gbl->clear_view[viewId] |= 2; }

void xview_Disable_View_Copy(int16_t viewId) { view_gbl->clear_view[viewId] &= ~2; }

int32_t xview_Get_View_Time(void) { return view_gbl->time; }

#ifdef LANDRU_GAME_XW
/* The original only clears an unread legacy word. */
void xview_ResetLegacySceneState(void) {}
#endif
