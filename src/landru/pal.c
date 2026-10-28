#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <landru/fourcc.h>
#include <landru/memptr.h>
#include <landru/pal.h>
#include <landru/res.h>
#include <landru/view.h>

#include "host_internal.h"

static bool s_palette_module = false;
// GLOBAL: TIE 0xD2C4C
static Palette* screen_pal_gbl = NULL;
// GLOBAL: TIE 0xD2C50
static Palette* src_pal_gbl = NULL;
// GLOBAL: TIE 0xD2C54
static Palette* dst_pal_gbl = NULL;
// GLOBAL: TIE 0xD2C48
static Palette* work_pal_gbl = NULL;
// GLOBAL: TIE 0xD2C58
static Palette* video_pal_gbl = NULL;
// GLOBAL: TIE 0xD2C44
static Palette* first_palette_gbl = NULL;

/* XPAL_Set_VGA_Palette waits for a mode-13h retrace before every changed
 * palette range. Queue those waits on one absolute 70 Hz timeline so host
 * presentation frequency cannot alter their duration. */
static uint64_t s_vga_next_retrace_us;
static uint64_t s_vga_wait_until_us;

static void xpal_schedule_vga_retrace(void) {
	uint64_t now_us = landru_host_now_us();
	if (s_vga_next_retrace_us <= now_us) {
		uint64_t elapsed_periods = (now_us - s_vga_next_retrace_us) / LANDRU_VGA_RETRACE_PERIOD_US + 1u;
		s_vga_next_retrace_us += elapsed_periods * LANDRU_VGA_RETRACE_PERIOD_US;
	}
	s_vga_wait_until_us = s_vga_next_retrace_us;
	s_vga_next_retrace_us += LANDRU_VGA_RETRACE_PERIOD_US;
}

void xpal_Set_VGA_Palette(RGBStruct* pal, int16_t start, int16_t len) {
	landru_host_palette_set((const uint8_t*)pal, (int)start, (int)len);
	xpal_schedule_vga_retrace();
}

uint64_t xpal_Next_VGA_Delay_Us(void) {
	uint64_t now_us = landru_host_now_us();
	return s_vga_wait_until_us > now_us ? s_vga_wait_until_us - now_us : UINT64_MAX;
}

void xpal_Create_Palette_Module(void) {
	int16_t ok = 1;
	s_palette_module = true;
	s_vga_next_retrace_us = landru_host_now_us() + LANDRU_VGA_RETRACE_PERIOD_US;
	s_vga_wait_until_us = 0;

	screen_pal_gbl = xpal_Alloc_Palette(0, 256);
	if (screen_pal_gbl)
		xpal_Clear_Palette(screen_pal_gbl);
	else
		ok = 0;

	src_pal_gbl = xpal_Alloc_Palette(0, 256);
	if (src_pal_gbl)
		xpal_Clear_Palette(src_pal_gbl);
	else
		ok = 0;

	dst_pal_gbl = xpal_Alloc_Palette(0, 256);
	if (dst_pal_gbl)
		xpal_Clear_Palette(dst_pal_gbl);
	else
		ok = 0;

	work_pal_gbl = xpal_Alloc_Palette(0, 256);
	if (work_pal_gbl)
		xpal_Clear_Palette(work_pal_gbl);
	else
		ok = 0;

	video_pal_gbl = xpal_Alloc_Palette(0, 256);
	if (video_pal_gbl)
		xpal_Set_Palette_RGB(video_pal_gbl, 63, 63, 63, 0, 256);
	else
		ok = 0;

	s_palette_module = ok;
}

void xpal_Destroy_Palette_Module(void) {
	s_vga_next_retrace_us = 0;
	s_vga_wait_until_us = 0;
	if (screen_pal_gbl) {
		xpal_Free_Palette(screen_pal_gbl);
		screen_pal_gbl = NULL;
	}
	if (src_pal_gbl) {
		xpal_Free_Palette(src_pal_gbl);
		src_pal_gbl = NULL;
	}
	if (dst_pal_gbl) {
		xpal_Free_Palette(dst_pal_gbl);
		dst_pal_gbl = NULL;
	}
	if (work_pal_gbl) {
		xpal_Free_Palette(work_pal_gbl);
		work_pal_gbl = NULL;
	}
	if (video_pal_gbl) {
		xpal_Free_Palette(video_pal_gbl);
		video_pal_gbl = NULL;
	}
	s_palette_module = false;
}

Palette* xpal_Ask_Palette_List(void) { return first_palette_gbl; }

void xpal_Add_Palette_To_System(Palette* pal) {
	Palette* cur = first_palette_gbl;
	Palette* tail = NULL;
	while (cur) {
		tail = cur;
		cur = cur->next;
	}
	pal->next = NULL;
	if (tail)
		tail->next = pal;
	else
		first_palette_gbl = pal;
}

void xpal_Free_Palette_From_System(Palette* pal) {
	Palette* cur = first_palette_gbl;
	Palette* prev = NULL;
	while (cur && cur != pal) {
		prev = cur;
		cur = cur->next;
	}
	if (cur == pal) {
		if (prev)
			prev->next = pal->next;
		else
			first_palette_gbl = pal->next;
		pal->next = NULL;
	}
}

Palette* xpal_Alloc_Palette(int16_t start, int16_t numColors) {
	Palette* pal = xmemptr_Alloc_System_Pointer(sizeof(Palette));
	if (!pal)
		return NULL;

	LandruHandle colors = LANDRU_NULL_HANDLE;
	if (numColors) {
		colors = xpal_Alloc_RGB(numColors);
		if (!colors) {
			xmemptr_Free_System_Pointer(pal);
			return NULL;
		}
	}

	pal->colors = colors;
	pal->cycle_count = 0;
	pal->cycle_active = 0;
	pal->varptr = NULL;
	pal->varhdl = LANDRU_NULL_HANDLE;
	pal->next = NULL;
	pal->start = start;
	pal->len = numColors;
	pal->end = start + numColors - 1;
	return pal;
}

void xpal_Free_Palette(Palette* pal) {
#ifndef LANDRU_GAME_XW
	xmemhdl_Free_Handle(pal->varhdl);
#endif
	if (pal->colors) {
		xmemhdl_Free_Handle(pal->colors);
	}
	if (pal->varptr) {
		xmemptr_Free_System_Pointer(pal->varptr);
	}
#ifdef LANDRU_GAME_XW
	xmemhdl_Free_Handle(pal->varhdl);
#endif
	xmemptr_Free_System_Pointer(pal);
}

void xpal_Free_Palettes(Palette* pal) {
	while (pal) {
		Palette* next = pal->next;
		xpal_Free_Palette_From_System(pal);
		xpal_Free_Palette(pal);
		pal = next;
	}
}

LandruHandle xpal_Alloc_RGB(int16_t numColors) {
	return numColors > 0 ? xmemhdl_Alloc_Handle(numColors * sizeof(RGBStruct), LANDRU_MEMORY_DEFAULT)
						 : LANDRU_NULL_HANDLE;
}

void xpal_Free_RGB(LandruHandle rgb) {
	if (rgb)
		xmemhdl_Free_Handle(rgb);
}

#ifdef LANDRU_GAME_XW
Palette* xpal_Res_Palette(ResFile* rf, const char* palName) {
#else
Palette* xpal_Res_Palette(const char* palName) {
	ResFile* rf = xres_Find_File_With_Resource(FOURCC_PLTT, palName);
#endif
	int offset;
	uint32_t size;
	if (!rf || !xres_Get_Resource_Offset(rf, FOURCC_PLTT, palName, &offset, &size))
		return NULL;
	LandruHandle palData = xres_Load_Resource_Data(rf, offset, size, LANDRU_MEMORY_DEFAULT);
	if (!palData)
		return NULL;

	uint8_t* palBuf = xmemhdl_Lock_Handle(palData);
	uint32_t offsetInData = 0;
	uint8_t palStart = palBuf[offsetInData++];
	uint8_t palEnd = palBuf[offsetInData++];
	int16_t numColors = palEnd - palStart + 1;

	Palette* pal = xpal_Alloc_Palette(palStart, numColors);
	if (!pal) {
		xmemhdl_Free_Handle(palData);
		return NULL;
	}

	RGBStruct* colors = xmemhdl_Lock_Handle(pal->colors);
	for (int i = 0; i < numColors; i++) {
		colors[i].r = palBuf[offsetInData + 0] >> 2;
		colors[i].g = palBuf[offsetInData + 1] >> 2;
		colors[i].b = palBuf[offsetInData + 2] >> 2;
		offsetInData += sizeof(RGBStruct);
	}
	pal->cycle_count = palBuf[offsetInData++];

	if (pal->cycle_count > 4)
		pal->cycle_count = 4;

	for (int i = 0; i < pal->cycle_count; i++) {
		pal->cycles[i].rate = palBuf[offsetInData + 0] + palBuf[offsetInData + 1] * 256;
		pal->cycles[i].rate = 4915 / pal->cycles[i].rate;

		uint8_t raw_start = palBuf[offsetInData + 2];
		uint8_t raw_end = palBuf[offsetInData + 3];
		if (raw_start < raw_end) {
			pal->cycles[i].dir = 1;
			pal->cycles[i].high = raw_end;
			pal->cycles[i].low = raw_start;
		} else {
			pal->cycles[i].dir = -1;
			pal->cycles[i].high = raw_start;
			pal->cycles[i].low = raw_end;
		}
		pal->cycles[i].active = 0;
		offsetInData += sizeof(uint16_t) + 2 * sizeof(uint8_t);
	}

	xmemhdl_Free_Handle(palData);

	xpal_Set_Palette_Name(pal, FOURCC_PLTT, palName);
	xpal_Add_Palette_To_System(pal);
	return pal;
}

void xpal_Set_Palette_Name(Palette* pal, uint32_t res_type, const char* pal_name) {
	pal->res_type = res_type;
	int i;
	for (i = 0; i < 8 && pal_name[i]; i++)
		pal->res_name[i] = tolower(pal_name[i]);
	for (; i < 8; i++)
		pal->res_name[i] = 0;
}

void xpal_Clear_Palette(Palette* pal) { xpal_Set_Palette_RGB(pal, 0, 0, 0, pal->start, pal->len); }

void xpal_Copy_Palette(Palette* dstPal, Palette* srcPal, int16_t start, int16_t length, int16_t offset) {
	int16_t srcSlot, dstSlot, dir;

	if (start < offset) {
		srcSlot = start + length - 1;
		dstSlot = offset + length - 1;
		dir = -1;
	} else {
		srcSlot = start;
		dstSlot = offset;
		dir = 1;
	}

	RGBStruct* src = xmemhdl_Lock_Handle(srcPal->colors);
	RGBStruct* dst = xmemhdl_Lock_Handle(dstPal->colors);
	while (srcSlot >= start && srcSlot < length + start) {
		if (srcSlot >= srcPal->start && srcSlot <= srcPal->end && dstSlot >= dstPal->start &&
			dstSlot <= dstPal->end) {
			dst[dstSlot - dstPal->start] = src[srcSlot - srcPal->start];
		}
		srcSlot += dir;
		dstSlot += dir;
	}
}

void xpal_Set_Palette_RGB(Palette* pal, uint8_t r, uint8_t g, uint8_t b, int16_t start, int16_t length) {
	RGBStruct* dst = xmemhdl_Lock_Handle(pal->colors);
	for (int16_t slot = start; slot < start + length; slot++) {
		if (slot >= pal->start && slot <= pal->end) {
			dst[slot - pal->start].r = r;
			dst[slot - pal->start].g = g;
			dst[slot - pal->start].b = b;
		}
	}
}

bool xpal_Compare_Palette(Palette* dstPal, Palette* srcPal, int16_t start, int16_t length, int16_t offset) {
	RGBStruct* src = xmemhdl_Lock_Handle(srcPal->colors);
	RGBStruct* dst = xmemhdl_Lock_Handle(dstPal->colors);

	for (int16_t srcSlot = start, dstSlot = offset; srcSlot < start + length; srcSlot++, dstSlot++) {
		if (srcSlot >= srcPal->start && srcSlot <= srcPal->end && dstSlot >= dstPal->start &&
			dstSlot <= dstPal->end) {
			RGBStruct s = src[srcSlot - srcPal->start];
			RGBStruct d = dst[dstSlot - dstPal->start];
			if (d.r != s.r || d.g != s.g || d.b != s.b)
				return false;
		}
	}
	return true;
}

bool xpal_Compare_Palette_RGB(Palette* pal, uint8_t r, uint8_t g, uint8_t b, int16_t start, int16_t count) {
	RGBStruct* data = xmemhdl_Lock_Handle(pal->colors);
	for (int16_t i = start; i < start + count; i++) {
		if (i >= pal->start && i <= pal->end) {
			RGBStruct c = data[i - pal->start];
			if (c.r != r || c.g != g || c.b != b)
				return false;
		}
	}
	return true;
}

bool xpal_Get_Palette_Index_RGB(Palette* pal, int16_t* r, int16_t* g, int16_t* b, int16_t color_idx) {
	RGBStruct* colors = xmemhdl_Lock_Handle(pal->colors);
	if (color_idx < pal->start || color_idx > pal->end) {
		*r = *g = *b = 0;
		return false;
	}
	RGBStruct c = colors[color_idx - pal->start];
	*r = c.r;
	*g = c.g;
	*b = c.b;
	return true;
}

void xpal_Set_Palette_Index_RGB(Palette* pal, uint8_t r, uint8_t g, uint8_t b, int16_t color_idx) {
	RGBStruct* colors = xmemhdl_Lock_Handle(pal->colors);
	if (color_idx >= pal->start && color_idx <= pal->end) {
		colors[color_idx - pal->start].r = r;
		colors[color_idx - pal->start].g = g;
		colors[color_idx - pal->start].b = b;
	}
}

void xpal_Start_Cycle(Palette* pal) {
	if (!pal->cycle_active) {
		pal->cycle_active = 1;
		for (int i = 0; i < pal->cycle_count; i++) {
			pal->cycles[i].active = 1;
			pal->cycles[i].current = pal->cycles[i].low;
		}
	}
}

void xpal_Stop_Cycle(Palette* pal) {
	if (pal->cycle_active) {
		pal->cycle_active = 0;
		for (int i = 0; i < pal->cycle_count; i++) {
			pal->cycles[i].current = pal->cycles[i].low;
			pal->cycles[i].active = 0;
			xpal_Restore_Screen_Palette(pal->cycles[i].low, pal->cycles[i].high);
		}
	}
}

void xpal_Cycle_Screen(void) {
	Palette* curPal = first_palette_gbl;
	bool active = false;
	while (curPal) {
		active |= curPal->cycle_active;
		curPal = curPal->next;
	}

	int16_t min = 256;
	int16_t max = -1;
	int32_t time = xview_Get_View_Time();

	if (active) {
		xpal_Copy_Palette(work_pal_gbl, screen_pal_gbl, 0, 256, 0);
		curPal = first_palette_gbl;
		while (curPal) {
			if (curPal->cycle_active) {
				for (int i = 0; i < curPal->cycle_count; i++) {
					int16_t low = curPal->cycles[i].low;
					int16_t high = curPal->cycles[i].high;
					int16_t current = curPal->cycles[i].current;

					if (curPal->cycles[i].active && curPal->cycles[i].rate) {
						if ((time % curPal->cycles[i].rate) == 0) {
							if (curPal->cycles[i].dir < 0) {
								current--;
								if (current < low)
									current = high;
							} else {
								current++;
								if (current > high)
									current = low;
							}
							curPal->cycles[i].current = current;
						}

						if (min > low)
							min = low;
						if (max < high)
							max = high;
						xpal_Set_Cycle_Range(low, high, current);
					}
				}
			}
			curPal = curPal->next;
		}
		if (min < max) {
			xpal_Set_Video_Palette(work_pal_gbl, min, max - min + 1, min);
		}
	}
}

void xpal_Cycles_To_Start(void) {
	Palette* curPal = first_palette_gbl;
	while (curPal) {
		if (curPal->cycle_active) {
			for (int i = 0; i < curPal->cycle_count; i++) {
				int16_t low = curPal->cycles[i].low;
				int16_t high = curPal->cycles[i].high;
				curPal->cycles[i].current = low;
				xpal_Restore_Screen_Palette(low, high);
			}
		}
		curPal = curPal->next;
	}
}

void xpal_Stop_All_Cycles(void) {
	Palette* curPal = first_palette_gbl;
	while (curPal) {
		curPal->cycle_active = 0;
		for (int i = 0; i < curPal->cycle_count; i++) {
			curPal->cycles[i].active = 0;
		}
		curPal = curPal->next;
	}
	xpal_Restore_Screen_Palette(0, 255);
}

void xpal_Set_Screen_Palette(Palette* pal) {
	xpal_Set_Video_Palette(pal, pal->start, pal->len, pal->start);
	xpal_Copy_Palette(screen_pal_gbl, pal, pal->start, pal->len, pal->start);
}

bool xpal_Compare_Screen_Palette(Palette* pal) {
	return xpal_Compare_Palette(screen_pal_gbl, pal, pal->start, pal->len, pal->start);
}

void xpal_Set_Screen_RGB(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b) {
	int16_t len = (end - start) + 1;
	xpal_Set_Palette_RGB(screen_pal_gbl, r, g, b, start, len);
	xpal_Put_Screen_Pal_Range(start, len);
}

bool xpal_Compare_Screen_RGB(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b) {
	return xpal_Compare_Palette_RGB(screen_pal_gbl, r, g, b, start, end - start + 1);
}

void xpal_Put_Screen_Palette(void) { xpal_Put_Screen_Pal_Range(0, 256); }

void xpal_Put_Screen_Pal_Range(int16_t start, int16_t len) {
	xpal_Set_Video_Palette(screen_pal_gbl, start, len, start);
}

void xpal_Set_Video_Palette(Palette* pal, int16_t start, int16_t len, int16_t dstStart) {
	RGBStruct* src = xmemhdl_Lock_Handle(pal->colors);
	RGBStruct* dst = xmemhdl_Lock_Handle(video_pal_gbl->colors);

	int16_t palStart = pal->start;
	int16_t palEnd = pal->end;
	int16_t count = 0;

	RGBStruct* palVga = NULL;
	int16_t palVgaStart = 0;

	for (int16_t srcSlot = start, dstSlot = dstStart; srcSlot < start + len; srcSlot++, dstSlot++) {
		if (srcSlot >= palStart && srcSlot <= palEnd) {
			RGBStruct s = src[srcSlot - palStart];
			if (s.r != dst[dstSlot].r || s.g != dst[dstSlot].g || s.b != dst[dstSlot].b) {
				if (!count) {
					palVga = &src[srcSlot - palStart];
					palVgaStart = dstSlot;
				}
				dst[dstSlot] = s;
				count++;
			} else {
				if (count)
					count++;
			}
		}
	}
	if (count) {
		xpal_Set_VGA_Palette(palVga, palVgaStart, count);
	}
}

void xpal_Set_Cycle_Range(int16_t start, int16_t stop, int16_t offset) {
	int16_t len = (stop - offset) + 1;
	xpal_Copy_Palette(work_pal_gbl, screen_pal_gbl, start, len, offset);
	if ((offset - start) != 0) {
		xpal_Copy_Palette(work_pal_gbl, screen_pal_gbl, start + len, offset - start, start);
	}
}

void xpal_Restore_Screen_Palette(int16_t start, int16_t stop) {
	xpal_Set_Video_Palette(screen_pal_gbl, start, (stop - start) + 1, start);
}

Palette* xpal_Get_Screen_Palette(void) { return screen_pal_gbl; }

Palette* xpal_Get_Source_Palette(void) { return src_pal_gbl; }

Palette* xpal_Get_Dest_Palette(void) { return dst_pal_gbl; }

Palette* xpal_Get_Work_Palette(void) { return work_pal_gbl; }

void xpal_Screen_To_Src_Palette(int16_t offset, int16_t start, int16_t stop) {
	xpal_Copy_Palette(src_pal_gbl, screen_pal_gbl, start, stop - start + 1, offset);
}

void xpal_Src_To_Screen_Palette(int16_t offset, int16_t start, int16_t stop) {
	xpal_Copy_Palette(screen_pal_gbl, src_pal_gbl, start, stop - start + 1, offset);
}

void xpal_Screen_To_Dest_Palette(int16_t offset, int16_t start, int16_t stop) {
	xpal_Copy_Palette(dst_pal_gbl, screen_pal_gbl, start, stop - start + 1, offset);
}

void xpal_Dest_To_Screen_Palette(int16_t offset, int16_t start, int16_t stop) {
	xpal_Copy_Palette(screen_pal_gbl, dst_pal_gbl, start, stop - start + 1, offset);
}

void xpal_Dest_To_Src_Palette(int16_t offset, int16_t start, int16_t stop) {
	xpal_Copy_Palette(src_pal_gbl, dst_pal_gbl, start, stop - start + 1, offset);
}

void xpal_Src_To_Dest_Palette(int16_t offset, int16_t start, int16_t stop) {
	xpal_Copy_Palette(dst_pal_gbl, src_pal_gbl, start, stop - start + 1, offset);
}

void xpal_Set_Source_Palette(Palette* pal) {
	xpal_Copy_Palette(src_pal_gbl, pal, pal->start, pal->len, pal->start);
}

bool xpal_Compare_Source_Palette(Palette* pal) {
	return xpal_Compare_Palette(src_pal_gbl, pal, pal->start, pal->len, pal->start);
}

void xpal_Set_Src_Pal_Color(int16_t start, int16_t stop, uint8_t r, uint8_t g, uint8_t b) {
	xpal_Set_Palette_RGB(src_pal_gbl, r, g, b, start, stop - start + 1);
}

bool xpal_Compare_Src_Pal_Color(int16_t start, int16_t stop, uint8_t r, uint8_t g, uint8_t b) {
	return xpal_Compare_Palette_RGB(src_pal_gbl, r, g, b, start, stop - start + 1);
}

void xpal_Set_Dest_Palette(Palette* pal) {
	xpal_Copy_Palette(dst_pal_gbl, pal, pal->start, pal->len, pal->start);
}

bool xpal_Compare_Dest_Palette(Palette* pal) {
	return xpal_Compare_Palette(dst_pal_gbl, pal, pal->start, pal->len, pal->start);
}

void xpal_Set_Dest_Pal_Color(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b) {
	xpal_Set_Palette_RGB(dst_pal_gbl, r, g, b, start, end - start + 1);
}

bool xpal_Compare_Dest_Pal_Color(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b) {
	return xpal_Compare_Palette_RGB(dst_pal_gbl, r, g, b, start, end - start + 1);
}

bool xpal_Compare_Src_Dest_Palette(void) {
	return xpal_Compare_Palette(src_pal_gbl, dst_pal_gbl, src_pal_gbl->start, src_pal_gbl->len,
								src_pal_gbl->start);
}

void xpal_Build_Fade_Palette(int16_t type, int16_t start, int16_t stop, int16_t step, int16_t fr, int16_t fg,
							 int16_t fb) {
	switch (type) {
		case 0:
			xpal_Build_Palette_To_Palette(start, stop, step);
			break;
		case 1:
			xpal_Build_Mono_To_Palette(start, stop, step, fr, fg, fb);
			break;
		case 2:
			xpal_Build_Palette_To_Mono(start, stop, step, fr, fg, fb);
			break;
		default:
			break;
	}
}

void xpal_Build_Palette_To_Palette(int16_t start, int16_t stop, int16_t step) {
	RGBStruct* src1 = xmemhdl_Lock_Handle(src_pal_gbl->colors);
	RGBStruct* src2 = xmemhdl_Lock_Handle(dst_pal_gbl->colors);
	RGBStruct* dst = xmemhdl_Lock_Handle(screen_pal_gbl->colors);

	for (int32_t slot = start; slot <= stop; slot++) {
		int32_t dr = (int32_t)src2[slot].r - (int32_t)src1[slot].r;
		int32_t dg = (int32_t)src2[slot].g - (int32_t)src1[slot].g;
		int32_t db = (int32_t)src2[slot].b - (int32_t)src1[slot].b;

		dst[slot].r = src1[slot].r + (dr * step) / 256;
		dst[slot].g = src1[slot].g + (dg * step) / 256;
		dst[slot].b = src1[slot].b + (db * step) / 256;
	}
}

void xpal_Build_Palette_To_Mono(int16_t start, int16_t stop, int16_t step, int16_t fr, int16_t fg,
								int16_t fb) {
	RGBStruct* src = xmemhdl_Lock_Handle(src_pal_gbl->colors);
	RGBStruct* dst = xmemhdl_Lock_Handle(screen_pal_gbl->colors);

	for (int32_t slot = start; slot <= stop; slot++) {
		int32_t dr = (int32_t)fr - (int32_t)src[slot].r;
		int32_t dg = (int32_t)fg - (int32_t)src[slot].g;
		int32_t db = (int32_t)fb - (int32_t)src[slot].b;

		dst[slot].r = src[slot].r + (dr * step) / 256;
		dst[slot].g = src[slot].g + (dg * step) / 256;
		dst[slot].b = src[slot].b + (db * step) / 256;
	}
}

void xpal_Build_Mono_To_Palette(int16_t start, int16_t stop, int16_t step, int16_t fr, int16_t fg,
								int16_t fb) {
	RGBStruct* src = xmemhdl_Lock_Handle(dst_pal_gbl->colors);
	RGBStruct* dst = xmemhdl_Lock_Handle(screen_pal_gbl->colors);

	for (int32_t slot = start; slot <= stop; slot++) {
		int32_t dr = (int32_t)src[slot].r - (int32_t)fr;
		int32_t dg = (int32_t)src[slot].g - (int32_t)fg;
		int32_t db = (int32_t)src[slot].b - (int32_t)fb;

		dst[slot].r = fr + (dr * step) / 256;
		dst[slot].g = fg + (dg * step) / 256;
		dst[slot].b = fb + (db * step) / 256;
	}
}

#ifdef LANDRU_GAME_XW
void xpal_ResetScenePointers(void) {
	first_palette_gbl = NULL;
	work_pal_gbl = NULL;
	screen_pal_gbl = NULL;
	src_pal_gbl = NULL;
	dst_pal_gbl = NULL;
	video_pal_gbl = NULL;
}
#endif
