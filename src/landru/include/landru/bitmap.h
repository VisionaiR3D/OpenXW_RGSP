#ifndef LANDRU_BITMAP_H
#define LANDRU_BITMAP_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/rect.h>

typedef struct {
	void* data;
	int32_t offset;
	Rect clip;
	int16_t w;
	int16_t h;
	uint8_t type;
	uint8_t flags;
} BitmapStruct;

void xbitmap_Init_Bitmap(BitmapStruct* bitmap);
int16_t xbitmap_Alloc_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height);
int16_t xbitmap_Alloc_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height);
int16_t xbitmap_Alloc_Trans_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height);
int16_t xbitmap_Alloc_Trans_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height);
int16_t xbitmap_Alloc_Extra_Trans_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height,
												int32_t extend);
int16_t xbitmap_Alloc_Extra_Trans_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height, int32_t extend);
int16_t xbitmap_Alloc_Extra_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height,
										  int32_t extend);
int16_t xbitmap_Alloc_Extra_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height, int32_t extend);
void xbitmap_Free_Bitmap(BitmapStruct* bitmap);
void* xbitmap_Lock_Bitmap(BitmapStruct* bitmap);
void* xbitmap_Lock_Bitmap_Line(BitmapStruct* bitmap, int16_t line);
void xbitmap_Unlock_Bitmap(BitmapStruct* bitmap);
bool xbitmap_Clip_Rect_To_Bitmap(BitmapStruct* bitmap, Rect* rect);
void xbitmap_Get_Bitmap_Clipping(BitmapStruct* bitmap, Rect* out);
void xbitmap_Set_Bitmap_Clipping(BitmapStruct* bitmap, Rect* rect);
void xbitmap_Max_Bitmap_Clipping(BitmapStruct* bitmap);
void xbitmap_Set_Bitmap_Offset(BitmapStruct* bitmap, int32_t offset);
int32_t xbitmap_Get_Bitmap_Offset(BitmapStruct* bitmap);
void xbitmap_Erase_Bitmap(BitmapStruct* bitmap);
int16_t xbitmap_Copy_Bitmap(BitmapStruct* dst, BitmapStruct* src, int16_t x, int16_t y);
int16_t xbitmap_Copy_Bitmap_Clip(BitmapStruct* dst, BitmapStruct* src, int16_t x, int16_t y);
int16_t xbitmap_Copy_Bitmap_Offset_Clip(BitmapStruct* dst, BitmapStruct* src, int16_t x, int16_t y);
int16_t xbitmap_Copy_Bitmap_Portion(BitmapStruct* dst, BitmapStruct* src, Rect* clipRect, int16_t x,
									int16_t y);
void xbitmap_Copy_Bitmap_Rect(BitmapStruct* dst, BitmapStruct* src, Rect* dst_rect, Rect* src_rect);
void xbitmap_Copy_Bitmap_Data(int32_t dst_stride, int32_t num_lines, int32_t copy_width, int32_t src_stride,
							  uint8_t* dst, const uint8_t* src);
void xbitmap_Copy_Trans_Bitmap_Data(int32_t dst_stride, int32_t num_lines, int32_t copy_width,
									int32_t src_stride, uint8_t* dst, const uint8_t* src);

#ifdef LANDRU_GAME_XW
enum { LANDRU_RASTER_COPY = 0, LANDRU_RASTER_DOUBLE = 2, LANDRU_RASTER_MODE_MASK = 14 };

/* Supports the copy and doubled-copy modes used by X-Wing. */
void Raster_BlitIndexed8Clipped(uint8_t* destination, int32_t destination_stride, int32_t destination_height,
								const uint8_t* source, int32_t width, int32_t height, int32_t source_stride,
								int32_t x, int32_t y, int32_t mode);
#endif

#endif
