#ifndef LANDRU_PAL_H
#define LANDRU_PAL_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <landru/res.h>
#include <stdbool.h>
#include <stdint.h>

#define LANDRU_VGA_RETRACE_PERIOD_US 14286u

typedef struct {
	int16_t dir;
	int16_t rate;
	uint8_t active;
	uint8_t current;
	uint8_t low;
	uint8_t high;
} CycleStruct;

typedef struct {
	uint8_t r, g, b;
} RGBStruct;

typedef struct Palette Palette;

struct Palette {
	uint32_t res_type;
	uint8_t res_name[8];
	Palette* next;
	LandruHandle colors;
	CycleStruct cycles[4];
	uint8_t cycle_count;
	uint8_t cycle_active;
	int16_t start;
	int16_t end;
	int16_t len;

#ifndef LANDRU_GAME_XW
	uint16_t flags;
#endif
	void* varptr;
	LandruHandle varhdl;
};

void xpal_Set_VGA_Palette(RGBStruct* pal, int16_t start, int16_t len);
/* Delay until all changed VGA palette writes have crossed their original
 * retrace waits. UINT64_MAX means that no wait is pending. */
uint64_t xpal_Next_VGA_Delay_Us(void);
void xpal_Create_Palette_Module(void);
void xpal_Destroy_Palette_Module(void);
Palette* xpal_Ask_Palette_List(void);
void xpal_Add_Palette_To_System(Palette* pal);
void xpal_Free_Palette_From_System(Palette* pal);
Palette* xpal_Alloc_Palette(int16_t start, int16_t numColors);
void xpal_Free_Palette(Palette* pal);
void xpal_Free_Palettes(Palette* pal);
LandruHandle xpal_Alloc_RGB(int16_t numColors);
void xpal_Free_RGB(LandruHandle rgb);
#ifdef LANDRU_GAME_XW
Palette* xpal_Res_Palette(ResFile* rf, const char* palName);
#else
Palette* xpal_Res_Palette(const char* palName);
#endif
void xpal_Set_Palette_Name(Palette* pal, uint32_t res_type, const char* pal_name);
void xpal_Clear_Palette(Palette* pal);
void xpal_Copy_Palette(Palette* dstPal, Palette* srcPal, int16_t start, int16_t length, int16_t offset);
void xpal_Set_Palette_RGB(Palette* pal, uint8_t r, uint8_t g, uint8_t b, int16_t start, int16_t length);
bool xpal_Compare_Palette(Palette* dstPal, Palette* srcPal, int16_t start, int16_t length, int16_t offset);
bool xpal_Compare_Palette_RGB(Palette* pal, uint8_t r, uint8_t g, uint8_t b, int16_t start, int16_t count);
bool xpal_Get_Palette_Index_RGB(Palette* pal, int16_t* r, int16_t* g, int16_t* b, int16_t color_idx);
void xpal_Set_Palette_Index_RGB(Palette* pal, uint8_t r, uint8_t g, uint8_t b, int16_t color_idx);
void xpal_Start_Cycle(Palette* pal);
void xpal_Stop_Cycle(Palette* pal);
void xpal_Cycle_Screen(void);
void xpal_Cycles_To_Start(void);
void xpal_Stop_All_Cycles(void);
void xpal_Set_Screen_Palette(Palette* pal);
bool xpal_Compare_Screen_Palette(Palette* pal);
void xpal_Set_Screen_RGB(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b);
bool xpal_Compare_Screen_RGB(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b);
void xpal_Put_Screen_Palette(void);
void xpal_Put_Screen_Pal_Range(int16_t start, int16_t len);
void xpal_Set_Video_Palette(Palette* pal, int16_t start, int16_t len, int16_t dstStart);
void xpal_Set_Cycle_Range(int16_t start, int16_t stop, int16_t offset);
void xpal_Restore_Screen_Palette(int16_t start, int16_t stop);
Palette* xpal_Get_Screen_Palette(void);
Palette* xpal_Get_Source_Palette(void);
Palette* xpal_Get_Dest_Palette(void);
Palette* xpal_Get_Work_Palette(void);
void xpal_Screen_To_Src_Palette(int16_t offset, int16_t start, int16_t stop);
void xpal_Src_To_Screen_Palette(int16_t offset, int16_t start, int16_t stop);
void xpal_Screen_To_Dest_Palette(int16_t offset, int16_t start, int16_t stop);
void xpal_Dest_To_Screen_Palette(int16_t offset, int16_t start, int16_t stop);
void xpal_Dest_To_Src_Palette(int16_t offset, int16_t start, int16_t stop);
void xpal_Src_To_Dest_Palette(int16_t offset, int16_t start, int16_t stop);
void xpal_Set_Source_Palette(Palette* pal);
bool xpal_Compare_Source_Palette(Palette* pal);
void xpal_Set_Src_Pal_Color(int16_t start, int16_t stop, uint8_t r, uint8_t g, uint8_t b);
bool xpal_Compare_Src_Pal_Color(int16_t start, int16_t stop, uint8_t r, uint8_t g, uint8_t b);
void xpal_Set_Dest_Palette(Palette* pal);
bool xpal_Compare_Dest_Palette(Palette* pal);
void xpal_Set_Dest_Pal_Color(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b);
bool xpal_Compare_Dest_Pal_Color(int16_t start, int16_t end, uint8_t r, uint8_t g, uint8_t b);
bool xpal_Compare_Src_Dest_Palette(void);
void xpal_Build_Fade_Palette(int16_t type, int16_t start, int16_t stop, int16_t step, int16_t fr, int16_t fg,
							 int16_t fb);
void xpal_Build_Palette_To_Palette(int16_t start, int16_t stop, int16_t step);
void xpal_Build_Palette_To_Mono(int16_t start, int16_t stop, int16_t step, int16_t fr, int16_t fg,
								int16_t fb);
void xpal_Build_Mono_To_Palette(int16_t start, int16_t stop, int16_t step, int16_t fr, int16_t fg,
								int16_t fb);

#ifdef LANDRU_GAME_XW
void xpal_ResetScenePointers(void);
#endif

#endif
