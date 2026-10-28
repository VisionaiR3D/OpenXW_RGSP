#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <landru/actanim.h>
#include <landru/actcust.h>
#include <landru/actdelt.h>
#include <landru/actor.h>
#include <landru/actraw.h>
#include <landru/canvas.h>
#include <landru/fade.h>
#include <landru/file.h>
#include <landru/film.h>
#include <landru/fourcc.h>
#include <landru/memptr.h>
#include <landru/pal.h>
#include <landru/render.h>
#include <landru/res.h>
#include <landru/sound.h>
#include <landru/timer.h>
#include <landru/view.h>
#include <landru/viewadd.h>

#include "render_internal.h"

// GLOBAL: TIE 0xD2FFC
Film* first_film_gbl;
int16_t film_zplane_gbl;
// GLOBAL: TIE 0xD3002
int16_t film_color_fade_gbl;
// GLOBAL: TIE 0xD3000
int16_t film_fade_gbl;

Film* xfilm_Ask_Film_List(void) { return first_film_gbl; }

void xfilm_Add_Film_To_System(Film* film) {
	Film* curFilm = first_film_gbl;
	Film* lastFilm = NULL;

	while (curFilm && (film->zplane < curFilm->zplane)) {
		lastFilm = curFilm;
		curFilm = curFilm->next;
	}

	film->next = curFilm;
	if (lastFilm)
		lastFilm->next = film;
	else
		first_film_gbl = film;
}

void xfilm_Free_Film_From_System(Film* film) {
	Film* curFilm = first_film_gbl;
	Film* lastFilm = NULL;

	while (curFilm && (curFilm != film)) {
		lastFilm = curFilm;
		curFilm = curFilm->next;
	}

	if (curFilm) {
		if (!lastFilm) {
			first_film_gbl = curFilm->next;
		} else {
			lastFilm->next = curFilm->next;
		}
		film->next = NULL;
	}
}

Film* xfilm_Alloc_Film(int16_t extend) {
	Film* film = xmemptr_Alloc_Clear_System_Pointer(sizeof(Film) + extend);
	if (film) {
		film->def_palette = NULL;
		film->start = 0;
		film->stop = -1;
		film->flags = FF_REFRESH;
		film->next = NULL;
	}
	return film;
}

void xfilm_Free_Film(Film* film) {
	if (film->varptr) {
		xmemptr_Free_System_Pointer(film->varptr);
	}
	if (film->varhdl) {
		xmemhdl_Free_Handle(film->varhdl);
	}
	if (xfilm_Is_Discard_Film_Data(film)) {
		xfilm_Free_Film_Data(film);
	}
	xmemptr_Free_System_Pointer(film);
}

void xfilm_Free_Film_Data(Film* film) {
	if (film->array) {
		for (int i = 0; i < film->array_size; i++) {
			xfilm_Free_Film_Data_Object(film, i);
		}
		xmemhdl_Free_Handle(film->array);
		film->array = LANDRU_NULL_HANDLE;
	}
}

void xfilm_Free_Film_Data_Object(Film* film, int16_t objId) {
	LandruHandle* array = xmemhdl_Lock_Handle(film->array);
	FilmObject* obj = xmemhdl_Lock_Handle(array[objId]);
	if (!obj)
		return;

	if (obj->object) {
		switch (obj->id) {
			case FTC_ACTOR: {
				Actor* actor = (Actor*)obj->object;
				xactor_Free_Actor_From_System(actor);
				xactor_Free_Actor(actor);
			} break;
			case FTC_PALETTE: {
				Palette* pal = (Palette*)obj->object;
				xpal_Free_Palette_From_System(pal);
				xpal_Free_Palette(pal);
			} break;
			case FTC_SOUND:
				xsound_Free_Sound((Sound*)obj->object);
				break;
		}
	}
	xmemhdl_Free_Handle(array[objId]);
	array[objId] = LANDRU_NULL_HANDLE;
}

void xfilm_Free_Films(Film* film) {
	while (film) {
		Film* next = film->next;
		xfilm_Free_Film_From_System(film);
		xfilm_Free_Film(film);
		film = next;
	}
}

#ifdef LANDRU_GAME_XW
Film* xfilm_Res_Film(ResFile* filmRes, const char* name, Rect* frame, int16_t x, int16_t y, int16_t zPlane) {
	return xfilm_Res_Callback_Film(filmRes, name, frame, x, y, zPlane, NULL);
}
#else
Film* xfilm_Res_Film(const char* name, Rect* frame, int16_t x, int16_t y, int16_t zPlane) {
	return xfilm_Res_Callback_Film(name, frame, x, y, zPlane, NULL);
}
#endif

#ifdef LANDRU_GAME_XW
Film* xfilm_Res_Callback_Film(ResFile* filmRes, const char* name, Rect* frame, int16_t x, int16_t y,
							  int16_t zPlane, xfilmCallbackFunc cbFunc) {
#else
Film* xfilm_Res_Callback_Film(const char* name, Rect* frame, int16_t x, int16_t y, int16_t zPlane,
							  xfilmCallbackFunc cbFunc) {
#endif
	Film* film = xfilm_Alloc_Film(0);
	if (!film)
		return NULL;

#ifdef LANDRU_GAME_XW
	int offset;
	uint32_t size;
	if (filmRes && (!xres_Get_Resource_Offset(filmRes, FOURCC_FILM, name, &offset, &size) ||
					!xres_Open_Resource_Data(filmRes, offset)))
		filmRes = NULL;
#else
	ResFile* filmRes = xres_Open_Resource_Data(FOURCC_FILM, name);
#endif
	if (!filmRes) {
		fprintf(stderr, "landru.film: unavailable film %.8s\n", name);
		xfilm_Free_Film(film);
		return NULL;
	}

	uint16_t magic = xres_Read_Resource_Word(filmRes);
	uint16_t numCels = xres_Read_Resource_Word(filmRes);
	uint16_t numObjects = xres_Read_Resource_Word(filmRes);
	if (magic != 4 || numObjects > INT16_MAX) {
		fprintf(stderr, "landru.film: invalid film %.8s header %u/%u\n", name, magic, numObjects);
		xres_Close_Resource_Data(filmRes);
		xfilm_Free_Film(film);
		return NULL;
	}
	film->array = xmemhdl_Alloc_Clear_Handle(sizeof(LandruHandle) * numObjects, LANDRU_MEMORY_DEFAULT);
	film->cur_cel = 0;
	film->array_size = (int16_t)numObjects;
	film->cels = numCels;
	bool loaded = film->array && xfilm_Res_Film_Data(filmRes, film->array, film->array_size);
	xres_Close_Resource_Data(filmRes);
	if (!loaded || !xfilm_Res_Film_Objects(film, cbFunc)) {
		fprintf(stderr, "landru.film: failed loading film %.8s (%s)\n", name, loaded ? "objects" : "data");
		xfilm_Free_Film_Data(film);
		xfilm_Free_Film(film);
		return NULL;
	}

	xfilm_Init_Film(film, film->array, frame, x, y, zPlane);
	xfilm_Set_Film_Name(film, FOURCC_FILM, name);

	return film;
}

bool xfilm_Res_Film_Object(uint32_t objType, const char* objName, void** outObj) {
	Rect canvasBounds;
	Actor* actor = NULL;
	void* object = NULL;
#ifdef LANDRU_GAME_XW
	ResFile* resFile = NULL;
	*outObj = NULL;
	if (objType != FOURCC_VIEW && objType != FOURCC_CUST) {
		resFile = xres_Find_File_With_Resource(objType, objName);
		if (!resFile)
			return false;
	}
#endif

	/* VIEW objects are no-ops — the film system doesn't instantiate them.
	 * Return true so the loading loop continues to subsequent objects. */
	if (objType == FOURCC_VIEW)
		return true;

	xcanvas_Get_Drawing_Canvas_Bounds(&canvasBounds);
	if (objType == FOURCC_CUST) {
		actor = xactcust_Alloc_Custom_Actor(0, &canvasBounds, 0, 0, 0);
		if (actor) {
			xactor_Set_Actor_Draw_Function(actor, 0);
		}
	} else if (objType == FOURCC_DELT) {
		actor = xfilm_Clone_Film_Actor(objType, objName);
		if (!actor) {
#ifdef LANDRU_GAME_XW
			actor = xactdelt_Res_Delta_Actor(resFile, objName, &canvasBounds, 0, 0, 0);
#else
			actor = xactdelt_Res_Delta_Actor(objName, &canvasBounds, 0, 0, 0);
#endif
		}
#ifndef LANDRU_GAME_XW
	} else if (objType == FOURCC_RAW) {
		actor = xfilm_Clone_Film_Actor(objType, objName);
		if (!actor) {
			actor = xactraw_Res_Raw_Actor(objName, &canvasBounds, 0, 0, 0);
		}
#endif
	} else if (objType == FOURCC_ANIM) {
		actor = xfilm_Clone_Film_Actor(objType, objName);
		if (!actor) {
#ifdef LANDRU_GAME_XW
			actor = xactanim_Res_Anim_Actor(resFile, objName, &canvasBounds, 0, 0, 0);
#else
			actor = xactanim_Res_Anim_Actor(objName, &canvasBounds, 0, 0, 0);
#endif
		}
	} else if (objType == FOURCC_PLTT) {
#ifdef LANDRU_GAME_XW
		object = xpal_Res_Palette(resFile, objName);
#else
		object = xpal_Res_Palette(objName);
#endif
#ifndef LANDRU_GAME_XW
	} else if (objType == FOURCC_GMID) {
		object = xsound_Res_Music(objName);
#endif
	} else if (objType == FOURCC_VOIC) {
#ifdef LANDRU_GAME_XW
		object = xfilm_Clone_Film_Sound(FOURCC_VOIC, objName);
#else
		object = xfilm_Clone_Film_Sound(digitalSound, objName);
#endif
		if (!object) {
#ifdef LANDRU_GAME_XW
			object = xsound_Res_Digital_Sound(resFile, objName);
#else
			object = xsound_Res_Digital_Sound(objName);
#endif
		}
#ifdef LANDRU_GAME_XW
	} else {
		return true;
#endif
	}

	if (actor) {
		xactor_Set_Actor_Time(actor, -1, -1);
		*outObj = actor;
		return true;
	} else if (object) {
		*outObj = object;
		return true;
	}

	return false;
}

Actor* xfilm_Clone_Film_Actor(uint32_t actType, const char* actName) {
	Actor* curActor = xactor_Ask_Actor_List();
	Actor* actor = NULL;
	while (curActor && !actor) {
		if (curActor->res_type == actType && memcmp(curActor->res_name, actName, 8) == 0) {
			actor = curActor;
		}
		curActor = curActor->next;
	}

	curActor = NULL;

	if (actor) {
		Rect canvasClip;
		xcanvas_Get_Drawing_Canvas_Clip(&canvasClip);
		if (actor->res_type == FOURCC_DELT) {
			curActor = xactdelt_Alloc_Delta_Actor(0, &canvasClip, 0, 0, 0);
		} else if (actor->res_type == FOURCC_RAW) {
			curActor = xactraw_Alloc_Raw_Actor(0, &canvasClip, 0, 0, 0);
		} else if (actor->res_type == FOURCC_ANIM) {
			curActor = xactanim_Alloc_Anim_Actor(0, &canvasClip, 0, 0, 0);
		}

		if (curActor) {
			xactor_Copy_Actor_Data(curActor, actor);
			xactor_Set_Actor_Name(curActor, actor->res_type, actor->res_name);
		}
	}

	return curActor;
}

Sound* xfilm_Clone_Film_Sound(uint32_t sndType, const char* sndName) {
	Sound* curSound = xsound_Ask_Sound_List();
	Sound* sound = NULL;

	while (curSound && !sound) {
		if (curSound->res_type == sndType && memcmp(curSound->res_name, sndName, 8) == 0) {
			sound = curSound;
		}
		curSound = curSound->next;
	}

	curSound = NULL;
	if (sound) {
		if (sound->res_type == FOURCC_VOIC) {
			curSound = xsound_Alloc_Sound(LANDRU_NULL_HANDLE, 0, 0);
		}
		if (curSound) {
			xsound_Copy_Sound_Data(curSound, sound);
			xsound_Set_Sound_Name(curSound, sndType, sndName);
		}
	}

	return curSound;
}

bool xfilm_Res_Film_Data(ResFile* fp, LandruHandle arrayHandle, int16_t count) {
	LandruHandle* array = xmemhdl_Lock_Handle(arrayHandle);
	if (!array)
		return false;
	for (int16_t i = 0; i < count; i++) {
		char objName[8];
		uint32_t objType = xres_Read_Resource_Long(fp);
		objType = xfile_Swap_DWord(objType);
		xres_Read_Resource_Buffer_Data(fp, objName, 8);
		xres_Read_Resource_Long(fp);
		uint16_t objTypeId = xres_Read_Resource_Word(fp);
		xres_Read_Resource_Word(fp);
		uint16_t objLength = xres_Read_Resource_Word(fp);
		LandruHandle objectHandle =
			xmemhdl_Alloc_Handle(sizeof(FilmObject) + objLength, LANDRU_MEMORY_DEFAULT);
		FilmObject* filmObj = xmemhdl_Lock_Handle(objectHandle);
		if (!filmObj)
			return false;
		filmObj->res_type = objType;
		memcpy(filmObj->res_name, objName, 8);
		filmObj->id = objTypeId;
		filmObj->offset = 0;
		filmObj->object = NULL;
		if (objLength && !xres_Resource_Data_To_Buffer(fp->file, filmObj + 1, objLength)) {
			xmemhdl_Free_Handle(objectHandle);
			return false;
		}
		array[i] = objectHandle;
	}
	return true;
}

bool xfilm_Res_Film_Objects(Film* film, xfilmCallbackFunc callback) {
	LandruHandle* array = xmemhdl_Lock_Handle(film->array);
	for (int16_t i = 0; i < film->array_size; i++) {
		FilmObject* filmObj = xmemhdl_Lock_Handle(array[i]);
		if (!filmObj)
			return false;

		if (!xfilm_Res_Film_Object(filmObj->res_type, filmObj->res_name, &filmObj->object)) {
			fprintf(stderr, "landru.film: missing object %08x/%.8s\n", filmObj->res_type, filmObj->res_name);
			return false;
		}

#ifdef LANDRU_MODERN
		if (filmObj->object && filmObj->id == FTC_ACTOR)
			((Actor*)filmObj->object)->film_entry_index = i;
#endif

		if (callback && callback(film, filmObj))
			xfilm_Free_Film_Data_Object(film, i);
	}
	return true;
}

void xfilm_Init_Film(Film* film, LandruHandle array, Rect* frame, int16_t x, int16_t y, int16_t z) {
	xrect_Copy_Rect(&film->frame, frame);
	film->x = x;
	film->y = y;
	film->zplane = z;
	xfilm_Discard_Film_Data(film);
	film->draw = NULL;
	film->array = array;
	film->update = xfilm_fupdate_Film;
	xfilm_Add_Film_To_System(film);
}

void xfilm_Update_Films(int32_t time) {
	Film* film = first_film_gbl;
	while (film) {
		if (time == film->start) {
			xfilm_Start_Film(film);
			if (time == film->stop) {
				xfilm_Stop_Film(film);
			}
		} else if (time == film->stop) {
			xfilm_Stop_Film(film);
		} else if (!film->cels) {
			if (xfilm_Is_Film_Active(film)) {
				xfilm_Stop_Film(film);
			}
		} else if (film->cur_cel < film->cels) {
			if (film->update && xfilm_Is_Film_Active(film)) {
				film->update(film);
			}
		} else {
			if (xfilm_Is_Repeat_Film(film)) {
				xfilm_Rewind_Film(film);
			} else {
				xfilm_Stop_Film(film);
			}
		}
		film = film->next;
	}
}

void xfilm_User_Films(int32_t time) {
	Film* film = first_film_gbl;
	while (film) {
		if (film->user) {
			film->user(film, time);
		}
		film = film->next;
	}
}

#ifdef LANDRU_GAME_XW
void xfilm_Draw_Films(Film* list, int16_t refresh) {
#else
void xfilm_Draw_Films(int16_t refresh) {
#endif
#ifndef LANDRU_GAME_XW
	Film* list = first_film_gbl;
#endif
	int16_t x, y;
	for (int16_t i = 0; i < 4; i++) {
		Rect rect, clipRect;
		xview_Get_View_Frame(i, &rect);
		if (xrect_Empty_Rect(&rect))
			continue;

		Film* film = list;
		while (film) {
			if (film->draw && xfilm_Is_Film_Visible(film)) {
				if (xviewadd_Clip_Object_To_View(i, film->zplane, &film->frame, &rect, &clipRect)) {
					int16_t needs_draw =
						refresh | xfilm_Is_Film_Refresh(film) | xfilm_Is_Film_Refreshable(film);
					xcanvas_Set_Drawing_Canvas_Clip(&clipRect);
					xfilm_Get_Film_Relative_XY(film, &rect, &x, &y);
					film->draw(film, &rect, &clipRect, x, y, needs_draw);
				}
			}
			film = film->next;
		}
	}

	for (Film* film = list; film; film = film->next) {
		xfilm_Non_Refresh_Film(film);
	}
}

void xfilm_fupdate_Film(Film* film) { xfilm_Step_Film_Objects(film); }

void xfilm_Get_Film_Relative_XY(Film* film, Rect* rect, int16_t* outX, int16_t* outY) {
	*outX = (film->x - film->frame.left) + rect->left;
	*outY = (film->y - film->frame.top) + rect->top;
}

void xfilm_Set_Film_Name(Film* film, uint32_t res_type, const char* name) {
	film->res_type = res_type;
	int i;
	for (i = 0; i < 8 && name[i]; i++)
		film->res_name[i] = name[i];
	for (; i < 8; i++)
		film->res_name[i] = 0;
}

void xfilm_Set_Film_Def_Palette(Film* film, Palette* palette) { film->def_palette = palette; }

bool xfilm_Is_Film_Visible(Film* film) { return film->flags & FF_VISIBLE; }

bool xfilm_Is_Film_Active(Film* film) { return film->flags & FF_ACTIVE; }

void xfilm_Start_Film(Film* film) {
	xfilm_Rewind_Film(film);
	film->flags |= (FF_VISIBLE | FF_ACTIVE);
}

void xfilm_Stop_Film(Film* film) { film->flags &= ~(FF_VISIBLE | FF_ACTIVE); }

void xfilm_Rewind_Film(Film* film) { xfilm_Rewind_Film_Objects(film); }

void xfilm_Non_Refresh_Film(Film* film) { film->flags &= ~FF_REFRESH; }

bool xfilm_Is_Film_Refresh(Film* film) { return film->flags & FF_REFRESH; }

bool xfilm_Is_Film_Refreshable(Film* film) { return film->flags & FF_REFRESHABLE; }

void xfilm_Discard_Film_Data(Film* film) { film->flags |= FF_DISCARD; }

bool xfilm_Is_Discard_Film_Data(Film* film) { return film->flags & FF_DISCARD; }

bool xfilm_Is_Repeat_Film(Film* film) { return film->flags & FF_REPEAT; }

void xfilm_Check_Film_ZPlanes(void) {
	if (!film_zplane_gbl)
		return;

	film_zplane_gbl = 0;
	Film* filmSort = NULL;
	Film* filmBase = xfilm_Ask_Film_List();
	while (filmBase) {
		Film* curFilmSort = filmSort;
		Film* curFilmBase = filmBase;
		filmBase = filmBase->next;

		Film* lastFilmSort = NULL;
		curFilmBase->next = NULL;

		while (curFilmSort && curFilmSort->zplane >= curFilmBase->zplane) {
			lastFilmSort = curFilmSort;
			curFilmSort = curFilmSort->next;
		}

		if (!curFilmSort) {
			if (!lastFilmSort) {
				filmSort = curFilmBase;
			} else {
				lastFilmSort->next = curFilmBase;
			}
		} else if (!lastFilmSort) {
			filmSort = curFilmBase;
			curFilmBase->next = curFilmSort;
		} else {
			lastFilmSort->next = curFilmBase;
			curFilmBase->next = curFilmSort;
		}
	}

	first_film_gbl = filmSort;
}

void xfilm_Clear_Film_Fade(void) {
	film_color_fade_gbl = 0;
	film_fade_gbl = 0;
}

void xfilm_Start_Film_Fade(int16_t direction) {
	int16_t lock = 1;
	/* PAL_TO_PAL with a non-wipe (or SNAP) fade type runs as a
	 * single-shot palette interpolation rather than a sustained
	 * task — drop the lock so Push_Fade_To_Video_Screen_Task
	 * doesn't extend the end_time. */
	if (film_color_fade_gbl == FADE_COLOR_PAL_TO_PAL && film_fade_gbl < FADE_WIPE_RIGHT) {
		lock = 0;
	}

	xfade_Start_Full_Fade(film_fade_gbl, film_color_fade_gbl, direction, 0, lock);
	xfilm_Clear_Film_Fade();
}

bool xfilm_Is_Film_Fade(void) { return film_fade_gbl || film_color_fade_gbl; }

void xfilm_Rewind_Film_Objects(Film* film) {
	LandruHandle* array = xmemhdl_Lock_Handle(film->array);
	film->cur_cel = 0;
	xpal_Screen_To_Dest_Palette(0, 0, 255);
	xpal_Screen_To_Src_Palette(0, 0, 255);
	xpal_Set_Dest_Pal_Color(0, 255, 0, 0, 0);
	if (film->def_palette) {
		xpal_Set_Dest_Palette(film->def_palette);
	}

	for (int i = 0; i < film->array_size; i++) {
		FilmObject* obj = xmemhdl_Lock_Handle(array[i]);
		if (!obj)
			continue;
		switch (obj->id) {
			case FTC_VIEW:
				xfilm_Rewind_View_Film(film, obj, (uint8_t*)(obj + 1));
				break;
			case FTC_ACTOR:
				xfilm_Rewind_Actor_Film(film, obj, (uint8_t*)(obj + 1));
				break;
			case FTC_PALETTE:
				xfilm_Rewind_Palette_Film(film, obj, (uint8_t*)(obj + 1));
				break;
			case FTC_SOUND:
				xfilm_Rewind_Sound_Film(film, obj, (uint8_t*)(obj + 1));
				break;
		}
	}

	if (!xpal_Compare_Src_Dest_Palette()) {
		if (xfilm_Is_Film_Fade()) {
			xfilm_Start_Film_Fade(true);
		} else if (!xfade_Fade_Active()) {
			xfade_Start_Color_Fade(1, 1, 1, 0, 1);
		} else {
			xpal_Dest_To_Screen_Palette(0, 0, 255);
			xpal_Put_Screen_Palette();
		}
	} else if (xfilm_Is_Film_Fade()) {
		xfilm_Start_Film_Fade(false);
	}
	film->cur_cel++;
}

void xfilm_Rewind_View_Film(Film* film, FilmObject* obj, void* data) {
	obj->offset = 0;
	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		xfilm_Step_View_Film(film, obj, data);
	}
}

void xfilm_Rewind_Palette_Film(Film* film, FilmObject* obj, void* data) {
	obj->offset = 0;
	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		xfilm_Step_Palette_Film(film, obj, data);
	}
}

void xfilm_Rewind_Actor_Film(Film* film, FilmObject* obj, void* data) {
	Actor* actor = obj->object;
	xcanvas_Get_Drawing_Canvas_Clip(&actor->frame);
	xactor_Set_Actor_Pos(actor, 0, 0, 0, 0);
	xactor_Set_Actor_Speed(actor, 0, 0, 0, 0);
	xactor_Set_Actor_State(actor, 0, 0);
	xactor_Set_Actor_State_Speed(actor, 0, 0);
	xactor_Set_Actor_ZPlane(actor, 0);
	xactor_Set_Actor_Flip(actor, 0, 0);
	if (xactor_Is_Actor_Visible(actor)) {
		xactor_Hide_Actor(actor);
	}
	actor->var1 = 0;
	actor->var2 = 0;
	obj->offset = 0;
	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		xfilm_Step_Actor_Film(film, obj, data);
	}
}

void xfilm_Rewind_Sound_Film(Film* film, FilmObject* obj, void* data) {
	obj->offset = 0;
	Sound* snd = obj->object;
	xsound_Stop_Landru_Sound();
	snd->var1 = 0;
	snd->var2 = 0;
	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		xfilm_Step_Sound_Film(film, obj, data);
	}
}

void xfilm_Step_Film_Objects(Film* film) {
	LandruHandle* array = xmemhdl_Lock_Handle(film->array);
	if (!xfade_Fade_Active()) {
		xpal_Screen_To_Dest_Palette(0, 0, 255);
		xpal_Screen_To_Src_Palette(0, 0, 255);
	}
	for (int i = 0; i < film->array_size; i++) {
		FilmObject* obj = xmemhdl_Lock_Handle(array[i]);
		if (!obj)
			continue;
		switch (obj->id) {
			case FTC_VIEW:
				xfilm_Step_View_Film(film, obj, (uint8_t*)(obj + 1));
				break;
			case FTC_ACTOR:
				xfilm_Step_Actor_Film(film, obj, (uint8_t*)(obj + 1));
				break;
			case FTC_PALETTE:
				xfilm_Step_Palette_Film(film, obj, (uint8_t*)(obj + 1));
				break;
			case FTC_SOUND:
				xfilm_Step_Sound_Film(film, obj, (uint8_t*)(obj + 1));
				break;
		}
	}

	if (!xfade_Fade_Active()) {
		if (!xpal_Compare_Src_Dest_Palette()) {
			if (xfilm_Is_Film_Fade()) {
				xfilm_Start_Film_Fade(1);
			} else if (!xfade_Fade_Active()) {
				xfade_Start_Color_Fade(1, 1, 1, 0, 1);
			} else {
				xpal_Dest_To_Screen_Palette(0, 0, 255);
				xpal_Put_Screen_Palette();
			}
		} else if (xfilm_Is_Film_Fade()) {
			xfilm_Start_Film_Fade(0);
		}
	}
	film->cur_cel++;
}

void xfilm_Step_View_Film(Film* film, FilmObject* obj, void* data) {
	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		while (true) {
			xfilm_Step_Film_Frame(obj, data);
			if (xfilm_Is_Film_Next_Frame(obj, data))
				break;
			xfilm_Set_View_To_Film(film, obj, data);
		}
	}
}

void xfilm_Set_View_To_Film(Film* film, FilmObject* obj, void* data) {
	int16_t* chunk = (int16_t*)((uint8_t*)data + obj->offset);
	uint8_t* bChunk = (uint8_t*)chunk;
	uint16_t type = chunk[1];

	switch (type) {
		case FCMD_VIEW_SETRGB: {
			uint16_t start = bChunk[4];
			uint16_t stop = bChunk[5];
			uint16_t r = bChunk[6];
			uint16_t g = bChunk[7];
			uint16_t b = bChunk[8];
			xpal_Set_Dest_Pal_Color(start, stop, r, g, b);
		} break;
		case FCMD_VIEW_DEFPAL:
			xpal_Set_Dest_Pal_Color(0, 255, 0, 0, 0);
			if (film->def_palette) {
				xpal_Set_Dest_Palette(film->def_palette);
			}
			break;
		case FCMD_VIEW_FADE:
			film_fade_gbl = chunk[2] & 0xff;
			film_color_fade_gbl = chunk[3] & 0xff;
			break;
	}
}

void xfilm_Step_Palette_Film(Film* film, FilmObject* obj, void* data) {
	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		while (true) {
			xfilm_Step_Film_Frame(obj, data);
			if (xfilm_Is_Film_Next_Frame(obj, data))
				break;
			xfilm_Set_Palette_To_Film(obj, data);
		}
	}
}

void xfilm_Set_Palette_To_Film(FilmObject* obj, void* data) {
	int16_t* chunk = (int16_t*)((uint8_t*)data + obj->offset);
	if (chunk[1] == FCMD_PALETTE_SET) {
		xpal_Set_Dest_Palette(obj->object);
	}
}

void xfilm_Step_Actor_Film(Film* film, FilmObject* obj, void* data) {
	Actor* actor = obj->object;
	if (actor && actor->update) {
		actor->update(actor);
	}

	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		while (true) {
			xfilm_Step_Film_Frame(obj, data);
			if (xfilm_Is_Film_Next_Frame(obj, data))
				break;
			xfilm_Set_Actor_To_Film(obj, data);
		}
	}
}

void xfilm_Set_Actor_To_Film(FilmObject* obj, void* data) {
	int16_t* chunk = (int16_t*)((uint8_t*)data + obj->offset);
	uint16_t type = chunk[1];
	Actor* actor = obj->object;
	chunk += 2;

	switch (type) {
		case FCMD_ACTOR_POS:
			xactor_Set_Actor_Pos(actor, chunk[0], chunk[1], chunk[2], chunk[3]);
			break;
		case FCMD_ACTOR_VEL:
			xactor_Set_Actor_Speed(actor, chunk[0], chunk[1], chunk[2], chunk[3]);
			break;
		case FCMD_ACTOR_Z:
			xactor_Set_Actor_ZPlane(actor, chunk[0]);
			break;
		case FCMD_ACTOR_STATE:
			xactor_Set_Actor_State(actor, chunk[0], chunk[1]);
			break;
		case FCMD_ACTOR_STATEV:
			xactor_Set_Actor_State_Speed(actor, chunk[0], chunk[1]);
			break;
		case FCMD_ACTOR_VAR1:
			actor->var1 = chunk[0];
			break;
		case FCMD_ACTOR_VAR2:
			actor->var2 = chunk[0];
			break;
		case FCMD_ACTOR_CLIP:
			xrect_Set_Rect(&actor->frame, chunk[0], chunk[1], chunk[2], chunk[3]);
			break;
		case FCMD_ACTOR_CLIPV:
			xrect_Set_Rect(&actor->frame_v, chunk[0], chunk[1], chunk[2], chunk[3]);
			break;
		case FCMD_ACTOR_SHOW:
			if (chunk[0]) {
				xactor_Show_Actor(actor);
			} else {
				xactor_Hide_Actor(actor);
			}
			break;
		case FCMD_ACTOR_FLIP:
			xactor_Set_Actor_Flip(actor, chunk[0], chunk[1]);
			break;
	}
}

void xfilm_Step_Sound_Film(Film* film, FilmObject* obj, void* data) {
	if (xfilm_Is_Film_Time_Stamp(film, obj, data)) {
		while (true) {
			xfilm_Step_Film_Frame(obj, data);
			if (xfilm_Is_Film_Next_Frame(obj, data))
				break;
			xfilm_Set_Sound_To_Film(obj, data);
		}
	}
}

void xfilm_Set_Sound_To_Film(FilmObject* obj, void* data) {
	int16_t* chunk = (int16_t*)((uint8_t*)data + obj->offset);
	uint16_t type = chunk[1];
	Sound* sound = obj->object;
	chunk += 2;

	switch (type) {
		case FCMD_SOUND_START:
			if (sound->var2 == 1) {
				xsound_Start_Speech(sound);
			} else {
				xsound_Start_SFX(sound);
			}
			break;
		case FCMD_SOUND_STOP:
			xsound_Stop_Sound(sound);
			break;
		case FCMD_SOUND_VOLUME:
			xsound_Set_Sound_Volume(sound, chunk[0]);
			break;
		case FCMD_SOUND_FADE:
			xsound_Set_Sound_Fade(sound, chunk[0], chunk[1]);
			break;
		case FCMD_SOUND_VAR1:
			sound->var1 = chunk[0];
			if (sound->var1 == 1) {
				xsound_Set_Sound_Keep(sound);
			}
			break;
		case FCMD_SOUND_VAR2:
			sound->var2 = chunk[0];
			break;
		case FCMD_SOUND_CMD:
			if (chunk[0]) {
				if (sound->var2 == 1) {
					xsound_Start_Speech(sound);
				} else {
					xsound_Start_SFX(sound);
				}
			}
			if (chunk[1]) {
				xsound_Set_Sound_Volume(sound, chunk[1]);
			}
			if (chunk[2] || chunk[3]) {
				xsound_Set_Sound_Fade(sound, chunk[2], chunk[3]);
			}
			break;
		case FCMD_SOUND_CMD2:
			if (chunk[0]) {
				if (sound->var2 == 1) {
					xsound_Start_Speech(sound);
				} else {
					xsound_Start_SFX(sound);
				}
			}
			if (chunk[1]) {
				xsound_Set_Sound_Volume(sound, chunk[1]);
			}
			if (chunk[2] || chunk[3]) {
				xsound_Set_Sound_Fade(sound, chunk[2], chunk[3]);
			}
			if (chunk[4]) {
				xsound_Set_Sound_Pan(sound, chunk[4]);
			}
			if (chunk[5] || chunk[6]) {
				xsound_Set_Sound_Pan_Fade(sound, chunk[5], chunk[6]);
			}
			break;
	}
}

void xfilm_Step_Film_Frame(FilmObject* obj, void* data) {
	int16_t* chunk = (int16_t*)((uint8_t*)data + obj->offset);
	obj->offset += chunk[0];
}

bool xfilm_Is_Film_Time_Stamp(Film* film, FilmObject* obj, void* data) {
	int16_t* chunk = (int16_t*)((uint8_t*)data + obj->offset);
	return (chunk[1] == FCMD_TIMESTAMP) && (chunk[2] == film->cur_cel);
}

bool xfilm_Is_Film_Next_Frame(FilmObject* obj, void* data) {
	int16_t* chunk = (int16_t*)((uint8_t*)data + obj->offset);
	return (chunk[1] == FCMD_TIMESTAMP) || (chunk[1] == FCMD_END);
}

void xfilm_Set_Film_Frame(Film* film, Rect* frame) { xrect_Copy_Rect(&film->frame, frame); }

void xfilm_Get_Film_Frame(Film* film, Rect* out) { xrect_Copy_Rect(out, &film->frame); }

void xfilm_Set_Film_Pos(Film* film, int16_t x, int16_t y) {
	film->x = x;
	film->y = y;
}

void xfilm_Get_Film_Pos(Film* film, int16_t* x, int16_t* y) {
	*x = film->x;
	*y = film->y;
}

void xfilm_Set_Film_ZPlane(Film* film, int16_t z) {
	film->zplane = z;
	film_zplane_gbl = 1;
}

void xfilm_Get_Film_ZPlane(Film* film, int16_t* z) { *z = film->zplane; }

void xfilm_Set_Film_Time(Film* film, int32_t s, int32_t e) {
	film->start = s;
	film->stop = e;
}

void xfilm_Get_Film_Time(Film* film, int32_t* s, int32_t* e) {
	*s = film->start;
	*e = film->stop;
}

void xfilm_Set_Film_Array(Film* film, LandruHandle a) { film->array = a; }

void xfilm_Get_Film_Array(Film* film, LandruHandle* out) { *out = film->array; }

Palette* xfilm_Get_Film_Def_Palette(Film* film) { return film->def_palette; }

void xfilm_Set_Film_Draw_Function(Film* film, xfilmDrawFunc f) { film->draw = f; }

void xfilm_Get_Film_Draw_Function(Film* film, xfilmDrawFunc* f) { *f = film->draw; }

void xfilm_Set_Film_Update_Function(Film* film, xfilmUpdateFunc f) { film->update = f; }

void xfilm_Get_Film_Update_Function(Film* film, xfilmUpdateFunc* f) { *f = film->update; }

void xfilm_Set_Film_User_Function(Film* film, xfilmCallback f) { film->user = f; }

void xfilm_Get_Film_User_Function(Film* film, xfilmCallback* f) { *f = film->user; }

void xfilm_Show_Film(Film* film) { film->flags |= FF_VISIBLE; }

void xfilm_Hide_Film(Film* film) { film->flags &= ~FF_VISIBLE; }

void xfilm_Activate_Film(Film* film) { film->flags |= FF_ACTIVE; }

void xfilm_Deactivate_Film(Film* film) { film->flags &= ~FF_ACTIVE; }

void xfilm_Refresh_Film(Film* film) { film->flags |= FF_REFRESH; }

void xfilm_Refreshable_Film(Film* film) { film->flags |= FF_REFRESHABLE; }

void xfilm_Non_Refreshable_Film(Film* film) { film->flags &= ~FF_REFRESHABLE; }

void xfilm_Non_Discard_Film_Data(Film* film) { film->flags &= ~FF_DISCARD; }

void xfilm_Repeat_Film(Film* film) { film->flags |= FF_REPEAT; }

void xfilm_Non_Repeat_Film(Film* film) { film->flags &= ~FF_REPEAT; }

void xfilm_Set_Film_Flag1(Film* film) { film->flags |= FF_FLAG1; }

void xfilm_Clear_Film_Flag1(Film* film) { film->flags &= ~FF_FLAG1; }

bool xfilm_Is_Film_Flag1(Film* film) { return film->flags & FF_FLAG1; }

void xfilm_Set_Film_Flag2(Film* film) { film->flags |= FF_FLAG2; }

void xfilm_Clear_Film_Flag2(Film* film) { film->flags &= ~FF_FLAG2; }

bool xfilm_Is_Film_Flag2(Film* film) { return film->flags & FF_FLAG2; }

void xfilm_Set_Film_Fade(uint8_t fade, uint8_t colorFade) {
	film_fade_gbl = fade;
	film_color_fade_gbl = colorFade;
}

static void sort_film_list(void) {
	Film* sorted = NULL;
	Film* cur = first_film_gbl;
	while (cur) {
		Film* node = cur;
		cur = cur->next;
		node->next = NULL;
		Film *pos = sorted, *prev = NULL;
		while (pos && pos->zplane >= node->zplane) {
			prev = pos;
			pos = pos->next;
		}
		if (prev)
			prev->next = node;
		else
			sorted = node;
		if (pos)
			node->next = pos;
	}
	first_film_gbl = sorted;
}

void xfilm_Sort_Film_ZPlanes(void) { sort_film_list(); }

/* --- Render capture --- */

void xfilm_emit_render_state(void) {
	bool any_film = false;
	for (Film* f = first_film_gbl; f; f = f->next) {
		any_film = true;
		LandruFilmRenderState state = { 0 };
		LandruFilmRenderState* out = &state;
		out->res_type = f->res_type;
		memcpy(out->res_name, f->res_name, sizeof out->res_name);
		out->x = f->x;
		out->y = f->y;
		out->zplane = f->zplane;
		out->cur_cel = f->cur_cel;
		out->cels = f->cels;
		out->flags = f->flags;
		landru_render_film(out);
	}

	/* Publish the current engine frame and sub-cel progress so consumers may
	 * interpolate presentation without retiming the simulation. */
	if (any_film)
		landru_render_scene_clock(xview_Get_View_Time(), xtimer_Frame_Progress(), xtimer_Frame_Period_Us());
}

#ifdef LANDRU_GAME_XW
void xfilm_Draw_System_Films(void) {
	xfilm_Draw_Films(first_film_gbl, view_gbl->refresh_films);
	view_gbl->refresh_films = 0;
}
#endif

#ifdef LANDRU_GAME_XW
void xfilm_ResetSceneState(void) {
	first_film_gbl = NULL;
	film_fade_gbl = 0;
	film_color_fade_gbl = 0;
	film_zplane_gbl = 0;
}
#endif
