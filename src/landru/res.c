#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <landru/file.h>
#include <landru/fourcc.h>
#include <landru/memptr.h>
#include <landru/res.h>

static ResFile* s_first_res_file;
static bool s_resource_module_gbl;

#ifdef LANDRU_GAME_XW
static char resource_default_path[RESOURCE_FILENAME_CAPACITY] = { 0 };
static char resource_alternate_path[RESOURCE_FILENAME_CAPACITY] = { 0 };
static bool resource_alternate_enabled = false;

void xres_SetResourcePath(const char* path, int16_t setDefault) {
	char* destination = setDefault ? resource_default_path : resource_alternate_path;
	size_t length = strlen(path);
	if (length >= sizeof(resource_default_path))
		return;
	memcpy(destination, path, length + 1);
	if (!setDefault)
		resource_alternate_enabled = true;
}

static bool fetch_resource_path(ResFile* rf, const char* prefix, const char* filename) {
	size_t prefix_length = strlen(prefix);
	size_t filename_length = strlen(filename);
	if (prefix_length >= sizeof(rf->filename) || filename_length >= sizeof(rf->filename) - prefix_length)
		return false;
	memcpy(rf->filename, prefix, prefix_length);
	memcpy(rf->filename + prefix_length, filename, filename_length + 1);
	return xres_Fetch_Resource_Files(rf);
}
#endif

void xres_Create_Resource_Module(void) {
	s_first_res_file = NULL;
	s_resource_module_gbl = true;
}

void xres_Destroy_Resource_Module(void) {
#ifdef LANDRU_MODERN
	while (s_first_res_file)
		xres_Close_Resource(s_first_res_file);
#endif
	s_resource_module_gbl = false;
}

ResFile* xres_Alloc_Res_File(void) {
	ResFile* rf = xmemptr_Alloc_System_Pointer(sizeof(ResFile));
	if (rf)
		memset(rf, 0, sizeof(ResFile));
	return rf;
}

void xres_Free_Res_File(ResFile* rf) {
	assert(rf != NULL);
#ifdef LANDRU_MODERN
	xres_Close_Resource_Data(rf);
#endif
	if (rf->res_map) {
		xmemhdl_Free_Handle(rf->res_map);
		rf->res_map = LANDRU_NULL_HANDLE;
	}
	free(rf);
}

ResFile* xres_Find_File_With_Resource(uint32_t res_type, const char* res_name) {
	ResFile* rf = s_first_res_file;
	while (rf) {
		if (xres_Is_File_In_Resource(rf, res_type, res_name))
			break;
		rf = rf->next;
	}
	return rf;
}

bool xres_Is_File_In_Resource(ResFile* rf, uint32_t res_type, const char* res_name) {
	ResEntry* entry = xmemhdl_Lock_Handle(rf->res_map);
	for (uint32_t i = 0; i < rf->res_count; i++) {
		if (xres_Resource_Compare(&entry[i], res_type, res_name))
			return true;
	}
	return false;
}

ResFile* xres_Open_Resource(const char* fileName) {
	ResFile* rf = xres_Alloc_Res_File();
	if (!rf)
		return NULL;

#ifdef LANDRU_GAME_XW
	bool fetched;
	if (fileName[0] == ':') {
		fetched = fetch_resource_path(rf, "", fileName + 1);
	} else {
		fetched = resource_alternate_enabled && fetch_resource_path(rf, resource_alternate_path, fileName);
		if (!fetched) {
			if (rf->res_map) {
				xmemhdl_Free_Handle(rf->res_map);
				rf->res_map = LANDRU_NULL_HANDLE;
			}
			fetched = fetch_resource_path(rf, resource_default_path, fileName);
		}
	}
	if (!fetched) {
		xres_Free_Res_File(rf);
		return NULL;
	}
#else
	strcpy(rf->filename, fileName);
	if (!xres_Fetch_Resource_Files(rf)) {
		xres_Free_Res_File(rf);
		return NULL;
	}
#endif

	rf->next = s_first_res_file;
	s_first_res_file = rf;
	return rf;
}

#ifdef LANDRU_GAME_XW
int16_t xres_Close_Resource(ResFile* rf) {
#else
void xres_Close_Resource(ResFile* rf) {
#endif
#ifndef LANDRU_GAME_XW
	assert(rf != NULL);
#endif
	ResFile* prev = NULL;
	ResFile* cur = s_first_res_file;

	while (cur && cur != rf) {
		prev = cur;
		cur = cur->next;
	}

	if (cur) {
		if (prev)
			prev->next = rf->next;
		else
			s_first_res_file = rf->next;
		xres_Free_Res_File(rf);
	}
#ifdef LANDRU_GAME_XW
	return 1;
#endif
}

LandruHandle xres_Load_Resource_Data(ResFile* rf, int offset, uint32_t size, uint16_t flags) {
	LandruHandle data = LANDRU_NULL_HANDLE;
	LandruFile* fp = xfile_Open_File(LANDRU_FILE_ROOT_ASSET, rf->filename, "rb");
	if (fp) {
		if (xfile_Seek_File(fp, offset, LANDRU_SEEK_SET) == 0)
			data = xres_Resource_Data_To_Handle(fp, size, flags);
		xfile_Close_File(fp);
	}
	return data;
}

#ifdef LANDRU_GAME_XW
int16_t xres_Open_Resource_Data(ResFile* rf, int32_t offset) {
	rf->file = xfile_Open_File(LANDRU_FILE_ROOT_ASSET, rf->filename, "rb");
	if (!rf->file)
		return 0;
	xfile_Seek_File(rf->file, offset, LANDRU_SEEK_SET);
	return 1;
}
#else
ResFile* xres_Open_Resource_Data(uint32_t res_type, const char* res_name) {
	assert(res_name != NULL && res_name[0] != '\0');
	ResFile* rf = xres_Find_File_With_Resource(res_type, res_name);
	if (!rf)
		return NULL;

	LandruFile* fp = xfile_Open_File(LANDRU_FILE_ROOT_ASSET, rf->filename, "rb");
	if (!fp)
		return NULL;

	int res_offset;
	uint32_t res_size;
	xres_Get_Resource_Offset(rf, res_type, res_name, &res_offset, &res_size);
	xfile_Seek_File(fp, res_offset, LANDRU_SEEK_SET);
	rf->file = fp;
	return rf;
}

#endif

void xres_Close_Resource_Data(ResFile* rf) {
	assert(rf != NULL);
	if (rf->file) {
		xfile_Close_File(rf->file);
	}
	rf->file = NULL;
}

uint8_t xres_Read_Resource_Byte(ResFile* rf) {
	assert(rf != NULL);
	if (!rf->file)
		return 0;
	uint8_t data;
	xfile_Read_Byte_From_File(rf->file, &data);
	return data;
}

uint16_t xres_Read_Resource_Word(ResFile* rf) {
	assert(rf != NULL);
	if (!rf->file)
		return 0;
#ifdef LANDRU_GAME_XW
	uint16_t data;
#else
	int16_t data;
#endif
	xfile_Read_Word_From_File(rf->file, &data);
	return data;
}

uint32_t xres_Read_Resource_Long(ResFile* rf) {
	assert(rf != NULL);
	if (!rf->file)
		return 0;
#ifdef LANDRU_GAME_XW
	uint32_t data;
#else
	int32_t data;
#endif
	xfile_Read_Long_From_File(rf->file, &data);
	return data;
}

#ifdef LANDRU_GAME_XW
void xres_Read_Resource_Buffer_Data(ResFile* rf, void* dest, uint16_t len) {
#else
void xres_Read_Resource_Buffer_Data(ResFile* rf, void* dest, uint32_t len) {
#endif
	assert(rf != NULL);
#ifndef LANDRU_GAME_XW
	assert(dest != NULL);
	assert(len != 0);
#endif
	if (rf->file) {
		xres_Resource_Data_To_Buffer(rf->file, dest, len);
	}
}

LandruHandle xres_Read_Resource_Data(ResFile* rf, uint32_t len, uint16_t flags) {
	assert(rf != NULL);
	if (rf->file) {
		return xres_Resource_Data_To_Handle(rf->file, len, flags);
	}
	return LANDRU_NULL_HANDLE;
}

#ifdef LANDRU_GAME_XW
int32_t xres_Resource_Data_To_Buffer(LandruFile* fileHandle, void* buf, uint16_t len) {
#else
bool xres_Resource_Data_To_Buffer(LandruFile* fileHandle, void* buf, uint32_t len) {
#endif
#ifndef LANDRU_GAME_XW
	assert(fileHandle != NULL);
	assert(buf != NULL);
	assert(len != 0);
#endif
	/* Both games retain this fallback after any diagnostic assertions. */
	if (!buf)
		return 0;
	return xfile_Read_Data_From_File(fileHandle, buf, len);
}

LandruHandle xres_Resource_Data_To_Handle(LandruFile* fileHandle, uint32_t len, uint16_t flags) {
	if (!fileHandle || len == 0 || len > INT32_MAX)
		return LANDRU_NULL_HANDLE;
	LandruHandle data = xmemhdl_Alloc_Handle(len, flags);
	if (data && !xfile_Read_Data_From_File(fileHandle, xmemhdl_Lock_Handle(data), len)) {
		xmemhdl_Free_Handle(data);
		return LANDRU_NULL_HANDLE;
	}
	return data;
}

#ifdef LANDRU_GAME_XW
int16_t xres_Get_Resource_Offset(ResFile* rf, uint32_t res_type, const char* res_name, int* out_offset,
								 uint32_t* out_size) {
#else
bool xres_Get_Resource_Offset(ResFile* rf, uint32_t res_type, const char* res_name, int* out_offset,
							  uint32_t* out_size) {
#endif
	int cur_offset = (rf->res_count + 2) * sizeof(ResEntry);
	ResEntry* entry = xmemhdl_Lock_Handle(rf->res_map);

	for (uint32_t i = 0; i < rf->res_count; i++) {
		if (xres_Resource_Compare(&entry[i], res_type, res_name)) {
			*out_size = entry[i].size;
			*out_offset = cur_offset;
			return true;
		}
		cur_offset += entry[i].size + sizeof(ResEntry);
	}

	*out_size = 0;
	*out_offset = 0;
	return false;
}

bool xres_Fetch_Resource_Files(ResFile* rf) {
	assert(rf != NULL);

	LandruFile* fp = xfile_Open_File(LANDRU_FILE_ROOT_ASSET, rf->filename, "rb");
	if (!fp)
		return false;

#ifdef LANDRU_GAME_XW
	uint32_t res_type;
#else
	int32_t res_type;
#endif
	char hdr_name[8];
#ifdef LANDRU_GAME_XW
	uint32_t rmap_len;
#else
	int32_t rmap_len;
#endif

	xfile_Read_Long_From_File(fp, &res_type);
	res_type = xfile_Swap_DWord(res_type);
	xfile_Read_Data_From_File(fp, hdr_name, 8);
	xfile_Read_Long_From_File(fp, &rmap_len);

	bool ok = true;
	if (res_type != FOURCC_RMAP) {
		ok = false;
	} else {
		rf->res_map = xres_Resource_Data_To_Handle(fp, rmap_len, LANDRU_MEMORY_DEFAULT);
		if (!rf->res_map) {
			ok = false;
		} else {
			rf->res_count = rmap_len / 16;
			ResEntry* entry = xmemhdl_Lock_Handle(rf->res_map);
			for (uint32_t i = 0; i < rf->res_count; i++) {
				entry[i].type = xfile_Swap_DWord(entry[i].type);
			}
		}
	}

	xfile_Close_File(fp);
	return ok;
}

bool xres_Resource_Compare(ResEntry* entry, uint32_t res_type, const char* res_name) {
	assert(entry != NULL);
	assert(res_name != NULL && res_name[0] != '\0');

	if (res_type != entry->type)
		return false;

	int i;
	for (i = 0; i < 8 && res_name[i] != '\0'; i++) {
		if (entry->name[i] != res_name[i])
			return false;
	}

	if (i < 8 && entry->name[i] != '\0')
		return false;

	return true;
}

void xres_Res_Type_To_String(uint32_t type, char* out) {
	out[0] = (type >> 24) & 0xFF;
	out[1] = (type >> 16) & 0xFF;
	out[2] = (type >> 8) & 0xFF;
	out[3] = type & 0xFF;
	out[4] = '\0';
}

void xres_Res_String_To_Type(uint32_t* out_type, const char* name) {
	assert(name != NULL && name[0] != '\0');
	*out_type = ((uint32_t)(uint8_t)name[0] << 24) | ((uint32_t)(uint8_t)name[1] << 16) |
				((uint32_t)(uint8_t)name[2] << 8) | (uint8_t)name[3];
}

#ifdef LANDRU_GAME_XW
void xres_ResetSceneList(void) { s_first_res_file = NULL; }
#endif
