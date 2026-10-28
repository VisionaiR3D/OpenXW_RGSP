#ifndef LANDRU_STREAM_H
#define LANDRU_STREAM_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdint.h>

#ifdef LANDRU_GAME_XW
enum { LANDRU_STREAM_CHANNEL_COUNT = 2, LANDRU_STREAM_FILENAME_SIZE = 256, LANDRU_STREAM_BLOCK_SIZE = 8192 };

void xstream_ConfigureChannel(int channel, uint8_t bypassPenalty, char driveLetter);
int16_t xstream_Rotate_Current_Stream_File(int channel);
void xstream_Set_Stream_Tick_Counts(char driveLetter, const char* benchmarkFilename);
#else
enum { LANDRU_STREAM_CHANNEL_COUNT = 2, LANDRU_STREAM_MAX_CHAINS = 10, LANDRU_STREAM_FILENAME_SIZE = 64 };
#endif

int16_t xstream_Init_Stream_Engine(int channel, int32_t buff_size, int32_t prefetch_amount);
int16_t xstream_Exit_Stream_Engine(int channel);
int16_t xstream_Chain_Stream_File(int channel, const char* filename);
int16_t xstream_Unchain_Current_Stream_File(int channel);
int16_t xstream_Use_Stream_File(int channel, const char* filename);
int32_t xstream_Read_From_Stream_Buffer(int channel, LandruHandle destination, uint32_t offset, uint32_t size,
										int16_t blocking);
int16_t xstream_Get_Stream_Flag(void);
void xstream_Adjust_For_Penalty(uint32_t size);
#ifndef LANDRU_GAME_XW
void xstream_Set_Stream_Tick_Counts(void);
#endif

#endif
