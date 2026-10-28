#include <landru/actrect.h>

#include <stddef.h>

#include <landru/actcust.h>
#include <landru/fourcc.h>

Actor* xactrect_Alloc_Blank_Actor(int32_t unusedData, const Rect* frame, int16_t x, int16_t y, int16_t z) {
	(void)unusedData;
	Actor* actor = xactor_Alloc_Actor(0);
	if (!actor)
		return NULL;
	Rect bounds = *frame;
	xactcust_Init_Custom_Actor(actor, &bounds, x, y, z);
	xactor_Set_Actor_Name(actor, FOURCC_RECT, "BLANK");
	actor->data = LANDRU_NULL_HANDLE;
	actor->foreColor = 0;
	xactor_Non_Discard_Actor_Data(actor);
#ifdef LANDRU_MODERN
	xactor_Set_Actor_Render_Capture_Hidden(actor, true);
#endif
	return actor;
}
