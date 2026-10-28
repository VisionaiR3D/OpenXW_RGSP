#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <landru/fourcc.h>
#include <landru/memptr.h>
#include <landru/res.h>
#include <landru/sound.h>

#include "host_internal.h"

// GLOBAL: TIE 0xD2C28
static Sound* first_sound_gbl = NULL;
// GLOBAL: TIE 0xD2C2C
static SoundActionFunc sound_func = NULL;

Sound* xsound_Ask_Sound_List(void) { return first_sound_gbl; }

void xsound_Open_Sound(void) {
	sound_func = NULL;
	first_sound_gbl = NULL;
}

void xsound_Close_Sound(void) {
	Sound* s = first_sound_gbl;
	while (s) {
		xsound_Clear_Sound_Keep(s);
		xsound_Clear_Sound_Keepable(s);
		s = s->next;
	}
	xsound_Free_Sounds(first_sound_gbl);
	first_sound_gbl = NULL;
}

void xsound_Purge_All_Sounds(void) {
	Sound* s = first_sound_gbl;
	while (s) {
		s->flags &= ~0x3000;
		s = s->next;
	}
	s = first_sound_gbl;
	while (s) {
		Sound* next = s->next;
		xsound_Free_Sound(s);
		s = next;
	}
	first_sound_gbl = NULL;
}

/* X-Wing's sudden scene exit discards music while retaining other resources. */
void xsound_PurgeGmidSounds(void) {
	Sound* sound = first_sound_gbl;
	while (sound) {
		Sound* next = sound->next;
		if (sound->res_type == FOURCC_GMID || sound->res_type == FOURCC_ADLB ||
			sound->res_type == FOURCC_RLND) {
			xsound_Clear_Sound_Keep(sound);
			xsound_Clear_Sound_Keepable(sound);
			xsound_Free_Playing_Sound(sound, 1);
		}
		sound = next;
	}
}

Sound* xsound_Alloc_Sound(LandruHandle data, int16_t extend, uint32_t size) {
	Sound* sound = xmemptr_Alloc_Clear_System_Pointer(sizeof(Sound) + extend);
	if (!sound)
		return NULL;

	xsound_Init_Sound(sound);
	sound->data = data;
	sound->size = size;
	sound->next = first_sound_gbl;
	first_sound_gbl = sound;
	return sound;
}

void xsound_Init_Sound(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->type = noSound;
	sound->id = 0;
	sound->flags = 0;
	sound->volume = 128;
	sound->var1 = 0;
	sound->var2 = 0;
	sound->varptr = NULL;
	sound->varhdl = LANDRU_NULL_HANDLE;
	sound->user = NULL;
	sound->data = LANDRU_NULL_HANDLE;
	sound->size = 0;
	sound->next = NULL;
}

#ifdef LANDRU_GAME_XW
Sound* xsound_Res_Music(ResFile* rf, const char* name) {
	if (landru_host_has_music_loader())
		return landru_host_music_load(rf, name);
	return xsound_Res_Sound_Data(rf, landru_host_music_type(), name, gmidiSound);
}
#else
Sound* xsound_Res_Music(const char* name) {
	ResFile* rf = xres_Find_File_With_Resource(FOURCC_GMID, name);
	if (!rf)
		return NULL;

	int res_offset;
	uint32_t res_size;
	if (!xres_Get_Resource_Offset(rf, FOURCC_GMID, name, &res_offset, &res_size))
		return NULL;

	Sound* sound = xsound_Alloc_Sound(LANDRU_NULL_HANDLE, 0, 0);
	if (!sound)
		return NULL;

	xsound_Set_Sound_Name(sound, FOURCC_GMID, name);
	xsound_Discard_Sound_Data(sound);
	sound->type = gmidiSound;
	sound->size = res_size;
	sound->data = xres_Load_Resource_Data(rf, res_offset, res_size, LANDRU_MEMORY_DEFAULT);
	if (!sound->data) {
		xsound_Free_Playing_Sound(sound, 0);
		return NULL;
	}
	return sound;
}

#endif

#ifdef LANDRU_GAME_XW
Sound* xsound_Res_Digital_Sound(ResFile* rf, const char* name) {
	return xsound_Res_Sound_Data(rf, FOURCC_VOIC, name, digitalSound);
}
#else
Sound* xsound_Res_Digital_Sound(const char* name) {
	ResFile* rf = xres_Find_File_With_Resource(FOURCC_VOIC, name);
	if (!rf)
		return NULL;

	int res_offset;
	uint32_t res_size;
	if (!xres_Get_Resource_Offset(rf, FOURCC_VOIC, name, &res_offset, &res_size))
		return NULL;

	Sound* sound = xsound_Alloc_Sound(LANDRU_NULL_HANDLE, 0, 0);
	if (!sound)
		return NULL;

	xsound_Set_Sound_Name(sound, FOURCC_VOIC, name);
	xsound_Discard_Sound_Data(sound);
	sound->type = digitalSound;
	sound->size = res_size;
	sound->data = xres_Load_Resource_Data(rf, res_offset, res_size, LANDRU_MEMORY_DEFAULT);
	if (!sound->data) {
		xsound_Free_Playing_Sound(sound, 0);
		return NULL;
	}
	return sound;
}

#endif

Sound* xsound_Res_Digital_Voice(const char* name) {
#ifdef LANDRU_GAME_XW
	return xsound_Res_Digital_Sound(xres_Find_File_With_Resource(FOURCC_VOIC, name), name);
#else
	return xsound_Res_Digital_Sound(name);
#endif
}

#ifdef LANDRU_GAME_XW
Sound* xsound_Res_Sound_Data(ResFile* rf, uint32_t res_type, const char* name, int16_t kind) {
	int offset;
	uint32_t size;
	if (!rf || !xres_Get_Resource_Offset(rf, res_type, name, &offset, &size))
		return NULL;

	uint16_t flags = res_type == FOURCC_VOIC ? LANDRU_MEMORY_DEFAULT : LANDRU_MEMORY_RESOURCE;
	LandruHandle data = xres_Load_Resource_Data(rf, offset, size, flags);
	if (!data)
		return NULL;

	Sound* sound = xsound_Alloc_Sound(data, 0, size);
	if (!sound) {
		xmemhdl_Free_Handle(data);
		return NULL;
	}
	xsound_Set_Sound_Name(sound, res_type, name);
	xsound_Discard_Sound_Data(sound);
	sound->type = kind;
	return sound;
}
#else
Sound* xsound_Res_Sound_Data(uint32_t res_type, const char* name, int16_t kind) {
	ResFile* rf = xres_Find_File_With_Resource(res_type, name);
	if (!rf)
		return NULL;

	int res_offset;
	uint32_t res_size;
	if (!xres_Get_Resource_Offset(rf, res_type, name, &res_offset, &res_size))
		return NULL;

	Sound* sound = xmemptr_Alloc_System_Pointer(sizeof(Sound));
	if (!sound)
		return NULL;

	xsound_Init_Sound(sound);
	sound->next = first_sound_gbl;
	first_sound_gbl = sound;

	xsound_Set_Sound_Name(sound, res_type, name);
	xsound_Discard_Sound_Data(sound);
	sound->type = kind;
	sound->size = res_size;

	/* iMUS resources load as VOIC with no allocation flags. */
	if (res_type == FOURCC_IMUS) {
		rf = xres_Find_File_With_Resource(FOURCC_VOIC, name);
		if (!rf || !xres_Get_Resource_Offset(rf, FOURCC_VOIC, name, &res_offset, &res_size)) {
			xsound_Free_Playing_Sound(sound, 0);
			return NULL;
		}
	}
	sound->data = xres_Load_Resource_Data(rf, res_offset, res_size, LANDRU_MEMORY_DEFAULT);

	if (!sound->data) {
		xsound_Free_Playing_Sound(sound, 0);
		return NULL;
	}
	return sound;
}

#endif

void xsound_Free_Sound(Sound* sound) { xsound_Free_Playing_Sound(sound, 1); }

void xsound_Free_Playing_Sound(Sound* sound, int16_t notify) {
	if (!sound)
		return;

	if (xsound_Is_Sound_Keep(sound) || xsound_Is_Sound_Keepable(sound)) {
		xsound_Clear_Sound_Keep(sound);
		if (!xsound_Is_Sound_User_Keep(sound))
			sound->user = NULL;
		return;
	}

	/* Unlink from list */
	Sound* cur = first_sound_gbl;
	Sound* prev = NULL;
	while (cur && cur != sound) {
		prev = cur;
		cur = cur->next;
	}
	if (cur == sound) {
		if (prev)
			prev->next = sound->next;
		else
			first_sound_gbl = sound->next;
		sound->next = NULL;
	}

	if (sound->type == gmidiSound)
		landru_host_music_release(sound);

	if (notify && sound_func)
		sound_func(SOUND_ACTION_STOP, sound, 0, 0);

	/* A clone shares the handle. Transfer ownership before deleting its owner. */
	if (sound->type == gmidiSound && xsound_Is_Discard_Sound_Data(sound) && sound->data) {
		for (Sound* other = first_sound_gbl; other; other = other->next) {
			if (other->data == sound->data) {
				xsound_Discard_Sound_Data(other);
				xsound_Non_Discard_Sound_Data(sound);
				break;
			}
		}
	}
	if (xsound_Is_Discard_Sound_Data(sound) && sound->data) {
		xmemhdl_Free_Handle(sound->data);
		sound->data = LANDRU_NULL_HANDLE;
	}
	if (sound->varptr) {
		xmemptr_Free_System_Pointer(sound->varptr);
		sound->varptr = NULL;
	}
	if (sound->varhdl) {
		xmemhdl_Free_Handle(sound->varhdl);
		sound->varhdl = LANDRU_NULL_HANDLE;
	}
	xmemptr_Free_System_Pointer(sound);
}

void xsound_Free_Sounds(Sound* sound) {
	while (sound) {
		Sound* next = sound->next;
		xsound_Free_Playing_Sound(sound, 1);
		sound = next;
	}
}

void xsound_Free_Sound_From_User(Sound* sound) { sound->flags |= 0x01; }

void xsound_Free_User_Sounds(Sound* sound) {
	while (sound) {
		Sound* next = sound->next;
		if (sound->flags & 0x01)
			xsound_Free_Playing_Sound(sound, 1);
		sound = next;
	}
}

void xsound_Set_Sound_Action_Function(SoundActionFunc func) { sound_func = func; }

void xsound_Pause_Sounds(void) {
	if (sound_func)
		sound_func(SOUND_ACTION_PAUSE, NULL, 0, 0);
}

void xsound_Resume_Sounds(void) {
	if (sound_func)
		sound_func(SOUND_ACTION_RESUME, NULL, 0, 0);
}

void xsound_Start_Music(Sound* sound) {
	if (sound_func)
		sound_func(SOUND_ACTION_START_MUSIC, sound, 0, 0);
}

void xsound_Start_SFX(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	if (sound_func)
		sound_func(SOUND_ACTION_START_SFX, sound, 0, 0);
}

void xsound_Start_Speech(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	if (sound_func)
		sound_func(SOUND_ACTION_START_SPEECH, sound, 0, 0);
}

void xsound_Stop_Sound(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	if (sound_func)
		sound_func(SOUND_ACTION_STOP, sound, 0, 0);
}

void xsound_Set_Sound_Volume(Sound* sound, int16_t volume) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	if (sound_func)
		sound_func(SOUND_ACTION_SET_VOLUME, sound, volume, 0);
}

void xsound_Set_Sound_Fade(Sound* sound, int16_t volume, int16_t time) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	if (sound_func)
		sound_func(SOUND_ACTION_FADE_VOLUME, sound, volume, time);
}

void xsound_Set_Sound_Pan(Sound* sound, int16_t pan) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	if (sound_func)
		sound_func(SOUND_ACTION_SET_PAN, sound, pan, 0);
}

void xsound_Set_Sound_Pan_Fade(Sound* sound, int16_t pan, int16_t time) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	if (sound_func)
		sound_func(SOUND_ACTION_FADE_PAN, sound, pan, time);
}

void xsound_User_Sounds(int32_t time) {
	Sound* sound = first_sound_gbl;
	while (sound) {
		if (sound->user)
			sound->user(sound, time);
		sound = sound->next;
	}
}

void xsound_Copy_Sound_Data(Sound* dst, Sound* src) {
	if (dst == src)
		return;
	if (dst->type == gmidiSound) {
		landru_host_music_release(dst);
		if (xsound_Is_Discard_Sound_Data(dst) && dst->data) {
			for (Sound* other = first_sound_gbl; other; other = other->next) {
				if (other != dst && other->data == dst->data) {
					xsound_Discard_Sound_Data(other);
					xsound_Non_Discard_Sound_Data(dst);
					break;
				}
			}
		}
	}
	if (xsound_Is_Discard_Sound_Data(dst) && dst->data) {
		xmemhdl_Free_Handle(dst->data);
		dst->data = LANDRU_NULL_HANDLE;
		dst->size = 0;
	}

	xsound_Non_Discard_Sound_Data(dst);
	xsound_Set_Sound_Name(dst, src->res_type, src->res_name);
	dst->data = src->data;
	dst->size = src->size;
	dst->id = src->id;
	dst->type = src->type;
#ifdef LANDRU_MODERN
	if (dst->type == gmidiSound)
		dst->id = (intptr_t)dst;
#endif
}

Sound* xsound_Find_Gmid(const char* name) { return xsound_Find_Sound_Type(name, landru_host_music_type()); }

Sound* xsound_Find_Sound_Type(const char* name, uint32_t type) {
	Sound* sound = first_sound_gbl;
	while (sound) {
		if (sound->res_type == type) {
			int i;
			bool match = true;
			for (i = 0; i < 8 && match && name[i]; i++) {
				if (tolower((unsigned char)name[i]) != sound->res_name[i])
					match = false;
			}
			if (match &&
#ifndef LANDRU_GAME_XW
				i > 0 &&
#endif
				(i >= LANDRU_RESOURCE_NAME_LENGTH || !sound->res_name[i]))
				return sound;
		}
		sound = sound->next;
	}
	return NULL;
}

void xsound_Discard_Sound_Data(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->flags |= 0x0002;
}

void xsound_Non_Discard_Sound_Data(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->flags &= ~0x0002;
}

bool xsound_Is_Discard_Sound_Data(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return false;
#endif
	return sound->flags & 0x0002;
}

void xsound_Set_Sound_User_Keep(Sound* sound) { sound->flags |= 0x0004; }

void xsound_Clear_Sound_User_Keep(Sound* sound) { sound->flags &= ~0x0004; }

bool xsound_Is_Sound_User_Keep(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return false;
#endif
	return sound->flags & 0x0004;
}

void xsound_Set_Sound_Keep(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->flags |= 0x1000;
}

void xsound_Clear_Sound_Keep(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->flags &= ~0x1000;
}

bool xsound_Is_Sound_Keep(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return false;
#endif
	return sound->flags & 0x1000;
}

void xsound_Set_Sound_Keepable(Sound* sound) { sound->flags |= 0x2000; }

void xsound_Clear_Sound_Keepable(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->flags &= ~0x2000;
}

bool xsound_Is_Sound_Keepable(Sound* sound) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return false;
#endif
	return sound->flags & 0x2000;
}

void xsound_Set_Sound_Flag1(Sound* sound) { sound->flags |= 0x4000; }

void xsound_Clear_Sound_Flag1(Sound* sound) { sound->flags &= ~0x4000; }

bool xsound_Is_Sound_Flag1(Sound* sound) { return sound->flags & 0x4000; }

void xsound_Set_Sound_Flag2(Sound* sound) { sound->flags |= 0x8000; }

void xsound_Clear_Sound_Flag2(Sound* sound) { sound->flags &= ~0x8000; }

bool xsound_Is_Sound_Flag2(Sound* sound) { return sound->flags & 0x8000; }

void xsound_Set_Sound_Name(Sound* sound, uint32_t type, const char* name) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->res_type = type;
	int i;
	for (i = 0; i < LANDRU_RESOURCE_NAME_LENGTH && name[i]; i++)
		sound->res_name[i] = tolower((unsigned char)name[i]);
	for (; i < LANDRU_RESOURCE_NAME_LENGTH; i++)
		sound->res_name[i] = 0;
#ifdef LANDRU_GAME_XW
	sound->res_name[LANDRU_RESOURCE_NAME_LENGTH] = 0;
#endif
}

void xsound_Set_Sound_User_Function(Sound* sound, SoundUserFunc func) {
#ifdef LANDRU_GAME_XW
	if (!sound)
		return;
#endif
	sound->user = func;
}

void xsound_Get_Sound_User_Function(Sound* sound, SoundUserFunc* out) { *out = sound->user; }

/* Empty in the binary too. */
void xsound_Stop_Landru_Sound(void) {}

#ifdef LANDRU_GAME_XW
void xsound_ResetSceneList(void) { first_sound_gbl = NULL; }
#endif
