#include <stddef.h>

#include <landru/actcust.h>
#include <landru/actor.h>
#include <landru/dirty.h>
#include <landru/fourcc.h>
#include <landru/paint.h>

// GLOBAL: TIE 0xD2F74
bool custom_actor_module_gbl = false;

void xactcust_Create_Custom_Actor_Module(void) {
	xactor_Create_Actor_Type(FOURCC_CUST, NULL, NULL);
	custom_actor_module_gbl = true;
}

void xactcust_Destroy_Custom_Actor_Module(void) {
	if (custom_actor_module_gbl) {
		xactor_Destroy_Actor_Type(FOURCC_CUST);
		custom_actor_module_gbl = false;
	}
}

Actor* xactcust_Alloc_Custom_Actor(LandruHandle data, Rect* frame, int16_t x, int16_t y, int16_t z) {
	Actor* actor = xactor_Alloc_Actor(0);
	if (!actor) {
		return NULL;
	}

	xactcust_Init_Custom_Actor(actor, frame, x, y, z);
	actor->data = data;
	xactor_Set_Actor_Name(actor, FOURCC_CUST, "CUSTOM");
	xactor_Non_Discard_Actor_Data(actor);
	/* A custom actor's callback, rather than its generic pose and resource
	 * fields, defines its output. Suppress the otherwise misleading standard
	 * actor record; the callback may publish its concrete operations. */
#ifdef LANDRU_MODERN
	xactor_Set_Actor_Render_Capture_Hidden(actor, true);
#endif
	return actor;
}

void xactcust_Init_Custom_Actor(Actor* actor, Rect* frame, int16_t x, int16_t y, int16_t z) {
	xrect_Set_Rect(&actor->frame, frame->left, frame->top, frame->right, frame->bottom);
	actor->x = x;
	actor->y = y;
	actor->zplane = z;
	xactor_Discard_Actor_Data(actor);
	actor->draw = xactcust_Draw_Custom_Actor;
	actor->update = xactcust_Update_Custom_Actor;
	xactor_Add_Actor_To_System(actor);
}

int16_t xactcust_Draw_Custom_Actor(Actor* actor, Rect* rect, Rect* clipRect, int16_t x, int16_t y,
								   int16_t refresh) {
	(void)rect;
	(void)clipRect;
	if (!refresh)
		return 0;

	Rect pRect;
	xrect_Set_Rect(&pRect, x, y, x + actor->w, y + actor->h);
	xpaint_Paint_Clipped_Rect(&pRect, actor->foreColor);
	if (xactor_Is_Actor_Dirty(actor)) {
		xdirty_Dirty_Rect(&pRect);
	}
	return 1;
}

void xactcust_Update_Custom_Actor(Actor* actor) {
	xactor_Move_Actor(actor);
	xactor_Move_Actor_Frame(actor);
}
