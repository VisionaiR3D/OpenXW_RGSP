#include <landru/io.h>
#include <landru/timer.h>
#include <landru/vesa.h>

#include "host_internal.h"
#include "timer_internal.h"

/* Host-clock implementation of the 250 Hz PIT timer interface. */

/* Globals */
static int16_t timer_module_gbl;
// GLOBAL: TIE 0xFB650
static int32_t timer_ticks_gbl;
static int16_t timer_count_gbl;
#ifdef LANDRU_GAME_XW
static int16_t frame_rate_gbl = LANDRU_FRAME_TICKS_2;
static int32_t frame_preset_gbl = LANDRU_FRAME_PRESET_2;
static bool elapsed_sample_started;
static uint64_t elapsed_sample_us;
static uint32_t elapsed_sample_residual_us;
static uint32_t elapsed_period_us = LANDRU_TIMER_TICK_US;
#else
static int16_t frame_rate_gbl;
#endif
static uint32_t frame_elapsed_ticks_gbl;
// GLOBAL: TIE 0xD2B84
static int32_t time_gbl;

/* --- Tick source --- */

static uint64_t s_last_us;
static uint32_t s_residual_us;

/* Advance timer_ticks_gbl to match elapsed host time. The cursor
 * crosses one PIT tick per 4 ms and accumulates
 * sub-tick remainders, so behaviour is bit-stable across init/reset
 * boundaries. */
static void xtimer_sync_ticks(void) {
	uint64_t now_us = landru_host_now_us();
	if (now_us < s_last_us) {
		s_last_us = now_us;
		s_residual_us = 0;
		return;
	}
	uint64_t elapsed_us = now_us - s_last_us;
	s_last_us = now_us;
	uint64_t total_us = elapsed_us + s_residual_us;
	uint64_t ticks = total_us / LANDRU_TIMER_TICK_US;
	s_residual_us = (uint32_t)(total_us % LANDRU_TIMER_TICK_US);
	timer_ticks_gbl = (int32_t)((uint32_t)timer_ticks_gbl + (uint32_t)ticks);
	uint32_t phase = (uint32_t)(timer_count_gbl - 1);
	phase = (phase + 14u - (uint32_t)(ticks % 14u)) % 14u;
	timer_count_gbl = (int16_t)(phase + 1u);
}

/* The original keeps continuous engine time and frame-lock time as separate
 * accumulators. Frame commits may discard an overrun, but elapsed engine time
 * must remain available to input, streaming, and blink timers. */
static void xtimer_consume_elapsed_ticks(void) {
	xtimer_sync_ticks();
	uint32_t elapsed = (uint32_t)timer_ticks_gbl;
	time_gbl = (int32_t)((uint32_t)time_gbl + elapsed);
	if (UINT32_MAX - frame_elapsed_ticks_gbl < elapsed)
		frame_elapsed_ticks_gbl = UINT32_MAX;
	else
		frame_elapsed_ticks_gbl += elapsed;
	timer_ticks_gbl = 0;
}

/* --- Module lifecycle --- */

void xtimer_Create_Timer_Interrupt(void) {
	timer_ticks_gbl = 0;
	timer_count_gbl = 14;
	s_last_us = landru_host_now_us();
	s_residual_us = 0;
	frame_elapsed_ticks_gbl = 0;
	timer_module_gbl = 1;
}

void xtimer_Destroy_Timer_Interrupt(void) {
	if (timer_module_gbl)
		timer_module_gbl = 0;
}

/* --- Frame pacing --- */

int xtimer_Frame_Budget_Pending(void) {
	xtimer_consume_elapsed_ticks();
	if (frame_rate_gbl <= 0)
		return 0;
	if (frame_elapsed_ticks_gbl >= (uint32_t)frame_rate_gbl)
		return 0;
	/* Pixel writes during the wait remain buffered for the host to present. */
	xio_Poll_Fast_Input();
	return 1;
}

uint64_t xtimer_Next_Frame_Delay_Us(void) {
	if (frame_rate_gbl <= 0)
		return UINT64_MAX;
	xtimer_sync_ticks();
	uint64_t elapsed_ticks = (uint64_t)frame_elapsed_ticks_gbl + (uint32_t)timer_ticks_gbl;
	if (elapsed_ticks >= (uint32_t)frame_rate_gbl)
		return 0;
	uint64_t remaining_us = ((uint32_t)frame_rate_gbl - elapsed_ticks) * LANDRU_TIMER_TICK_US;
	return remaining_us - s_residual_us;
}

float xtimer_Frame_Progress(void) {
	if (frame_rate_gbl <= 0)
		return 0.0f;
	xtimer_sync_ticks();
	uint64_t elapsed_ticks = (uint64_t)frame_elapsed_ticks_gbl + (uint32_t)timer_ticks_gbl;
	if (elapsed_ticks >= (uint32_t)frame_rate_gbl)
		return 1.0f;
	uint64_t elapsed_us = elapsed_ticks * LANDRU_TIMER_TICK_US + s_residual_us;
	uint32_t budget_us = (uint32_t)frame_rate_gbl * LANDRU_TIMER_TICK_US;
	return (float)elapsed_us / (float)budget_us;
}

void xtimer_Commit_Frame(void) {
	xtimer_consume_elapsed_ticks();
	/* Both TIE95 and TIE98 clear the whole frame-lock accumulator after an
	 * accepted frame. This permits one frame after a blocking fade or stall,
	 * then discards the excess instead of replaying overdue film cels. */
	frame_elapsed_ticks_gbl = 0;
}

/* Periodic yield point inside long inner loops. It drains accumulated PIT
 * ticks and consumes the latest input state. */
void xtimer_Often(void) {
	xtimer_consume_elapsed_ticks();
	xio_Poll_Fast_Input();
}

/* --- Time queries --- */

int32_t xtimer_Current_Time(void) {
	/* Include uncommitted host ticks so mid-frame callers see live time. */
	xtimer_sync_ticks();
	return (int32_t)((uint32_t)time_gbl + (uint32_t)timer_ticks_gbl);
}

/* --- Frame rate control --- */

void xtimer_Set_Frame_Rate(int16_t rate) { frame_rate_gbl = rate; }

/* Cel budget in microseconds at the active frame rate (rate × 4 ms).
 * Returns 0 when no rate is set so consumers can detect the
 * "no scene-clock cadence" case. */
uint32_t xtimer_Frame_Period_Us(void) {
	if (frame_rate_gbl <= 0)
		return 0;
	return (uint32_t)frame_rate_gbl * LANDRU_TIMER_TICK_US;
}

#ifdef LANDRU_GAME_XW
void xtimer_Reset_Elapsed_Sample(void) {
	elapsed_period_us = LANDRU_TIMER_TICK_US;
	elapsed_sample_started = false;
	elapsed_sample_us = 0;
	elapsed_sample_residual_us = 0;
}

void xtimer_Set_Elapsed_Period_Us(uint32_t period_us) {
	elapsed_period_us = period_us ? period_us : LANDRU_TIMER_TICK_US;
	elapsed_sample_started = true;
	elapsed_sample_us = landru_host_now_us();
	elapsed_sample_residual_us = 0;
}

uint32_t xtimer_Elapsed_Period_Us(void) { return elapsed_period_us; }

int32_t xtimer_Time_Elapsed(void) {
	uint64_t now = landru_host_now_us();
	if (!elapsed_sample_started || now < elapsed_sample_us) {
		elapsed_sample_started = true;
		elapsed_sample_us = now;
		elapsed_sample_residual_us = 0;
		return 0;
	}
	uint64_t elapsed = now - elapsed_sample_us + elapsed_sample_residual_us;
	elapsed_sample_us = now;
	uint32_t ticks = (uint32_t)(elapsed / elapsed_period_us);
	elapsed_sample_residual_us = (uint32_t)(elapsed % elapsed_period_us);
	return (int32_t)ticks;
}

uint64_t xtimer_Elapsed_Delay_Us(uint32_t ticks) {
	if (!ticks)
		return 0;
	uint64_t required = (uint64_t)ticks * elapsed_period_us;
	uint64_t now = landru_host_now_us();
	uint64_t available = elapsed_sample_started && now >= elapsed_sample_us
							 ? now - elapsed_sample_us + elapsed_sample_residual_us
							 : 0;
	return available < required ? required - available : 0;
}

void xtimer_Set_FrameRatePreset(int32_t preset) {
	static const int16_t budgets[LANDRU_FRAME_PRESET_COUNT] = {
		LANDRU_FRAME_TICKS_0, LANDRU_FRAME_TICKS_1, LANDRU_FRAME_TICKS_2,
		LANDRU_FRAME_TICKS_3, LANDRU_FRAME_TICKS_4, LANDRU_FRAME_TICKS_5,
	};
	if (preset == frame_preset_gbl)
		return;
	frame_preset_gbl = preset;
	if (preset >= 0 && preset < LANDRU_FRAME_PRESET_COUNT)
		xtimer_Set_Frame_Rate(budgets[preset]);
}
#endif
