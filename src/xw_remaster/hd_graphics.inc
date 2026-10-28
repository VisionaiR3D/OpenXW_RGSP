/* Included once by dos_mesh.c. Keep the renderer mode fixed for this process:
 * cached models and textures must never mix old and new preferences. */
#include "xw_remaster/hd_graphics.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>
#include <unistd.h>
static bool hd_initialized, hd_active = true, hd_selected = true;
static char hd_preference_path[1024];
static void HdGraphicsInit(void) {
 if (hd_initialized) return;
 hd_initialized = true;
 char* pref = SDL_GetPrefPath("TotallyOpen", "OpenXW-HD");
 if (!pref) return;
 int n = snprintf(hd_preference_path, sizeof hd_preference_path, "%shd-graphics.cfg", pref);
 SDL_free(pref);
 if (n < 0 || n >= (int)sizeof hd_preference_path) { hd_preference_path[0] = 0; return; }
 FILE* f = fopen(hd_preference_path, "r");
 if (f) {
  int value; char extra;
  if (fscanf(f, "%d %c", &value, &extra) == 1 && (value == 0 || value == 1))
   hd_active = hd_selected = value != 0;
  fclose(f);
 }
}
bool XwHdGraphics_Active(void) { HdGraphicsInit(); return hd_active; }
bool XwHdGraphics_Selected(void) { HdGraphicsInit(); return hd_selected; }
bool XwHdGraphics_Set(bool enabled) {
 HdGraphicsInit();
 if (!hd_preference_path[0]) return false;
 char tmp[1040]; snprintf(tmp, sizeof tmp, "%s.tmp", hd_preference_path);
 FILE* f = fopen(tmp, "w");
 if (!f) return false;
 bool ok = fprintf(f, "%d\n", enabled ? 1 : 0) > 0;
 if (fflush(f) != 0) ok = false;
 if (ok && fsync(fileno(f)) != 0) ok = false;
 if (fclose(f) != 0) ok = false;
 if (!ok || rename(tmp, hd_preference_path) != 0) { remove(tmp); return false; }
 hd_selected = enabled;
 return true;
}
