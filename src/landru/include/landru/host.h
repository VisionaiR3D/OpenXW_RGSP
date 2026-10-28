#ifndef LANDRU_HOST_H
#define LANDRU_HOST_H

#include <landru/config.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void LandruFile;
typedef void LandruDir;

typedef enum LandruFileRoot {
	LANDRU_FILE_ROOT_ASSET,
	LANDRU_FILE_ROOT_AUXILIARY_ASSET,
	LANDRU_FILE_ROOT_USER,
	LANDRU_FILE_ROOT_TEMP,
} LandruFileRoot;

enum {
	LANDRU_SEEK_SET = 0,
	LANDRU_SEEK_CUR = 1,
	LANDRU_SEEK_END = 2,
};

#define LANDRU_DIR_NAME_MAX 256

typedef struct LandruDirEntry {
	char name[LANDRU_DIR_NAME_MAX];
	int is_dir;
#if defined(__WATCOMC__)
	uint32_t size;
#else
	uint64_t size;
#endif
} LandruDirEntry;

typedef enum LandruLogLevel {
	LANDRU_LOG_TRACE,
	LANDRU_LOG_INFO,
	LANDRU_LOG_WARN,
	LANDRU_LOG_ERROR,
} LandruLogLevel;

typedef struct LandruPlatformVideo {
	void (*set_mode)(void* userdata, uint16_t mode);
	void (*lock)(void* userdata);
	void (*unlock)(void* userdata);
	void (*copy_to_present_surface)(void* userdata);
	void (*present)(void* userdata);
#ifdef LANDRU_MODERN
	void (*logical_size_changed)(void* userdata, int width, int height);
#endif
} LandruPlatformVideo;

struct Sound;
struct ResFile;

typedef struct LandruHost {
	void* userdata;

	void (*log)(void* userdata, LandruLogLevel level, const char* format, va_list args);

	LandruFile* (*file_open)(void* userdata, LandruFileRoot root, const char* path, const char* mode);
	size_t (*file_read)(void* userdata, void* buffer, size_t size, size_t count, LandruFile* file);
	size_t (*file_write)(void* userdata, const void* buffer, size_t size, size_t count, LandruFile* file);
	int (*file_seek)(void* userdata, LandruFile* file, long offset, int origin);
	long (*file_tell)(void* userdata, LandruFile* file);
	int (*file_close)(void* userdata, LandruFile* file);

	LandruDir* (*dir_open)(void* userdata, LandruFileRoot root, const char* path);
	int (*dir_next)(void* userdata, LandruDir* dir, LandruDirEntry* entry);
	void (*dir_close)(void* userdata, LandruDir* dir);
	int (*path_is_dir)(void* userdata, LandruFileRoot root, const char* path);

	int (*key_pending)(void* userdata);
	int (*key_read)(void* userdata);
	int (*modifier_keys)(void* userdata);

	void (*mouse_position)(void* userdata, int16_t* buttons, int16_t* x, int16_t* y);
	void (*mouse_movement)(void* userdata, int16_t* x, int16_t* y);
	void (*mouse_set_position)(void* userdata, int16_t x, int16_t y);
	void (*mouse_show)(void* userdata, bool show);

	int (*joystick_count)(void* userdata);
	void (*joystick_read)(void* userdata, int port, int16_t* axes, int axis_count, uint16_t* buttons);
#ifdef LANDRU_GAME_XW
	/* Optional game-facing read. On failure write LANDRU_JOYSTICK_READ_FAILED
	 * to all axes and return zero. When polling is suppressed, preserve the
	 * supplied axes and return the cached button/POV mask. */
	uint32_t (*joystick_read_xw)(void* userdata, int32_t* x, int32_t* y, int32_t* z, int16_t device);
#endif

	void (*palette_set)(void* userdata, const uint8_t* rgb, int start, int count);
	void (*frontend_audio_pump)(void* userdata);
	/* Optional music arrangement/ownership adapter. NULL retains GMID loading. */
	uint32_t (*music_type)(void* userdata);
	struct Sound* (*music_load)(void* userdata, struct ResFile* resource, const char* name);
	void (*music_release)(void* userdata, struct Sound* sound);
	LandruPlatformVideo video;
	uint64_t (*now_us)(void* userdata);
} LandruHost;

/* Install the host before creating modules or resource handles. The callback
 * table is copied; userdata remains owned by the caller. */
bool landru_set_host(const LandruHost* host);
/* Stop tasks/audio and destroy modules first. Clears the host and releases
 * any remaining resource handles; do not use this for scene cleanup. */
void landru_clear_host(void);

#endif
