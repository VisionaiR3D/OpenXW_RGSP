#ifndef LANDRU_JOY_H
#define LANDRU_JOY_H

#include <landru/config.h>

#include <stdint.h>

/* Host-normalized axis range and the fixed subtractive input deadzone. */
enum { LANDRU_JOYSTICK_AXIS_LIMIT = 127, LANDRU_JOYSTICK_DEADZONE = 12 };

/* Suppresses the normalized cursor/TIE input service. */
extern int16_t ngstickflag;

#ifdef LANDRU_GAME_XW
enum { LANDRU_JOYSTICK_READ_FAILED = 32000 };

/* Host supplies X-Wing normalization, failure/suppression semantics and the
 * full button mask (28 buttons followed by four POV bits). */
uint32_t xjoy_Joystick_Read(int32_t* outX, int32_t* outY, int32_t* outZ, int16_t deviceIndex);
#endif

/* Returns the number of joysticks detected (0..2). */
int16_t xjoy_Joystick_Init(void);

#ifndef LANDRU_GAME_XW
/* Read axes (-127..+127 with dead zone) and return first 4 buttons in
 * the low nibble. */
int16_t xjoy_Joystick_Read(int16_t* outX, int16_t* outY, int16_t port);
#endif

/* Read up to n_axes analog channels into axes[] (-127..+127 with dead
 * zone). Axis 0 = X / yaw, 1 = Y / pitch, 2 = roll (second-stick or
 * twist on devices that have it). Returns first 4 buttons in low
 * nibble. Hosts without a requested axis leave it zero. */
int16_t xjoy_Joystick_Read_Axes(int16_t* axes, int n_axes, int16_t port);

#endif
