#ifndef LANDRU_MEMCOM_H
#define LANDRU_MEMCOM_H

#include <landru/config.h>

#include <stdint.h>

void xmemcom_Add_Memory_Callback(void (*callback)(int16_t), int16_t flags);
void xmemcom_Free_Memory_Callback(void (*callback)(int16_t));

#endif
