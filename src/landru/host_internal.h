#ifndef LANDRU_HOST_INTERNAL_H
#define LANDRU_HOST_INTERNAL_H

#include <landru/host.h>

void landru_host_log(LandruLogLevel level, const char* format, ...);
LandruFile* landru_host_file_open(LandruFileRoot root, const char* path, const char* mode);
size_t landru_host_file_read(void* buffer, size_t size, size_t count, LandruFile* file);
size_t landru_host_file_write(const void* buffer, size_t size, size_t count, LandruFile* file);
int landru_host_file_seek(LandruFile* file, long offset, int origin);
long landru_host_file_tell(LandruFile* file);
int landru_host_file_close(LandruFile* file);
LandruDir* landru_host_dir_open(LandruFileRoot root, const char* path);
int landru_host_dir_next(LandruDir* dir, LandruDirEntry* entry);
void landru_host_dir_close(LandruDir* dir);
int landru_host_path_is_dir(LandruFileRoot root, const char* path);
int landru_host_key_pending(void);
int landru_host_key_read(void);
int landru_host_modifier_keys(void);
void landru_host_mouse_position(int16_t* buttons, int16_t* x, int16_t* y);
void landru_host_mouse_movement(int16_t* x, int16_t* y);
void landru_host_mouse_set_position(int16_t x, int16_t y);
void landru_host_mouse_show(bool show);
int landru_host_joystick_count(void);
void landru_host_joystick_read(int port, int16_t* axes, int axis_count, uint16_t* buttons);
#ifdef LANDRU_GAME_XW
uint32_t landru_host_joystick_read_xw(int32_t* x, int32_t* y, int32_t* z, int16_t device);
#endif
void landru_host_palette_set(const uint8_t* rgb, int start, int count);
void landru_host_frontend_audio_pump(void);
uint32_t landru_host_music_type(void);
struct Sound* landru_host_music_load(struct ResFile* resource, const char* name);
bool landru_host_has_music_loader(void);
void landru_host_music_release(struct Sound* sound);
bool landru_host_has_platform_video(void);
void landru_host_video_set_mode(uint16_t mode);
void landru_host_video_lock(void);
void landru_host_video_unlock(void);
void landru_host_video_copy_to_present_surface(void);
void landru_host_video_present(void);
#ifdef LANDRU_MODERN
void landru_host_video_logical_size_changed(int width, int height);
#endif
uint64_t landru_host_now_us(void);

#endif
