#include <stdint.h>

#include <landru/config.h>
void* xmemptr_Create_System_Pointer(uint16_t size);
void* xmemptr_Alloc_System_Pointer(uint16_t size);
void* xmemptr_Alloc_Clear_System_Pointer(uint16_t size);
void xmemptr_Free_System_Pointer(void* ptr);
char* xmemptr_Duplicate_String(const char* value);
