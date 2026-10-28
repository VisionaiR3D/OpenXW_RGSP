#ifndef LANDRU_VIEW_H
#define LANDRU_VIEW_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/actor.h>
#include <landru/input.h>
#include <landru/rect.h>

enum {
	VIEW_LAYER_COUNT = 4,
	VIEW_RESERVED_FRAME_BYTES = 32,
	VIEW_RESERVED_TRACK_BYTES = 16,
	VIEW_LEGACY_DEBUG_BYTES = 48,
	VIEW_INPUT_WIDTH = 320,
	VIEW_INPUT_HEIGHT = 200
};
typedef struct ViewStruct ViewStruct;
#ifdef LANDRU_GAME_XW
struct ViewStruct {
	Input* input_list;
	Input* dialog_list;
	Rect frame[VIEW_LAYER_COUNT];
	int16_t zstart[VIEW_LAYER_COUNT];
	int16_t zstop[VIEW_LAYER_COUNT];
	int16_t rel_x[VIEW_LAYER_COUNT];
	int16_t rel_y[VIEW_LAYER_COUNT];
	int16_t x_vel[VIEW_LAYER_COUNT];
	int16_t y_vel[VIEW_LAYER_COUNT];
	uint8_t reserved_frames[VIEW_RESERVED_FRAME_BYTES];
	Actor* trackActor[VIEW_LAYER_COUNT];
	uint8_t reserved_speeds[VIEW_RESERVED_TRACK_BYTES];
	int16_t track_active[VIEW_LAYER_COUNT];
	uint8_t reserved_offsets[VIEW_RESERVED_TRACK_BYTES];
	int16_t clear_view[VIEW_LAYER_COUNT];
	Rect input_frame;
	Rect clip_frame;
	int32_t time;
	int16_t refresh_world;
	int16_t refresh_films;
	int16_t refresh_actors;
	int16_t refresh_inputs;
	int16_t refresh_dialogs;
	int16_t clear;
	int16_t step;
	int16_t stepCount;
	int16_t coords;
	int16_t frameCount;
	void (*update)(int32_t);
	void (*dialog_update)(int32_t);
};
#else
struct ViewStruct {
	Rect frame[VIEW_LAYER_COUNT];
	int16_t zstart[VIEW_LAYER_COUNT];
	int16_t zstop[VIEW_LAYER_COUNT];
	int16_t rel_x[VIEW_LAYER_COUNT];
	int16_t rel_y[VIEW_LAYER_COUNT];
	int16_t x_vel[VIEW_LAYER_COUNT];
	int16_t y_vel[VIEW_LAYER_COUNT];
	Rect trackFrame[VIEW_LAYER_COUNT];
	Actor* trackActor[VIEW_LAYER_COUNT];
	int16_t max_track_xv[VIEW_LAYER_COUNT];
	int16_t max_track_yv[VIEW_LAYER_COUNT];
	int16_t track_active[VIEW_LAYER_COUNT];
	int16_t track_off_x[VIEW_LAYER_COUNT];
	int16_t track_off_y[VIEW_LAYER_COUNT];
	int16_t clear_view[VIEW_LAYER_COUNT];
	uint8_t reserved[VIEW_LEGACY_DEBUG_BYTES];
	Rect clip_frame;
	int32_t time;
	int16_t refresh_world;
	int16_t clear;
	int16_t step;
	int16_t stepCount;
	int16_t coords;
	int16_t frameCount;
	void (*update)(int32_t);
};
#endif

extern ViewStruct* view_default_gbl;
extern ViewStruct* view_gbl;
extern bool view_module_gbl;

void xview_Create_View_Module(void);
void xview_Destroy_View_Module(void);
void xview_Init_View(ViewStruct* view);
ViewStruct* xview_Alloc_View(void);
void xview_Free_View(ViewStruct* view);
void xview_Free_All_From_View(ViewStruct* view);
void xview_Set_Current_View(ViewStruct* view);
ViewStruct* xview_Get_Current_View(void);
void xview_Move_View(void);
void xview_Origin_To_View(int16_t layer, int16_t* x, int16_t* y);
void xview_View_To_Origin(int16_t layer, int16_t* x, int16_t* y);
#ifndef LANDRU_GAME_XW
void xview_Track_View(int16_t viewId, int16_t snap);
void xview_Track_Actor(int16_t layer, Actor* actor, Rect* bounds, int16_t max_vx, int16_t max_vy);
void xview_Activate_Tracking(int16_t viewId);
void xview_Deactivate_Tracking(int16_t viewId);
void xview_Set_Tracking_Frame(int16_t viewId, Rect* frame);
void xview_Get_Tracking_Frame(int16_t viewId, Rect* outFrame);
void xview_Set_Tracking_Max_Vel(int16_t viewId, int16_t vx, int16_t vy);
void xview_Get_Tracking_Max_Vel(int16_t viewId, int16_t* out_vx, int16_t* out_vy);
void xview_Set_Tracking_Offset(int16_t viewId, int16_t ox, int16_t oy);
void xview_Get_Tracking_Offset(int16_t viewId, int16_t* out_ox, int16_t* out_oy);
#endif
void xview_Set_View_Frame(int16_t viewId, Rect* frame);
void xview_Get_View_Frame(int16_t viewId, Rect* outFrame);
int16_t xview_Get_View_Clip_Frame(int16_t viewId, Rect* outFrame);
void xview_Set_Full_View_Clip_Frame(Rect* frame);
void xview_Get_Full_View_Clip_Frame(Rect* outFrame);
void xview_Restore_Full_View_Clip_Frame(void);
void xview_Set_View_ZPlane(int16_t viewId, int16_t z_near, int16_t z_far);
void xview_Get_View_ZPlane(int16_t viewId, int16_t* out_near, int16_t* out_far);
void xview_Set_View_Pos(int16_t viewId, int16_t x, int16_t y);
void xview_Get_View_Pos(int16_t viewId, int16_t* out_x, int16_t* out_y);
void xview_Set_View_Speed(int16_t viewId, int16_t vx, int16_t vy);
void xview_Get_View_Speed(int16_t viewId, int16_t* out_vx, int16_t* out_vy);
void xview_Set_View_Update_Function(void (*updateFunc)(int32_t));
void xview_Get_View_Update_Function(void (**out)(int32_t));
void xview_Clear_View_Update_Function(void);
void xview_Refresh_View(void);
bool xview_Is_View_Erase(int16_t viewId);
void xview_Enable_View_Erase(int16_t viewId);
void xview_Disable_View_Erase(int16_t viewId);
void xview_Enable_Global_View_Erase(void);
void xview_Disable_Global_View_Erase(void);
void xview_Enable_All_View_Erase(void);
void xview_Disable_All_View_Erase(void);
bool xview_Is_View_Copy(int16_t viewId);
void xview_Enable_View_Copy(int16_t viewId);
void xview_Disable_View_Copy(int16_t viewId);
int32_t xview_Get_View_Time(void);

#ifdef LANDRU_GAME_XW
void xview_ResetLegacySceneState(void);
#endif

#endif
