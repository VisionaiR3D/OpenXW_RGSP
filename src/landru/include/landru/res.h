#ifndef LANDRU_RES_H
#define LANDRU_RES_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

#include <landru/host.h>

typedef struct ResEntry ResEntry;
typedef struct ResFile ResFile;

struct ResEntry {
	uint32_t type;
	char name[8];
	uint32_t size;
};

enum {
#ifdef LANDRU_GAME_XW
	RESOURCE_FILENAME_CAPACITY = 64
#else
	RESOURCE_FILENAME_CAPACITY = 50
#endif
};

struct ResFile {
	ResFile* next;
	char filename[RESOURCE_FILENAME_CAPACITY];
	LandruFile* file;
	LandruHandle res_map;
	uint32_t res_count;
};

void xres_Create_Resource_Module(void);
void xres_Destroy_Resource_Module(void);
ResFile* xres_Alloc_Res_File(void);
void xres_Free_Res_File(ResFile* resFile);
ResFile* xres_Find_File_With_Resource(uint32_t res_type, const char* res_name);
bool xres_Is_File_In_Resource(ResFile* resFile, uint32_t res_type, const char* res_name);
ResFile* xres_Open_Resource(const char* fileName);
#ifdef LANDRU_GAME_XW
int16_t xres_Close_Resource(ResFile* resFile);
#else
void xres_Close_Resource(ResFile* resFile);
#endif
LandruHandle xres_Load_Resource_Data(ResFile* rf, int offset, uint32_t size, uint16_t flags);
#ifdef LANDRU_GAME_XW
int16_t xres_Open_Resource_Data(ResFile* rf, int32_t offset);
#else
ResFile* xres_Open_Resource_Data(uint32_t res_type, const char* res_name);
#endif
void xres_Close_Resource_Data(ResFile* resFile);
uint8_t xres_Read_Resource_Byte(ResFile* resFile);
uint16_t xres_Read_Resource_Word(ResFile* resFile);
uint32_t xres_Read_Resource_Long(ResFile* resFile);
#ifdef LANDRU_GAME_XW
void xres_Read_Resource_Buffer_Data(ResFile* resFile, void* dest, uint16_t len);
#else
void xres_Read_Resource_Buffer_Data(ResFile* resFile, void* dest, uint32_t len);
#endif
LandruHandle xres_Read_Resource_Data(ResFile* resFile, uint32_t len, uint16_t flags);
#ifdef LANDRU_GAME_XW
int32_t xres_Resource_Data_To_Buffer(LandruFile* fileHandle, void* buf, uint16_t len);
#else
bool xres_Resource_Data_To_Buffer(LandruFile* fileHandle, void* buf, uint32_t len);
#endif
LandruHandle xres_Resource_Data_To_Handle(LandruFile* fileHandle, uint32_t len, uint16_t flags);
#ifdef LANDRU_GAME_XW
int16_t xres_Get_Resource_Offset(ResFile* resFile, uint32_t res_type, const char* res_name, int* out_offset,
								 uint32_t* out_size);
#else
bool xres_Get_Resource_Offset(ResFile* resFile, uint32_t res_type, const char* res_name, int* out_offset,
							  uint32_t* out_size);
#endif
bool xres_Fetch_Resource_Files(ResFile* resFile);
bool xres_Resource_Compare(ResEntry* entry, uint32_t res_type, const char* res_name);
void xres_Res_Type_To_String(uint32_t type, char* out);
void xres_Res_String_To_Type(uint32_t* out_type, const char* name);

#ifdef LANDRU_GAME_XW
void xres_ResetSceneList(void);
void xres_SetResourcePath(const char* path, int16_t setDefault);
#endif

#endif
