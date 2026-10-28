#ifndef LANDRU_DIRTY_H
#define LANDRU_DIRTY_H

#include <landru/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <landru/rect.h>

void xdirty_Create_Dirty_List_Module(int16_t max_rects);
void xdirty_Destroy_Dirty_List_Module(void);
void xdirty_Set_Dirty_Disable(void);
void xdirty_Clear_Dirty_Disable(void);
int16_t xdirty_Is_Dirty_Disable(void);
void xdirty_Set_Dirty_Merge(void);
void xdirty_Clear_Dirty_Merge(void);
int16_t xdirty_Is_Dirty_Merge(void);
void xdirty_Clear_Master_Dirty_List(void);
void xdirty_Dirty_Master_Rect(Rect* r);
void xdirty_Max_Dirty_List(void);
int xdirty_Dirty_Rect(Rect* r);
void xdirty_Swap_Dirty_List(int16_t save_working);
void xdirty_Prepare_Dirty_List(void);
int xdirty_Next_Dirty_Rect(Rect* dst);
int xdirty_Next_Dirty_Rect_Or_Restore(Rect* dst);
void xdirty_Prepare_Old_Dirty_List(void);
int xdirty_Next_Old_Dirty_Rect(Rect* dst);

#endif
