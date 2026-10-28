#ifndef LANDRU_SOUND_H
#define LANDRU_SOUND_H

#include <landru/config.h>

#include <landru/memhdl.h>
#include <landru/res.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct Sound Sound;

enum SoundAction {
	SOUND_ACTION_PAUSE = 1,
	SOUND_ACTION_RESUME = 2,
	SOUND_ACTION_START_MUSIC = 3,
	SOUND_ACTION_START_SFX = 4,
	SOUND_ACTION_START_SPEECH = 5,
	SOUND_ACTION_STOP = 6,
	SOUND_ACTION_SET_VOLUME = 7,
	SOUND_ACTION_FADE_VOLUME = 8,
	SOUND_ACTION_SET_PAN = 9,
	SOUND_ACTION_FADE_PAN = 10
};

typedef void (*SoundActionFunc)(int16_t action, Sound* snd, int16_t value, int16_t duration);
typedef void (*SoundUserFunc)(Sound* snd, int32_t time);

typedef enum {
	noSound = 0,
	gmidiSound = 1,
	digitalSound = 2,
} SoundType;

enum {
#ifdef LANDRU_GAME_XW
	SOUND_NAME_CAPACITY = LANDRU_RESOURCE_NAME_LENGTH + 1
#else
	SOUND_NAME_CAPACITY = LANDRU_RESOURCE_NAME_LENGTH
#endif
};

struct Sound {
	uint32_t res_type;
	char res_name[SOUND_NAME_CAPACITY];
	Sound* next;
	int32_t type;
#ifdef LANDRU_GAME_XW
#ifdef LANDRU_MODERN
	intptr_t id;
#else
	uint32_t id;
#endif
#else
	int16_t id;
#endif
	int16_t flags;
	int16_t volume;
	int16_t var1;
	int16_t var2;
	void* varptr;
	LandruHandle varhdl;
	SoundUserFunc user;
	LandruHandle data;
	int32_t size;
};

Sound* xsound_Ask_Sound_List(void);
void xsound_Open_Sound(void);
void xsound_Close_Sound(void);
void xsound_Purge_All_Sounds(void);
void xsound_PurgeGmidSounds(void);
Sound* xsound_Alloc_Sound(LandruHandle data, int16_t extend, uint32_t size);
void xsound_Init_Sound(Sound* sound);
#ifdef LANDRU_GAME_XW
Sound* xsound_Res_Music(ResFile* rf, const char* name);
#else
Sound* xsound_Res_Music(const char* name);
#endif
#ifdef LANDRU_GAME_XW
Sound* xsound_Res_Digital_Sound(ResFile* rf, const char* name);
#else
Sound* xsound_Res_Digital_Sound(const char* name);
#endif
Sound* xsound_Res_Digital_Voice(const char* name);
#ifdef LANDRU_GAME_XW
Sound* xsound_Res_Sound_Data(ResFile* rf, uint32_t res_type, const char* name, int16_t kind);
#else
Sound* xsound_Res_Sound_Data(uint32_t res_type, const char* name, int16_t kind);
#endif
void xsound_Free_Sound(Sound* sound);
void xsound_Free_Playing_Sound(Sound* sound, int16_t notify);
void xsound_Free_Sounds(Sound* sound);
void xsound_Free_Sound_From_User(Sound* sound);
void xsound_Free_User_Sounds(Sound* sound);
void xsound_Set_Sound_Action_Function(SoundActionFunc func);
void xsound_Pause_Sounds(void);
void xsound_Resume_Sounds(void);
void xsound_Start_Music(Sound* sound);
void xsound_Start_SFX(Sound* sound);
void xsound_Start_Speech(Sound* sound);
void xsound_Stop_Sound(Sound* sound);
void xsound_Set_Sound_Volume(Sound* sound, int16_t volume);
void xsound_Set_Sound_Fade(Sound* sound, int16_t volume, int16_t time);
void xsound_Set_Sound_Pan(Sound* sound, int16_t pan);
void xsound_Set_Sound_Pan_Fade(Sound* sound, int16_t pan, int16_t time);
void xsound_User_Sounds(int32_t time);
void xsound_Copy_Sound_Data(Sound* dst, Sound* src);
Sound* xsound_Find_Gmid(const char* name);
Sound* xsound_Find_Sound_Type(const char* name, uint32_t type);
void xsound_Discard_Sound_Data(Sound* sound);
void xsound_Non_Discard_Sound_Data(Sound* sound);
bool xsound_Is_Discard_Sound_Data(Sound* sound);
void xsound_Set_Sound_User_Keep(Sound* sound);
void xsound_Clear_Sound_User_Keep(Sound* sound);
bool xsound_Is_Sound_User_Keep(Sound* sound);
void xsound_Set_Sound_Keep(Sound* sound);
void xsound_Clear_Sound_Keep(Sound* sound);
bool xsound_Is_Sound_Keep(Sound* sound);
void xsound_Set_Sound_Keepable(Sound* sound);
void xsound_Clear_Sound_Keepable(Sound* sound);
bool xsound_Is_Sound_Keepable(Sound* sound);
void xsound_Set_Sound_Flag1(Sound* sound);
void xsound_Clear_Sound_Flag1(Sound* sound);
bool xsound_Is_Sound_Flag1(Sound* sound);
void xsound_Set_Sound_Flag2(Sound* sound);
void xsound_Clear_Sound_Flag2(Sound* sound);
bool xsound_Is_Sound_Flag2(Sound* sound);
void xsound_Set_Sound_Name(Sound* sound, uint32_t type, const char* name);
void xsound_Set_Sound_User_Function(Sound* sound, SoundUserFunc func);
void xsound_Get_Sound_User_Function(Sound* sound, SoundUserFunc* out);
void xsound_Stop_Landru_Sound(void);

#ifdef LANDRU_GAME_XW
void xsound_ResetSceneList(void);
#endif

#endif
