#include <landru/file.h>

#ifdef LANDRU_GAME_XW
#include <landru/timer.h>
#endif

#include "host_internal.h"

#include <assert.h>
#include <string.h>

static int16_t file_count_gbl;

/* --- File I/O --- */

LandruFile* xfile_Open_File(LandruFileRoot root, const char* name, const char* mode) {
	assert(name && *name);
	assert(mode);

	/* Translate DOS backslash paths to Unix forward slashes */
	char path[256];
	strncpy(path, name, sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0';
	for (char* p = path; *p; p++)
		if (*p == '\\')
			*p = '/';

	/* Case-insensitive file lookup: try original, then uppercase, then lowercase */
	LandruFile* f = landru_host_file_open(root, path, mode);
	if (!f) {
		for (char* p = path; *p; p++)
			if (*p >= 'a' && *p <= 'z')
				*p -= 32;
		f = landru_host_file_open(root, path, mode);
	}
	if (!f) {
		for (char* p = path; *p; p++)
			if (*p >= 'A' && *p <= 'Z' && *p != '/')
				*p += 32;
		f = landru_host_file_open(root, path, mode);
	}

	if (f)
		file_count_gbl++;
	return f;
}

#ifdef LANDRU_GAME_XW
int32_t xfile_Close_File(LandruFile* file) {
#else
int16_t xfile_Close_File(LandruFile* file) {
#endif
	assert(file);
	if (file) {
		file_count_gbl--;
		return landru_host_file_close(file);
	}
	return 0;
}

int16_t xfile_Seek_File(LandruFile* file, int32_t offset, int16_t whence) {
	assert(file);
	return landru_host_file_seek(file, offset, whence);
}

int32_t xfile_Tell_File(LandruFile* file) {
	assert(file);
	return landru_host_file_tell(file);
}

/* --- File read --- */

int16_t xfile_Read_Byte_From_File(LandruFile* file, uint8_t* data) {
	assert(file);
	assert(data);
	return landru_host_file_read(data, 1, 1, file) == 1;
}

#ifdef LANDRU_GAME_XW
int16_t xfile_Read_Word_From_File(LandruFile* file, uint16_t* data) {
#else
int16_t xfile_Read_Word_From_File(LandruFile* file, int16_t* data) {
#endif
	assert(file);
	assert(data);
#ifdef LANDRU_GAME_XW
	size_t count = landru_host_file_read(data, 1, sizeof(*data), file);
	xtimer_Often();
	return count != sizeof(*data);
#else
	return landru_host_file_read(data, 1, 2, file) == 2;
#endif
}

#ifdef LANDRU_GAME_XW
int16_t xfile_Read_Long_From_File(LandruFile* file, uint32_t* data) {
#else
int16_t xfile_Read_Long_From_File(LandruFile* file, int32_t* data) {
#endif
	assert(file);
	assert(data);
#ifdef LANDRU_GAME_XW
	size_t count = landru_host_file_read(data, 1, sizeof(*data), file);
	xtimer_Often();
	return count != sizeof(*data);
#else
	return landru_host_file_read(data, 1, 4, file) == 4;
#endif
}

#ifdef LANDRU_GAME_XW
int32_t xfile_Read_Data_From_File(LandruFile* file, void* data, uint32_t size) {
#else
int16_t xfile_Read_Data_From_File(LandruFile* file, void* data, int32_t size) {
#endif
	assert(file);
	assert(data);
#ifdef LANDRU_GAME_XW
	size_t count = landru_host_file_read(data, 1, size, file);
	xtimer_Often();
	return count == size;
#else
	assert(size > 0);
	return landru_host_file_read(data, 1, size, file) == (size_t)size;
#endif
}

/* --- File write --- */

int16_t xfile_Write_Byte_To_File(LandruFile* file, uint8_t data) {
	assert(file);
	return landru_host_file_write(&data, 1, 1, file) == 1;
}

#ifdef LANDRU_GAME_XW
int16_t xfile_Write_Word_To_File(LandruFile* file, uint16_t data) {
#else
int16_t xfile_Write_Word_To_File(LandruFile* file, int16_t data) {
#endif
	assert(file);
#ifdef LANDRU_GAME_XW
	size_t count = landru_host_file_write(&data, 1, sizeof(data), file);
	xtimer_Often();
	return count != sizeof(data);
#else
	return landru_host_file_write(&data, 1, 2, file) == 2;
#endif
}

#ifdef LANDRU_GAME_XW
int16_t xfile_Write_Long_To_File(LandruFile* file, uint32_t data) {
#else
int16_t xfile_Write_Long_To_File(LandruFile* file, int32_t data) {
#endif
	assert(file);
#ifdef LANDRU_GAME_XW
	size_t count = landru_host_file_write(&data, 1, sizeof(data), file);
	xtimer_Often();
	return count != sizeof(data);
#else
	return landru_host_file_write(&data, 1, 4, file) == 4;
#endif
}

#ifdef LANDRU_GAME_XW
int32_t xfile_Write_Data_To_File(LandruFile* file, const void* data, uint32_t size) {
#else
int16_t xfile_Write_Data_To_File(LandruFile* file, const void* data, int32_t size) {
#endif
	assert(file);
	assert(data);
#ifdef LANDRU_GAME_XW
	size_t count = landru_host_file_write(data, 1, size, file);
	xtimer_Often();
	return count == size;
#else
	assert(size > 0);
	return landru_host_file_write(data, 1, size, file) == (size_t)size;
#endif
}

/* --- Buffer read --- */

uint8_t xfile_Read_Byte_From_Buffer(const uint8_t* buffer, int32_t* offset) { return buffer[(*offset)++]; }

int16_t xfile_Read_Word_From_Buffer(const uint8_t* buffer, int32_t* offset) {
	int16_t val;
	memcpy(&val, &buffer[*offset], 2);
	*offset += 2;
	return val;
}

int32_t xfile_Read_Long_From_Buffer(const uint8_t* buffer, int32_t* offset) {
	int32_t val;
	memcpy(&val, &buffer[*offset], 4);
	*offset += 4;
	return val;
}

void xfile_Read_Data_From_Buffer(const uint8_t* buffer, void* data, int32_t* offset, int32_t size) {
	memcpy(data, &buffer[*offset], size);
	*offset += size;
}

/* --- Buffer write --- */

void xfile_Write_Byte_To_Buffer(uint8_t* buffer, int32_t* offset, uint8_t val) { buffer[(*offset)++] = val; }

void xfile_Write_Word_To_Buffer(uint8_t* buffer, int32_t* offset, int16_t val) {
	memcpy(&buffer[*offset], &val, 2);
	*offset += 2;
}

void xfile_Write_Long_To_Buffer(uint8_t* buffer, int32_t* offset, int32_t val) {
	memcpy(&buffer[*offset], &val, 4);
	*offset += 4;
}

void xfile_Write_Data_To_Buffer(uint8_t* buffer, const void* data, int32_t* offset, int32_t size) {
	assert(buffer);
	assert(data);
	assert(size > 0);
	memcpy(&buffer[*offset], data, size);
	*offset += size;
}

/* --- Byte-order swap --- */

int16_t xfile_Swap_Word(int16_t value) {
	uint16_t v = (uint16_t)value;
	return (int16_t)((v >> 8) | (v << 8));
}

int32_t xfile_Swap_DWord(int32_t value) {
	uint32_t v = (uint32_t)value;
	return (int32_t)((v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24));
}
