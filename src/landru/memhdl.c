#include <stdlib.h>
#include <string.h>

#include <landru/memhdl.h>

static void* handle_pointers[LANDRU_HANDLE_CAPACITY] = { NULL };
static uint32_t handle_sizes[LANDRU_HANDLE_CAPACITY] = { 0 };
static uint32_t next_handle = 1;

void xmemhdl_ResetHandleTables(const void* memory) {
	(void)memory;
	memset(handle_pointers, 0, sizeof(handle_pointers));
	memset(handle_sizes, 0, sizeof(handle_sizes));
	next_handle = 1;
}

void xmem_Close_Memory(void) {
	for (uint32_t i = 1; i < LANDRU_HANDLE_CAPACITY; i++) {
		free(handle_pointers[i]);
		handle_pointers[i] = NULL;
		handle_sizes[i] = 0;
	}
	next_handle = 1;
}

LandruHandle xmemhdl_Create_Handle(uint32_t size, uint16_t flags) {
	(void)flags;
	for (uint32_t i = 0; i < LANDRU_HANDLE_CAPACITY - 1; i++) {
		uint32_t slot = 1 + (next_handle - 1 + i) % (LANDRU_HANDLE_CAPACITY - 1);
		if (!handle_pointers[slot]) {
			void* data = malloc(size);
			if (!data)
				return LANDRU_NULL_HANDLE;
			handle_pointers[slot] = data;
			handle_sizes[slot] = size;
			next_handle = slot + 1;
			if (next_handle == LANDRU_HANDLE_CAPACITY)
				next_handle = 1;
			return (LandruHandle)slot;
		}
	}
	return LANDRU_NULL_HANDLE;
}

LandruHandle xmemhdl_Alloc_Handle(uint32_t size, uint16_t flags) {
	return xmemhdl_Create_Handle(size, flags);
}

LandruHandle xmemhdl_Alloc_Clear_Handle(uint32_t size, uint16_t flags) {
	LandruHandle handle = xmemhdl_Create_Handle(size, flags);
	xmemhdl_Handle_nSet(handle, 0, size);
	return handle;
}

void* xmemhdl_Lock_Handle(LandruHandle handle) {
	if (handle == LANDRU_NULL_HANDLE || handle >= LANDRU_HANDLE_CAPACITY)
		return NULL;
	return handle_pointers[handle];
}

void xmemhdl_Unlock_Handle(LandruHandle handle) { (void)handle; }

void xmemhdl_Free_Handle(LandruHandle handle) {
	if (handle != LANDRU_NULL_HANDLE && handle < LANDRU_HANDLE_CAPACITY) {
		free(handle_pointers[handle]);
		handle_pointers[handle] = NULL;
		handle_sizes[handle] = 0;
	}
}

uint32_t xmemhdl_Get_Handle_Size(LandruHandle handle) {
	return xmemhdl_Lock_Handle(handle) ? handle_sizes[handle] : 0;
}

void xmemhdl_Handle_nSet(LandruHandle handle, uint8_t value, uint32_t size) {
	void* data = xmemhdl_Lock_Handle(handle);
	if (data && size <= xmemhdl_Get_Handle_Size(handle))
		memset(data, value, size);
}

void xmemhdl_Handle_Set(LandruHandle handle, uint8_t value) {
	xmemhdl_Handle_nSet(handle, value, xmemhdl_Get_Handle_Size(handle));
}

LandruHandle xmemhdl_Data_To_Handle(const void* data, uint32_t size, uint16_t flags) {
	if (!data)
		return LANDRU_NULL_HANDLE;
	LandruHandle handle = xmemhdl_Alloc_Handle(size, flags);
	if (handle)
		memcpy(xmemhdl_Lock_Handle(handle), data, size);
	return handle;
}

int32_t xmemhdl_Resize_Handle(LandruHandle* handle, uint32_t size, uint16_t flags) {
	if (!handle)
		return 0;
	if (size == 0) {
		xmemhdl_Free_Handle(*handle);
		*handle = LANDRU_NULL_HANDLE;
		return 1;
	}
	uint32_t old_size = xmemhdl_Get_Handle_Size(*handle);
	if (size == old_size)
		return 1;
	LandruHandle replacement = xmemhdl_Alloc_Handle(size, flags);
	if (!replacement)
		return 0;
	if (old_size) {
		uint32_t copy_size = old_size < size ? old_size : size;
		memcpy(xmemhdl_Lock_Handle(replacement), xmemhdl_Lock_Handle(*handle), copy_size);
	}
	xmemhdl_Free_Handle(*handle);
	*handle = replacement;
	return 1;
}

void xmemhdl_Handle_Copy_Portion(LandruHandle destination, LandruHandle source, uint32_t size) {
	void* dst = xmemhdl_Lock_Handle(destination);
	const void* src = xmemhdl_Lock_Handle(source);
	if (dst && src && size <= xmemhdl_Get_Handle_Size(destination) && size <= xmemhdl_Get_Handle_Size(source))
		memmove(dst, src, size);
}

void xmemhdl_Handle_Copy(LandruHandle destination, LandruHandle source) {
	uint32_t size = xmemhdl_Get_Handle_Size(source);
	uint32_t capacity = xmemhdl_Get_Handle_Size(destination);
	xmemhdl_Handle_Copy_Portion(destination, source, size < capacity ? size : capacity);
}
