#include "xw_remaster/dos_scene.h"
#include "xw_remaster/component_animation.h"
#include "xw_remaster/config.h"
#include "xw_remaster/dos_draw.h"
#include "xw_remaster/dos_sky.h"
#include "xw_remaster/dos_sprites.h"
#include "xw_remaster/hud_renderer.h"
#include "xw_remaster/special_world.h"
#include <aeron/aeron.h>
#include <aeron/scene/draw_list2d.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "xw_runtime/snapshot/render_dos_assets.h"

typedef struct DosFailure {
	uint64_t mission, world, config;
	XwRenderAssetId assets;
	int width, height;
	bool valid;
} DosFailure;

static AeronScene3D* scene;
static XwFlightOutput output;
static uint64_t output_revision;
static DosFailure failed;
static int width, height, samples;
static uint64_t palette_revision;
static AeronDrawList2D* bars;

void XwDosScene_Invalidate(void) { memset(&output, 0, sizeof output); }

static bool Ensure(int w, int h, int msaa) {
	if (scene && width == w && height == h && samples == msaa)
		return true;
	AeronScene3D* next =
		AeronScene_Create(&(AeronScene3DDesc) { .rt_width = w,
												.rt_height = h,
												.color_format = AERON_TEXTURE_FORMAT_RGBA16_FLOAT,
												.sample_count = (AeronSampleCount)msaa,
												.view_space_to_meters = 1 });
	if (!next)
		return false;
	AeronScene_SetClearColor(next, (const float[4]) { 0, 0, 0, 1 });
	AeronScene_SetTemporal(next, &(AeronSceneTemporalDesc) { .mode = AERON_TEMPORAL_OFF });
	AeronScene_SetPost(next, &(AeronScenePostDesc) { 0 });
	AeronScene_SetPassHook(next, AERON_SCENE_HOOK_BEFORE_OPAQUE, XwDosSky_Draw, NULL);
	AeronScene_Destroy(scene);
	scene = next;
	width = w;
	height = h;
	samples = msaa;
	XwDosScene_Invalidate();
	return true;
}

bool XwDosScene_PrepareResources(int w, int h) {
	int msaa = XwRemasterConfig_Effective()->dos_msaa_samples;
	return Ensure(w, h, msaa) && XwDosDraw_PrepareResources(AeronScene_SampleCount(scene)) &&
		   XwDosSprites_Begin(AeronScene_SampleCount(scene));
}

static bool Fail(DosFailure key, const char* stage) {
	Aeron_LogError("xw.render", "DOS scene failed: stage=%s mission=%llu world=%llu assets=%llu size=%dx%d", stage, (unsigned long long)key.mission, (unsigned long long)key.world, (unsigned long long)key.assets, key.width, key.height);
	failed = key;
	XwSpecialWorld_Shutdown();
	XwDosScene_Invalidate();
	XwDosSky_Shutdown();
	char context[200];
	snprintf(context,sizeof context,"DOS flight scene preparation: %s",stage);
	Aeron_RequestFatalRendererError(context);
	return false;
}

static void Hud(AeronCommandBuffer* cmd, AeronRenderPass* pass, int w, int h, void* user) {
	(void)w;
	(void)h;
	XwHudRenderer_Draw(cmd, pass, AeronScene_SceneRt(user));
	AeronDrawList_RenderIntoPass(bars, cmd, pass, AeronScene_SceneRt(user));
}

static void Effects(AeronCommandBuffer* cmd, AeronRenderPass* pass, int w, int h, void* user) {
	(void)cmd;
	(void)w;
	(void)h;
	(void)user;
	XwDosSprites_Draw(pass, false);
}

static bool PrepareBars(AeronCommandBuffer* cmd, const XwPreparedFlight* frame) {
	if (!bars)
		bars = AeronDrawList_Create(4);
	if (!bars)
		return false;
	int w = frame->view.camera.viewport.width, h = frame->view.camera.viewport.height;
	AeronRectI r = frame->content_rect;
	const float black[4] = { 0, 0, 0, 1 };
	AeronDrawList_Begin(bars, NULL, w, h, AERON_DRAWLIST2D_LOAD, NULL);
	if (r.x > 0)
		AeronDrawList_AddFill(bars, 0, 0, r.x, h, black, AERON_BLIT2D_BLEND_NONE, NULL);
	if (r.x + r.width < w)
		AeronDrawList_AddFill(bars, r.x + r.width, 0, w - r.x - r.width, h, black, AERON_BLIT2D_BLEND_NONE,
							  NULL);
	if (r.y > 0)
		AeronDrawList_AddFill(bars, r.x, 0, r.width, r.y, black, AERON_BLIT2D_BLEND_NONE, NULL);
	if (r.y + r.height < h)
		AeronDrawList_AddFill(bars, r.x, r.y + r.height, r.width, h - r.y - r.height, black,
							  AERON_BLIT2D_BLEND_NONE, NULL);
	return AeronDrawList_Prepare(bars, cmd);
}

static void Markers(AeronCommandBuffer* cmd, AeronRenderPass* pass, int w, int h, void* user) {
	(void)w;
	(void)h;
	XwHudRenderer_DrawWorldMarkers(cmd, pass, AeronScene_ColorRt(user));
}

static bool Select(const XwRenderSnapshot* s, const XwPreparedFlight* frame) {
	const XwRenderAssetSetView* set = XwRenderAssets_Set(s->flight_assets);
	if (!set || set->failed || set->bindings.flight_version != s->flight_version)
		return false;
	XwDosSelection selection;
	for (unsigned i = 0; i < s->object_count; ++i) {
		if (!XwDosShip_Select(s, &set->bindings, &s->objects[i], &frame->view,
							  XwComponentAnimation_Angles(&s->objects[i], false), &selection)) {
			const XwSnapObject* o=&s->objects[i];
			Aeron_LogError("xw.render","Ship selection failed: kind=%u slot=%u generation=%u type=%u genus=%u source_type=%u craft=%u animation=%u secondary=%u state=%u objects=%u crafts=%u",o->id.kind,o->id.slot,o->id.generation,o->type,o->genus,o->source_type,o->craft_index,o->animation_state,o->secondary_animation_state,o->state,s->object_count,s->craft_count);
			if(o->type<set->bindings.type_count){
			 const XwSnapType* t=&set->bindings.types[o->type];
			 const XwRenderSource* g=XwRenderAssets_Source(t->geometry);
			 const XwRenderSource* d=XwRenderAssets_Source(t->dos_components);
			 if(g&&g->kind==XW_SOURCE_DOS_MODEL&&g->size==sizeof(XwRenderDosModel)){
			  const XwRenderDosModel* m=g->data;
			  Aeron_LogError("xw.render","Model metadata: geometry=%llu components=%u nodes=%u lods=%u clamp=%u",(unsigned long long)t->geometry,m->component_count,m->node_count,m->lod_count,m->clamp_components);
			 }
			 if(d&&d->kind==XW_SOURCE_DOS_COMPONENTS&&d->size>=sizeof(XwRenderDosDescriptors)){
			  const XwRenderDosDescriptors* desc=d->data;
			  Aeron_LogError("xw.render","Descriptor metadata: count=%u components=%u variants=%u",desc->count,desc->component_count,desc->variant_count);
			 }
			}
			if(o->craft_index<s->craft_count){
			 const XwSnapCraft* c=&s->crafts[o->craft_index];
			 for(unsigned j=0;j<c->component_count&&j<XW_SNAP_COMPONENTS;j++)
			  Aeron_LogError("xw.render","Component %u: state=%u hp=%u rotation=%u",j,c->component_state[j],c->component_hp[j],c->mesh_rotation[j]);
			}
			return false;
		}
		if (!XwDosSprites_Effects(s, &frame->view, &selection)) {
			Aeron_LogError("xw.render","Object effects failed: slot=%u type=%u bitmaps=%u",s->objects[i].id.slot,s->objects[i].type,selection.bitmap_count);
			return false;
		}
		for (unsigned part = 0; part < selection.part_count; ++part)
			if (!XwDosDraw_Add(&selection.parts[part])) {
				Aeron_LogError("xw.render","Mesh submission failed: slot=%u type=%u part=%u geometry=%llu lod=%u component=%u",s->objects[i].id.slot,s->objects[i].type,part,(unsigned long long)selection.parts[part].geometry,selection.parts[part].lod,selection.parts[part].component);
				return false;
			}
	}
	return true;
}

bool XwDosScene_Frame(const XwRenderSnapshot* s, const XwPreparedFlight* frame) {
	if (!s || (s->flight_version != 93 && s->flight_version != 94) || !s->world_valid || !s->hud_valid ||
		s->owner != XW_SNAP_OWNER_FLIGHT || !frame || !frame->valid || !frame->drawable ||
		frame->assets != s->flight_assets || frame->host_serial != s->host_serial ||
		frame->object_count != s->object_count || s->object_count > XW_SNAP_OBJECTS ||
		s->craft_count > XW_SNAP_CRAFTS) {
		XwDosScene_Invalidate();
		return false;
	}
	int w = frame->view.camera.viewport.width, h = frame->view.camera.viewport.height;
	DosFailure key = { .mission = s->key.mission_generation,
					   .world = s->key.world_generation,
					   .config = XwRemasterConfig_Generation(),
					   .assets = s->flight_assets,
					   .width = w,
					   .height = h,
					   .valid = true };
	if (failed.valid && failed.mission == key.mission && failed.world == key.world &&
		failed.config == key.config && failed.assets == key.assets && failed.width == w && failed.height == h)
		return false;
	int msaa = XwRemasterConfig_Effective()->dos_msaa_samples;
	bool reset =
		!scene || width != w || height != h || samples != msaa || frame->reset_history || !output.texture;
	if (!reset && !frame->render_needed && palette_revision == s->appearance.palette_revision)
		return true;
	XwDosScene_Invalidate();
	if (!Ensure(w,h,msaa)) return Fail(key,"render target allocation");
	if (!AeronScene_Begin(scene,&frame->view.camera)) return Fail(key,"scene begin");
	AeronCommandBuffer* cmd=Aeron_AcquireCommandBuffer();
	if (!cmd) return Fail(key,"command buffer acquisition");
#define XW_PREPARE_STAGE(label, expression) do { if (!(expression)) { Aeron_CancelCommandBuffer(cmd); return Fail(key,label); } } while(0)
	XW_PREPARE_STAGE("mesh resources", XwDosDraw_Begin(cmd,s,&frame->view,(AeronSampleCount)msaa));
	XW_PREPARE_STAGE("sprite resources", XwDosSprites_Begin((AeronSampleCount)msaa));
	XW_PREPARE_STAGE("object selection and submission", Select(s,frame));
	XW_PREPARE_STAGE("special world", XwSpecialWorld_Prepare(cmd,scene,s,frame,reset));
	XW_PREPARE_STAGE("sky", XwDosSky_Prepare(cmd,scene,s,&frame->view));
	XW_PREPARE_STAGE("sprite upload", XwDosSprites_Upload(cmd));
	XW_PREPARE_STAGE("letterbox bars", PrepareBars(cmd,frame));
	XW_PREPARE_STAGE("mesh upload", XwDosDraw_Upload(cmd));
	XW_PREPARE_STAGE("target markers", XwHudRenderer_PrepareWorldMarkers(cmd,s,&frame->view,scene));
#undef XW_PREPARE_STAGE
	unsigned background = s->special.surface_active && s->camera.rows[8] < -0.75f ? 0xB1 : 0xF7;
	float clear[4] = { 0, 0, 0, 1 };
	for (unsigned a = 0; a < 3; ++a) {
		float value = ((s->appearance.palette_argb[background] >> (16 - 8 * a)) & 255) / 255.0f;
		clear[a] = value <= .04045f ? value / 12.92f : powf((value + .055f) / 1.055f, 2.4f);
	}
	AeronScene_SetClearColor(scene, clear);
	AeronScene_SetPassHook(scene, AERON_SCENE_HOOK_AFTER_OPAQUE, Effects, NULL);
	AeronScene_SetPassHook(scene, AERON_SCENE_HOOK_AFTER_TRANSPARENT, Markers, scene);
	AeronScene_SetPassHook(scene, AERON_SCENE_HOOK_AFTER_UPSCALE, Hud, scene);
	if (!AeronScene_Render(scene, cmd)) {
		Aeron_CancelCommandBuffer(cmd);
		return Fail(key,"scene rendering");
	}
	if (!Aeron_SubmitCommandBuffer(cmd))
		return Fail(key,"command buffer submission");
	output = (XwFlightOutput) { .texture = Aeron_RenderTargetGetTexture(AeronScene_SceneRt(scene)),
								.key = s->key,
								.width = w,
								.height = h,
								.revision = ++output_revision,
								.color_space = AERON_COLOR_SPACE_LINEAR_DISPLAY };
	palette_revision = s->appearance.palette_revision;
	failed.valid = false;
	return output.texture != NULL;
}

const XwFlightOutput* XwDosScene_Output(void) { return output.texture ? &output : NULL; }

void XwDosScene_Shutdown(void) {
	XwDosSky_Shutdown();
	XwDosSprites_Shutdown();
	AeronDrawList_Destroy(bars);
	bars = NULL;
	AeronScene_Destroy(scene);
	scene = NULL;
	XwDosDraw_Shutdown();
	width = height = samples = 0;
	palette_revision = 0;
	memset(&failed, 0, sizeof failed);
	XwDosScene_Invalidate();
}
