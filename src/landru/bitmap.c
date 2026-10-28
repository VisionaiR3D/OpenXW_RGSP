#include <stdlib.h>
#include <string.h>

#include <landru/bitmap.h>
#include <landru/rect.h>
#include <landru/vesa.h>

#include "host_internal.h"

void xbitmap_Init_Bitmap(BitmapStruct* bitmap) {
	xrect_Clear_Rect(&bitmap->clip);
	bitmap->w = 0;
	bitmap->h = 0;
	bitmap->data = NULL;
	bitmap->offset = 0;
	bitmap->type = 0;
	bitmap->flags = 0;
}

int16_t xbitmap_Alloc_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height) {
	return xbitmap_Alloc_Extra_System_Bitmap(bitmap, width, height, 0);
}

int16_t xbitmap_Alloc_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height) {
	return xbitmap_Alloc_Extra_Bitmap(bitmap, width, height, 0);
}

int16_t xbitmap_Alloc_Trans_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height) {
	int16_t result = xbitmap_Alloc_Extra_System_Bitmap(bitmap, width, height, 0);
	if (result)
		bitmap->flags = 1;
	return result;
}

int16_t xbitmap_Alloc_Trans_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height) {
	int16_t result = xbitmap_Alloc_Extra_Bitmap(bitmap, width, height, 0);
	if (result)
		bitmap->flags = 1;
	return result;
}

int16_t xbitmap_Alloc_Extra_Trans_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height,
												int32_t extend) {
	int16_t result = xbitmap_Alloc_Extra_System_Bitmap(bitmap, width, height, extend);
	if (result)
		bitmap->flags = 1;
	return result;
}

int16_t xbitmap_Alloc_Extra_Trans_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height,
										 int32_t extend) {
	int16_t result = xbitmap_Alloc_Extra_Bitmap(bitmap, width, height, extend);
	if (result)
		bitmap->flags = 1;
	return result;
}

int16_t xbitmap_Alloc_Extra_System_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height,
										  int32_t extend) {
	void* buf = malloc(width * height + extend);
	if (!buf) {
		xbitmap_Init_Bitmap(bitmap);
		return 0;
	}
	bitmap->data = buf;
	bitmap->offset = 0;
	xrect_Set_Rect(&bitmap->clip, 0, 0, width, height);
	bitmap->w = width;
	bitmap->h = height;
	return 1;
}

int16_t xbitmap_Alloc_Extra_Bitmap(BitmapStruct* bitmap, int16_t width, int16_t height, int32_t extend) {
	void* buf = malloc(width * height + extend);
	if (!buf) {
		xbitmap_Init_Bitmap(bitmap);
		return 0;
	}
	bitmap->data = buf;
	bitmap->offset = 0;
	xrect_Set_Rect(&bitmap->clip, 0, 0, width, height);
	bitmap->w = width;
	bitmap->h = height;
	return 1;
}

void xbitmap_Free_Bitmap(BitmapStruct* bitmap) {
	if (bitmap->data)
		free(bitmap->data);
	xbitmap_Init_Bitmap(bitmap);
}

void* xbitmap_Lock_Bitmap(BitmapStruct* bitmap) {
	if (!bitmap->data)
		return NULL;
	return (uint8_t*)bitmap->data + bitmap->offset;
}

void* xbitmap_Lock_Bitmap_Line(BitmapStruct* bitmap, int16_t line) {
	if (!bitmap->data)
		return NULL;
	return (uint8_t*)bitmap->data + bitmap->offset + line * bitmap->w;
}

/* Binary calls XMEMHDL_Unlock_Handle when bitmap->type == 0. With malloc
 * there's no compaction, so no lock/unlock pairing is needed. */
void xbitmap_Unlock_Bitmap(BitmapStruct* bitmap) { (void)bitmap; }

bool xbitmap_Clip_Rect_To_Bitmap(BitmapStruct* bitmap, Rect* rect) {
	return xrect_Clip_Rect(rect, &bitmap->clip);
}

void xbitmap_Get_Bitmap_Clipping(BitmapStruct* bitmap, Rect* out) { *out = bitmap->clip; }

void xbitmap_Set_Bitmap_Clipping(BitmapStruct* bitmap, Rect* rect) {
	xbitmap_Max_Bitmap_Clipping(bitmap);
	xrect_Clip_Rect(&bitmap->clip, rect);
}

void xbitmap_Max_Bitmap_Clipping(BitmapStruct* bitmap) {
	xrect_Set_Rect(&bitmap->clip, 0, 0, bitmap->w, bitmap->h);
}

void xbitmap_Set_Bitmap_Offset(BitmapStruct* bitmap, int32_t offset) { bitmap->offset = offset; }

int32_t xbitmap_Get_Bitmap_Offset(BitmapStruct* bitmap) { return bitmap->offset; }

void xbitmap_Erase_Bitmap(BitmapStruct* bitmap) {
	if (!bitmap->data)
		return;
	void* ptr = xbitmap_Lock_Bitmap(bitmap);
	memset(ptr, 0, bitmap->w * bitmap->h);
	xbitmap_Unlock_Bitmap(bitmap);
}

int16_t xbitmap_Copy_Bitmap(BitmapStruct* dst, BitmapStruct* src, int16_t x, int16_t y) {
	Rect rect;
	xrect_Set_Rect(&rect, 0, 0, src->w, src->h);
	return xbitmap_Copy_Bitmap_Portion(dst, src, &rect, x, y);
}

int16_t xbitmap_Copy_Bitmap_Clip(BitmapStruct* dst, BitmapStruct* src, int16_t x, int16_t y) {
	Rect src_clip = src->clip;
	return xbitmap_Copy_Bitmap_Portion(dst, src, &src_clip, x - src->clip.left, y - src->clip.top);
}

int16_t xbitmap_Copy_Bitmap_Offset_Clip(BitmapStruct* dst, BitmapStruct* src, int16_t x, int16_t y) {
	return xbitmap_Copy_Bitmap_Portion(dst, src, &src->clip, x, y);
}

int16_t xbitmap_Copy_Bitmap_Portion(BitmapStruct* dst, BitmapStruct* src, Rect* clipRect, int16_t x,
									int16_t y) {
	Rect src_clipped;
	Rect dst_rect;
	Rect dst_clipped;

	xrect_Copy_Rect(&src_clipped, clipRect);
	if (!xrect_Clip_Rect(&src_clipped, &src->clip))
		return 0;

	xrect_Copy_Rect(&dst_rect, &src_clipped);
	xrect_Offset_Rect(&dst_rect, x - clipRect->left, y - clipRect->top);
	xrect_Copy_Rect(&dst_clipped, &dst_rect);
	if (!xrect_Clip_Rect(&dst_clipped, &dst->clip))
		return 0;

	src_clipped.left += dst_clipped.left - dst_rect.left;
	src_clipped.top += dst_clipped.top - dst_rect.top;
	src_clipped.right += dst_clipped.right - dst_rect.right;
	src_clipped.bottom += dst_clipped.bottom - dst_rect.bottom;
	xbitmap_Copy_Bitmap_Rect(dst, src, &dst_clipped, &src_clipped);
	return 1;
}

void xbitmap_Copy_Bitmap_Rect(BitmapStruct* dst, BitmapStruct* src, Rect* dst_rect, Rect* src_rect) {
	uint8_t* pSrc = xbitmap_Lock_Bitmap(src);
	uint8_t* pDst = xbitmap_Lock_Bitmap(dst);

	if (pSrc && pDst) {
		int16_t sw = src->w;
		int16_t dw = dst->w;
		int16_t copy_w = dst_rect->right - dst_rect->left;
		int16_t copy_h = dst_rect->bottom - dst_rect->top;
		uint8_t* s = pSrc + src_rect->left + src_rect->top * sw;
		uint8_t* d = pDst + dst_rect->left + dst_rect->top * dw;

		if (src->flags & 1)
			xbitmap_Copy_Trans_Bitmap_Data(dw, copy_h, copy_w, sw, d, s);
		else
			xbitmap_Copy_Bitmap_Data(dw, copy_h, copy_w, sw, d, s);
	}

	xbitmap_Unlock_Bitmap(dst);
	xbitmap_Unlock_Bitmap(src);
}

void xbitmap_Copy_Bitmap_Data(int32_t dst_stride, int32_t num_lines, int32_t copy_width, int32_t src_stride,
							  uint8_t* dst, const uint8_t* src) {
	if (copy_width <= 0)
		return;
	for (int32_t row = 0; row < num_lines; ++row)
		memcpy(dst + row * dst_stride, src + row * src_stride, (size_t)copy_width);
}

#ifdef LANDRU_GAME_XW
void Raster_BlitIndexed8Clipped(uint8_t* destination, int32_t destination_stride, int32_t destination_height,
								const uint8_t* source, int32_t width, int32_t height, int32_t source_stride,
								int32_t x, int32_t y, int32_t mode) {
	int32_t source_x = 0;
	int32_t source_y = 0;
	if (x < 0) {
		width += x;
		source_x = -x;
		x = 0;
	}
	if (y < 0) {
		height += y;
		source_y = -y;
		y = 0;
	}
	if (x + width > landru_logical_width_gbl)
		width = landru_logical_width_gbl - x;
	if (y + height > landru_logical_height_gbl)
		height = landru_logical_height_gbl - y;
	if (width <= 0 || height <= 0)
		return;
	source += source_y * source_stride + source_x;
	switch (mode & LANDRU_RASTER_MODE_MASK) {
		case LANDRU_RASTER_COPY:
			xbitmap_Copy_Bitmap_Data(destination_stride, height, width, source_stride,
									 destination + y * destination_stride + x, source);
			break;
		case LANDRU_RASTER_DOUBLE: {
			int32_t top = 2 * y + (destination_height - 2 * landru_logical_height_gbl) / 2;
			for (int32_t row = 0; row < height; ++row) {
				uint8_t* first = destination + (top + 2 * row) * destination_stride + 2 * x;
				for (int32_t column = 0; column < width; ++column) {
					uint8_t pixel = source[row * source_stride + column];
					first[2 * column] = pixel;
					first[2 * column + 1] = pixel;
				}
				memcpy(first + destination_stride, first, (size_t)width * 2);
			}
			break;
		}
		default:
			landru_host_log(LANDRU_LOG_ERROR, "unsupported Landru raster mode %d\n", mode);
			break;
	}
}
#endif

void xbitmap_Copy_Trans_Bitmap_Data(int32_t dst_stride, int32_t num_lines, int32_t copy_width,
									int32_t src_stride, uint8_t* dst, const uint8_t* src) {
	int32_t dst_pad = dst_stride - copy_width;
	int32_t src_pad = src_stride - copy_width;

	while (num_lines--) {
		for (int32_t x = 0; x < copy_width; x++) {
			if (src[x])
				dst[x] = src[x];
		}
		dst += copy_width + dst_pad;
		src += copy_width + src_pad;
	}
}
