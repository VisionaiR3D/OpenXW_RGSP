#include <landru/actor.h>
#include <landru/canvas.h>
#ifdef LANDRU_GAME_XW
#include <landru/fourcc.h>
#endif
#include <landru/memptr.h>
#include <landru/rect.h>
#include <landru/render.h>
#include <landru/timer.h>
#include <landru/view.h>
#include <landru/viewadd.h>

#include "render_internal.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// GLOBAL: TIE 0xFBB80
static ActorType* actor_type_gbl;
// GLOBAL: TIE 0xD2C1C
static Actor* actor_list_gbl;
// GLOBAL: TIE 0xD2C22
static bool actor_module_gbl;
// GLOBAL: TIE 0xFBB84
static bool refresh_actors_gbl;
// GLOBAL: TIE 0xD2C20
static bool zplane_build_gbl;

/* --- Module lifecycle --- */

void xactor_Create_Actor_Module(void) {
	actor_type_gbl = NULL;
	actor_list_gbl = NULL;
	actor_module_gbl = true;
}

void xactor_Destroy_Actor_Module(void) { actor_module_gbl = false; }

/* --- Actor type registry --- */

bool xactor_Create_Actor_Type(uint32_t type, xactorFrameFunc frameFunc, xactorStateFunc stateFunc) {
	ActorType* t = xmemptr_Alloc_Clear_System_Pointer(sizeof(ActorType));
	if (t) {
		t->type = type;
		t->frameFunc = frameFunc;
		t->stateFunc = stateFunc;
		t->next = actor_type_gbl;
		actor_type_gbl = t;
	}
	return t != NULL;
}

void xactor_Destroy_Actor_Type(uint32_t type) {
	ActorType* cur = actor_type_gbl;
	ActorType* prev = NULL;
	while (cur && cur->type != type) {
		prev = cur;
		cur = cur->next;
	}
	if (cur) {
		if (prev)
			prev->next = cur->next;
		else
			actor_type_gbl = actor_type_gbl->next;
		xmemptr_Free_System_Pointer(cur);
	}
}

ActorType* xactor_Find_Actor_Type(uint32_t type) {
	for (ActorType* t = actor_type_gbl; t; t = t->next)
		if (t->type == type)
			return t;
	return NULL;
}

/* --- Actor list management --- */

Actor* xactor_Ask_Actor_List(void) { return actor_list_gbl; }

void xactor_Add_Actor_To_System(Actor* actor) {
	Actor* cur = actor_list_gbl;
	Actor* prev = NULL;
	while (cur) {
		if (actor->zplane >= cur->zplane)
			break;
		prev = cur;
		cur = cur->next;
	}
	actor->next = cur;
	if (prev)
		prev->next = actor;
	else
		actor_list_gbl = actor;
	actor->id = 0;
}

void xactor_Free_Actor_From_System(Actor* actor) {
	Actor* cur = actor_list_gbl;
	Actor* prev = NULL;
	while (cur && cur != actor) {
		prev = cur;
		cur = cur->next;
	}
	if (cur) {
		if (prev)
			prev->next = cur->next;
		else
			actor_list_gbl = cur->next;
		actor->next = NULL;
	}
}

/* --- Alloc / Free --- */

Actor* xactor_Alloc_Actor(int16_t extend) {
	Actor* actor = xmemptr_Alloc_Clear_System_Pointer(sizeof(Actor) + extend);
	if (actor) {
		actor->start = 0;
		actor->stop = -1;
		actor->flags = AF_DIRTY | AF_REFRESHABLE | AF_REFRESH;
		actor->foreColor = 15;
		actor->backColor = 0;
		actor->xscale = 256;
		actor->yscale = 256;

#ifdef LANDRU_MODERN
		actor->prev_xscale = 256;
		actor->prev_yscale = 256;
		actor->film_entry_index = -1;
#endif
		actor->next = NULL;
	}
	return actor;
}

void xactor_Free_Actor(Actor* actor) {
	xactor_Free_Actor_Data(actor);
	if (actor->varptr)
		xmemptr_Free_System_Pointer(actor->varptr);
	if (actor->varhdl)
		xmemhdl_Free_Handle(actor->varhdl);
	xmemptr_Free_System_Pointer(actor);
}

void xactor_Free_Actor_Data(Actor* actor) {
	if (xactor_Is_Discard_Actor_Data(actor)) {
		if (actor->data)
			xmemhdl_Free_Handle(actor->data);
		if (actor->array) {
			LandruHandle* array = xmemhdl_Lock_Handle(actor->array);
			for (int16_t i = 0; i < actor->arraySize; i++) {
				if (array[i])
					xmemhdl_Free_Handle(array[i]);
			}
			xmemhdl_Free_Handle(actor->array);
		}
	}
	actor->array = LANDRU_NULL_HANDLE;
	actor->arraySize = 0;
	actor->data = LANDRU_NULL_HANDLE;
}

void xactor_Free_Actors(Actor* actor) {
	while (actor) {
		Actor* next = actor->next;
		xactor_Free_Actor_From_System(actor);
		xactor_Free_Actor(actor);
		actor = next;
	}
}

/* --- Draw / Update / User loops --- */

void xactor_Refresh_Actors(void) {
#ifdef LANDRU_GAME_XW
	view_gbl->refresh_actors = 1;
#else
	refresh_actors_gbl = true;
#endif
}

#ifdef LANDRU_GAME_XW
void xactor_Draw_Actors(Actor* list, int16_t refresh) {
#else
void xactor_Draw_Actors(int16_t refresh) {
#endif
#ifdef LANDRU_GAME_XW
	int16_t do_refresh = refresh;
	Actor* head = list;
#else
	int16_t do_refresh = refresh | refresh_actors_gbl;
	refresh_actors_gbl = false;

	Actor* head = actor_list_gbl;
#endif
	for (int16_t vi = 0; vi < 4; vi++) {
		Rect r;
		xview_Get_View_Frame(vi, &r);
		if (xrect_Empty_Rect(&r))
			continue;

		for (Actor* cur = head; cur; cur = cur->next) {
			xtimer_Often();
			if (!cur->draw)
				continue;
			if (!xactor_Is_Actor_Visible(cur))
				continue;

			Rect clip;
			if (!xviewadd_Clip_Object_To_View(vi, cur->zplane, &cur->frame, &r, &clip))
				continue;

			int16_t needs_draw = do_refresh | xactor_Is_Actor_Refresh(cur) | xactor_Is_Actor_Refreshable(cur);

			xcanvas_Set_Drawing_Canvas_Clip(&clip);
			int16_t rel_x, rel_y;
			xactor_Get_Actor_Relative_XY(cur, &r, &rel_x, &rel_y);
			cur->draw(cur, &r, &clip, rel_x, rel_y, needs_draw);
		}
	}

	for (Actor* it = head; it; it = it->next)
		xactor_Non_Refresh_Actor(it);
}

void xactor_Update_Actors(int32_t time) {
	for (Actor* cur = actor_list_gbl; cur; cur = cur->next) {
		if (time == cur->start) {
			xactor_Start_Actor(cur);
			if (cur->start == cur->stop)
				xactor_Stop_Actor(cur);
			continue;
		}
		if (time == cur->stop) {
			xactor_Stop_Actor(cur);
		} else if (cur->update && xactor_Is_Actor_Active(cur)) {
			cur->update(cur);
		}
	}
}

void xactor_User_Actors(int32_t time) {
	for (Actor* cur = actor_list_gbl; cur; cur = cur->next)
		if (cur->user)
			cur->user(cur, time);
}

/* --- Find --- */

Actor* xactor_Find_Actor(uint32_t type, const char* name) {
	for (Actor* cur = actor_list_gbl; cur; cur = cur->next) {
		if (cur->res_type != type)
			continue;
		bool match = true;
		int i;
		for (i = 0; i < 8 && match && name[i]; i++) {
			if (tolower((unsigned char)name[i]) != (unsigned char)cur->res_name[i])
				match = false;
		}
		if (match && (i >= 8 || !cur->res_name[i]))
			return cur;
	}
	return NULL;
}

/* --- Flag accessors --- */

void xactor_Show_Actor(Actor* actor) {
#ifdef LANDRU_GAME_XW
	if (!actor)
		return;
#endif
	actor->flags |= AF_VISIBLE;
}

void xactor_Hide_Actor(Actor* actor) {
#ifdef LANDRU_GAME_XW
	if (!actor)
		return;
#endif
	actor->flags &= ~AF_VISIBLE;
}

LandruBoolResult xactor_Is_Actor_Visible(Actor* actor) { return actor->flags & AF_VISIBLE; }

void xactor_Activate_Actor(Actor* actor) { actor->flags |= AF_ACTIVE; }

void xactor_Deactivate_Actor(Actor* actor) { actor->flags &= ~AF_ACTIVE; }

bool xactor_Is_Actor_Active(Actor* actor) { return actor->flags & AF_ACTIVE; }

void xactor_Start_Actor(Actor* actor) { actor->flags |= (AF_VISIBLE | AF_ACTIVE); }

void xactor_Stop_Actor(Actor* actor) { actor->flags &= ~(AF_VISIBLE | AF_ACTIVE); }

void xactor_Dirty_Actor(Actor* actor) { actor->flags |= AF_DIRTY; }

void xactor_Non_Dirty_Actor(Actor* actor) { actor->flags &= ~AF_DIRTY; }

bool xactor_Is_Actor_Dirty(Actor* actor) { return actor->flags & AF_DIRTY; }

void xactor_Refresh_Actor(Actor* actor) {
#ifdef LANDRU_GAME_XW
	/* X-Wing permits absent optional film actors (0x46BAB0). */
	if (!actor)
		return;
#endif
	actor->flags |= AF_REFRESH;
}

void xactor_Non_Refresh_Actor(Actor* actor) {
#ifdef LANDRU_GAME_XW
	if (!actor)
		return;
#endif
	actor->flags &= ~AF_REFRESH;
}

int16_t xactor_Is_Actor_Refresh(Actor* actor) { return actor->flags & AF_REFRESH; }

void xactor_Refreshable_Actor(Actor* actor) { actor->flags |= AF_REFRESHABLE; }

void xactor_Non_Refreshable_Actor(Actor* actor) {
#ifdef LANDRU_GAME_XW
	/* Same optional-actor contract as the original X-Wing entry point (0x46BAE0). */
	if (!actor)
		return;
#endif
	actor->flags &= ~AF_REFRESHABLE;
}

int16_t xactor_Is_Actor_Refreshable(Actor* actor) { return actor->flags & AF_REFRESHABLE; }

void xactor_Discard_Actor_Data(Actor* actor) { actor->flags |= AF_DISCARD; }

void xactor_Non_Discard_Actor_Data(Actor* actor) { actor->flags &= ~AF_DISCARD; }

bool xactor_Is_Discard_Actor_Data(Actor* actor) { return actor->flags & AF_DISCARD; }

void xactor_Set_Actor_Flag1(Actor* actor) { actor->flags |= AF_FLAG1; }

void xactor_Clear_Actor_Flag1(Actor* actor) { actor->flags &= ~AF_FLAG1; }

int16_t xactor_Is_Actor_Flag1(Actor* actor) { return actor->flags & AF_FLAG1; }

void xactor_Set_Actor_Flag2(Actor* actor) { actor->flags |= AF_FLAG2; }

void xactor_Clear_Actor_Flag2(Actor* actor) { actor->flags &= ~AF_FLAG2; }

bool xactor_Is_Actor_Flag2(Actor* actor) { return actor->flags & AF_FLAG2; }

/* --- Movement --- */

void xactor_Move_Actor(Actor* actor) {
#ifdef LANDRU_MODERN
	/* Capture the pre-integration position and the velocity being integrated.
	 * Render state ships (prev_x, x) per cel for consumer interpolation.
	 * prev_xv/yv let the consumer's teleport check compare against the
	 * vel that produced dx — the FILM script may rewrite actor->xv
	 * for the NEXT cel after Move_Actor returns (FCMD_ACTOR_VEL is
	 * processed inside xfilm_Step_Actor_Film AFTER the actor->update
	 * call), and using the post-script xv would misflag legitimate
	 * velocity-change cels (e.g. logoluke 15→12 at cel 10) as
	 * teleports. */
	actor->prev_x = actor->x;
	actor->prev_y = actor->y;
	actor->prev_xv = actor->xv;
	actor->prev_yv = actor->yv;
	/* Scale has no engine-driven velocity — user callbacks (e.g.
	 * title.c's Star-Wars-logo zoom) write actor->xscale/yscale
	 * directly. Capture the cel-start scale here so the render state
	 * ships (prev → current) for the compositor to lerp. */
	actor->prev_xscale = actor->xscale;
	actor->prev_yscale = actor->yscale;
#endif
	actor->xf += actor->xvf;
	actor->yf += actor->yvf;
	actor->x += actor->xv + actor->xf / 256;
	actor->y += actor->yv + actor->yf / 256;
	while (actor->xf >= 256)
		actor->xf -= 256;
	while (actor->yf >= 256)
		actor->yf -= 256;
	while (actor->xf <= -256)
		actor->xf += 256;
	while (actor->yf <= -256)
		actor->yf += 256;
}

void xactor_Move_Actor_Frame(Actor* actor) {
	actor->frame.left += actor->frame_v.left;
	actor->frame.top += actor->frame_v.top;
	actor->frame.right += actor->frame_v.right;
	actor->frame.bottom += actor->frame_v.bottom;
}

void xactor_Move_Actor_State(Actor* actor) {
	actor->state_f += actor->state_vf;
	actor->state += actor->state_v + actor->state_f / 256;
	while (actor->state_f >= 256)
		actor->state_f -= 256;
	while (actor->state_f <= -256)
		actor->state_f += 256;
}

/* --- Data copy --- */

void xactor_Copy_Actor_Data(Actor* dst, Actor* src) {
	xactor_Free_Actor_Data(dst);
	xactor_Non_Discard_Actor_Data(dst);
	xactor_Set_Actor_Name(dst, src->res_type, src->res_name);
	dst->data = src->data;
	dst->array = src->array;
	dst->arraySize = src->arraySize;
	xrect_Copy_Rect(&dst->bounds, &src->bounds);
	dst->w = src->w;
	dst->h = src->h;
}

bool xactor_Clip_Actor_To_Actor(Actor* actor, Actor* clip_to) {
	return xrect_Clip_Rect(&actor->frame, &clip_to->frame);
}

/* --- Z-plane sorting --- */

static void sort_actor_list(void) {
	Actor* sorted = NULL;
	Actor* cur = actor_list_gbl;
	while (cur) {
		Actor* node = cur;
		cur = cur->next;
		node->next = NULL;
		Actor *pos = sorted, *prev = NULL;
		while (pos && pos->zplane >= node->zplane) {
			prev = pos;
			pos = pos->next;
		}
		if (prev)
			prev->next = node;
		else
			sorted = node;
		if (pos)
			node->next = pos;
	}
	actor_list_gbl = sorted;
}

void xactor_Check_Actor_ZPlanes(void) {
	if (!zplane_build_gbl)
		return;
	zplane_build_gbl = false;
	sort_actor_list();
}

void xactor_Sort_Actor_ZPlanes(void) { sort_actor_list(); }

/* --- Position / coordinate helpers --- */

void xactor_Get_Actor_Relative_XY(Actor* actor, Rect* rect, int16_t* outX, int16_t* outY) {
	*outX = rect->left + actor->x - actor->frame.left;
	*outY = rect->top + actor->y - actor->frame.top;
}

void xactor_Get_Actor_Offset(Actor* actor, int16_t* outX, int16_t* outY) {
	ActorType* t = xactor_Find_Actor_Type(actor->res_type);
	if (t && t->frameFunc) {
		Rect frame;
		t->frameFunc(actor, &frame);
		*outX = frame.left;
		*outY = frame.top;
	} else {
		*outX = 0;
		*outY = 0;
	}
}

void xactor_Set_Actor_Name(Actor* actor, uint32_t res_type, const char* name) {
	actor->res_type = res_type;
	int i;
	for (i = 0; i < 8 && name[i]; i++)
		actor->res_name[i] = tolower((unsigned char)name[i]);
	for (; i < 8; i++)
		actor->res_name[i] = 0;
}

/* --- Getters / Setters --- */

void xactor_Set_Actor_Frame(Actor* actor, Rect* frame) { xrect_Copy_Rect(&actor->frame, frame); }

void xactor_Get_Actor_Frame(Actor* actor, Rect* out) { xrect_Copy_Rect(out, &actor->frame); }

void xactor_Set_Actor_Bounds(Actor* actor, Rect* b) { xrect_Copy_Rect(&actor->bounds, b); }

void xactor_Get_Actor_Bounds(Actor* actor, Rect* out) { xrect_Copy_Rect(out, &actor->bounds); }

void xactor_Set_Actor_Pos(Actor* actor, int16_t x, int16_t y, int16_t xf, int16_t yf) {
	actor->x = x;
	actor->y = y;
	actor->xf = xf;
	actor->yf = yf;
#ifdef LANDRU_MODERN
	/* Reseed prev_x/y so a freshly-placed actor (or one that's just
	 * teleported via FCMD_ACTOR_POS / Rewind_Actor_Film) doesn't
	 * render as a slide from its previous-cel commit position. Zero
	 * prev_xv/yv so the teleport-detection tolerance matches the
	 * (zero) integration that just "happened" — i.e. the lerp
	 * collapses to a no-op at the snap target. */
	actor->prev_x = x;
	actor->prev_y = y;
	actor->prev_xv = 0;
	actor->prev_yv = 0;
#endif
}

void xactor_Get_Actor_Pos(Actor* actor, int16_t* x, int16_t* y, int16_t* xf, int16_t* yf) {
	*x = actor->x;
	*y = actor->y;
	*xf = actor->xf;
	*yf = actor->yf;
}

void xactor_Get_Actor_Center(Actor* actor, int16_t* cx, int16_t* cy) {
	*cx = actor->x + actor->w / 2;
	*cy = actor->y + actor->h / 2;
}

void xactor_Set_Actor_Size(Actor* actor, int16_t w, int16_t h) {
	actor->w = w;
	actor->h = h;
}

void xactor_Get_Actor_Size(Actor* actor, int16_t* w, int16_t* h) {
	*w = actor->w;
	*h = actor->h;
}

void xactor_Get_Actor_Rect(Actor* actor, Rect* out) {
	xactor_Get_Actor_Base_Rect(actor, out);
	xrect_Offset_Rect(out, actor->x, actor->y);
}

void xactor_Get_Actor_Base_Rect(Actor* actor, Rect* out) {
	ActorType* t = xactor_Find_Actor_Type(actor->res_type);
	if (t && t->frameFunc) {
		t->frameFunc(actor, out);
#ifdef LANDRU_GAME_XW
		/* DOS X-Wing mirrors ANIM/DELT bounds before tracking dirty regions. */
		if ((actor->res_type == FOURCC_ANIM || actor->res_type == FOURCC_DELT) && !xrect_Empty_Rect(out))
			xrect_Flip_Rect(out, &actor->bounds, actor->flags & AF_HFLIP, actor->flags & AF_VFLIP);
#endif
	} else {
		xrect_Set_Rect(out, 0, 0, actor->w, actor->h);
	}
}

void xactor_Set_Actor_ZPlane(Actor* actor, int16_t z) {
	actor->zplane = z;
	zplane_build_gbl = true;
}

int16_t xactor_Get_Actor_ZPlane(Actor* actor) { return actor->zplane; }

void xactor_Set_Actor_Time(Actor* actor, int32_t s, int32_t e) {
	actor->start = s;
	actor->stop = e;
}

void xactor_Get_Actor_Time(Actor* actor, int32_t* s, int32_t* e) {
	*s = actor->start;
	*e = actor->stop;
}

void xactor_Set_Actor_State(Actor* actor, int16_t state, int16_t fract) {
	ActorType* t = xactor_Find_Actor_Type(actor->res_type);
	if (t && t->stateFunc)
		t->stateFunc(actor, state, fract);
	else {
		actor->state = state;
		actor->state_f = fract;
	}
}

void xactor_Get_Actor_State(Actor* actor, int16_t* s, int16_t* f) {
	*s = actor->state;
	*f = actor->state_f;
}

void xactor_Set_Actor_State_Speed(Actor* actor, int16_t v, int16_t f) {
	actor->state_v = v;
	actor->state_vf = f;
}

void xactor_Get_Actor_State_Speed(Actor* actor, int16_t* v, int16_t* f) {
	*v = actor->state_v;
	*f = actor->state_vf;
}

void xactor_Set_Actor_Color(Actor* actor, int16_t fg, int16_t bg) {
	actor->foreColor = fg;
	actor->backColor = bg;
}

void xactor_Get_Actor_Color(Actor* actor, int16_t* fg, int16_t* bg) {
	*fg = actor->foreColor;
	*bg = actor->backColor;
}

void xactor_Set_Actor_Remap_Color(Actor* actor, int16_t color) {
	actor->foreColor = color;
	actor->flags |= AF_REMAP_COLOR;
}

int16_t xactor_Get_Actor_Remap_Color(Actor* actor) { return actor->foreColor; }

void xactor_Clear_Actor_Remap_Color(Actor* actor) { actor->flags &= ~AF_REMAP_COLOR; }

void xactor_Set_Actor_Scale(Actor* actor, int16_t xs, int16_t ys) {
	actor->xscale = xs;
	actor->yscale = ys;
	/* No prev_xscale/yscale reseed — unlike Set_Actor_Pos (whose
	 * caller is the FILM script's ACTOR_POS teleport opcode),
	 * Set_Actor_Scale is invoked by user callbacks running INSIDE
	 * each cel as the per-frame step of a smooth scale animation
	 * (title.c's Star-Wars-logo zoom is the canonical case).
	 * Reseeding here would set prev = current, killing the lerp;
	 * letting it ride keeps prev = cel-start scale captured at
	 * Move_Actor, so a consumer can interpolate prev to current. */
}

void xactor_Get_Actor_Scale(Actor* actor, int16_t* xs, int16_t* ys) {
	*xs = actor->xscale;
	*ys = actor->yscale;
}

void xactor_Set_Actor_Flip(Actor* actor, int16_t hFlip, int16_t vFlip) {
	if (hFlip)
		actor->flags |= AF_HFLIP;
	else
		actor->flags &= ~AF_HFLIP;
	if (vFlip)
		actor->flags |= AF_VFLIP;
	else
		actor->flags &= ~AF_VFLIP;
}

void xactor_Get_Actor_Flip(Actor* actor, int16_t* hFlip, int16_t* vFlip) {
	*hFlip = actor->flags & AF_HFLIP;
	*vFlip = actor->flags & AF_VFLIP;
}

static uint16_t actor_render_flags(uint16_t flags) {
	uint16_t result = 0;
	if (flags & AF_VISIBLE)
		result |= LANDRU_ACTOR_RENDER_VISIBLE;
	if (flags & AF_ACTIVE)
		result |= LANDRU_ACTOR_RENDER_ACTIVE;
	if (flags & AF_HFLIP)
		result |= LANDRU_ACTOR_RENDER_HFLIP;
	if (flags & AF_VFLIP)
		result |= LANDRU_ACTOR_RENDER_VFLIP;
	if (flags & AF_REMAP_COLOR)
		result |= LANDRU_ACTOR_RENDER_REMAP_COLOR;
	return result;
}

void xactor_Set_Actor_Speed(Actor* actor, int16_t xv, int16_t yv, int16_t xvf, int16_t yvf) {
	actor->xv = xv;
	actor->yv = yv;
	actor->xvf = xvf;
	actor->yvf = yvf;
}

void xactor_Get_Actor_Speed(Actor* actor, int16_t* xv, int16_t* yv, int16_t* xvf, int16_t* yvf) {
	*xv = actor->xv;
	*yv = actor->yv;
	*xvf = actor->xvf;
	*yvf = actor->yvf;
}

void xactor_Set_Actor_Frame_Speed(Actor* actor, int16_t l, int16_t t, int16_t r, int16_t b) {
	xrect_Set_Rect(&actor->frame_v, l, t, r, b);
}

void xactor_Get_Actor_Frame_Speed(Actor* actor, int16_t* l, int16_t* t, int16_t* r, int16_t* b) {
	*l = actor->frame_v.left;
	*t = actor->frame_v.top;
	*r = actor->frame_v.right;
	*b = actor->frame_v.bottom;
}

void xactor_Set_Actor_Data(Actor* actor, LandruHandle data) { actor->data = data; }

void xactor_Get_Actor_Data(Actor* actor, LandruHandle* out) { *out = actor->data; }

void xactor_Set_Actor_Array(Actor* actor, LandruHandle array) { actor->array = array; }

void xactor_Get_Actor_Array(Actor* actor, LandruHandle* out) { *out = actor->array; }

LandruHandle xactor_Get_Actor_Array_Data(Actor* actor, int16_t index) {
	LandruHandle* array = xmemhdl_Lock_Handle(actor->array);
	if (!array || index < 0 || (uint32_t)index >= xmemhdl_Get_Handle_Size(actor->array) / sizeof(*array))
		return LANDRU_NULL_HANDLE;
	return array[index];
}

void xactor_Set_Actor_Draw_Function(Actor* actor, xactorDrawFunc f) { actor->draw = f; }

void xactor_Get_Actor_Draw_Function(Actor* actor, xactorDrawFunc* f) { *f = actor->draw; }

void xactor_Set_Actor_Update_Function(Actor* actor, xactorUpdateFunc f) { actor->update = f; }

void xactor_Get_Actor_Update_Function(Actor* actor, xactorUpdateFunc* f) { *f = actor->update; }

void xactor_Set_Actor_User_Function(Actor* actor, xactorCallback f) { actor->user = f; }

void xactor_Get_Actor_User_Function(Actor* actor, xactorCallback* f) { *f = actor->user; }

#ifdef LANDRU_MODERN
/* --- Render capture --- */

void xactor_fill_render_state(const Actor* a, LandruActorRenderState* out) {
	if (!a || !out)
		return;
	out->res_type = a->res_type;
	/* res_name is fixed 8 bytes in both source and dest. */
	memcpy(out->res_name, a->res_name, sizeof out->res_name);
	out->id = a->id;
	out->film_entry_index = a->film_entry_index;

	/* Ship the SCREEN POSITION of the actor's current frame,
	 * not the FILM-driven offset. The engine's draw path lays
	 * the actor at `frame.left + actor->x` where frame is the
	 * current bbox via the actor type's frameFunc.
	 *
	 * AF_HFLIP / AF_VFLIP also relocate the bbox within the
	 * actor's enclosing bounds — see
	 * xactdelt_Draw_Flipped_Clipped_Delta:
	 *
	 *     br_x = bounds.right + bounds.left - 1
	 *     left = (br_x - hdr[2]) + xoff
	 *
	 * For ANIMs whose frames sit at different bbox positions
	 * (e.g. tie07 in city1_f, where frame 0 is right-side and
	 * frame N is left-side) HFLIP mirrors each frame across the
	 * enclosing-bounds center. Compute the same flipped-frame
	 * origin here so renderers see the engine's actual screen
	 * position. For DELT/RAW with a single frame the bounds
	 * equals the frame and the flipped origin reduces to
	 * frame.left — no behaviour change. */
	int16_t fx = a->bounds.left, fy = a->bounds.top;
	ActorType* t = xactor_Find_Actor_Type(a->res_type);
	if (t && t->frameFunc) {
		Rect frame;
		t->frameFunc((Actor*)a, &frame);
		if (a->flags & AF_HFLIP)
			fx = (int16_t)(a->bounds.left + a->bounds.right - frame.right);
		else
			fx = frame.left;
		if (a->flags & AF_VFLIP)
			fy = (int16_t)(a->bounds.top + a->bounds.bottom - frame.bottom);
		else
			fy = frame.top;
	}
	/* Emit pre-scale, pre-recentering position and dimensions.
	 * Renderers apply Watcom Q8 scale (256 = identity, set by
	 * xactor_Set_Actor_Scale) on their own — see LandruActorRenderState
	 * for the formula. Pre-applying integer recentering on the
	 * emit side introduced ±0.5-pixel jitter as xscale parity flipped;
	 * doing the math in floating point on the
	 * consumer side keeps the scaled bbox center stable to sub-
	 * pixel precision. The classic-FB renderer (xactdelt_Draw_
	 * Scaled_Clipped_Delta) still uses the same int recenter
	 * math, so framebuffer pixels are unaffected. */
	out->x = (int16_t)(fx + a->x);
	out->y = (int16_t)(fy + a->y);
	out->w = a->w;
	out->h = a->h;
	out->xscale = a->xscale;
	out->yscale = a->yscale;
	out->zplane = a->zplane;
	out->state = a->state;
	out->flags = actor_render_flags(a->flags);

	/* Sub-cel smoothing channel: ship the pre-Move_Actor
	 * position alongside the velocity that produced this cel's
	 * delta. The frame-origin offset (fx/fy) is the same for
	 * the pre and post positions because frameFunc is keyed on
	 * actor state which doesn't change inside Move_Actor — so
	 * we can apply the same offset to actor->prev_x/y as we
	 * did to actor->x/y. */
	out->prev_x = (int16_t)(fx + a->prev_x);
	out->prev_y = (int16_t)(fy + a->prev_y);
	out->prev_xv = a->prev_xv;
	out->prev_yv = a->prev_yv;
	out->xv = a->xv;
	out->yv = a->yv;
	out->xvf = a->xvf;
	out->yvf = a->yvf;
	out->prev_xscale = a->prev_xscale;
	out->prev_yscale = a->prev_yscale;

	/* ACTOR_CLIP target. Default = canvas bounds (0,0,320,200)
	 * which is a no-op for renderers; FILM-driven ACTOR_CLIP
	 * commands narrow it. */
	out->clip_left = a->frame.left;
	out->clip_top = a->frame.top;
	out->clip_right = a->frame.right;
	out->clip_bottom = a->frame.bottom;

	/* Palette index for AF_REMAP_COLOR. */
	out->fore_color = a->foreColor;
}

void xactor_emit_render_state(void) {
	for (Actor* a = actor_list_gbl; a; a = a->next) {
		/* Skip actors whose callback makes their standard pose an invalid
		 * description of the rendered output. */
		if (a->flags & AF_NO_RENDER_CAPTURE)
			continue;
		LandruActorRenderState state = { 0 };
		xactor_fill_render_state(a, &state);
		landru_render_actor(&state);
	}
}

void xactor_Set_Actor_Render_Capture_Hidden(Actor* actor, bool hidden) {
	if (!actor)
		return;
	if (hidden)
		actor->flags |= AF_NO_RENDER_CAPTURE;
	else
		actor->flags &= ~AF_NO_RENDER_CAPTURE;
}

void xactor_emit_draw(Actor* actor, int16_t xoff, int16_t yoff) {
	if (!actor)
		return;
	/* Non-screen coordinates are published only when the caller explicitly
	 * selects the auxiliary render target. */
	if (!xcanvas_Render_Emit_Allowed())
		return;
	LandruDrawCommand command = { 0 };
	LandruDrawCommand* out = &command;
	out->res_type = actor->res_type;
	memcpy(out->res_name, actor->res_name, sizeof out->res_name);
	out->film_entry_index = actor->film_entry_index;

	/* xoff/yoff are FRAME-RELATIVE, not screen-absolute. The standard
	 * draw pass passes `rect.left + actor.x - frame.left` (per
	 * xactor_Get_Actor_Relative_XY); imperative callers pass values that
	 * the rendering loop combines with
	 * the resource data's natural origin via `frame.left + xoff`.
	 * Recover screen-space position the same way xactor_emit_render_state
	 * does for actors_2d: query the actor type's frameFunc for the
	 * current frame's natural rect, account for HFLIP/VFLIP, then add
	 * xoff/yoff. This makes draws_2d positions directly comparable to
	 * actors_2d positions for the same actor at the same instant. */
	int16_t fx = actor->bounds.left, fy = actor->bounds.top;
	ActorType* t = xactor_Find_Actor_Type(actor->res_type);
	if (t && t->frameFunc) {
		Rect frame;
		t->frameFunc(actor, &frame);
		if (actor->flags & AF_HFLIP)
			fx = (int16_t)(actor->bounds.left + actor->bounds.right - frame.right);
		else
			fx = frame.left;
		if (actor->flags & AF_VFLIP)
			fy = (int16_t)(actor->bounds.top + actor->bounds.bottom - frame.bottom);
		else
			fy = frame.top;
	}
	out->x = (int16_t)(fx + xoff);
	out->y = (int16_t)(fy + yoff);
	out->w = actor->w;
	out->h = actor->h;
	out->state = actor->state;
	/* Effective clip = current drawing canvas clip ∩ actor->frame.
	 * The canvas clip is what classic actually rasters against (set by
	 * the actor pass to the actor's view-clipped rect, or by the input
	 * pass to the input's frame when an idraw_* delegates the actor
	 * draw — e.g. map.c idraw_Map calling xactanim_Draw_Anim_Actor on
	 * cmbticons inside a 21×32 button rect). actor->frame alone misses
	 * the input-pass narrowing and lets the cel bleed past the input's
	 * visible region (visible at the brfbutns/brfpnl seam). */
	Rect canvas_clip;
	xcanvas_Get_Raster_Clip(&canvas_clip);
	int16_t cl = canvas_clip.left > actor->frame.left ? canvas_clip.left : actor->frame.left;
	int16_t ct = canvas_clip.top > actor->frame.top ? canvas_clip.top : actor->frame.top;
	int16_t cr = canvas_clip.right < actor->frame.right ? canvas_clip.right : actor->frame.right;
	int16_t cb = canvas_clip.bottom < actor->frame.bottom ? canvas_clip.bottom : actor->frame.bottom;
	out->clip_left = cl;
	out->clip_top = ct;
	out->clip_right = cr;
	out->clip_bottom = cb;
	out->flags = actor_render_flags(actor->flags);
	/* Palette index for AF_REMAP_COLOR. Set by callers like
	 * player_Draw_Display_Ship which paints reticle silhouettes as a
	 * flat target_color band over the iconsgrn cel. */
	out->fore_color = actor->foreColor;
	out->target = xcanvas_Render_Emit_Target();
	landru_render_draw(out);
}

#endif

#ifdef LANDRU_GAME_XW
void xactor_Draw_System_Actors(void) {
	xactor_Draw_Actors(actor_list_gbl, view_gbl->refresh_actors);
	view_gbl->refresh_actors = 0;
}
#endif

#ifdef LANDRU_GAME_XW
void xactor_ResetSceneState(void) {
	actor_list_gbl = NULL;
	zplane_build_gbl = false;
}
#endif
