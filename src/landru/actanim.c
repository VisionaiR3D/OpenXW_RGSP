#include <stdlib.h>

#include <landru/actanim.h>
#include <landru/actdelt.h>
#include <landru/actor.h>
#include <landru/fourcc.h>
#include <landru/rect.h>
#include <landru/res.h>

// GLOBAL: TIE 0xD2E70
static bool anim_actor_module_gbl = false;

void xactanim_Create_Anim_Actor_Module(void) {
	xactor_Create_Actor_Type(FOURCC_ANIM, (xactorFrameFunc)xactanim_Get_Anim_Actor_Frame,
							 (xactorStateFunc)xactanim_Set_Anim_Actor_State);
	anim_actor_module_gbl = true;
}

void xactanim_Destroy_Anim_Actor_Module(void) {
	if (anim_actor_module_gbl) {
		xactor_Destroy_Actor_Type(FOURCC_ANIM);
		anim_actor_module_gbl = false;
	}
}

void xactanim_Get_Anim_Actor_Frame(Actor* actor, Rect* outFrame) {
	xrect_Set_Rect(outFrame, 0, 0, 0, 0);
	int16_t* frame = xmemhdl_Lock_Handle(xactor_Get_Actor_Array_Data(actor, actor->state));
	if (frame && frame[0] != -1 && frame[1] != -1) {
		xrect_Set_Rect(outFrame, frame[0], frame[1], frame[2] + 1, frame[3] + 1);
	}
}

void xactanim_Set_Anim_Actor_State(Actor* actor, int16_t state, int16_t stateFract) {
	actor->state = state;
	actor->state_f = stateFract;
	Rect frame;
	xactanim_Get_Anim_Actor_Frame(actor, &frame);
	actor->w = frame.right - frame.left;
	actor->h = frame.bottom - frame.top;
}

Actor* xactanim_Alloc_Anim_Actor(LandruHandle frames, Rect* rect, int16_t x, int16_t y, int16_t z) {
	Actor* actor = xactor_Alloc_Actor(0);
	if (!actor)
		return NULL;

	xactanim_Init_Anim_Actor(actor, frames, rect, x, y, z);
	xactor_Set_Actor_Name(actor, FOURCC_ANIM, "");
	xactor_Non_Discard_Actor_Data(actor);
	return actor;
}

#ifdef LANDRU_GAME_XW
Actor* xactanim_Res_Anim_Actor(ResFile* resFile, const char* resName, Rect* rect, int16_t x, int16_t y,
							   int16_t z) {
#else
Actor* xactanim_Res_Anim_Actor(const char* resName, Rect* rect, int16_t x, int16_t y, int16_t z) {
#endif
	Actor* actor = xactor_Alloc_Actor(0);
	if (!actor)
		return NULL;

#ifdef LANDRU_GAME_XW
	int offset;
	uint32_t size;
	if (resFile && (!xres_Get_Resource_Offset(resFile, FOURCC_ANIM, resName, &offset, &size) ||
					!xres_Open_Resource_Data(resFile, offset)))
		resFile = NULL;
#else
	ResFile* resFile = xres_Open_Resource_Data(FOURCC_ANIM, resName);
#endif
	if (!resFile) {
		xactor_Free_Actor(actor);
		return NULL;
	}

	int16_t frameCount = xres_Read_Resource_Word(resFile);
	LandruHandle framesHandle =
		frameCount > 0 ? xmemhdl_Alloc_Clear_Handle(sizeof(LandruHandle) * frameCount, LANDRU_MEMORY_DEFAULT)
					   : LANDRU_NULL_HANDLE;
	LandruHandle* frames = xmemhdl_Lock_Handle(framesHandle);
	if (!frames) {
		xres_Close_Resource_Data(resFile);
		xactor_Free_Actor(actor);
		return NULL;
	}

	for (int i = 0; i < frameCount; i++) {
		uint32_t len = xres_Read_Resource_Long(resFile);
		if (len <= 0) {
			frames[i] = LANDRU_NULL_HANDLE;
		} else {
			frames[i] = xres_Read_Resource_Data(resFile, len, LANDRU_MEMORY_DEFAULT);
			if (!frames[i]) {
				/* Cleanup on failure */
				for (int j = 0; j < i; j++)
					xmemhdl_Free_Handle(frames[j]);
				xmemhdl_Free_Handle(framesHandle);
				xres_Close_Resource_Data(resFile);
				xactor_Free_Actor(actor);
				return NULL;
			}
		}
	}
	xres_Close_Resource_Data(resFile);

	xactanim_Init_Anim_Actor(actor, framesHandle, rect, x, y, z);
	xactor_Set_Actor_Name(actor, FOURCC_ANIM, resName);
	actor->arraySize = frameCount;
	xactanim_Get_Anim_Actor_Bounds(actor, &actor->bounds);
	return actor;
}

void xactanim_Init_Anim_Actor(Actor* actor, LandruHandle frames, Rect* rect, int16_t x, int16_t y,
							  int16_t z) {
	xrect_Set_Rect(&actor->frame, rect->left, rect->top, rect->right, rect->bottom);
	actor->x = x;
	actor->y = y;
	actor->zplane = z;
	xactor_Discard_Actor_Data(actor);
	actor->draw = xactanim_Draw_Anim_Actor;
	actor->array = frames;
	actor->update = (xactorUpdateFunc)xactanim_Update_Anim_Actor;
	xactor_Add_Actor_To_System(actor);

	Rect frame;
	xactanim_Get_Anim_Actor_Frame(actor, &frame);
	actor->w = frame.right - frame.left;
	actor->h = frame.bottom - frame.top;
}

void xactanim_Get_Anim_Actor_Bounds(Actor* actor, Rect* outRect) {
	xrect_Clear_Rect(outRect);
	for (int i = 0; i < actor->arraySize; i++) {
		int16_t* frame = xmemhdl_Lock_Handle(xactor_Get_Actor_Array_Data(actor, i));
		if (frame && frame[0] != -1 && frame[1] != -1) {
			Rect frameRect;
			xrect_Set_Rect(&frameRect, frame[0], frame[1], frame[2] + 1, frame[3] + 1);
			xrect_Enclose_Rect(outRect, &frameRect);
		}
	}
}

int16_t xactanim_Draw_Anim_Actor(Actor* actor, Rect* clip, Rect* dest, int16_t xoff, int16_t yoff,
								 int16_t refresh) {
	(void)clip;
	(void)dest;
	if (!refresh)
		return 0;

	LandruHandle frame = xactor_Get_Actor_Array_Data(actor, actor->state);
	if (!frame)
		return 0;

#ifdef LANDRU_MODERN
	xactor_emit_draw(actor, xoff, yoff);
#endif

	int dirty = xactor_Is_Actor_Dirty(actor);

	if (actor->xscale == 256 && actor->yscale == 256) {
		if ((actor->flags & AF_HFLIP) || (actor->flags & AF_VFLIP)) {
			return xactdelt_Draw_Flipped_Clipped_Delta(actor, frame, xoff, yoff, dirty);
		}
		if (actor->flags & AF_REMAP_COLOR) {
			return xactdelt_Draw_Color_Clipped_Delta(frame, xoff, actor->foreColor, yoff, dirty);
		}
		return xactdelt_Draw_Clipped_Delta(frame, xoff, yoff, dirty);
	}
	return xactdelt_Draw_Scaled_Clipped_Delta(actor, frame, xoff, yoff, dirty);
}

void xactanim_Update_Anim_Actor(Actor* actor) {
	int16_t prev_state = actor->state;

	xactor_Move_Actor(actor);
	xactor_Move_Actor_Frame(actor);
	xactor_Move_Actor_State(actor);

	if (actor->arraySize == 0) {
		actor->state = 0;
	} else {
		while (actor->state < 0)
			actor->state += actor->arraySize;
		while (actor->state >= actor->arraySize)
			actor->state -= actor->arraySize;
	}

	if (prev_state != actor->state) {
		Rect frame;
		xactanim_Get_Anim_Actor_Frame(actor, &frame);
		actor->w = frame.right - frame.left;
		actor->h = frame.bottom - frame.top;
	}
}

#ifdef LANDRU_GAME_XW
void xactanim_Get_Anim_Actor_Offset(Actor* actor, int16_t* x, int16_t* y) {
	Rect frame;
	xactanim_Get_Anim_Actor_Frame(actor, &frame);
	*x = frame.left;
	*y = frame.top;
}
#endif
