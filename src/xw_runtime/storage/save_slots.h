#ifndef XW_SAVE_SLOTS_H
#define XW_SAVE_SLOTS_H
#include <stdbool.h>
#include <stddef.h>
enum { XW_SAVE_SLOT_COUNT=5 };
bool XwSaveSlots_Save(int slot);
bool XwSaveSlots_RequestLoad(int slot);
bool XwSaveSlots_Info(int slot,char* text,size_t size);
const char* XwSaveSlots_Error(void);
bool XwSaveSlots_BootResume(void);
int XwSaveSlots_MissionScene(void);
bool XwSaveSlots_IsResuming(void);
void XwSaveSlots_FinishResume(void);
#endif
