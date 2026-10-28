#include <landru/actback.h>

#include <stddef.h>

#include <landru/canvas.h>

typedef struct BackHeader {
	int16_t x, y, width, height;
} BackHeader;

static bool back_fits(const BackHeader* header, uint32_t capacity) {
	return header->width >= 0 && header->height >= 0 && (uint32_t)header->width * header->height <= capacity;
}

static int16_t back_draw(Actor* actor, Rect* frame, Rect* clip, int16_t x, int16_t y, int16_t refresh) {
	(void)frame;
	(void)clip;
	if (!refresh || !actor->data)
		return 0;
	uint32_t size = xmemhdl_Get_Handle_Size(actor->data);
	if (size < sizeof(BackHeader))
		return 0;
	BackHeader* header = xmemhdl_Lock_Handle(actor->data);
	if (!header)
		return 0;
	uint8_t* pixels = (uint8_t*)(header + 1);
	uint32_t capacity = size - sizeof(*header);
	int16_t copied = 0;
	Rect rect;
	if (actor->state == BACK_RESTORE_CAPTURE || actor->state == BACK_RESTORE) {
		if (!back_fits(header, capacity))
			return 0;
		xrect_Set_Rect(&rect, 0, 0, header->width, header->height);
		stub_Copy_From_Clipped_Buffer(pixels, &rect, header->x, header->y, header->width, header->height);
		copied = 1;
	}
	if (actor->state == BACK_CAPTURE || actor->state == BACK_RESTORE_CAPTURE) {
		BackHeader capture = { x, y, actor->w, actor->h };
		if (!back_fits(&capture, capacity))
			return copied;
		*header = capture;
		xrect_Set_Rect(&rect, 0, 0, header->width, header->height);
		stub_Copy_To_Clipped_Buffer(pixels, &rect, x, y, header->width, header->height);
		copied = 1;
	}
	return copied;
}

static void back_update(Actor* actor) {
	xactor_Move_Actor(actor);
	xactor_Move_Actor_Frame(actor);
	if (actor->state & BACK_HOLD_STATE) {
		actor->state ^= BACK_HOLD_STATE;
	} else {
		switch (actor->state & BACK_STATE_MASK) {
			case BACK_CAPTURE:
				actor->state = BACK_RESTORE_CAPTURE;
				break;
			case BACK_RESTORE_CAPTURE:
				actor->state = BACK_RESTORE;
				break;
			case BACK_RESTORE:
				actor->state = BACK_IDLE;
				break;
			default:
				break;
		}
	}
	if (actor->state & BACK_STATE_MASK)
		xactor_Refresh_Actor(actor);
}

Actor* xactback_Alloc_Back_Actor(LandruHandle buffer, const Rect* frame, int16_t x, int16_t y, int16_t z) {
	Actor* actor = xactor_Alloc_Actor(0);
	if (!actor)
		return NULL;
	actor->frame = *frame;
	actor->x = x;
	actor->y = y;
	actor->zplane = z;
	actor->data = buffer;
	actor->draw = back_draw;
	actor->update = back_update;
	xactor_Add_Actor_To_System(actor);
	actor->w = 0;
	actor->h = 0;
	xactor_Non_Discard_Actor_Data(actor);
	xactor_Non_Refreshable_Actor(actor);
	return actor;
}

void xactback_Alloc_Back_Actor_Buffer(Actor* actor, uint32_t pixelBytes) {
	actor->data = pixelBytes <= UINT32_MAX - sizeof(BackHeader)
					  ? xmemhdl_Alloc_Handle(pixelBytes + sizeof(BackHeader), LANDRU_MEMORY_DEFAULT)
					  : LANDRU_NULL_HANDLE;
	xactor_Discard_Actor_Data(actor);
}

void xactback_Set_Back_Actor_Rect(Actor* actor, const Rect* rect) {
	int16_t state = actor->state & BACK_STATE_MASK;
	if (state == BACK_IDLE) {
		actor->state = BACK_CAPTURE;
		xactor_Refresh_Actor(actor);
	} else if (state == BACK_RESTORE) {
		actor->state = BACK_RESTORE_CAPTURE;
	}
	actor->x = rect->left;
	actor->y = rect->top;
	actor->w = rect->right - rect->left;
	actor->h = rect->bottom - rect->top;
}

void xactback_Stop_Back_Actor(Actor* actor) {
	xactor_Non_Refresh_Actor(actor);
	actor->state = BACK_IDLE;
}
