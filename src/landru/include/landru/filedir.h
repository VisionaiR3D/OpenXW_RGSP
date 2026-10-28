#ifndef LANDRU_FILEDIR_H
#define LANDRU_FILEDIR_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <stdbool.h>
#include <stdint.h>

enum {
	FILEDIR_MAX_ENTRIES = 384,
	FILEDIR_PATTERN_CAPACITY = 34,
	FILEDIR_BYTES_PER_KIB = 1024,
#ifdef LANDRU_GAME_XW
	FILEDIR_NAME_CAPACITY = 256,
#else
	FILEDIR_DEFAULT_NAME_LENGTH = 12,
	FILEDIR_MAX_NAME_LENGTH = 32,
	FILEDIR_NAME_CAPACITY = FILEDIR_MAX_NAME_LENGTH + 1,
#endif
};

typedef struct {
	char name[FILEDIR_NAME_CAPACITY];
	uint8_t sort_key; /* 1 = directory, 0 = file */
#ifdef LANDRU_GAME_XW
	uint16_t size_kb;
#else
	int16_t size_kb;
#endif
} DirEntry;

typedef struct {
	char pattern[FILEDIR_PATTERN_CAPACITY];
	LandruHandle entries;
	int16_t field_36;
	int16_t field_38;
	int16_t count;
	int16_t flags;
	int16_t ext_count;
#ifndef LANDRU_GAME_XW
	uint8_t max_name_length;
#endif
} Directory;

void xfiledir_Create_Directory_Module(void);
void xfiledir_Destroy_Directory_Module(void);
void xfiledir_Init_Directory(Directory* dir, const char* pattern, int16_t flags);
#ifndef LANDRU_GAME_XW
void xfiledir_Set_Name_Length(Directory* dir, uint8_t max_name_length);
#endif
void xfiledir_Free_Directory(Directory* dir);
void xfiledir_Read_Directory(Directory* dir);
void xfiledir_Add_File_To_Directory(Directory* dir, DirEntry* entries, DirEntry* entry);
void xfiledir_Change_Current_Drive(Directory* dir, int drive);
void xfiledir_Change_System_Drive(int drive);
int xfiledir_Get_Current_Drive(void);
void xfiledir_Change_Current_Dir(Directory* dir, const char* path);
char* xfiledir_Get_Current_Dir(void);
void xfiledir_Set_Current_File(const char* filename);
char* xfiledir_Get_Current_File(void);
char* xfiledir_Get_Current_Path(void);

#endif
