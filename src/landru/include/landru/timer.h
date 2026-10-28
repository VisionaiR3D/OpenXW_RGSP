#ifndef LANDRU_TIMER_H
#define LANDRU_TIMER_H

#include <landru/config.h>

#include <stdint.h>

enum { LANDRU_TIMER_TICK_US = 4000 };

#ifdef LANDRU_GAME_XW
enum LandruFrameRatePreset {
	LANDRU_FRAME_PRESET_0,
	LANDRU_FRAME_PRESET_1,
	LANDRU_FRAME_PRESET_2,
	LANDRU_FRAME_PRESET_3,
	LANDRU_FRAME_PRESET_4,
	LANDRU_FRAME_PRESET_5,
	LANDRU_FRAME_PRESET_COUNT,
};

enum LandruFrameTickBudget {
	LANDRU_FRAME_TICKS_0 = 54,
	LANDRU_FRAME_TICKS_1 = 34,
	LANDRU_FRAME_TICKS_2 = 27,
	LANDRU_FRAME_TICKS_3 = 23,
	LANDRU_FRAME_TICKS_4 = 14,
	LANDRU_FRAME_TICKS_5 = 11,
};

/* Samples game elapsed time independently of Landru's frame accumulator.
 * The first call after host installation establishes the sample epoch. */
int32_t xtimer_Time_Elapsed(void);
/* Owner-thread flight cadence, independent of Landru's current/frame clocks.
 * Setting it rebases the sample and clears its residual; zero selects 4000 us. */
void xtimer_Set_Elapsed_Period_Us(uint32_t period_us);
uint32_t xtimer_Elapsed_Period_Us(void);
/* Remaining delay without consuming the flight elapsed sampler. */
uint64_t xtimer_Elapsed_Delay_Us(uint32_t ticks);
void xtimer_Set_FrameRatePreset(int32_t preset);
#endif

void xtimer_Create_Timer_Interrupt(void);
void xtimer_Destroy_Timer_Interrupt(void);

/* Frame-budget gate for tasks.
 *
 * xtimer_Frame_Budget_Pending: returns 1 if frame_rate_gbl is set AND
 * the per-scene budget has not yet elapsed since the last frame
 * commit. Tasks yield when 1 is reported; pending pixels remain buffered.
 *
 * xtimer_Commit_Frame: budget elapsed; consume accumulated PIT ticks and
 * clear the entire frame-lock accumulator. Tasks call this once the gate
 * returns 0 to mark the frame as begun. */
int xtimer_Frame_Budget_Pending(void);
void xtimer_Commit_Frame(void);
/* Remaining synthetic-clock delay to the current scheduled frame deadline. */
uint64_t xtimer_Next_Frame_Delay_Us(void);

/* Clamped progress through the current scheduled frame, or zero when idle. */
float xtimer_Frame_Progress(void);

/* Periodic yield point inside long inner loops — drains accumulated
 * PIT ticks into the engine's time_gbl and pumps fast-input. */
void xtimer_Often(void);

/* Live PIT-tick time (time_gbl + unconsumed ticks). */
int32_t xtimer_Current_Time(void);

/* Set the per-scene frame target in PIT ticks without resetting elapsed time. */
void xtimer_Set_Frame_Rate(int16_t rate);

/* Cel budget in microseconds (frame_rate_gbl × 4 × 1000). Returns
 * 0 when no rate is set. Render emitters publish it as an interpolation
 * hint. */
uint32_t xtimer_Frame_Period_Us(void);

#endif
