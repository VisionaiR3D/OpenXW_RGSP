#ifndef LANDRU_FILE_H
#define LANDRU_FILE_H

#include <landru/config.h>

#include <stdint.h>

#include <landru/host.h>

/* File I/O */
LandruFile* xfile_Open_File(LandruFileRoot root, const char* name, const char* mode);
#ifdef LANDRU_GAME_XW
int32_t xfile_Close_File(LandruFile* file);
#else
int16_t xfile_Close_File(LandruFile* file);
#endif
int16_t xfile_Seek_File(LandruFile* file, int32_t offset, int16_t whence);
int32_t xfile_Tell_File(LandruFile* file);

/* File transfers use native endian, without swapping. X-Wing word/long
 * helpers return 0 on success and 1 on failure. Byte/bulk helpers and all
 * TIE transfer helpers return 1 on success and 0 on failure. */

/* File read */
int16_t xfile_Read_Byte_From_File(LandruFile* file, uint8_t* data);
#ifdef LANDRU_GAME_XW
int16_t xfile_Read_Word_From_File(LandruFile* file, uint16_t* data);
int16_t xfile_Read_Long_From_File(LandruFile* file, uint32_t* data);
int32_t xfile_Read_Data_From_File(LandruFile* file, void* data, uint32_t size);
#else
int16_t xfile_Read_Word_From_File(LandruFile* file, int16_t* data);
int16_t xfile_Read_Long_From_File(LandruFile* file, int32_t* data);
int16_t xfile_Read_Data_From_File(LandruFile* file, void* data, int32_t size);
#endif

/* File write */
int16_t xfile_Write_Byte_To_File(LandruFile* file, uint8_t data);
#ifdef LANDRU_GAME_XW
int16_t xfile_Write_Word_To_File(LandruFile* file, uint16_t data);
int16_t xfile_Write_Long_To_File(LandruFile* file, uint32_t data);
int32_t xfile_Write_Data_To_File(LandruFile* file, const void* data, uint32_t size);
#else
int16_t xfile_Write_Word_To_File(LandruFile* file, int16_t data);
int16_t xfile_Write_Long_To_File(LandruFile* file, int32_t data);
int16_t xfile_Write_Data_To_File(LandruFile* file, const void* data, int32_t size);
#endif

/* Buffer read (advances *offset) */
uint8_t xfile_Read_Byte_From_Buffer(const uint8_t* buffer, int32_t* offset);
int16_t xfile_Read_Word_From_Buffer(const uint8_t* buffer, int32_t* offset);
int32_t xfile_Read_Long_From_Buffer(const uint8_t* buffer, int32_t* offset);
void xfile_Read_Data_From_Buffer(const uint8_t* buffer, void* data, int32_t* offset, int32_t size);

/* Buffer write (advances *offset) */
void xfile_Write_Byte_To_Buffer(uint8_t* buffer, int32_t* offset, uint8_t val);
void xfile_Write_Word_To_Buffer(uint8_t* buffer, int32_t* offset, int16_t val);
void xfile_Write_Long_To_Buffer(uint8_t* buffer, int32_t* offset, int32_t val);
void xfile_Write_Data_To_Buffer(uint8_t* buffer, const void* data, int32_t* offset, int32_t size);

/* Byte-order swap */
int16_t xfile_Swap_Word(int16_t value);
int32_t xfile_Swap_DWord(int32_t value);

#endif
