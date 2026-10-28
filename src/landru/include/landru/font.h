#ifndef LANDRU_FONT_H
#define LANDRU_FONT_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

#include <landru/rect.h>
#include <landru/res.h>

enum { FONT_MAX_HEIGHT_XW = 44 };

typedef struct FontStruct {
#ifdef LANDRU_GAME_XW
	struct FontStruct* next;
#else
	uint32_t res_type;
#endif
	uint16_t firstChar;
	uint16_t numChars;
	int16_t spaceBetween;
	int16_t linesBetween;
	int16_t width;
	int16_t height;
	int16_t baseline;
	int16_t charSize;
	int16_t boldColor;
	int16_t shadowColor;
	int16_t shadow;
	int16_t isColor;
	LandruHandle widthArray;
	LandruHandle data;
} FontStruct;

extern uint8_t* s_draw_buff;

void xfont_Create_Font_Module(void);
void xfont_Destroy_Font_Module(void);
void xfont_Free_Font(FontStruct* font);
void xfont_Init_Font(FontStruct* font);
#ifdef LANDRU_GAME_XW
FontStruct* xfont_Alloc_Font(void);
void xfont_Add_Font_To_System(FontStruct* font);
void xfont_Free_Font_List(FontStruct* head);
void xfont_ResetSceneList(void);
FontStruct* xfont_Resolve_FontID(uint16_t fontId);
FontStruct* xfont_Res_Font(ResFile* resFile, const char* name);
#else
FontStruct* xfont_Alloc_Font(uint16_t fontId);
bool xfont_Add_Font_To_System(uint16_t fontId, FontStruct* font);
void xfont_Free_Font_From_System(uint16_t fontId);
bool xfont_Res_Font(const char* name, uint16_t fontId);
#endif
#ifdef LANDRU_GAME_XW
int16_t xfont_Get_FontID_Height(uint16_t fontId);
#else
uint16_t xfont_Get_FontID_Height(uint16_t fontId);
#endif
void xfont_Set_FontID_Spacing(uint16_t fontId, int16_t hspc, int16_t vspc);
void xfont_Get_FontID_Spacing(uint16_t fontId, int16_t* spacingX, int16_t* spacingY);
void xfont_Enable_FontID_Shadow(uint16_t fontId);
void xfont_Disable_FontID_Shadow(uint16_t fontId);
int16_t xfont_Is_FontID_Shadow(uint16_t fontId);
uint16_t xfont_Set_FontID_Shadow_Color(uint16_t fontId, uint16_t color);
uint16_t xfont_Get_FontID_Shadow_Color(uint16_t fontId);
#ifdef LANDRU_GAME_XW
void xfont_Set_FontID_Bold_Color(uint16_t fontId, uint16_t color);
#else
uint16_t xfont_Set_FontID_Bold_Color(uint16_t fontId, uint16_t color);
#endif
uint16_t xfont_Get_FontID_Bold_Color(uint16_t fontId);
#ifdef LANDRU_GAME_XW
int16_t xfont_Get_String_Width(const FontStruct* font, const char* text);
int16_t xfont_Get_String_Width_0(uint16_t fontId, const char* text);
void xfont_Allign_String_In_Rect(uint16_t fontId, const Rect* frame, const char* text, int16_t* x,
								 int16_t* y);
void xfont_Draw_Text(const char* text, int16_t x, int16_t y, uint16_t fontId, uint8_t color);
void xfont_Draw_Font_Text(const FontStruct* font, const char* text, int16_t x, int16_t y, uint8_t color,
						  const uint8_t* widths, const uint8_t* data);
int16_t xfont_Print_Centered_Text(const char* text, const Rect* rect, uint16_t fontId, int16_t color);
int16_t xfont_Print_Clipped_Text(const char* text, int16_t x, int16_t y, uint16_t fontId, int16_t color);
int16_t xfont_Draw_Clipped_Text(const char* text, int16_t x, int16_t y, int16_t width, const FontStruct* font,
								uint16_t fontId, int16_t color);
#else
uint16_t xfont_Set_Font(uint16_t fontId);
uint16_t xfont_Get_Font(void);
void xfont_Set_Font_Color(uint16_t color);
uint16_t xfont_Get_Font_Color(void);
void xfont_Set_Font_Pos(int16_t x, int16_t y);
void xfont_Get_Font_Pos(int16_t* out_x, int16_t* out_y);
void xfont_Move_Font_Pos(int16_t dx, int16_t dy);
void xfont_Return_Font_Pos(int16_t line_height);
void xfont_Home_Font_Pos(void);
int16_t xfont_Get_String_Width(const char* text);
void xfont_Get_String_Rect(const char* text, Rect* outRect);
void xfont_Allign_String_In_Rect(const char* text, Rect* parentRect, uint8_t xAlign, uint8_t yAlign);
void xfont_Draw_Text(const char* text);
void xfont_Draw_Font_Text(const char* text, uint8_t* widthArray, uint8_t* fontData);
void xfont_Print_Centered_Text(const char* str, Rect* r, int16_t color, int16_t font_id);
void xfont_Print_Clipped_Text(const char* str, int16_t x, int16_t y, int16_t font_id, int16_t color);
int16_t xfont_Draw_Clipped_Text(const char* str);
void xfont_Draw_Alligned_Text(const char* str, Rect* r, uint8_t halign, uint8_t valign);

#endif

typedef void (*textDrawFunc)(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor);

#endif
