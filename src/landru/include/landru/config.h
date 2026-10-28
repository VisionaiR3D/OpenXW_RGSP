#ifndef LANDRU_CONFIG_H
#define LANDRU_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#if defined(LANDRU_GAME_XW) && defined(LANDRU_GAME_TIE)
#error Select only one Landru game version
#endif

#if !defined(LANDRU_GAME_XW) && !defined(LANDRU_GAME_TIE)
#define LANDRU_GAME_TIE
#endif

#ifdef LANDRU_GAME_XW
typedef int16_t LandruBoolResult;
#else
typedef bool LandruBoolResult;
#endif

enum { LANDRU_RESOURCE_NAME_LENGTH = 8 };

#endif
