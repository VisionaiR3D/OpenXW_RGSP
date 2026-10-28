#include <stdlib.h>
#include <string.h>

#include <landru/memptr.h>

void* xmemptr_Create_System_Pointer(uint16_t size) { return malloc(size); }

void* xmemptr_Alloc_System_Pointer(uint16_t size) {
	void* memptr = xmemptr_Create_System_Pointer(size);
	if (memptr) {
		memset(memptr, 0xa3, size);
	}
	return memptr;
}

void* xmemptr_Alloc_Clear_System_Pointer(uint16_t size) {
	void* memptr = xmemptr_Create_System_Pointer(size);
	if (memptr) {
		memset(memptr, 0, size);
	}
	return memptr;
}

void xmemptr_Free_System_Pointer(void* ptr) { free(ptr); }

char* xmemptr_Duplicate_String(const char* value) {
	if (!value)
		return NULL;

	size_t length = strlen(value) + 1;
	char* copy = malloc(length);
	if (copy)
		memcpy(copy, value, length);
	return copy;
}
