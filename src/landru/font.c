#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <landru/bitmap.h>
#include <landru/canvas.h>
#include <landru/font.h>
#include <landru/fourcc.h>
#include <landru/memptr.h>
#include <landru/rect.h>
#include <landru/render.h>
#include <landru/res.h>

#include "canvas_internal.h"
#include "render_internal.h"

#ifdef LANDRU_GAME_XW
static FontStruct* s_font_list = NULL;
#else
static FontStruct* s_cur_font = NULL;
static FontStruct* s_fonts[6];

static uint16_t s_cur_font_id = 0;
static uint16_t s_font_clr = 10;
static int16_t s_font_x = 0;
static int16_t s_font_y = 0;
static int16_t s_font_left = 0;
static int16_t s_font_top = 0;
#endif
static bool s_font_module = false;

#ifdef LANDRU_GAME_XW
void xfont_ResetSceneList(void) { s_font_list = NULL; }

void xfont_Create_Font_Module(void) {
	xfont_ResetSceneList();
	s_font_module = true;
}

void xfont_Destroy_Font_Module(void) { xfont_Free_Font_List(s_font_list); }

void xfont_Add_Font_To_System(FontStruct* font) {
	FontStruct** head = &s_font_list;
	for (FontStruct* cur = *head; cur; cur = cur->next)
		head = &cur->next;
	*head = font;
	font->next = NULL;
}

FontStruct* xfont_Alloc_Font(void) {
	FontStruct* font = xmemptr_Alloc_System_Pointer(sizeof(FontStruct));
	if (font) {
		memset(font, 0, sizeof(*font));
		xfont_Init_Font(font);
		xfont_Add_Font_To_System(font);
	}
	return font;
}

void xfont_Free_Font_List(FontStruct* head) {
	while (head) {
		FontStruct* next = head->next;
		xfont_Free_Font(head);
		head = next;
	}
}

FontStruct* xfont_Resolve_FontID(uint16_t fontId) {
	FontStruct* font = s_font_list;
	for (uint16_t index = 0; index < fontId && font && font->next; index++)
		font = font->next;
	return font;
}
#else
void xfont_Create_Font_Module(void) {
	for (int i = 0; i < 6; i++) {
		s_fonts[i] = NULL;
	}
	s_cur_font = NULL;
	s_cur_font_id = 0xffff;
	s_font_clr = 0xf;
	s_font_x = 0;
	s_font_y = 0;
	s_font_module = true;
}

void xfont_Destroy_Font_Module(void) {
	if (!s_font_module)
		return;

	for (int i = 0; i < 6; i++) {
		if (s_fonts[i]) {
			xfont_Free_Font_From_System(i);
		}
	}
}

FontStruct* xfont_Alloc_Font(uint16_t fontId) {
	FontStruct* font = xmemptr_Alloc_System_Pointer(sizeof(FontStruct));
	if (font) {
		xfont_Init_Font(font);
		xfont_Add_Font_To_System(fontId, font);
	}
	return font;
}

#endif

static FontStruct* resolve_font(uint16_t id) {
#ifdef LANDRU_GAME_XW
	return xfont_Resolve_FontID(id);
#else
	return s_fonts[id];
#endif
}

void xfont_Free_Font(FontStruct* font) {
	if (!font)
		return;

	if (font->widthArray)
		xmemhdl_Free_Handle(font->widthArray);

	if (font->data)
		xmemhdl_Free_Handle(font->data);

	free(font);
}

void xfont_Init_Font(FontStruct* font) {
	assert(font != NULL);

	font->firstChar = 0x20;
	font->numChars = 0;
	font->spaceBetween = 1;
	font->linesBetween = 1;
	font->width = 0;
	font->height = 0;
	font->baseline = 0;
	font->charSize = 0;
	font->boldColor = 2;
	font->shadowColor = 16;
	font->shadow = 0;
	font->widthArray = LANDRU_NULL_HANDLE;
	font->data = LANDRU_NULL_HANDLE;
#ifdef LANDRU_GAME_XW
	font->next = NULL;
#else
	font->res_type = 0;
#endif
}

#ifndef LANDRU_GAME_XW
bool xfont_Add_Font_To_System(uint16_t fontId, FontStruct* font) {
	assert(fontId >= 0 && fontId < 6);

	if (s_fonts[fontId]) {
		xfont_Free_Font_From_System(fontId);
	}

	s_fonts[fontId] = font;
	return true;
}

void xfont_Free_Font_From_System(uint16_t fontId) {
	if (s_fonts[fontId]) {
		xfont_Free_Font(s_fonts[fontId]);
		s_fonts[fontId] = NULL;
	}
}

#endif

#ifdef LANDRU_GAME_XW
FontStruct* xfont_Res_Font(ResFile* resFile, const char* fontName) {
	int offset;
	uint32_t size;
	if (!xres_Get_Resource_Offset(resFile, FOURCC_FONT, fontName, &offset, &size) ||
		!xres_Open_Resource_Data(resFile, offset))
		return NULL;
	FontStruct* font = xfont_Alloc_Font();
#else
bool xfont_Res_Font(const char* fontName, uint16_t fontId) {
	ResFile* resFile = xres_Open_Resource_Data(FOURCC_FONT, fontName);
	if (!resFile)
		return false;
	FontStruct* font = xfont_Alloc_Font(fontId);
#endif
	if (!font) {
		xres_Close_Resource_Data(resFile);
		return 0;
	}

	font->firstChar = xres_Read_Resource_Word(resFile);
	font->numChars = xres_Read_Resource_Word(resFile);
	font->width = xres_Read_Resource_Word(resFile);
	font->height = xres_Read_Resource_Word(resFile);
	font->charSize = (font->width >> 3) * font->height;
#ifdef LANDRU_GAME_XW
	if ((uint16_t)font->height > FONT_MAX_HEIGHT_XW) {
		xres_Close_Resource_Data(resFile);
#ifdef LANDRU_MODERN
		/* The original frees this appended font without unlinking it. */
		FontStruct** head = &s_font_list;
		for (FontStruct* cur = *head; cur && cur != font; cur = cur->next)
			head = &cur->next;
		*head = font->next;
#endif
		xfont_Free_Font(font);
		return NULL;
	}
#else
	assert(font->height < 23);
#endif
	font->baseline = xres_Read_Resource_Word(resFile);
	font->isColor = xres_Read_Resource_Word(resFile);
	font->widthArray = xres_Read_Resource_Data(resFile, font->numChars, LANDRU_MEMORY_DEFAULT);
#ifndef LANDRU_GAME_XW
	if (!font->widthArray) {
		xres_Close_Resource_Data(resFile);
		xfont_Free_Font_From_System(fontId);
		return false;
	}
#endif

	font->data = xres_Read_Resource_Data(resFile, font->numChars * font->charSize, LANDRU_MEMORY_DEFAULT);
#ifndef LANDRU_GAME_XW
	if (!font->data) {
		xres_Close_Resource_Data(resFile);
		xfont_Free_Font_From_System(fontId);
		return false;
	}
#endif

	xres_Close_Resource_Data(resFile);
#ifdef LANDRU_GAME_XW
	return font;
#else
	return true;
#endif
}

#ifdef LANDRU_GAME_XW
int16_t xfont_Get_FontID_Height(uint16_t fontId) {
#else
uint16_t xfont_Get_FontID_Height(uint16_t fontId) {
#endif
	if (resolve_font(fontId)) {
		return resolve_font(fontId)->height;
	}
	return 0;
}

void xfont_Get_FontID_Spacing(uint16_t fontId, int16_t* spacingX, int16_t* spacingY) {
	if (!resolve_font(fontId)) {
		*spacingX = 0;
		*spacingY = 0;
	} else {
		*spacingX = resolve_font(fontId)->spaceBetween;
		*spacingY = resolve_font(fontId)->linesBetween;
	}
}

void xfont_Enable_FontID_Shadow(uint16_t fontId) {
	if (resolve_font(fontId)) {
		resolve_font(fontId)->shadow = 1;
	}
}

void xfont_Disable_FontID_Shadow(uint16_t fontId) {
	if (resolve_font(fontId)) {
		resolve_font(fontId)->shadow = 0;
	}
}

#ifdef LANDRU_GAME_XW
void xfont_Set_FontID_Bold_Color(uint16_t fontId, uint16_t color) { resolve_font(fontId)->boldColor = color; }
#else
uint16_t xfont_Set_FontID_Bold_Color(uint16_t fontId, uint16_t color) {
	uint16_t prevColor = 0;
	if (resolve_font(fontId)) {
		prevColor = resolve_font(fontId)->boldColor;
		resolve_font(fontId)->boldColor = color;
	}
	return prevColor;
}

#endif

uint16_t xfont_Get_FontID_Bold_Color(uint16_t fontId) {
	if (resolve_font(fontId)) {
		return resolve_font(fontId)->boldColor;
	} else {
		return 2;
	}
}

static int16_t string_width(const FontStruct* font, const char* text) {
	if (!font)
		return 0;
	uint8_t first = font->firstChar;
	uint8_t last = first + font->numChars - 1;
	int16_t width = 0;
	const uint8_t* widths = xmemhdl_Lock_Handle(font->widthArray);
	for (size_t index = 0; text[index]; index++) {
		uint8_t ch = (uint8_t)text[index];
		if (ch >= first && ch <= last) {
			width += widths[ch - first];
			if (text[index + 1])
				width += font->spaceBetween;
		}
	}
	if (font->shadow)
		width++;
	return width;
}

#ifdef LANDRU_GAME_XW
int16_t xfont_Get_String_Width(const FontStruct* font, const char* text) { return string_width(font, text); }

int16_t xfont_Get_String_Width_0(uint16_t fontId, const char* text) {
	return xfont_Get_String_Width(xfont_Resolve_FontID(fontId), text);
}

void xfont_Allign_String_In_Rect(uint16_t fontId, const Rect* frame, const char* text, int16_t* x,
								 int16_t* y) {
	int16_t width = xfont_Get_String_Width_0(fontId, text);
	int16_t height = xfont_Get_FontID_Height(fontId);
	*x = frame->left + ((frame->right - width - frame->left) >> 1);
	*y = frame->top + ((frame->bottom - height - frame->top) >> 1);
}

void xfont_Draw_Text(const char* text, int16_t x, int16_t y, uint16_t fontId, uint8_t color) {
	const FontStruct* font = xfont_Resolve_FontID(fontId);
	if (font)
		xfont_Draw_Font_Text(font, text, x, y, color, xmemhdl_Lock_Handle(font->widthArray),
							 xmemhdl_Lock_Handle(font->data));
}
#else
uint16_t xfont_Set_Font(uint16_t fontId) {
	uint16_t curFont = s_cur_font_id;
	if (fontId >= 0 && fontId < 6) {
		s_cur_font_id = fontId;
		s_cur_font = resolve_font(fontId);
	} else {
		s_cur_font_id = 0xffff;
		s_cur_font = NULL;
	}

	return curFont;
}

uint16_t xfont_Get_Font(void) { return s_cur_font_id; }

void xfont_Set_Font_Color(uint16_t color) { s_font_clr = color; }

uint16_t xfont_Get_Font_Color(void) { return s_font_clr; }

int16_t xfont_Get_String_Width(const char* text) { return string_width(s_cur_font, text); }

void xfont_Get_String_Rect(const char* text, Rect* outRect) {
	if (!s_cur_font) {
		xrect_Clear_Rect(outRect);
		return;
	}

	int16_t width = xfont_Get_String_Width(text);
	if (s_cur_font->shadow) {
		xrect_Set_Rect(outRect, 0, 0, width, s_cur_font->height + 1);
	} else {
		xrect_Set_Rect(outRect, 0, 0, width, s_cur_font->height);
	}
}

void xfont_Allign_String_In_Rect(const char* text, Rect* parentRect, uint8_t xAlign, uint8_t yAlign) {
	if (!s_cur_font)
		return;

	Rect stringRect;
	xfont_Get_String_Rect(text, &stringRect);
	xrect_Allign_Rect(&stringRect, parentRect, xAlign, yAlign);
	s_font_x = stringRect.left;
	s_font_y = stringRect.top;
}

void xfont_Draw_Text(const char* text) {
	if (!s_cur_font)
		return;

	xfont_Draw_Font_Text(text, xmemhdl_Lock_Handle(s_cur_font->widthArray),
						 xmemhdl_Lock_Handle(s_cur_font->data));
}

#endif

/* Font glyph draw callbacks — all share the same 4-param signature to match
   the function pointer table. Most don't use lineStride/shadowColor. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void xfont_Draw_Font_00(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {}

void xfont_Draw_Font_01(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[3] = color;
}

void xfont_Draw_Font_02(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[2] = color;
}

void xfont_Draw_Font_03(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[2] = color;
	dst[3] = color;
}

void xfont_Draw_Font_04(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
}

void xfont_Draw_Font_05(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
	dst[3] = color;
}

void xfont_Draw_Font_06(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
	dst[2] = color;
}

void xfont_Draw_Font_07(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
	dst[2] = color;
	dst[3] = color;
}

void xfont_Draw_Font_08(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
}

void xfont_Draw_Font_09(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[3] = color;
}

void xfont_Draw_Font_10(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[2] = color;
}

void xfont_Draw_Font_11(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[2] = color;
	dst[3] = color;
}

void xfont_Draw_Font_12(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
}

void xfont_Draw_Font_13(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
	dst[3] = color;
}

void xfont_Draw_Font_14(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
	dst[2] = color;
}

void xfont_Draw_Font_15(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
	dst[2] = color;
	dst[3] = color;
}

void xfont_Draw_Font_Shadow_00(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	(void)dst;
	(void)lineStride;
	(void)color;
	(void)shadowColor;
}

void xfont_Draw_Font_Shadow_01(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[3] = color;
	dst[4] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

void xfont_Draw_Font_Shadow_02(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[2] = color;
	dst[3] = shadowColor;
	dst[lineStride + 2] = shadowColor;
}

void xfont_Draw_Font_Shadow_03(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[2] = color;
	dst[3] = color;
	dst[4] = shadowColor;
	dst[lineStride + 2] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

void xfont_Draw_Font_Shadow_04(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
	dst[2] = shadowColor;
	dst[lineStride + 1] = shadowColor;
}

void xfont_Draw_Font_Shadow_05(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
	dst[3] = color;
	dst[2] = shadowColor;
	dst[4] = shadowColor;
	dst[lineStride + 1] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

void xfont_Draw_Font_Shadow_06(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
	dst[2] = color;
	dst[3] = shadowColor;
	dst[lineStride + 1] = shadowColor;
	dst[lineStride + 2] = shadowColor;
}

void xfont_Draw_Font_Shadow_07(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[1] = color;
	dst[2] = color;
	dst[3] = color;
	dst[4] = shadowColor;
	dst[lineStride + 1] = shadowColor;
	dst[lineStride + 2] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

void xfont_Draw_Font_Shadow_08(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = shadowColor;
	dst[lineStride] = shadowColor;
}

void xfont_Draw_Font_Shadow_09(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[3] = color;
	dst[1] = shadowColor;
	dst[4] = shadowColor;
	dst[lineStride] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

void xfont_Draw_Font_Shadow_10(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[2] = color;
	dst[1] = shadowColor;
	dst[3] = shadowColor;
	dst[lineStride] = shadowColor;
	dst[lineStride + 2] = shadowColor;
}

void xfont_Draw_Font_Shadow_11(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[2] = color;
	dst[3] = color;
	dst[1] = shadowColor;
	dst[4] = shadowColor;
	dst[lineStride] = shadowColor;
	dst[lineStride + 2] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

void xfont_Draw_Font_Shadow_12(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
	dst[2] = shadowColor;
	dst[lineStride] = shadowColor;
	dst[lineStride + 1] = shadowColor;
}

void xfont_Draw_Font_Shadow_13(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
	dst[3] = color;
	dst[2] = shadowColor;
	dst[4] = shadowColor;
	dst[lineStride] = shadowColor;
	dst[lineStride + 1] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

void xfont_Draw_Font_Shadow_14(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
	dst[2] = color;
	dst[3] = shadowColor;
	dst[lineStride] = shadowColor;
	dst[lineStride + 1] = shadowColor;
	dst[lineStride + 2] = shadowColor;
}

void xfont_Draw_Font_Shadow_15(uint8_t* dst, uint16_t lineStride, uint8_t color, uint8_t shadowColor) {
	dst[0] = color;
	dst[1] = color;
	dst[2] = color;
	dst[3] = color;
	dst[4] = shadowColor;
	dst[lineStride] = shadowColor;
	dst[lineStride + 1] = shadowColor;
	dst[lineStride + 2] = shadowColor;
	dst[lineStride + 3] = shadowColor;
}

static textDrawFunc textDrawFuncs[32] = {
	xfont_Draw_Font_00,        xfont_Draw_Font_01,        xfont_Draw_Font_02,
	xfont_Draw_Font_03,        xfont_Draw_Font_04,        xfont_Draw_Font_05,
	xfont_Draw_Font_06,        xfont_Draw_Font_07,        xfont_Draw_Font_08,
	xfont_Draw_Font_09,        xfont_Draw_Font_10,        xfont_Draw_Font_11,
	xfont_Draw_Font_12,        xfont_Draw_Font_13,        xfont_Draw_Font_14,
	xfont_Draw_Font_15,        xfont_Draw_Font_Shadow_00, xfont_Draw_Font_Shadow_01,
	xfont_Draw_Font_Shadow_02, xfont_Draw_Font_Shadow_03, xfont_Draw_Font_Shadow_04,
	xfont_Draw_Font_Shadow_05, xfont_Draw_Font_Shadow_06, xfont_Draw_Font_Shadow_07,
	xfont_Draw_Font_Shadow_08, xfont_Draw_Font_Shadow_09, xfont_Draw_Font_Shadow_10,
	xfont_Draw_Font_Shadow_11, xfont_Draw_Font_Shadow_12, xfont_Draw_Font_Shadow_13,
	xfont_Draw_Font_Shadow_14, xfont_Draw_Font_Shadow_15
};

#pragma GCC diagnostic pop

static int16_t draw_font_text(const FontStruct* font, const char* text, int16_t x, int16_t y, uint8_t color,
							  const uint8_t* widths, const uint8_t* data) {
	uint8_t current_color = color;
	int16_t advance = 0;
	int shadow_offset = font->shadow ? 16 : 0;
	for (size_t index = 0; text[index]; index++) {
		uint8_t character = (uint8_t)text[index];
		if (character == 1) {
			current_color = color;
		} else if (character == 2) {
			current_color = font->boldColor;
		} else {
			int glyph = character - font->firstChar;
			if (glyph < 0 || glyph >= font->numChars)
				continue;
			int row_bytes = (widths[glyph] + 7) >> 3;
			for (int row = 0; row < font->height; row++) {
				for (int column = 0; column < row_bytes; column++) {
					uint8_t bits = data[glyph * font->charSize + row * (font->width >> 3) + column];
					uint8_t* dst =
						(uint8_t*)draw_buff_gbl + (y + row) * draw_w_gbl + x + advance + column * 8;
					textDrawFuncs[(bits >> 4) + shadow_offset](dst, draw_w_gbl, current_color,
															   font->shadowColor);
					textDrawFuncs[(bits & 15) + shadow_offset](dst + 4, draw_w_gbl, current_color,
															   font->shadowColor);
				}
			}
			advance += widths[glyph] + font->spaceBetween;
		}
	}
	return advance;
}

#ifdef LANDRU_GAME_XW
void xfont_Draw_Font_Text(const FontStruct* font, const char* text, int16_t x, int16_t y, uint8_t color,
						  const uint8_t* widths, const uint8_t* data) {
	draw_font_text(font, text, x, y, color, widths, data);
}
#else
void xfont_Draw_Font_Text(const char* text, uint8_t* widths, uint8_t* data) {
	s_font_x += draw_font_text(s_cur_font, text, s_font_x, s_font_y, s_font_clr, widths, data);
}
#endif

void xfont_Set_FontID_Spacing(uint16_t fontId, int16_t hspc, int16_t vspc) {
	if (resolve_font(fontId)) {
		resolve_font(fontId)->spaceBetween = hspc;
		resolve_font(fontId)->linesBetween = vspc;
	}
}

uint16_t xfont_Get_FontID_Shadow_Color(uint16_t fontId) {
	if (resolve_font(fontId))
		return resolve_font(fontId)->shadowColor;
	return 16;
}

uint16_t xfont_Set_FontID_Shadow_Color(uint16_t fontId, uint16_t color) {
	uint16_t prev = 16;
	if (resolve_font(fontId)) {
		prev = resolve_font(fontId)->shadowColor;
		resolve_font(fontId)->shadowColor = color;
	}
	return prev;
}

int16_t xfont_Is_FontID_Shadow(uint16_t fontId) {
	if (resolve_font(fontId))
		return resolve_font(fontId)->shadow;
	return 0;
}

#ifdef LANDRU_GAME_XW
int16_t xfont_Print_Centered_Text(const char* text, const Rect* rect, uint16_t fontId, int16_t color) {
	int16_t x, y;
	xfont_Allign_String_In_Rect(fontId, rect, text, &x, &y);
	const FontStruct* font = xfont_Resolve_FontID(fontId);
	return xfont_Draw_Clipped_Text(text, x, y, xfont_Get_String_Width(font, text), font, fontId, color);
}

int16_t xfont_Print_Clipped_Text(const char* text, int16_t x, int16_t y, uint16_t fontId, int16_t color) {
	const FontStruct* font = xfont_Resolve_FontID(fontId);
	return xfont_Draw_Clipped_Text(text, x, y, xfont_Get_String_Width(font, text), font, fontId, color);
}
#else
void xfont_Set_Font_Pos(int16_t x, int16_t y) {
	s_font_x = x;
	s_font_y = y;
	s_font_left = x;
	s_font_top = y;
}

void xfont_Get_Font_Pos(int16_t* out_x, int16_t* out_y) {
	*out_x = s_font_x;
	*out_y = s_font_y;
}

void xfont_Move_Font_Pos(int16_t dx, int16_t dy) {
	s_font_x += dx;
	s_font_y += dy;
}

void xfont_Return_Font_Pos(int16_t line_height) {
	s_font_x = s_font_left;
	s_font_y += line_height;
}

void xfont_Home_Font_Pos(void) {
	s_font_x = s_font_left;
	s_font_y = s_font_top;
}

void xfont_Print_Centered_Text(const char* str, Rect* r, int16_t color, int16_t font_id) {
	xfont_Set_Font(font_id);
	xfont_Set_Font_Color(color);
	xfont_Allign_String_In_Rect(str, r, 1, 1);
	xfont_Draw_Clipped_Text(str);
}

void xfont_Print_Clipped_Text(const char* str, int16_t x, int16_t y, int16_t font_id, int16_t color) {
	xfont_Set_Font(font_id);
	xfont_Set_Font_Color(color);
	s_font_x = x;
	s_font_y = y;
	xfont_Draw_Clipped_Text(str);
}

#endif

static int16_t draw_clipped_text(const char* str, int16_t x, int16_t y, int16_t width, const FontStruct* font,
								 uint16_t font_id, uint8_t color, int16_t* advance) {
	if (!font) {
		*advance = 0;
		return 0;
	}
	/* All public text helpers converge here. Preserve the draw-time color,
	 * toggle, shadow, target and clipping state in one publication path. */
	/* Suppress emits when the active drawing target is an off-screen
	 * scratch. The text-specific gate also permits callers that publish
	 * persistent text separately to avoid duplicate records. */
	if (str && font && xcanvas_Render_Text_Emit_Allowed()) {
		LandruTextCommand command = { 0 };
		size_t len = strlen(str);
		if (len >= sizeof(command.text))
			len = sizeof(command.text) - 1;
		memcpy(command.text, str, len);
		command.text[len] = '\0';
		command.x = x;
		command.y = y;
		command.color_index = (uint8_t)color;
		command.bold_color_index = (uint8_t)font->boldColor;
		command.shadow_color_index = (uint8_t)font->shadowColor;
		command.shadow = font->shadow ? 1 : 0;
		command.font_id = (uint8_t)font_id;
		command.target = xcanvas_Render_Emit_Target();
		Rect clip;
		xcanvas_Get_Raster_Clip(&clip);
		command.clip_left = clip.left;
		command.clip_top = clip.top;
		command.clip_right = clip.right;
		command.clip_bottom = clip.bottom;
		landru_render_text(&command);
	}

	Rect a, b;
	xrect_Set_Rect(&a, 0, 0, width, font->height + (font->shadow != 0));
	xrect_Offset_Rect(&a, x, y);
	b = a;

	if (!xcanvas_Clip_Rect_To_Raster(&b)) {
		*advance = width;
		return 0;
	}

	if (xrect_Equal_Rect(&a, &b)) {
		*advance = draw_font_text(font, str, x, y, color, xmemhdl_Lock_Handle(font->widthArray),
								  xmemhdl_Lock_Handle(font->data));
		return 1;
	}

	/* Partially clipped — render to temp bitmap, blit clipped result */
	int16_t save_x = x;
	int16_t save_y = y;
	x = 0;
	y = 0;

	int16_t w = a.right - a.left;
	int16_t h = a.bottom - a.top;
	if (w & 0xF)
		w = (w | 0xF) + 1;

	*advance = 0;
	CanvasBinding saved_canvas;
	xcanvas_Save_Binding(&saved_canvas);
	BitmapStruct bm;
	xbitmap_Init_Bitmap(&bm);
	bm.w = w;
	bm.h = h;
	bm.data = calloc(w * h, 1);
	bm.flags = 1;

	if (bm.data) {
		xbitmap_Max_Bitmap_Clipping(&bm);
		xcanvas_Set_Drawing_Canvas_Bitmap(&bm);
		xcanvas_Erase_Canvas();
		*advance = draw_font_text(font, str, x, y, color, xmemhdl_Lock_Handle(font->widthArray),
								  xmemhdl_Lock_Handle(font->data));
		xcanvas_Restore_Binding(&saved_canvas);
		xcanvas_Copy_Bitmap_To_Canvas(&bm, save_x, save_y);
		free(bm.data);
	}

	return 1;
}

#ifdef LANDRU_GAME_XW
int16_t xfont_Draw_Clipped_Text(const char* text, int16_t x, int16_t y, int16_t width, const FontStruct* font,
								uint16_t fontId, int16_t color) {
	int16_t advance;
	return draw_clipped_text(text, x, y, width, font, fontId, color, &advance);
}
#else
int16_t xfont_Draw_Clipped_Text(const char* text) {
	int16_t advance;
	int16_t result = draw_clipped_text(text, s_font_x, s_font_y, string_width(s_cur_font, text), s_cur_font,
									   s_cur_font_id, s_font_clr, &advance);
	s_font_x += advance;
	return result;
}

void xfont_Draw_Alligned_Text(const char* str, Rect* r, uint8_t halign, uint8_t valign) {
	xfont_Allign_String_In_Rect(str, r, halign, valign);
	xfont_Draw_Clipped_Text(str);
}

#endif
