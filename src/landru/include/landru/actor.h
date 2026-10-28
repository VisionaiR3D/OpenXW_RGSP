#ifndef LANDRU_ACTOR_H
#define LANDRU_ACTOR_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

#include <landru/rect.h>
#include <landru/render.h>

typedef struct Actor Actor;

typedef int16_t (*xactorDrawFunc)(Actor*, Rect*, Rect*, int16_t, int16_t, int16_t);
typedef void (*xactorUpdateFunc)(Actor*);
typedef void (*xactorCallback)(Actor*, int32_t);
typedef void (*xactorFrameFunc)(Actor*, Rect*);
typedef void (*xactorStateFunc)(Actor*, int16_t, int16_t);

enum ActorFlags {
	AF_VISIBLE = 0x0001,
	AF_ACTIVE = 0x0002,
	AF_DISCARD = 0x0004,
	AF_DIRTY = 0x0008,
	AF_REFRESHABLE = 0x0010,
	AF_REFRESH = 0x0020,
	/* The custom draw callback does not render this actor's standard pose.
	 * Such actors are omitted from the render sink, while framebuffer
	 * rendering continues to call actor->draw. */
	AF_NO_RENDER_CAPTURE = 0x0040,
	AF_HFLIP = 0x0100,
	AF_VFLIP = 0x0200,
	AF_REMAP_COLOR = 0x0400,
	AF_FLAG1 = 0x4000,
	AF_FLAG2 = 0x8000,
};

typedef struct {
	uint32_t type;
	void* next;
	xactorFrameFunc frameFunc;
	xactorStateFunc stateFunc;
} ActorType;

struct Actor {
	uint32_t res_type;
	char res_name[8];
	Actor* next;
	int32_t start;
	int32_t stop;
	Rect frame;
	Rect frame_v;
	Rect bounds;
	int16_t x;
	int16_t y;
	int16_t xf;
	int16_t yf;
	int16_t xv;
	int16_t yv;
	int16_t xvf;
	int16_t yvf;
	int16_t w;
	int16_t h;
	int16_t zplane;
	uint16_t flags;
	int16_t id;
	int16_t state;
	int16_t state_f;
	int16_t state_v;
	int16_t state_vf;
	int16_t foreColor;
	int16_t backColor;
	int16_t xscale;
	int16_t yscale;
	int16_t var1;
	int16_t var2;
	void* varptr;
	LandruHandle varhdl;
	LandruHandle data;
	LandruHandle array;
	int16_t arraySize;
	xactorDrawFunc draw;
	xactorUpdateFunc update;
	xactorCallback user;
#ifdef LANDRU_MODERN
	/* State captured before the most recent movement for interpolation. */
	int16_t prev_x;
	int16_t prev_y;
	int16_t prev_xv;
	int16_t prev_yv;
	int16_t prev_xscale;
	int16_t prev_yscale;
	int16_t film_entry_index;
#endif
};

void xactor_Create_Actor_Module(void);
void xactor_Destroy_Actor_Module(void);
bool xactor_Create_Actor_Type(uint32_t type, xactorFrameFunc frameFunc, xactorStateFunc stateFunc);
void xactor_Destroy_Actor_Type(uint32_t type);
ActorType* xactor_Find_Actor_Type(uint32_t type);
Actor* xactor_Ask_Actor_List(void);
void xactor_Add_Actor_To_System(Actor* actor);
void xactor_Free_Actor_From_System(Actor* actor);
Actor* xactor_Alloc_Actor(int16_t extend);
void xactor_Free_Actor(Actor* actor);
void xactor_Free_Actor_Data(Actor* actor);
void xactor_Free_Actors(Actor* actor);
void xactor_Refresh_Actors(void);
#ifdef LANDRU_GAME_XW
void xactor_Draw_Actors(Actor* list, int16_t refresh);
void xactor_Draw_System_Actors(void);
#else
void xactor_Draw_Actors(int16_t refresh);
#endif
void xactor_Update_Actors(int32_t time);
void xactor_User_Actors(int32_t time);
Actor* xactor_Find_Actor(uint32_t type, const char* name);
void xactor_Show_Actor(Actor* actor);
void xactor_Hide_Actor(Actor* actor);
LandruBoolResult xactor_Is_Actor_Visible(Actor* actor);
void xactor_Activate_Actor(Actor* actor);
void xactor_Deactivate_Actor(Actor* actor);
bool xactor_Is_Actor_Active(Actor* actor);
void xactor_Start_Actor(Actor* actor);
void xactor_Stop_Actor(Actor* actor);
void xactor_Dirty_Actor(Actor* actor);
void xactor_Non_Dirty_Actor(Actor* actor);
bool xactor_Is_Actor_Dirty(Actor* actor);
void xactor_Refresh_Actor(Actor* actor);
void xactor_Non_Refresh_Actor(Actor* actor);
int16_t xactor_Is_Actor_Refresh(Actor* actor);
void xactor_Refreshable_Actor(Actor* actor);
void xactor_Non_Refreshable_Actor(Actor* actor);
int16_t xactor_Is_Actor_Refreshable(Actor* actor);
void xactor_Discard_Actor_Data(Actor* actor);
void xactor_Non_Discard_Actor_Data(Actor* actor);
bool xactor_Is_Discard_Actor_Data(Actor* actor);
void xactor_Set_Actor_Flag1(Actor* actor);
void xactor_Clear_Actor_Flag1(Actor* actor);
int16_t xactor_Is_Actor_Flag1(Actor* actor);
void xactor_Set_Actor_Flag2(Actor* actor);
void xactor_Clear_Actor_Flag2(Actor* actor);
bool xactor_Is_Actor_Flag2(Actor* actor);
void xactor_Move_Actor(Actor* actor);
void xactor_Move_Actor_Frame(Actor* actor);
void xactor_Move_Actor_State(Actor* actor);
void xactor_Copy_Actor_Data(Actor* dst, Actor* src);
bool xactor_Clip_Actor_To_Actor(Actor* actor, Actor* clip_to);
void xactor_Check_Actor_ZPlanes(void);
void xactor_Sort_Actor_ZPlanes(void);
void xactor_Get_Actor_Relative_XY(Actor* actor, Rect* rect, int16_t* outX, int16_t* outY);
void xactor_Get_Actor_Offset(Actor* actor, int16_t* outX, int16_t* outY);
void xactor_Set_Actor_Name(Actor* actor, uint32_t res_type, const char* name);
void xactor_Set_Actor_Frame(Actor* actor, Rect* frame);
void xactor_Get_Actor_Frame(Actor* actor, Rect* outFrame);
void xactor_Set_Actor_Bounds(Actor* actor, Rect* bounds);
void xactor_Get_Actor_Bounds(Actor* actor, Rect* outBounds);
void xactor_Set_Actor_Pos(Actor* actor, int16_t x, int16_t y, int16_t xf, int16_t yf);
void xactor_Get_Actor_Pos(Actor* actor, int16_t* outX, int16_t* outY, int16_t* outXf, int16_t* outYf);
void xactor_Get_Actor_Center(Actor* actor, int16_t* outCX, int16_t* outCY);
void xactor_Set_Actor_Size(Actor* actor, int16_t w, int16_t h);
void xactor_Get_Actor_Size(Actor* actor, int16_t* outW, int16_t* outH);
void xactor_Get_Actor_Rect(Actor* actor, Rect* outRect);
void xactor_Get_Actor_Base_Rect(Actor* actor, Rect* outRect);
void xactor_Set_Actor_ZPlane(Actor* actor, int16_t zplane);
int16_t xactor_Get_Actor_ZPlane(Actor* actor);
void xactor_Set_Actor_Time(Actor* actor, int32_t start, int32_t stop);
void xactor_Get_Actor_Time(Actor* actor, int32_t* outStart, int32_t* outStop);
void xactor_Set_Actor_State(Actor* actor, int16_t state, int16_t stateFract);
void xactor_Get_Actor_State(Actor* actor, int16_t* outState, int16_t* outStateFract);
void xactor_Set_Actor_State_Speed(Actor* actor, int16_t stateV, int16_t stateVf);
void xactor_Get_Actor_State_Speed(Actor* actor, int16_t* outV, int16_t* outVf);
void xactor_Set_Actor_Color(Actor* actor, int16_t fore, int16_t back);
void xactor_Get_Actor_Color(Actor* actor, int16_t* outFore, int16_t* outBack);
void xactor_Set_Actor_Remap_Color(Actor* actor, int16_t color);
int16_t xactor_Get_Actor_Remap_Color(Actor* actor);
void xactor_Clear_Actor_Remap_Color(Actor* actor);
void xactor_Set_Actor_Scale(Actor* actor, int16_t xScale, int16_t yScale);
void xactor_Get_Actor_Scale(Actor* actor, int16_t* outXScale, int16_t* outYScale);
void xactor_Set_Actor_Flip(Actor* actor, int16_t hFlip, int16_t vFlip);
void xactor_Get_Actor_Flip(Actor* actor, int16_t* outHFlip, int16_t* outVFlip);
void xactor_Set_Actor_Speed(Actor* actor, int16_t xv, int16_t yv, int16_t xvf, int16_t yvf);
void xactor_Get_Actor_Speed(Actor* actor, int16_t* outXv, int16_t* outYv, int16_t* outXvf, int16_t* outYvf);
void xactor_Set_Actor_Frame_Speed(Actor* actor, int16_t left, int16_t top, int16_t right, int16_t bottom);
void xactor_Get_Actor_Frame_Speed(Actor* actor, int16_t* outLeft, int16_t* outTop, int16_t* outRight,
								  int16_t* outBottom);
void xactor_Set_Actor_Data(Actor* actor, LandruHandle data);
void xactor_Get_Actor_Data(Actor* actor, LandruHandle* outData);
void xactor_Set_Actor_Array(Actor* actor, LandruHandle array);
void xactor_Get_Actor_Array(Actor* actor, LandruHandle* outArray);
LandruHandle xactor_Get_Actor_Array_Data(Actor* actor, int16_t index);
void xactor_Set_Actor_Draw_Function(Actor* actor, xactorDrawFunc drawFunc);
void xactor_Get_Actor_Draw_Function(Actor* actor, xactorDrawFunc* outDraw);
void xactor_Set_Actor_Update_Function(Actor* actor, xactorUpdateFunc updateFunc);
void xactor_Get_Actor_Update_Function(Actor* actor, xactorUpdateFunc* outUpdate);
void xactor_Set_Actor_User_Function(Actor* actor, xactorCallback userFunc);
void xactor_Get_Actor_User_Function(Actor* actor, xactorCallback* outUser);

#ifdef LANDRU_MODERN
/* Publish the live actor list to the optional render sink. */
void xactor_emit_render_state(void);

/* Populate one renderer-neutral state record from an Actor. The same per-actor
 * body xactor_emit_render_state uses, exposed so callers that need to
 * capture a frozen actor pose at a specific moment can reuse the position /
 * HFLIP / VFLIP / clip / scale / fore_color logic without
 * duplicating it. `out` is filled fully. */
void xactor_fill_render_state(const Actor* a, LandruActorRenderState* out);

/* Toggle AF_NO_RENDER_CAPTURE — see the flag's comment. Scenes call
 * this on orchestration actors whose custom draw callback renders
 * other state (or no pixels at all). */
void xactor_Set_Actor_Render_Capture_Hidden(Actor* actor, bool hidden);

/* Publish one imperative actor draw at the supplied classic coordinates.
 * Called from
 * inside lact{delt,anim,raw}_Draw_*_Actor at the moment of the
 * imperative draw — captures per-call state that the live actor list
 * cannot represent (dialog UI in
 * computer.c / register.c draws the same Actor* multiple times per
 * frame at different positions and states). */
void xactor_emit_draw(Actor* actor, int16_t xoff, int16_t yoff);

#endif

#ifdef LANDRU_GAME_XW
void xactor_ResetSceneState(void);
#endif

#endif
