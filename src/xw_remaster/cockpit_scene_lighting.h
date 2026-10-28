#ifndef XW_COCKPIT_SCENE_LIGHTING_H
#define XW_COCKPIT_SCENE_LIGHTING_H
#include "xw_runtime/snapshot/render_types.h"
enum { COCKPIT_FLASH_CAP=4 };
typedef struct XwCockpitFlash {
 float direction[3],color[3],strength;
} XwCockpitFlash;
typedef struct XwCockpitEnvironment {
 float sun_visibility;
 unsigned flash_count;
 XwCockpitFlash flashes[COCKPIT_FLASH_CAP];
} XwCockpitEnvironment;
void XwCockpitScene_Prepare(const XwRenderSnapshot* snapshot,XwCockpitEnvironment* out);
void XwCockpitScene_Reset(void);
#endif
