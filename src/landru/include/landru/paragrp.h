#ifndef LANDRU_PARAGRP_H
#define LANDRU_PARAGRP_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

#include <landru/rect.h>
#include <landru/res.h>

LandruHandle xparagrp_Res_Paragraph(ResFile* rf, const char* res_name);
void xparagrp_Free_Paragraph(LandruHandle paragraph);
void xparagrp_Get_Paragraph_String(LandruHandle text, char* dst, int16_t para_idx, int16_t str_idx);
int16_t xparagrp_Draw_Paragraph(LandruHandle text, Rect* clip, int16_t x, int16_t y, int16_t font_id,
								int16_t color, int16_t para_idx);
int16_t xparagrp_Count_Paragraphs(LandruHandle text);
int16_t xparagrp_Count_Paragraph_Strings(LandruHandle text, int16_t para_idx);
int16_t xparagrp_Get_Paragraph_Size(LandruHandle text, int16_t font_id, int16_t para_idx,
									int16_t* out_max_width, int16_t* out_total_height);
char* xparagrp_Find_Paragraph(char* buf, int16_t para_idx);
char* xparagrp_Find_Paragraph_String(char* buf, int16_t para_idx, int16_t str_idx);

#endif
