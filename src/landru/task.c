#include <landru/task.h>
#include <string.h>

#include <landru/pal.h>

#include "dialog_internal.h"

static LandruTaskFrame s_stack[LANDRU_TASK_STACK_DEPTH];
static int s_top;

void* landru_task_push(const LandruTaskVtable* vtable) {
	if (!vtable || !vtable->step || s_top >= LANDRU_TASK_STACK_DEPTH)
		return NULL;
	LandruTaskFrame* frame = &s_stack[s_top++];
	frame->vtable = vtable;
	memset(frame->storage.bytes, 0, sizeof frame->storage.bytes);
	return frame->storage.bytes;
}

void* landru_task_top(void) { return s_top > 0 ? s_stack[s_top - 1].storage.bytes : NULL; }

void landru_task_pop(void) {
	if (s_top == 0)
		return;
	LandruTaskFrame* frame = &s_stack[--s_top];
	if (frame->vtable && frame->vtable->end)
		frame->vtable->end(frame->storage.bytes);
	frame->vtable = NULL;
}

bool landru_task_stack_empty(void) { return s_top == 0; }

int landru_task_depth(void) { return s_top; }

LandruTaskStepResult landru_task_step_once(void) {
	if (s_top == 0)
		return LANDRU_TASK_STEP_DONE;
	LandruTaskFrame* frame = &s_stack[s_top - 1];
	LandruTaskStepResult result = frame->vtable->step(frame->storage.bytes);
	if (result == LANDRU_TASK_STEP_DONE)
		landru_task_pop();
	return result;
}

void landru_task_run_frame(void) {
	int budget = 64;
	while (budget-- > 0 && s_top > 0) {
		if (xpal_Next_VGA_Delay_Us() != UINT64_MAX)
			return;
		LandruTaskStepResult result = landru_task_step_once();
		if (result == LANDRU_TASK_STEP_YIELD || result == LANDRU_TASK_STEP_FRAME_COMPLETE ||
			xpal_Next_VGA_Delay_Us() != UINT64_MAX)
			return;
	}
}

void landru_task_service_wait(void) {
	if (s_top == 0)
		return;
	LandruTaskFrame* frame = &s_stack[s_top - 1];
	if (frame->vtable && frame->vtable->service_wait)
		frame->vtable->service_wait(frame->storage.bytes);
}

uint64_t landru_task_next_wake_delay_us(void) {
	uint64_t delay_us = xpal_Next_VGA_Delay_Us();
	if (delay_us != UINT64_MAX || s_top == 0)
		return delay_us;
	const LandruTaskFrame* frame = &s_stack[s_top - 1];
	if (!frame->vtable || !frame->vtable->next_wake_delay_us)
		return UINT64_MAX;
	return frame->vtable->next_wake_delay_us(frame->storage.bytes);
}

void landru_task_clear_all(void) {
#ifdef LANDRU_GAME_XW
	xdialog_Cancel_Pending_Sub_Dialog();
#endif
	while (s_top > 0)
		landru_task_pop();
}
