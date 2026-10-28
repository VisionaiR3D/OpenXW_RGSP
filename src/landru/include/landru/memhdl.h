#ifndef LANDRU_MEMHDL_H
#define LANDRU_MEMHDL_H

#include <landru/config.h>

#include <stdint.h>

typedef uint16_t LandruHandle;

enum {
	LANDRU_NULL_HANDLE = 0,
	LANDRU_HANDLE_CAPACITY = 32768,
	LANDRU_MEMORY_DEFAULT = 0,
	LANDRU_MEMORY_RESOURCE = 0x1000
};

/* Startup only: resetting does not release live allocations. */
void xmemhdl_ResetHandleTables(const void* memory);
/* Call after all modules and audio users have released their resources. */
void xmem_Close_Memory(void);
LandruHandle xmemhdl_Create_Handle(uint32_t size, uint16_t flags);
LandruHandle xmemhdl_Alloc_Handle(uint32_t size, uint16_t flags);
LandruHandle xmemhdl_Alloc_Clear_Handle(uint32_t size, uint16_t flags);
void* xmemhdl_Lock_Handle(LandruHandle handle);
void xmemhdl_Unlock_Handle(LandruHandle handle);
void xmemhdl_Free_Handle(LandruHandle handle);
uint32_t xmemhdl_Get_Handle_Size(LandruHandle handle);
void xmemhdl_Handle_nSet(LandruHandle handle, uint8_t value, uint32_t size);
void xmemhdl_Handle_Set(LandruHandle handle, uint8_t value);
LandruHandle xmemhdl_Data_To_Handle(const void* data, uint32_t size, uint16_t flags);
int32_t xmemhdl_Resize_Handle(LandruHandle* handle, uint32_t size, uint16_t flags);
void xmemhdl_Handle_Copy(LandruHandle destination, LandruHandle source);
void xmemhdl_Handle_Copy_Portion(LandruHandle destination, LandruHandle source, uint32_t size);

#endif
