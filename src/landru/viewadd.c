
#include <landru/actor.h>
#include <landru/canvas.h>
#include <landru/dialog.h>
#include <landru/dirty.h>
#include <landru/error.h>
#include <landru/fade.h>
#include <landru/film.h>
#include <landru/input.h>
#include <landru/io.h>
#include <landru/pal.h>
#include <landru/rect.h>
#include <landru/sound.h>
#include <landru/task.h>
#include <landru/timer.h>
#include <landru/vesa.h>
#include <landru/view.h>
#include <landru/viewadd.h>

#include "host_internal.h"

bool xviewadd_Clip_Object_To_View(int16_t viewId, int16_t zPlane, Rect* actFrame, Rect* outFull,
								  Rect* outClipped) {
	if (zPlane < view_gbl->zstart[viewId] || zPlane > view_gbl->zstop[viewId])
		return false;

	int16_t xOffset = view_gbl->frame[viewId].left - view_gbl->rel_x[viewId];
	int16_t yOffset = view_gbl->frame[viewId].top - view_gbl->rel_y[viewId];
	xrect_Copy_Rect(outFull, actFrame);
	xrect_Offset_Rect(outFull, xOffset, yOffset);
	xrect_Copy_Rect(outClipped, outFull);

	Rect clipFrame;
	xview_Get_View_Clip_Frame(viewId, &clipFrame);
	xrect_Clip_Rect(outClipped, &clipFrame);
	return !xrect_Empty_Rect(outClipped);
}

/* ------------------------------------------------------------------
 * Front-end view loop — fully task-driven.
 *
 * Pushed via xviewadd_Push_Handle_View_Task. The body decomposes
 * cleanly into Begin/Step/End because the iteration is stateless
 * apart from view_gbl: the loop just re-runs the update / erase /
 * draw / wait pipeline until the Landru "running" flag goes false.
 *
 * Sub-dialog opens from inside the view-update callback or input
 * callbacks go through xdialog_Schedule_Sub_Dialog; the channel is
 * drained at end of DRAW + AFTER_FRAME_FADE, the sub-dialog is
 * pushed on top with a handler invoked when it pops. */

typedef enum {
	VIEW_PHASE_DRAW = 0,
	VIEW_PHASE_WAIT,
	VIEW_PHASE_AFTER_FRAME_FADE,
	VIEW_PHASE_AFTER_SUB, /* sub-dialog popped; invoke handler */
} ViewAddPhase;

typedef struct ViewAddTask {
	ViewAddPhase phase;
	DialogSubResultHandler sub_handler;
	void* sub_ctx;
} ViewAddTask;

static LandruTaskStepResult viewadd_task_step(void* self) {
	ViewAddTask* t = (ViewAddTask*)self;

	if (!xerror_Is_Landru_Running())
		return LANDRU_TASK_STEP_DONE;

	if (t->phase == VIEW_PHASE_WAIT) {
		/* Yield until the per-scene frame budget elapses. YIELD (not
		 * CONTINUE) so landru_task_run_frame returns to the application
		 * and its clock can advance before the next frame. */
		if (xtimer_Frame_Budget_Pending())
			return LANDRU_TASK_STEP_YIELD;
		xtimer_Commit_Frame();
		xcanvas_Push_Fade_Screen_To_Video_Task(0);
		t->phase = VIEW_PHASE_AFTER_FRAME_FADE;
		return LANDRU_TASK_STEP_CONTINUE;
	}

	if (t->phase == VIEW_PHASE_AFTER_FRAME_FADE) {
		/* WAIT pushed a fade; FadeTask has popped (else we wouldn't
		 * be running). Run the post-fade per-frame work, drain any
		 * sub-dialog request the view-update callback staged, and
		 * return to DRAW (or AFTER_SUB if a sub-dialog was pushed).
		 * The view-update callback is the only callback in this
		 * pipeline that may legitimately schedule a sub-dialog
		 * (e.g. filmview's end_View at the penultimate cel). */
		if (!view_gbl->step) {
			xpal_Cycle_Screen();
			if (view_gbl->update)
				view_gbl->update(view_gbl->time);
			view_gbl->time++;
		}
		/* Copies the frame to the host surface; the application presents it
		 * when this step returns FRAME_COMPLETE. */
		(void)landru_port_Present_Platform_Video();
#ifdef LANDRU_GAME_XW
		xfade_RestoreTimedTextBackground();
		landru_host_frontend_audio_pump();
#endif
		if (xdialog_Try_Push_Pending_Sub_Dialog(&t->sub_handler, &t->sub_ctx)) {
			t->phase = VIEW_PHASE_AFTER_SUB;
			return LANDRU_TASK_STEP_FRAME_COMPLETE;
		}
		t->phase = VIEW_PHASE_DRAW;
		return LANDRU_TASK_STEP_FRAME_COMPLETE;
	}

	if (t->phase == VIEW_PHASE_AFTER_SUB) {
		/* Sub-dialog popped — invoke handler, return to DRAW. */
		xdialog_Invoke_Sub_Handler(t->sub_handler, t->sub_ctx);
		t->sub_handler = NULL;
		t->sub_ctx = NULL;
		t->phase = VIEW_PHASE_DRAW;
		return LANDRU_TASK_STEP_CONTINUE;
	}

	/* DRAW phase — runs once per frame; transitions to WAIT at the end. */
	xio_Poll_Input();
#ifdef LANDRU_GAME_XW
	if (xdialog_Try_Push_Pending_Sub_Dialog(&t->sub_handler, &t->sub_ctx)) {
		t->phase = VIEW_PHASE_AFTER_SUB;
		return LANDRU_TASK_STEP_CONTINUE;
	}
#endif

	if (!view_gbl->step) {
		int32_t time = view_gbl->time;
		xview_Move_View();
		xtimer_Often();
		xinput_Update_System_Inputs();
#ifdef LANDRU_GAME_XW
		if (xdialog_Try_Push_Pending_Sub_Dialog(&t->sub_handler, &t->sub_ctx)) {
			t->phase = VIEW_PHASE_AFTER_SUB;
			return LANDRU_TASK_STEP_CONTINUE;
		}
#endif
		xtimer_Often();
		xfilm_Update_Films(time);
		xtimer_Often();
		xactor_Update_Actors(time);
		xtimer_Often();

		int32_t time2 = view_gbl->time;
		xinput_User_System_Inputs(time2);
#ifdef LANDRU_GAME_XW
		if (xdialog_Try_Push_Pending_Sub_Dialog(&t->sub_handler, &t->sub_ctx)) {
			t->phase = VIEW_PHASE_AFTER_SUB;
			return LANDRU_TASK_STEP_CONTINUE;
		}
#endif
		xtimer_Often();
		xfilm_User_Films(time2);
		xfilm_Check_Film_ZPlanes();
		xtimer_Often();
		xactor_User_Actors(time2);
		xactor_Check_Actor_ZPlanes();
		xtimer_Often();
		xsound_User_Sounds(time2);
#ifndef LANDRU_GAME_XW
		landru_host_frontend_audio_pump();
#endif
		xsound_Free_User_Sounds(xsound_Ask_Sound_List());
		xtimer_Often();
	}

	/* Drain any sub-dialog request a system-input callback staged
	 * during this DRAW pass before continuing to erase/draw. */
	if (xdialog_Try_Push_Pending_Sub_Dialog(&t->sub_handler, &t->sub_ctx)) {
		t->phase = VIEW_PHASE_AFTER_SUB;
		return LANDRU_TASK_STEP_CONTINUE;
	}

	/* Erase phase */
	if (view_gbl->clear) {
		xcanvas_Erase_Canvas_Rect(&view_gbl->clip_frame);
	} else {
		for (int i = 0; i < 4; i++) {
			if (xview_Is_View_Erase(i)) {
				Rect rect;
				xview_Get_View_Clip_Frame(i, &rect);
				xcanvas_Erase_Canvas_Rect(&rect);
				xtimer_Often();
			}
		}
	}

#ifdef LANDRU_GAME_XW
	if (view_gbl->refresh_world) {
		xdirty_Max_Dirty_List();
		xviewadd_Redraw_View();
	} else {
		xviewadd_Draw_View();
	}
	view_gbl->refresh_world = 0;
#else
	/* Draw phase */
	int16_t refresh = view_gbl->refresh_world;
	if (refresh)
		xdirty_Max_Dirty_List();
	xfilm_Draw_Films(refresh);
	xtimer_Often();
	xactor_Draw_Actors(refresh);
	xtimer_Often();
	xinput_Draw_System_Inputs(refresh);
	xtimer_Often();
	xdialog_Draw_System_Dialogs(refresh);
	xtimer_Often();
	view_gbl->refresh_world = 0;

#endif

	xviewadd_Draw_View_Debug(view_gbl->time);

	t->phase = VIEW_PHASE_WAIT;
	return LANDRU_TASK_STEP_CONTINUE;
}

static uint64_t viewadd_task_next_wake_delay_us(const void* self) {
	const ViewAddTask* t = (const ViewAddTask*)self;
	if (t->phase == VIEW_PHASE_DRAW || t->phase == VIEW_PHASE_WAIT)
		return xtimer_Next_Frame_Delay_Us();
	return 0;
}

static void viewadd_task_service_wait(void* self) {
	const ViewAddTask* t = (const ViewAddTask*)self;
	if (t->phase == VIEW_PHASE_WAIT)
		xio_Poll_Fast_Input();
}

static void viewadd_task_end(void* self) {
	(void)self;
	xview_Free_All_From_View(view_gbl);
	xview_Init_View(view_gbl);
	xpal_Stop_All_Cycles();
}

static const LandruTaskVtable viewadd_task_vt = {
	.step = viewadd_task_step,
	.end = viewadd_task_end,
	.service_wait = viewadd_task_service_wait,
	.next_wake_delay_us = viewadd_task_next_wake_delay_us,
};

void xviewadd_Push_Handle_View_Task(void) {
	ViewAddTask* t = (ViewAddTask*)landru_task_push(&viewadd_task_vt);
	if (!t)
		return;
	t->phase = VIEW_PHASE_DRAW;
	t->sub_handler = NULL;
	t->sub_ctx = NULL;

	view_gbl->time = 0;
	view_gbl->step = 0;
	view_gbl->stepCount = 0;
	xio_Flush_Input();
}

void xviewadd_Clear_View(void) {
	if (view_gbl->clear) {
		xcanvas_Erase_Canvas_Rect(&view_gbl->clip_frame);
	} else {
		for (int i = 0; i < 4; i++) {
			if (xview_Is_View_Erase(i)) {
				Rect rect;
				xview_Get_View_Clip_Frame(i, &rect);
				xcanvas_Erase_Canvas_Rect(&rect);
				xtimer_Often();
			}
		}
	}
}

#ifdef LANDRU_GAME_XW
void xviewadd_Draw_View(void) {
	xfilm_Draw_System_Films();
	xtimer_Often();
	xactor_Draw_System_Actors();
	xtimer_Often();
	xinput_Draw_System_Inputs();
	xtimer_Often();
	xdialog_Draw_System_Dialogs();
	xtimer_Often();
}

void xviewadd_Redraw_View(void) {
	view_gbl->refresh_films = 1;
	xfilm_Draw_System_Films();
	xtimer_Often();
	view_gbl->refresh_actors = 1;
	xactor_Draw_System_Actors();
	xtimer_Often();
	view_gbl->refresh_inputs = 1;
	xinput_Draw_System_Inputs();
	xtimer_Often();
	view_gbl->refresh_dialogs = 1;
	xdialog_Draw_System_Dialogs();
	xtimer_Often();
}

void xviewadd_Draw_View_Under_Dialog(void) {
	view_gbl->refresh_films = 1;
	xfilm_Draw_System_Films();
	xtimer_Often();
	view_gbl->refresh_actors = 1;
	xactor_Draw_System_Actors();
	xtimer_Often();
	view_gbl->refresh_inputs = 1;
	xinput_Draw_System_Inputs();
	xtimer_Often();
}
#else
void xviewadd_Draw_View(int16_t refresh) {
	if (refresh)
		xdirty_Max_Dirty_List();
	xfilm_Draw_Films(refresh);
	xtimer_Often();
	xactor_Draw_Actors(refresh);
	xtimer_Often();
	xinput_Draw_System_Inputs(refresh);
	xtimer_Often();
	xdialog_Draw_System_Dialogs(refresh);
	xtimer_Often();
}

void xviewadd_Draw_View_Under_Dialog(void) {
	xfilm_Draw_Films(1);
	xtimer_Often();
	xactor_Draw_Actors(1);
	xtimer_Often();
	xinput_Draw_System_Inputs(1);
	xtimer_Often();
}

#endif

void xviewadd_Draw_View_Debug(int32_t time) {
	/* Reset the canvas clip to the full bitmap before the per-frame
	 * Fade_Screen_To_Video. The actor draw phase leaves bm->clip set to
	 * the last visible actor's view clip; without this reset, both
	 * Cursor_To_Back's canvas draw and the dirty-rect push would be
	 * constrained to that small rect. The binary does this here as the
	 * canonical end-of-draw-phase clip restore. */
	xcanvas_Max_Drawing_Canvas_Clip();
	(void)time;
}

void xviewadd_Update_View(int32_t time) {
	xview_Move_View();
	xtimer_Often();
	xinput_Update_System_Inputs();
	xtimer_Often();
	xfilm_Update_Films(time);
	xtimer_Often();
	xactor_Update_Actors(time);
	xtimer_Often();
}

void xviewadd_User_View(int32_t time) {
	xinput_User_System_Inputs(time);
	xtimer_Often();
	xfilm_User_Films(time);
	xfilm_Check_Film_ZPlanes();
	xtimer_Often();
	xactor_User_Actors(time);
	xactor_Check_Actor_ZPlanes();
	xtimer_Often();
	xsound_User_Sounds(time);
#ifndef LANDRU_GAME_XW
	landru_host_frontend_audio_pump();
#endif
	xsound_Free_User_Sounds(xsound_Ask_Sound_List());
	xtimer_Often();
}

void xviewadd_Update_End_View(void) {
	if (view_gbl->update)
		view_gbl->update(view_gbl->time);
}
