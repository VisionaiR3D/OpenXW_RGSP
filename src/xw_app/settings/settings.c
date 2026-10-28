/* Settings lifecycle and editors adapted from OpenXvT. */
#include "xw_app/settings/settings.h"
#include "xw_remaster/hd_graphics.h"
#include "xw_remaster/cockpit_lighting.h"
#include "xw_runtime/input/combos.h"
#include "xw_app/settings/audio_page.h"
#include "xw_app/settings/controller_page.h"
#include "xw_app/settings/game_page.h"
#include "xw_app/settings/installation_page.h"
#include "xw_app/settings/keyboard_page.h"
#include "xw_app/settings/mouse_page.h"
#include "xw_app/settings/video_options.h"
#include "xw_app/settings/video_page.h"
#include "xw_runtime/config/config.h"
#include "xw_runtime/config/preference_apply.h"
#include "xw_runtime/config/preferences.h"
#include "xw_runtime/input/capture.h"
#include "xw_runtime/input/input_bridge.h"
#include "xw_runtime/runtime/flight_sim.h"
#include "xw_runtime/runtime/frontend_task.h"
#include "xw_runtime/runtime/port.h"
#include <aeron/log.h>
#include "xw/frontend/register.h"
#include <landru/inpattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool targets_open;
static void TargetsOpen(void);
static void TargetsFrame(const AeronInputSnapshot*,float);
static bool pilot_keyboard_pending,pilot_keyboard_open;
static void PilotKeyboard_Frame(const AeronInputSnapshot*,float);
static void PilotKeyboard_Close(bool);
#include "xw/flight/player/user.h"
#include "xw_runtime/input/energy_editor.h"
static bool energy_open;
static bool slots_open;
static void EnergyCommit(void);
static void EnergyOpen(void);
static bool quick_enabled,quick_return;
static int quick_group;
static unsigned quick_previous;
static char quick_notice[180];
static void XwQuick_Frame(const AeronInputSnapshot*,float);
static void QuickMenu_Reset(void);
static struct {
	AeronUiContext* ui;
	XwSettings draft;
	XwControllerSettings controller;
	XwKeyboardSettings keyboard;
	bool ready, open, close_requested, exit_requested, captures;
	int page, exit_confirmation, observed_fullscreen;
	char error[1024];

	struct {
		uint32_t instance;
		bool down, armed;
	} start[AERON_CONTROLLER_MAX];
} menu;

XwSettings* XwSettingsMenu_Draft(void) { return &menu.draft; }

float XwSettingsMenu_ScrollHeight(void) {
	const AeronUiTheme* theme = AeronUi_GetTheme(menu.ui);
	/* Reserve the scroll-row gap, 4-unit tab ending, 9-unit separator, and button row. */
	float footer = 4.0f + 9.0f + theme->row_height + 3.0f * theme->item_spacing;
	if (menu.error[0])
		footer += AeronUi_MeasureHelpHeight(menu.ui, menu.error, 0.0f) + theme->row_height +
				  2.0f * theme->item_spacing;
	float height = AeronUi_AvailableHeight(menu.ui) - footer;
	return height > 0.0f ? height : 1.0f;
}

bool XwSettingsMenu_Open(void) { return menu.open; }

bool XwSettingsMenu_CapturesKeyboard(void) { return menu.open && AeronUi_KeyboardCaptureActive(menu.ui); }

void XwSettingsMenu_ReportError(const char* error) {
	snprintf(menu.error, sizeof menu.error, "%s", error);
	menu.close_requested = menu.exit_requested = false;
}

bool XwSettingsMenu_Init(XwAppUi* ui, char* error, size_t capacity) {
	memset(&menu, 0, sizeof menu);
	menu.ui = XwAppUi_Context(ui);
	menu.ready = XwInstallationPage_Init(error, capacity);
	return menu.ready;
}

void XwSettingsMenu_Show(void) {
	if (!menu.ready || menu.open)
		return;
	menu.open = true;
 XwCombos_Suspend(true);
	menu.draft = *XwConfig_Settings();
	menu.observed_fullscreen = menu.draft.fullscreen = Aeron_Fullscreen();
	XwControllerSettings_Open(&menu.controller, &menu.draft);
	XwKeyboardSettings_Open(&menu.keyboard, &menu.draft);
	menu.error[0] = 0;
	XwPort_SetSettingsOpen(1);
	XwInput_BeginCaptureFrame(Aeron_InputSnapshot(), true);
	Aeron_SetHostCursorVisible(1);
}

void XwSettingsMenu_RequestClose(void) {
	if (menu.open)
		menu.close_requested = true;
}

static void CompleteClose(void) {
	XwKeyboardSettings_CancelCapture(&menu.keyboard, menu.ui);
	XwControllerSettings_CancelCapture(&menu.controller, menu.ui);
	XwInstallationPage_CancelPicker();
	if(quick_enabled) XwCombos_ResumeQueued();
 targets_open=false;slots_open=false;energy_open=false;quick_enabled=quick_return=false;
 XwCombos_Suspend(false);
 menu.open = menu.close_requested = menu.captures = false;
	menu.exit_confirmation = 0;
	XwPort_SetSettingsOpen(0);
}

static bool Commit(char* error, size_t capacity) {
	if (XwPreferences_HasPendingSave()) {
		snprintf(error, capacity, "%s", XwPreferences_SaveError());
		return false;
	}
	if (!XwInstallationPage_Validate(error, capacity))
		return false;
	menu.draft.controller = menu.controller.draft;
	menu.draft.keyboard = menu.keyboard.draft;
	if (!XwControllerOptions_Validate(&menu.draft.controller, error, capacity))
		return false;
	AeronConfigFile* candidate = NULL;
	AeronConfigError detail = { 0 };
	if (!AeronConfigFile_Clone(XwConfig_UserDocument(), &candidate, &detail))
		return XwSettings_FileError(&detail, error, capacity) != 0;
	bool ok = XwSettings_WriteDocument(candidate, &menu.draft, XwConfig_Settings(),
									   XwConfig_DefaultSettings(), error, capacity) != 0;
	XwSettings previous = *XwConfig_Settings();
	previous.fullscreen = Aeron_Fullscreen();
	bool video_changed =
		menu.draft.fullscreen != previous.fullscreen ||
		memcmp(&menu.draft.presentation, &previous.presentation, sizeof previous.presentation) != 0;
	if (ok && video_changed)
		ok = XwVideoOptions_Apply(&previous, &menu.draft, error, capacity);
	if (ok) {
		ok = XwConfig_UpdateUser(candidate, 1, error, capacity) != 0;
		if (!ok && video_changed) {
			char rollback_error[1024];
			if (!XwVideoOptions_Apply(&menu.draft, &previous, rollback_error, sizeof rollback_error))
				Aeron_RequestFatalRendererError(rollback_error);
		}
	}
	AeronConfigFile_Destroy(candidate);
	if (!ok)
		return false;
	XwPreferences_ApplyPending();
	menu.draft = *XwConfig_Settings();
	menu.controller.original = menu.controller.draft = menu.draft.controller;
	menu.controller.dirty = false;
	menu.keyboard.original = menu.keyboard.draft = menu.draft.keyboard;
	menu.keyboard.dirty = menu.keyboard.restore_defaults = false;
	menu.error[0] = 0;
	return true;
}

void XwSettingsMenu_FlushForExit(void) {
	if (!menu.ready)
		return;
	char error[1024];
	if (menu.open && !XwPreferences_HasPendingSave() && !Commit(error, sizeof error))
		Aeron_LogError("xw.settings", "Unsaved settings: %s", error);
	XwPreferences_Flush();
}

void XwSettingsMenu_Shutdown(void) {
	if (menu.ui) {
		XwKeyboardSettings_CancelCapture(&menu.keyboard, menu.ui);
		XwControllerSettings_CancelCapture(&menu.controller, menu.ui);
	}
	XwInstallationPage_Shutdown();
	memset(&menu, 0, sizeof menu);
}

static bool ControllerStart(const AeronInputSnapshot* input) {
	bool pressed = false;
	for (int i = 0; i < AERON_CONTROLLER_MAX; ++i) {
		const AeronControllerSnapshot* d = input ? &input->controllers[i] : NULL;
		bool eligible = d && d->connected && d->kind == AERON_CONTROLLER_KIND_GAMEPAD;
		uint32_t instance = eligible ? d->instance_id : 0;
		bool down = eligible && (d->gamepad_buttons & (1u << AERON_GAMEPAD_BUTTON_START));
		if (instance != menu.start[i].instance || !input || !input->has_focus) {
			menu.start[i].instance = instance;
			menu.start[i].armed = !down;
		} else if (!down)
			menu.start[i].armed = true;
		else if (menu.start[i].armed && !menu.start[i].down)
			pressed = true;
		menu.start[i].down = down;
	}
	return pressed;
}

void XwPilotKeyboard_Request(REGISTER_RegStringButton*,int);
static bool PilotNameInputVisible(Input* list,Input* target) {
 for(Input* i=list;i;i=i->next) {
  if(!(i->flags & INPUT_VISIBLE)) continue;
  if(i==target) return true;
  if(PilotNameInputVisible(i->child,target)) return true;
 }
 return false;
}
bool XwSettingsMenu_BeginFrame(const AeronInputSnapshot* input) {
 if(input && input->has_focus && !menu.open && !pilot_keyboard_pending &&
    g_RegisterPilotNameInput && XwFrontend_IsActive() &&
    PilotNameInputVisible(xinput_Get_Active_Input_List(),&g_RegisterPilotNameInput->input)) {
  for(int i=0;i<AERON_CONTROLLER_MAX;i++) {
   const AeronControllerSnapshot* d=&input->controllers[i];
   if(d->connected && !strcmp(d->name,"H700 Gamepad") &&
      (d->gamepad_pressed_buttons&(1u<<AERON_GAMEPAD_BUTTON_WEST)))
    XwPilotKeyboard_Request(g_RegisterPilotNameInput,g_RegisterPilotNameInput->is_filename_mode ? 8 : 20);
  }
 }


 if(XwCombos_TakeTargetListRequest() && !menu.open && input && input->has_focus && XwFlightSim_IsPlayerControl() && !XwFlightSim_IsPaused()){
  XwSettingsMenu_Show();quick_enabled=true;quick_return=false;QuickMenu_Reset();TargetsOpen();return true;
 }
	bool was_open = menu.open;
 static bool bindings_preview_done;
 if(!bindings_preview_done && input && input->has_focus && !menu.open &&
    getenv("RGSP_BINDINGS_PREVIEW") && XwFrontend_IsActive()) {
  bindings_preview_done=true; XwSettingsMenu_Show(); quick_enabled=false; menu.page=6; return true;
 }

 static bool command_preview_done;
 if(!command_preview_done && input && input->has_focus && !menu.open &&
    getenv("RGSP_COMMAND_MENU_PREVIEW") && XwFrontend_IsActive()) {
  command_preview_done=true;XwSettingsMenu_Show();
  quick_enabled=true;quick_return=false;QuickMenu_Reset();
  if(getenv("RGSP_ENERGY_EDITOR_PREVIEW")) EnergyOpen();
  return true;
 }

 if(pilot_keyboard_pending && !menu.open) {
  pilot_keyboard_pending=false; XwSettingsMenu_Show(); pilot_keyboard_open=true;
  return true;
 }
	int fullscreen = Aeron_Fullscreen();
	if (menu.open && fullscreen != menu.observed_fullscreen &&
		menu.draft.fullscreen == menu.observed_fullscreen)
		menu.draft.fullscreen = fullscreen;
	menu.observed_fullscreen = fullscreen;
	if (menu.open && !XwPreferences_HasPendingSave())
		XwControllerSettings_Discover(&menu.controller, menu.ui, input,
									  &XwConfig_DefaultSettings()->gamepad_defaults);
	if (menu.close_requested || menu.exit_requested) {
		char error[1024];
		if (Commit(error, sizeof error)) {
			if (menu.exit_requested)
				Aeron_RequestQuit();
			menu.exit_requested = false;
			if (menu.close_requested) {
    if(quick_return) { quick_return=false; quick_enabled=true; menu.close_requested=false; }
    else CompleteClose();
   }
		} else
			XwSettingsMenu_ReportError(error);
	}
	bool start = ControllerStart(input);
 if(menu.open && energy_open && start){EnergyCommit();return true;}
	if (input && input->has_focus) {
		if (menu.open && start && !pilot_keyboard_open && !menu.captures && !XwSettingsMenu_CapturesKeyboard() &&
			!XwInstallationPage_PickerOpen())
			XwSettingsMenu_RequestClose();
		else if (!menu.open && !was_open && start && XwFlightSim_IsPlayerControl() && !XwFlightSim_IsPaused()) {
   XwSettingsMenu_Show(); quick_enabled=true; quick_return=false; quick_group=-1;
   quick_previous=0; quick_notice[0]=0; XwCombos_Clear(); QuickMenu_Reset();
  }
  else if (!menu.open && !was_open && XwInput_SettingsShortcutAllowed()) {
			int key = XwKeyboardMapping_Trigger(input, XW_KEYBOARD_SHORTCUT_SETTINGS);
			if ((key >= 0 && !XwInput_KeyBlocked(key)) || (start && !XwFlightSim_IsActive())) {
				if (key >= 0)
					XwInput_SuppressKey(key);
				menu.page = XW_SETTINGS_GAME;
				XwSettingsMenu_Show();
			}
		}
	}
	return !was_open && menu.open;
}

bool XwSettingsMenu_ConsumeRuntimeRequest(void) {
	if (!XwPort_ConsumeSettingsRequest())
		return false;
	bool was_open = menu.open;
	menu.page = XwPort_SettingsPage();
	XwSettingsMenu_Show();
	return !was_open && menu.open;
}

static void ExitConfirmation(AeronUiContext* ui) {
	if (!AeronUi_BeginModal(ui, "EXIT GAME", &menu.exit_confirmation, NULL))
		return;
	AeronUi_Help(ui, "Save settings and exit OpenXW?");
	AeronUi_BeginColumns(ui, 2, NULL);
	if (AeronUi_Button(ui, "Cancel"))
		menu.exit_confirmation = 0;
	AeronUi_NextColumn(ui);
	if (AeronUi_Button(ui, "Exit game##confirm")) {
		menu.exit_confirmation = 0;
		menu.exit_requested = true;
	}
	AeronUi_EndColumns(ui);
	AeronUi_EndModal(ui);
}

static void PendingSave(AeronUiContext* ui) {
	AeronUi_Error(ui, XwPreferences_SaveError());
	if (AeronUi_Button(ui, "Retry saving preferences")) {
		char error[1024];
		if (!XwPreferences_RetrySave(error, sizeof error))
			XwSettingsMenu_ReportError(error);
		else {
			menu.draft = *XwConfig_Settings();
			XwControllerSettings_Open(&menu.controller, &menu.draft);
			XwKeyboardSettings_Open(&menu.keyboard, &menu.draft);
			menu.error[0] = 0;
		}
	}
}

static void XwSettings_CombosPage(AeronUiContext* ui) {
 static int row,category,last_row=-1,key;
 static const char* const categories[]={"Wapens / shields","Targets","Snelheid","Zicht","Informatie","Systeem","Communicatie"};
 AeronUi_Header(ui,"Vrije knoppen en cycli (tijdens vliegen)");
 int n=XwCombos_Count();
 if(row>=n) row=n-1; if(row<0) row=0;
 const FlightBinding* list=XwCombos_Bindings();
 char rows[BINDING_CAP][180]; const char* row_labels[BINDING_CAP];
 for(int i=0;i<n;i++) {
  char mask[100]; XwCombos_FormatMask(list[i].mask,mask,sizeof mask);
  snprintf(rows[i],sizeof rows[i],"%d. %s: %s",i+1,mask,XwInputActions_DisplayName((XwInputAction)list[i].action)); row_labels[i]=rows[i];
 }
 if(n) {
  AeronUi_Selector(ui,"Toewijzing",&row,row_labels,n);
  FlightBinding b=list[row];
  if(last_row!=row) { category=XwInputActions_Category((XwInputAction)b.action); last_row=row; }
  AeronUi_Selector(ui,"Categorie",&category,categories,XW_INPUT_ACTION_CATEGORY_COUNT);
  int actions[XW_INPUT_ACTION_COUNT],count=1,selected=0;
  const char* labels[XW_INPUT_ACTION_COUNT]; actions[0]=0; labels[0]="Geen opdracht";
  for(int a=1;a<XW_INPUT_ACTION_COUNT;a++) if((int)XwInputActions_Category((XwInputAction)a)==category) {
   if(a==b.action) selected=count;
   actions[count]=a; labels[count++]=XwInputActions_DisplayName((XwInputAction)a);
  }
  if(AeronUi_Selector(ui,"Opdracht",&selected,labels,count)) { XwCombos_SetBinding(row,b.mask,actions[selected]); b=list[row]; }
  char mask[120]; XwCombos_FormatMask(b.mask,mask,sizeof mask); AeronUi_Help(ui,mask);
  const char* keys[BINDING_BITS]; for(int i=0;i<BINDING_BITS;i++) keys[i]=XwCombos_KeyName(i);
  AeronUi_Selector(ui,"Knop wijzigen",&key,keys,BINDING_BITS);
  if(AeronUi_Button(ui,(b.mask&(1u<<key)) ? "Deze knop verwijderen uit combinatie" : "Deze knop toevoegen aan combinatie")) {
   unsigned next=b.mask^(1u<<key);
   if(next&(1u<<key)) { if(key==8) next&=~512u; if(key==9) next&=~256u; if(key==10) next&=~2048u; if(key==11) next&=~1024u; }
   XwCombos_SetBinding(row,next,b.action); b=list[row];
  }
  if(AeronUi_Button(ui,"Combinatie wissen")) { XwCombos_SetBinding(row,0,b.action); b=list[row]; }
  int total=0,position=0;
  for(int i=0;i<n;i++) if(b.mask && list[i].mask==b.mask && list[i].action) { total++; if(i==row) position=total; }
  if(total>1) {
   char line[100]; snprintf(line,sizeof line,"Cyclus: stap %d van %d. Elke nieuwe druk: volgende stap.",position,total); AeronUi_Header(ui,line);
   for(int i=0,k=0;i<n;i++) if(list[i].mask==b.mask && list[i].action) { char line[180]; snprintf(line,sizeof line,"%d: %s",++k,XwInputActions_DisplayName((XwInputAction)list[i].action)); AeronUi_Help(ui,line); }
  }
  if(AeronUi_Button(ui,"Eerder in lijst / cyclus") && XwCombos_MoveBinding(row,-1)) { row--; last_row=-1; }
  if(AeronUi_Button(ui,"Later in lijst / cyclus") && XwCombos_MoveBinding(row,1)) { row++; last_row=-1; }
  if(AeronUi_Button(ui,"Toewijzing verwijderen")) { XwCombos_RemoveBinding(row); last_row=-1; }
 }
 if(AeronUi_Button(ui,"Nieuwe toewijzing")) { if(XwCombos_AddBinding()) row=XwCombos_Count()-1; last_row=-1; }
 AeronUi_Help(ui,"Kies een opdracht; voeg een of meer knoppen toe. Gelijke invoer = cyclus.");
 AeronUi_Help(ui,"De uitgebreidste combinatie krijgt voorrang. Geen extra korte actie erna.");
 AeronUi_Help(ui,"Een knop met combinaties voert zijn losse actie pas bij loslaten uit.");
 AeronUi_Help(ui,"Start en Select blijven menu/terug. Alleen tijdens vliegen actief.");
 AeronUi_Help(ui,"Direct opgeslagen. Cycli beginnen opnieuw na wijzigen of herstarten.");
 if(AeronUi_Button(ui,"Standaardindeling herstellen")) { XwCombos_RestoreDefaults(); last_row=-1; }
 if(XwCombos_Error()[0]) AeronUi_Error(ui,XwCombos_Error());
}

static void XwBindings_Frame(const AeronInputSnapshot* input,float seconds);
void XwSettingsMenu_Frame(const AeronInputSnapshot* input, float seconds) {
	if (!menu.open || !input)
		return;
	if(pilot_keyboard_open) { PilotKeyboard_Frame(input,seconds); return; }
 if(quick_enabled) { XwQuick_Frame(input,seconds); return; }
 if(menu.page==6) { XwBindings_Frame(input,seconds); return; }
 AeronUiContext* ui = menu.ui;
	static const char* const pages[] = { "Game", "Video", "Audio", "Controller", "Keyboard", "Mouse", "Combinaties" };
	AeronUi_BeginFrame(ui, &(AeronUiFrameDesc) { .input = input, .dt_seconds = seconds });
	if (AeronUi_BeginWindow(ui, "OpenXW SETTINGS",
							&(AeronUiWindowDesc) { .width_ref = 980, .height_ref = 1000, .centered = 1 })) {
		if (XwPreferences_HasPendingSave())
			PendingSave(ui);
		else {
			AeronUi_BeginTabBar(ui, "pages", pages, 8, &menu.page);
			if (menu.page != XW_SETTINGS_CONTROLLER) {
				XwControllerSettings_CancelCapture(&menu.controller, ui);
				menu.controller.restore_modal_open = 0;
			}
			if (menu.page != XW_SETTINGS_KEYBOARD)
				XwKeyboardSettings_CancelCapture(&menu.keyboard, ui);
			switch (menu.page) {
				case XW_SETTINGS_GAME:
					XwGamePage_Draw(ui, input);
					break;
				case XW_SETTINGS_VIDEO:
					XwVideoPage_Draw(ui, input);
					break;
				case XW_SETTINGS_AUDIO:
					XwAudioPage_Draw(ui);
					break;
				case XW_SETTINGS_CONTROLLER:
					XwControllerSettings_Draw(&menu.controller, ui, input);
					break;
				case XW_SETTINGS_KEYBOARD:
					XwKeyboardSettings_Draw(&menu.keyboard, ui);
					break;
				case 6:
     if(AeronUi_BeginScroll(ui,"Combinaties",XwSettingsMenu_ScrollHeight())) { XwSettings_CombosPage(ui); AeronUi_EndScroll(ui); }
     break;
    case XW_SETTINGS_MOUSE:
					XwMousePage_Draw(ui, input);
					break;
			}
			AeronUi_EndTabBar(ui);
		}
		if (menu.error[0]) {
			AeronUi_Error(ui, menu.error);
			if (AeronUi_Button(ui, "Dismiss error"))
				menu.error[0] = 0;
		}
		AeronUi_Separator(ui);
		if (menu.page == XW_SETTINGS_GAME) {
			AeronUi_BeginColumns(ui, 2, NULL);
			if (AeronUi_Button(ui, "Exit game"))
				menu.exit_confirmation = 1;
			AeronUi_NextColumn(ui);
		}
		if (AeronUi_Button(ui, "Close"))
			XwSettingsMenu_RequestClose();
		if (menu.page == XW_SETTINGS_GAME)
			AeronUi_EndColumns(ui);
		if (menu.page == XW_SETTINGS_CONTROLLER)
			XwControllerSettings_DrawModals(&menu.controller, ui, input, XwConfig_Settings());
		if (menu.page == XW_SETTINGS_KEYBOARD)
			XwKeyboardSettings_DrawModals(&menu.keyboard, ui);
		if (menu.exit_confirmation)
			ExitConfirmation(ui);
		AeronUi_EndWindow(ui);
	}
	XwInstallationPage_DrawPicker(ui);
	AeronUiOutput output = AeronUi_EndFrame(ui);
	menu.captures = output.capture_all != 0;
	if (output.cancel_pressed)
		XwSettingsMenu_RequestClose();
	if (!AeronUi_Submit(ui))
		Aeron_RequestFatalRendererError("Settings submission");
}

static int quick_group_selection,quick_action_selection;

/* Spatial navigation: one wide action above two rows of two actions. */
#include "cockpit_theme.inc"
#include "cockpit_art.h"
static bool XwQuick_Choice(AeronUiContext* ui,const char* label,bool selected,const AeronUiTheme* theme) {
 AeronUiTheme choice=*theme;
 if(selected) {
  memcpy(choice.widget_bg,(float[4]){0.28f,0.13f,0.025f,1.0f},sizeof choice.widget_bg);
  memcpy(choice.widget_border,(float[4]){1.0f,0.61f,0.16f,1.0f},sizeof choice.widget_border);
  memcpy(choice.text,(float[4]){1,.79f,.38f,1},sizeof choice.text);
  memcpy(choice.widget_bg_low,choice.widget_bg,sizeof choice.widget_bg_low);
  choice.widget_border_px=6.0f;
 }
 /* Only the D-pad selection glows; a parked mouse must not highlight a second action. */
 memcpy(choice.widget_bg_hot,choice.widget_bg,sizeof choice.widget_bg_hot);
 memset(choice.focus_outline,0,sizeof choice.focus_outline);
 choice.focus_px=0.0f;
 AeronUi_SetTheme(ui,&choice);
 bool clicked=AeronUi_Button(ui,label);
 AeronUi_SetTheme(ui,theme);
 return clicked;
}

static unsigned XwQuick_GroupsHeld(const AeronInputSnapshot* input) {
 unsigned held=0;
 if(!input) return 0;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++) {
  const AeronControllerSnapshot* d=&input->controllers[i];
  if(!d->connected || strcmp(d->name,"H700 Gamepad")) continue;
  if(d->gamepad_buttons & (1u<<AERON_GAMEPAD_BUTTON_WEST)) held|=1;
  if(d->gamepad_buttons & (1u<<AERON_GAMEPAD_BUTTON_NORTH)) held|=2;
  if(d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFT_TRIGGER]>16383) held|=4;
  if(d->gamepad_axes[AERON_GAMEPAD_AXIS_RIGHT_TRIGGER]>16383) held|=8;
 }
 return held;
}

#ifndef HANDHELD_COMMAND_NAVIGATION_H
#define HANDHELD_COMMAND_NAVIGATION_H
enum { COMMAND_PAGE_SIZE=6, COMMAND_COLUMNS=2 };
typedef struct { int action; const char* label; int group; int terminal; } CommandEntry;
static inline int CommandPageStart(int selected) { return selected/COMMAND_PAGE_SIZE*COMMAND_PAGE_SIZE; }
static inline int CommandMove(int selected,int direction,int count) {
 if(count<=0) return 0;
 if(selected<0 || selected>=count) selected=0;
 int next=selected;
 if(direction==0) next-=COMMAND_COLUMNS;
 if(direction==1) next+=COMMAND_COLUMNS;
 if(direction==2 && selected%COMMAND_COLUMNS) next--;
 if(direction==3 && !(selected%COMMAND_COLUMNS)) next++;
 return next>=0 && next<count ? next : selected;
}
static inline int CommandPage(int selected,int delta,int count) {
 if(count<=0) return 0;
 int page=selected/COMMAND_PAGE_SIZE+delta;
 int pages=(count+COMMAND_PAGE_SIZE-1)/COMMAND_PAGE_SIZE;
 page=(page%pages+pages)%pages;
 int next=page*COMMAND_PAGE_SIZE+selected%COMMAND_PAGE_SIZE;
 return next<count ? next : count-1;
}
static inline int CommandCanAppend(int action,int last,const CommandEntry* entries,int count) {
 int found=0;
 for(int i=0;i<count;i++) {
  if(entries[i].action==last && entries[i].terminal) return 0;
  if(entries[i].action==action) found=1;
 }
 return found;
}
#endif

/* Generated by build_catalog.py. */
static const CommandEntry command_entries[] = {
 {XW_INPUT_ACTION_TARGET_NEXT, "Volgend doel", 0, 0},
 {XW_INPUT_ACTION_TARGET_PREV, "Vorig doel", 0, 0},
 {XW_INPUT_ACTION_TARGET_NEAREST_FIGHTER, "Dichtste vijand", 0, 0},
 {XW_INPUT_ACTION_TARGET_MY_ATTACKER, "Mijn aanvaller", 0, 0},
 {XW_INPUT_ACTION_TARGET_CLEAR, "Doel wissen", 0, 0},
 {XW_INPUT_ACTION_IDENTIFY, "Identificatie wisselen", 0, 0},
 {XW_INPUT_ACTION_AUTO_TARGET, "Doel onder vizier", 0, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_RECALL_1, "Doel oproepen 1", 1, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_STORE_1, "Doel opslaan 1", 1, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_RECALL_2, "Doel oproepen 2", 1, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_STORE_2, "Doel opslaan 2", 1, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_RECALL_3, "Doel oproepen 3", 1, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_STORE_3, "Doel opslaan 3", 1, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_RECALL_4, "Doel oproepen 4", 1, 0},
 {XW_INPUT_ACTION_TARGET_PRESET_STORE_4, "Doel opslaan 4", 1, 0},
 {XW_INPUT_ACTION_THROTTLE_UP, "Sneller", 2, 0},
 {XW_INPUT_ACTION_THROTTLE_DOWN, "Langzamer", 2, 0},
 {XW_INPUT_ACTION_THROTTLE_ZERO, "Stoppen (0%)", 2, 0},
 {XW_INPUT_ACTION_THROTTLE_ONE_THIRD, "1/3 snelheid", 2, 0},
 {XW_INPUT_ACTION_THROTTLE_TWO_THIRDS, "2/3 snelheid", 2, 0},
 {XW_INPUT_ACTION_THROTTLE_FULL, "100% snelheid", 2, 0},
 {XW_INPUT_ACTION_MATCH_SPEED, "Snelheid doel volgen", 2, 0},
 {XW_INPUT_ACTION_CYCLE_CANNON_RECHARGE_RATE, "Laserlaadstand wisselen", 3, 0},
 {XW_INPUT_ACTION_CYCLE_SHIELD_RECHARGE_RATE, "Shieldlaadstand wisselen", 3, 0},
 {XW_INPUT_ACTION_CYCLE_SHIELD_MODE, "Shields verdelen", 3, 0},
 {XW_INPUT_ACTION_XFER_SHIELDS_TO_CANNON, "Shields naar lasers", 3, 0},
 {XW_INPUT_ACTION_XFER_CANNON_TO_SHIELDS, "Lasers naar shields", 3, 0},
 {XW_INPUT_ACTION_ENGINE_MORE, "Meer motorenergie", 3, 0},
 {XW_INPUT_ACTION_ENGINE_LESS, "Minder motorenergie", 3, 0},
 {XW_INPUT_ACTION_ENERGY_NORMALIZE, "Energie normaal", 3, 0},
 {XW_INPUT_ACTION_CANNON_RECHARGE_INCREASE, "Lasers sneller opladen", 3, 0},
 {XW_INPUT_ACTION_CANNON_RECHARGE_DECREASE, "Lasers langzamer opladen", 3, 0},
 {XW_INPUT_ACTION_SHIELDS_FORWARD, "Shields naar voren", 3, 0},
 {XW_INPUT_ACTION_SHIELDS_REAR, "Shields naar achteren", 3, 0},
 {XW_INPUT_ACTION_SHIELDS_BALANCED, "Shields gelijk verdelen", 3, 0},
 {XW_INPUT_ACTION_CYCLE_WEAPON_GROUP, "Wapen wisselen", 4, 0},
 {XW_INPUT_ACTION_CYCLE_WEAPON_FIRING_MODE, "Vuurmodus wisselen", 4, 0},
 {XW_INPUT_ACTION_TOGGLE_S_FOILS, "S-foils openen/sluiten", 4, 0},
 {XW_INPUT_ACTION_WEAPON_GROUP_PREVIOUS, "Vorig wapen", 4, 0},
 {XW_INPUT_ACTION_VIEW_TOGGLE_ELEVATED, "Hoge kijkhoek aan/uit", 5, 0},
 {XW_INPUT_ACTION_VIEW_LEFT_SHOULDER, "Linksachter kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_REAR, "Achteruit kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_RIGHT_SHOULDER, "Rechtsachter kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_LEFT_WING, "Links kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_STRAIGHT_UP, "Omhoog kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_RIGHT_WING, "Rechts kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_LEFT_FORWARD, "Linksvoor kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_FORWARD, "Vooruit kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_RIGHT_FORWARD, "Rechtsvoor kijken", 5, 0},
 {XW_INPUT_ACTION_VIEW_TOGGLE_COCKPIT, "Cockpit tonen/verbergen", 5, 0},
 {XW_INPUT_ACTION_VIEW_PLAYER, "Terug naar cockpit", 5, 0},
 {XW_INPUT_ACTION_VIEW_WARHEAD, "Raketcamera wisselen", 5, 0},
 {XW_INPUT_ACTION_VIEW_EXTERNAL, "Buitencamera aan/uit", 5, 0},
 {XW_INPUT_ACTION_VIEW_MANUAL_CAMERA, "Camera zelf bedienen", 5, 1},
 {XW_INPUT_ACTION_INFO_MAP, "Kaart openen", 6, 1},
 {XW_INPUT_ACTION_INFO_DAMAGE, "Schade en reparatie", 6, 1},
 {XW_INPUT_ACTION_INFO_BRIEFING, "Missiebriefing", 6, 1},
 {XW_INPUT_ACTION_COMM_HEAD_HOME, "Keer terug naar basis", 7, 0},
 {XW_INPUT_ACTION_COMM_STOP_AND_WAIT, "Stop en wacht", 7, 0},
 {XW_INPUT_ACTION_COMM_CONTINUE_MISSION, "Missie voortzetten", 7, 0},
 {XW_INPUT_ACTION_COMM_EVADE, "Ontwijk!", 7, 0},
 {XW_INPUT_ACTION_COMM_ASSIGN_TARGET, "Doel laten aanvallen", 7, 0},
 {XW_INPUT_ACTION_COMM_COVER_ME, "Geef mij dekking", 7, 0},
 {XW_INPUT_ACTION_COMM_IGNORE_TARGET, "Doel negeren", 7, 0},
 {XW_INPUT_ACTION_COMM_REPORT_ORDERS, "Rapporteer opdracht", 7, 0},
 {XW_INPUT_ACTION_REPLAY_RECORD, "Opname starten/stoppen", 8, 0},
 {XW_INPUT_ACTION_REPLAY_VIEW, "Opname bekijken", 8, 1},
 {XW_INPUT_ACTION_HYPERSPACE, "Hyperruimte aan/uit", 8, 0},
 {XW_INPUT_ACTION_EJECT, "Schip verlaten", 8, 1},
 {XW_INPUT_ACTION_CONFIRM_ORDER, "Bericht/opdracht akkoord", 8, 0},
 {XW_INPUT_ACTION_CYCLE_GRAPHICS_DETAIL, "Detailniveau wisselen", 9, 0},
 {XW_INPUT_ACTION_REBUILD_COCKPIT, "Cockpit opnieuw tekenen", 9, 0},
 {XW_INPUT_ACTION_TOGGLE_INTERLACE, "Interlace aan/uit", 9, 0},
 {XW_INPUT_ACTION_SHOW_VERSION, "Versienummer tonen", 9, 0},
 {XW_INPUT_ACTION_FRAME_RATE, "Framerate tonen", 9, 0},
 {XW_INPUT_ACTION_TOGGLE_MUSIC, "Muziek aan/uit", 9, 0},
 {XW_INPUT_ACTION_TOGGLE_SOUND, "Geluid aan/uit", 9, 0},
 {XW_INPUT_ACTION_ENGINE_LEVEL_0, "Minimaal - maximaal opladen", 3, 0},
 {XW_INPUT_ACTION_ENGINE_LEVEL_1, "Laag - opladen", 3, 0},
 {XW_INPUT_ACTION_ENGINE_LEVEL_2, "Normaal - lading behouden", 3, 0},
 {XW_INPUT_ACTION_ENGINE_LEVEL_3, "Hoog - langzaam leeglopen", 3, 0},
 {XW_INPUT_ACTION_ENGINE_LEVEL_4, "Maximaal - snel leeglopen", 3, 0},

};
static const char* const command_groups[] = {
"Targets", "Doelgeheugens", "Snelheid", "Energie en shields", "Wapens", "Camera", "Missie-informatie", "Wingmen", "Vlucht", "Beeld en geluid"
};

static const char* XwQuick_ActionLabel(int action) {
 for(unsigned c=0;c<sizeof command_entries/sizeof *command_entries;c++) if(command_entries[c].action==action) return command_entries[c].label;
 switch(action) {
  case XW_INPUT_ACTION_ENGINE_MORE: return "Meer motorenergie";
  case XW_INPUT_ACTION_ENGINE_LESS: return "Minder motorenergie";
  case XW_INPUT_ACTION_NONE: return "Vrij instelbaar";
  case XW_INPUT_ACTION_TARGET_NEXT: return "Volgend doel";
  case XW_INPUT_ACTION_TARGET_PREV: return "Vorig doel";
  case XW_INPUT_ACTION_TARGET_NEAREST_FIGHTER: return "Dichtste jager/mijn";
  case XW_INPUT_ACTION_TARGET_MY_ATTACKER: return "Mijn aanvaller";
  case XW_INPUT_ACTION_TARGET_CLEAR: return "Doel wissen";
  case XW_INPUT_ACTION_AUTO_TARGET: return "Doel onder vizier";
  case XW_INPUT_ACTION_MATCH_SPEED: return "Snelheid doel volgen";
  case XW_INPUT_ACTION_THROTTLE_FULL: return "Volle snelheid";
  case XW_INPUT_ACTION_THROTTLE_ZERO: return "Stoppen";
  case XW_INPUT_ACTION_THROTTLE_ONE_THIRD: return "1/3 snelheid";
  case XW_INPUT_ACTION_THROTTLE_TWO_THIRDS: return "2/3 snelheid";
  case XW_INPUT_ACTION_THROTTLE_UP: return "Sneller";
  case XW_INPUT_ACTION_THROTTLE_DOWN: return "Langzamer";
  case XW_INPUT_ACTION_CYCLE_SHIELD_MODE: return "Shields verdelen";
  case XW_INPUT_ACTION_CYCLE_CANNON_RECHARGE_RATE: return "Lasers opladen";
  case XW_INPUT_ACTION_CYCLE_SHIELD_RECHARGE_RATE: return "Shields opladen";
  case XW_INPUT_ACTION_XFER_SHIELDS_TO_CANNON: return "Shields > lasers";
  case XW_INPUT_ACTION_XFER_CANNON_TO_SHIELDS: return "Lasers > shields";
  case XW_INPUT_ACTION_CYCLE_WEAPON_GROUP: return "Wapen wisselen";
  case XW_INPUT_ACTION_CYCLE_WEAPON_FIRING_MODE: return "Vuurmodus";
  case XW_INPUT_ACTION_TOGGLE_S_FOILS: return "S-foils openen/sluiten";
  case XW_INPUT_ACTION_FIRE_WEAPON: return "Schieten";
  case XW_INPUT_ACTION_INFO_MAP: return "Kaart";
  case XW_INPUT_ACTION_INFO_DAMAGE: return "Schade";
  case XW_INPUT_ACTION_HYPERSPACE: return "Hyperruimte";
  case XW_INPUT_ACTION_EJECT: return "Uitwerpen";
  default: return XwInputActions_DisplayName((XwInputAction)action);
 }
}


static bool XwQuick_Close(void) { XwSettingsMenu_RequestClose(); return true; }
static void XwQuick_OpenSettings(void) { quick_enabled=false; quick_return=true; menu.page=6; }
static void XwQuick_Queue(int action) {
 if(action<=XW_INPUT_ACTION_NONE || action==XW_INPUT_ACTION_PAUSE || action==XW_INPUT_ACTION_ESCAPE) {
  snprintf(quick_notice,sizeof quick_notice,"Geen opdracht. Start hervat; Select opent opties."); return;
 }
 if(!XwCombos_Queue(action)) { snprintf(quick_notice,sizeof quick_notice,"Wachtrij vol (16). Hervat of wis een opdracht."); return; }
 quick_notice[0]=0;
 Aeron_LogInfo("xw.quick","queued: %s",XwInputActions_ToName(action));
}
#ifndef QUICK_MENU_MODEL_H
#define QUICK_MENU_MODEL_H
#define QUICK_PAGE_SIZE 6
#define QUICK_NODE_CAP 192
typedef struct { const char* label; int parent; int action; } QuickNode;
static int QuickChildren(const QuickNode* nodes,int count,int parent,int out[QUICK_NODE_CAP]) {
 if(parent==0){int roots[]={2,4,5,18,6,7};for(int i=0;i<6;i++)out[i]=roots[i];return 6;}
 int n=0;
 const int preferred[]={-10,-12,-11};
 for(unsigned p=0;p<3;p++)
  for(int i=1;i<count;i++) if(nodes[i].parent==parent && nodes[i].action==preferred[p]) out[n++]=i;
 for(int i=1;i<count;i++) if(nodes[i].parent==parent && n<QUICK_NODE_CAP &&
  !(nodes[i].action==-10 || nodes[i].action==-11 || nodes[i].action==-12)) out[n++]=i;
 for(int i=1;i<n;i++)if(nodes[out[i]].action==-4 || nodes[out[i]].action==-5){int first=out[i];memmove(out+1,out,i*sizeof *out);out[0]=first;}
 for(int i=1;i<n;i++)if(nodes[out[i]].action==XW_INPUT_ACTION_MATCH_SPEED){int first=out[i];memmove(out+1,out,i*sizeof *out);out[0]=first;}
 return n;
}
static int QuickMove(int selected,int direction,int count) {
 if(count<=0) return 0;
 int next=selected;
 if(direction==0) next-=2;
 if(direction==1) next+=2;
 if(direction==2 && (selected&1)) --next;
 if(direction==3 && !(selected&1)) ++next;
 return next>=0 && next<count ? next : selected;
}
static int QuickPage(int selected,int delta,int count) {
 if(count<=0) return 0;
 int pages=(count+QUICK_PAGE_SIZE-1)/QUICK_PAGE_SIZE;
 int page=((selected/QUICK_PAGE_SIZE+delta)%pages+pages)%pages;
 int next=page*QUICK_PAGE_SIZE+selected%QUICK_PAGE_SIZE;
 return next<count ? next : count-1;
}
#endif

/* Complete, hierarchical menu; generated by make-tree.py. */
static const QuickNode quick_nodes[] = {
 {"FLIGHT COMMAND", -1, 0},
 {"D-PAD ASSIST", 7, -1},
 {"TARGETS", 0, 0},
 {"SPEED", 4, 0},
 {"SHIP & POWER", 0, 0},
 {"WEAPONS", 0, 0},
 {"MISSION", 0, 0},
 {"VIEW & SETTINGS", 0, 0},
 {"TARGET COMMANDS", 2, 0},
 {"TARGET MEMORY", 2, 0},
 {"LASERS", 4, 0},
 {"SHIELDS", 4, 0},
 {"ENGINES", 4, 0},
 {"SHIELD TRANSFER", 4, 0},
 {"RECHARGE RATE", 10, 0},
 {"RECHARGE RATE", 11, 0},
 {"SHIELD DIRECTION", 4, -7},
 {"OBJECTIVES & INFO", 6, 0},
 {"WINGMEN", 0, 0},
 {"NAVIGATION", 6, 0},
 {"LEAVE MISSION", 6, 0},
 {"LOOK DIRECTION", 7, 0},
 {"COCKPIT & CAMERAS", 7, 0},
 {"FLIGHT RECORDER", 7, 0},
 {"GRAPHICS", 7, 0},
 {"TARGET NEXT", 8, XW_INPUT_ACTION_TARGET_NEXT},
 {"TARGET PREVIOUS", 8, XW_INPUT_ACTION_TARGET_PREV},
 {"NEAREST ENEMY", 8, XW_INPUT_ACTION_TARGET_NEAREST_FIGHTER},
 {"TARGET MY ATTACKER", 8, XW_INPUT_ACTION_TARGET_MY_ATTACKER},
 {"TARGET CLEAR", 8, XW_INPUT_ACTION_TARGET_CLEAR},
 {"TOGGLE IDENTIFICATION", 8, XW_INPUT_ACTION_IDENTIFY},
 {"TARGET IN CROSSHAIR", 8, XW_INPUT_ACTION_AUTO_TARGET},
 {"TARGET PRESET RECALL 1", 9, XW_INPUT_ACTION_TARGET_PRESET_RECALL_1},
 {"TARGET PRESET STORE 1", 9, XW_INPUT_ACTION_TARGET_PRESET_STORE_1},
 {"TARGET PRESET RECALL 2", 9, XW_INPUT_ACTION_TARGET_PRESET_RECALL_2},
 {"TARGET PRESET STORE 2", 9, XW_INPUT_ACTION_TARGET_PRESET_STORE_2},
 {"TARGET PRESET RECALL 3", 9, XW_INPUT_ACTION_TARGET_PRESET_RECALL_3},
 {"TARGET PRESET STORE 3", 9, XW_INPUT_ACTION_TARGET_PRESET_STORE_3},
 {"TARGET PRESET RECALL 4", 9, XW_INPUT_ACTION_TARGET_PRESET_RECALL_4},
 {"TARGET PRESET STORE 4", 9, XW_INPUT_ACTION_TARGET_PRESET_STORE_4},
 {"THROTTLE UP", 3, XW_INPUT_ACTION_THROTTLE_UP},
 {"THROTTLE DOWN", 3, XW_INPUT_ACTION_THROTTLE_DOWN},
 {"STOP (0%)", 3, XW_INPUT_ACTION_THROTTLE_ZERO},
 {"1/3 THROTTLE", 3, XW_INPUT_ACTION_THROTTLE_ONE_THIRD},
 {"2/3 THROTTLE", 3, XW_INPUT_ACTION_THROTTLE_TWO_THIRDS},
 {"FULL THROTTLE", 3, XW_INPUT_ACTION_THROTTLE_FULL},
 {"MATCH SPEED WITH TARGET", 3, XW_INPUT_ACTION_MATCH_SPEED},
 {"LASER RECHARGE", 10, XW_INPUT_ACTION_CYCLE_CANNON_RECHARGE_RATE},
 {"SHIELD RECHARGE", 11, XW_INPUT_ACTION_CYCLE_SHIELD_RECHARGE_RATE},
 {"SHIELD DIRECTION", 16, XW_INPUT_ACTION_CYCLE_SHIELD_MODE},
 {"SHIELDS TO LASERS", 13, XW_INPUT_ACTION_XFER_SHIELDS_TO_CANNON},
 {"LASERS TO SHIELDS", 13, XW_INPUT_ACTION_XFER_CANNON_TO_SHIELDS},
 {"MORE ENGINE POWER", 12, XW_INPUT_ACTION_ENGINE_MORE},
 {"LESS ENGINE POWER", 12, XW_INPUT_ACTION_ENGINE_LESS},
 {"BALANCED POWER", 4, XW_INPUT_ACTION_ENERGY_NORMALIZE},
 {"CANNON RECHARGE INCREASE", 10, XW_INPUT_ACTION_CANNON_RECHARGE_INCREASE},
 {"CANNON RECHARGE DECREASE", 10, XW_INPUT_ACTION_CANNON_RECHARGE_DECREASE},
 {"SHIELDS FORWARD", 16, XW_INPUT_ACTION_SHIELDS_FORWARD},
 {"SHIELDS REAR", 16, XW_INPUT_ACTION_SHIELDS_REAR},
 {"SHIELDS BALANCED", 16, XW_INPUT_ACTION_SHIELDS_BALANCED},
 {"SELECT WEAPON", 5, XW_INPUT_ACTION_CYCLE_WEAPON_GROUP},
 {"FIRING MODE", 5, -8},
 {"TOGGLE S-FOILS", 5, XW_INPUT_ACTION_TOGGLE_S_FOILS},
 {"PREVIOUS WEAPON", 5, XW_INPUT_ACTION_WEAPON_GROUP_PREVIOUS},
 {"HIGH ANGLE VIEW", 22, XW_INPUT_ACTION_VIEW_TOGGLE_ELEVATED},
 {"VIEW LEFT SHOULDER", 21, XW_INPUT_ACTION_VIEW_LEFT_SHOULDER},
 {"VIEW REAR", 21, XW_INPUT_ACTION_VIEW_REAR},
 {"VIEW RIGHT SHOULDER", 21, XW_INPUT_ACTION_VIEW_RIGHT_SHOULDER},
 {"VIEW LEFT WING", 21, XW_INPUT_ACTION_VIEW_LEFT_WING},
 {"VIEW STRAIGHT UP", 21, XW_INPUT_ACTION_VIEW_STRAIGHT_UP},
 {"VIEW RIGHT WING", 21, XW_INPUT_ACTION_VIEW_RIGHT_WING},
 {"VIEW LEFT FORWARD", 21, XW_INPUT_ACTION_VIEW_LEFT_FORWARD},
 {"VIEW FORWARD", 21, XW_INPUT_ACTION_VIEW_FORWARD},
 {"VIEW RIGHT FORWARD", 21, XW_INPUT_ACTION_VIEW_RIGHT_FORWARD},
 {"SHOW / HIDE COCKPIT", 22, XW_INPUT_ACTION_VIEW_TOGGLE_COCKPIT},
 {"RETURN TO COCKPIT", 22, XW_INPUT_ACTION_VIEW_PLAYER},
 {"WARHEAD CAMERA", 22, XW_INPUT_ACTION_VIEW_WARHEAD},
 {"VIEW EXTERNAL", 22, XW_INPUT_ACTION_VIEW_EXTERNAL},
 {"CAMERA CONTROL", 22, XW_INPUT_ACTION_VIEW_MANUAL_CAMERA},
 {"TACTICAL MAP", 17, XW_INPUT_ACTION_INFO_MAP},
 {"DAMAGE & REPAIRS", 17, XW_INPUT_ACTION_INFO_DAMAGE},
 {"MISSION BRIEFING", 17, XW_INPUT_ACTION_INFO_BRIEFING},
 {"RETURN TO BASE", 18, XW_INPUT_ACTION_COMM_HEAD_HOME},
 {"STOP AND WAIT", 18, XW_INPUT_ACTION_COMM_STOP_AND_WAIT},
 {"CONTINUE MISSION", 18, XW_INPUT_ACTION_COMM_CONTINUE_MISSION},
 {"EVADE", 18, XW_INPUT_ACTION_COMM_EVADE},
 {"ATTACK MY TARGET", 18, XW_INPUT_ACTION_COMM_ASSIGN_TARGET},
 {"COVER ME", 18, XW_INPUT_ACTION_COMM_COVER_ME},
 {"IGNORE TARGET", 18, XW_INPUT_ACTION_COMM_IGNORE_TARGET},
 {"REPORT ORDERS", 18, XW_INPUT_ACTION_COMM_REPORT_ORDERS},
 {"REPLAY RECORD", 23, XW_INPUT_ACTION_REPLAY_RECORD},
 {"REPLAY VIEW", 23, XW_INPUT_ACTION_REPLAY_VIEW},
 {"HYPERSPACE", 19, XW_INPUT_ACTION_HYPERSPACE},
 {"EJECT", 20, XW_INPUT_ACTION_EJECT},
 {"CONFIRM ORDER", 19, XW_INPUT_ACTION_CONFIRM_ORDER},
 {"CYCLE GRAPHICS DETAIL", 24, XW_INPUT_ACTION_CYCLE_GRAPHICS_DETAIL},
 {"REBUILD COCKPIT", 24, XW_INPUT_ACTION_REBUILD_COCKPIT},
 {"TOGGLE INTERLACE", 24, XW_INPUT_ACTION_TOGGLE_INTERLACE},
 {"SHOW VERSION", 24, XW_INPUT_ACTION_SHOW_VERSION},
 {"FRAME RATE", 24, XW_INPUT_ACTION_FRAME_RATE},
 {"TOGGLE MUSIC", 24, XW_INPUT_ACTION_TOGGLE_MUSIC},
 {"TOGGLE SOUND", 24, XW_INPUT_ACTION_TOGGLE_SOUND},
 {"FIRE ONCE", 5, XW_INPUT_ACTION_FIRE_WEAPON},
 {"FAST DRAIN (0)", 14, XW_INPUT_ACTION_LASER_RATE_0},
 {"SLOW DRAIN (1)", 14, XW_INPUT_ACTION_LASER_RATE_1},
 {"MAINTAIN (2)", 14, XW_INPUT_ACTION_LASER_RATE_2},
 {"RECHARGE (3)", 14, XW_INPUT_ACTION_LASER_RATE_3},
 {"MAX RECHARGE (4)", 14, XW_INPUT_ACTION_LASER_RATE_4},
 {"FAST DRAIN (0)", 15, XW_INPUT_ACTION_SHIELD_RATE_0},
 {"SLOW DRAIN (1)", 15, XW_INPUT_ACTION_SHIELD_RATE_1},
 {"MAINTAIN (2)", 15, XW_INPUT_ACTION_SHIELD_RATE_2},
 {"RECHARGE (3)", 15, XW_INPUT_ACTION_SHIELD_RATE_3},
 {"MAX RECHARGE (4)", 15, XW_INPUT_ACTION_SHIELD_RATE_4},
 {"ENGINE PRESETS", 12, 0},
 {"MINIMUM ENGINES", 113, XW_INPUT_ACTION_ENGINE_LEVEL_0},
 {"LOW ENGINES", 113, XW_INPUT_ACTION_ENGINE_LEVEL_1},
 {"BALANCED ENGINES", 113, XW_INPUT_ACTION_ENGINE_LEVEL_2},
 {"HIGH ENGINES", 113, XW_INPUT_ACTION_ENGINE_LEVEL_3},
 {"FULL ENGINES", 113, XW_INPUT_ACTION_ENGINE_LEVEL_4},
 {"SAVE / LOAD", -1, -3},
 {"TARGET LIST", 2, -4},
 {"POWER DISTRIBUTION", 4, -5},
 {"CONTROLS", 7, -6},
 {"LEAD CROSSHAIR", 7, -9},
 {"MATCH SPEED WITH TARGET", 2, XW_INPUT_ACTION_MATCH_SPEED},
 {"MODELS & TEXTURES", 7, -10},
 {"ANTI-ALIASING", 24, -11},
 {"COCKPIT LIGHTING", 7, -12},
};

static int quick_node,quick_selected,quick_saved_selection[QUICK_NODE_CAP];
static int quick_children[QUICK_NODE_CAP];
static bool quick_leaving;
static bool weapons_open;
static void WeaponsOpen(void);
static void WeaponsFrame(const AeronInputSnapshot*,float);
#include "equipment_state.inc"
#include "quick_energy.inc"
#include "xw_targets.inc"
#define TB_UI menu.ui
#define TB_CLEAR() XwCombos_Clear()
#define TB_CLOSE() XwQuick_Close()
#define TB_CHOICE XwQuick_Choice
#define TB_UNCAPTURE() do {menu.captures=false;} while(0)
#include "target_browser.inc"
static void QuickMenu_Reset(void) {
 weapons_open=false;
 targets_open=false;
 slots_open=false;energy_open=false;
 quick_node=quick_selected=0;quick_leaving=false;
 memset(quick_saved_selection,0,sizeof quick_saved_selection);
 XwCombos_Clear();
}
static void QuickBack(void) {
 if(quick_node>0) {
  quick_node=quick_nodes[quick_node].parent;
  quick_selected=quick_saved_selection[quick_node];
 } else { XwCombos_Clear(); (void)XwQuick_Close(); }
}
static bool QuickMsaaSupported(int samples) {
 return samples==1 ||
  (Aeron_TextureFormatSupportsSampleCount(AERON_TEXTURE_FORMAT_RGBA16_FLOAT,(AeronSampleCount)samples) &&
   Aeron_TextureFormatSupportsSampleCount(AERON_TEXTURE_FORMAT_D32_FLOAT,(AeronSampleCount)samples));
}
static void QuickGraphicsLabel(int action,char* label,size_t capacity) {
 if(action==-12)snprintf(label,capacity,"COCKPIT LIGHTING: %s",XwCockpitLighting_Name());
 if(action==-10) snprintf(label,capacity,"MODELS & TEXTURES: %s",XwHdGraphics_Selected()?"HD":"ORIGINAL");
 if(action==-11) {
  int samples=menu.draft.render.dos_msaa_samples;
  if(samples<=1) snprintf(label,capacity,"ANTI-ALIASING: OFF");
  else snprintf(label,capacity,"ANTI-ALIASING: %dX MSAA",samples);
 }
}
static void QuickChoose(int child) {
 const QuickNode* n=&quick_nodes[child];
 if(n->action==-12) {
  if(!XwCockpitLighting_SetMode((XwCockpitLighting_Mode()+1)%3))
   snprintf(quick_notice,sizeof quick_notice,"Could not save cockpit lighting.");
  else snprintf(quick_notice,sizeof quick_notice,"Saved. X-Wing front cockpit; applies on resume.");
  return;
 }

 if(n->action==-10) {
  if(!XwHdGraphics_Set(!XwHdGraphics_Selected()))
   snprintf(quick_notice,sizeof quick_notice,"Could not save graphics mode.");
  else snprintf(quick_notice,sizeof quick_notice,"%s",XwHdGraphics_Selected()!=XwHdGraphics_Active()?
   "Saved. Restart game to change models/textures.":"Saved. Current graphics mode selected.");
  return;
 }
 if(n->action==-11) {
  int previous=menu.draft.render.dos_msaa_samples,next=previous;
  do {next=next>=8?1:next*2;} while(!QuickMsaaSupported(next));
  menu.draft.render.dos_msaa_samples=next;
  char error[1024];
  if(!Commit(error,sizeof error)) {
   menu.draft.render.dos_msaa_samples=previous;
   snprintf(quick_notice,sizeof quick_notice,"Could not save anti-aliasing.");
  } else snprintf(quick_notice,sizeof quick_notice,"Anti-aliasing saved. Applies on resume.");
  return;
 }
 if(n->action==-9){if(!XwCombos_SetLead(!XwCombos_LeadEnabled()))snprintf(quick_notice,sizeof quick_notice,"Could not save lead crosshair setting.");return;}
 if(child==5||n->action==-8){WeaponsOpen();return;}
 if(n->action==-7){EnergyOpen();if(energy_open)energy_draft.row=2;return;}
 if(n->action==-6){XwQuick_OpenSettings();return;}
 if(n->action==-4){TargetsOpen();return;}
 if(n->action==-5){EnergyOpen();return;}
 if(n->action==-1){
  if(!XwCombos_SetAssist(!XwCombos_AssistEnabled()))snprintf(quick_notice,sizeof quick_notice,"Could not save steering assist.");
  else quick_notice[0]=0;
  return;
 }
 if(n->action==0) {
  quick_saved_selection[quick_node]=quick_selected;
  quick_node=child;quick_selected=0; return;
 }
 if(quick_leaving) return;
 XwCombos_Clear();
 if(!XwCombos_Queue(n->action)) {
  snprintf(quick_notice,sizeof quick_notice,"Could not execute command."); return;
 }
 quick_leaving=true;
 Aeron_LogInfo("xw.command","single command selected: %s",XwInputActions_ToName(n->action));
 if(!XwQuick_Close()) { quick_leaving=false; XwCombos_Clear(); }
}
static void XwQuick_Frame(const AeronInputSnapshot* input,float dt_seconds) {
 if(weapons_open){WeaponsFrame(input,dt_seconds);return;}
 if(targets_open){TargetsFrame(input,dt_seconds);return;}
 if(energy_open){EnergyFrame(input,dt_seconds);return;}
 if(quick_leaving) return;
 unsigned pressed=0;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++) {
  const AeronControllerSnapshot* d=&input->controllers[i];
  if(d->connected && !strcmp(d->name,"H700 Gamepad")) pressed|=d->gamepad_pressed_buttons;
 }
 int count=QuickChildren(quick_nodes,sizeof quick_nodes/sizeof *quick_nodes,quick_node,quick_children);
 if(quick_selected<0 || quick_selected>=count) quick_selected=0;
 if(input->has_focus) {
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_BACK)) { XwQuick_OpenSettings(); return; }
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_EAST)) { QuickBack(); return; }
  unsigned pad=(pressed>>AERON_GAMEPAD_BUTTON_DPAD_UP)&15u;
  if(pad && !(pad&(pad-1))) {
   int d=0;while(!(pad&(1u<<d))) d++;
   quick_selected=QuickMove(quick_selected,d,count);
  }
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_LEFT_SHOULDER)) quick_selected=QuickPage(quick_selected,-1,count);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_RIGHT_SHOULDER)) quick_selected=QuickPage(quick_selected,1,count);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH)) {
   QuickChoose(quick_children[quick_selected]);
   if(quick_leaving) return;
   count=QuickChildren(quick_nodes,sizeof quick_nodes/sizeof *quick_nodes,quick_node,quick_children);
  }
 }
 /* Direct command buttons own controller input; mouse/keyboard widgets remain usable. */
 static AeronInputSnapshot ui_input; ui_input=*input;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++) if(!strcmp(ui_input.controllers[i].name,"H700 Gamepad")) {
  ui_input.controllers[i].gamepad_buttons=0;
  ui_input.controllers[i].gamepad_pressed_buttons=0;
  ui_input.controllers[i].gamepad_released_buttons=0;
  memset(ui_input.controllers[i].gamepad_axes,0,sizeof ui_input.controllers[i].gamepad_axes);
 }
 AeronUiContext* ui=menu.ui;
 AeronUiTheme original=*AeronUi_GetTheme(ui),large=original;
 large.text_px=40.0f; large.title_px=48.0f; large.help_px=38.0f;
 large.row_height=150.0f; large.title_height=76.0f;
 large.window_pad=20.0f; large.item_spacing=12.0f; large.column_spacing=24.0f;
 large.widget_border_px=3.0f; large.focus_px=4.0f; large.corner_radius=10.0f;
 memcpy(large.text,(float[4]){1.0f,1.0f,1.0f,1.0f},sizeof large.text);
 memcpy(large.text_dim,(float[4]){0.90f,0.94f,1.0f,1.0f},sizeof large.text_dim);
 memcpy(large.accent,(float[4]){0.40f,0.82f,1.0f,1.0f},sizeof large.accent);
 memcpy(large.surface,(float[4]){0.014f,0.022f,0.040f,1.0f},sizeof large.surface);
 memcpy(large.widget_bg,(float[4]){0.06f,0.11f,0.19f,1.0f},sizeof large.widget_bg);
 memcpy(large.widget_bg_hot,(float[4]){0.12f,0.26f,0.42f,1.0f},sizeof large.widget_bg_hot);
 memcpy(large.widget_border,(float[4]){0.30f,0.49f,0.70f,1.0f},sizeof large.widget_border);
 memcpy(large.focus_outline,(float[4]){0.40f,0.85f,1.0f,1.0f},sizeof large.focus_outline);
 CommandCockpitTheme(&large);
 AeronUi_SetTheme(ui,&large);
 AeronUi_BeginFrame(ui,&(AeronUiFrameDesc){.input=&ui_input,.dt_seconds=dt_seconds});


 char title[160];int start=quick_selected/QUICK_PAGE_SIZE*QUICK_PAGE_SIZE;
 snprintf(title,sizeof title,"%s  |  PAUSED  %d/%d",quick_nodes[quick_node].label,start/QUICK_PAGE_SIZE+1,(count+QUICK_PAGE_SIZE-1)/QUICK_PAGE_SIZE);
 AeronUiWindowDesc window={.width_ref=1560.0f,.height_ref=1050.0f,.centered=1,.flags=(1u<<8)};
 int activate=-1,turn=0;bool back=false,options=false,resume=false;
 if(AeronUi_CockpitBegin(ui,quick_node?quick_nodes[quick_node].label:"COMMAND MENU",0,0)) {

 int hull=-1,shield=-1;CraftData*c=g_playerFlightState.craft;
 if(c){hull=TargetPercent((int)c->hullMax-c->hullDamage,c->hullMax);
  int max=g_craftTypeDefs[c->craftTypeIndex].nominalShieldEnergy[0]+g_craftTypeDefs[c->craftTypeIndex].nominalShieldEnergy[1];
  shield=TargetPercent((int)c->shieldEnergy[0]+c->shieldEnergy[1],max);}

  AeronUi_CockpitStatus(ui,hull,shield,start/QUICK_PAGE_SIZE+1,(count+QUICK_PAGE_SIZE-1)/QUICK_PAGE_SIZE);
  for(int item=start;item<start+QUICK_PAGE_SIZE;item++){
   if(item>=count){AeronUi_CockpitMenuButton(ui,"",item-start,0,0);continue;}
   int child=quick_children[item];const QuickNode*n=&quick_nodes[child];char label[100];
   if(n->action==-1)snprintf(label,sizeof label,"D-PAD ASSIST: %s",XwCombos_AssistEnabled()?"ON":"OFF");
   else snprintf(label,sizeof label,"%s",n->label);
   if(n->action==-9)snprintf(label,sizeof label,"LEAD CROSSHAIR: %s",XwCombos_LeadEnabled()?"ON":"OFF");
   QuickGraphicsLabel(n->action,label,sizeof label);
   if(AeronUi_CockpitMenuButton(ui,label,item-start,quick_selected==item,quick_node==0)){quick_selected=item;activate=child;}
  }
  if(AeronUi_CockpitButton(ui,"A  SELECT",65,897,430,92,0,0))activate=quick_children[quick_selected];
  if(AeronUi_CockpitButton(ui,"B  BACK",570,897,395,92,0,0))back=true;
  if(AeronUi_CockpitButton(ui,"START  RESUME",1040,897,440,92,0,0))resume=true;
  if(quick_notice[0])AeronUi_CockpitText(ui,quick_notice,545,165,890,68,28,2,0);
 } else if(AeronUi_BeginWindow(ui,title,&window)) {
  AeronUi_Help(ui,"D-PAD NAVIGATE   |   A SELECT   |   B BACK");
  for(int row=0;row<3;row++) {
   AeronUi_BeginColumns(ui,2,NULL);
   for(int col=0;col<2;col++) {
    int item=start+row*2+col;
    if(item<count) {
     int child=quick_children[item];const QuickNode* n=&quick_nodes[child];
     char label[96];
     if(n->action==-1) snprintf(label,sizeof label,"D-PAD ASSIST: %s",XwCombos_AssistEnabled()?"ON":"OFF");
     else if(!n->action) snprintf(label,sizeof label,"%s  >",n->label);
     else snprintf(label,sizeof label,"%s",n->label);
   if(n->action==-9)snprintf(label,sizeof label,"LEAD CROSSHAIR: %s",XwCombos_LeadEnabled()?"ON":"OFF");
   QuickGraphicsLabel(n->action,label,sizeof label);
     if(XwQuick_Choice(ui,label,quick_selected==item,&large)) { quick_selected=item;activate=child; }
    } else AeronUi_Spacer(ui,large.row_height);
    if(!col) AeronUi_NextColumn(ui);
   }
   AeronUi_EndColumns(ui);
  }
  AeronUiTheme compact=large;compact.text_px=40.0f;compact.row_height=80.0f;
  AeronUi_SetTheme(ui,&compact);
  if(count>QUICK_PAGE_SIZE) {
   char line[96];snprintf(line,sizeof line,"%d COMMANDS  |  L1 / R1 CHANGE PAGE",count);
   AeronUi_Help(ui,line);
   AeronUi_BeginColumns(ui,2,NULL);
   if(AeronUi_Button(ui,"L1 PREVIOUS")) turn=-1;
   AeronUi_NextColumn(ui);
   if(AeronUi_Button(ui,"R1 NEXT")) turn=1;
   AeronUi_EndColumns(ui);
  } else AeronUi_Help(ui,"FLIGHT COMMAND CONSOLE");
  AeronUi_Help(ui,"Select an action to execute it and resume flight.");
  if(quick_notice[0]) AeronUi_Error(ui,quick_notice);
  AeronUi_BeginColumns(ui,3,NULL);
  if(AeronUi_Button(ui,"B BACK")) back=true;
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"SELECT SETTINGS")) options=true;
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"START RESUME")) resume=true;
  AeronUi_EndColumns(ui);
  AeronUi_EndWindow(ui);
 }
 AeronUiOutput out=AeronUi_EndFrame(ui);
 if(out.cancel_pressed) back=true;
 menu.captures=false;
 AeronUi_Submit(ui);AeronUi_SetTheme(ui,&original);
 if(activate>=0) QuickChoose(activate);
 else if(back) QuickBack();
 else if(options) { XwQuick_OpenSettings(); }
 else if(resume) { XwCombos_Clear(); (void)XwQuick_Close(); }
 else if(turn) quick_selected=QuickPage(quick_selected,turn,count);
}
#include "binding_editor.h"




/* Gamepad name entry edits a private copy; only Klaar updates the live field. */
static REGISTER_RegStringButton* pilot_keyboard_target;
static int pilot_keyboard_limit,pilot_keyboard_selected;

static char pilot_keyboard_text[48];

void XwPilotKeyboard_Request(REGISTER_RegStringButton* target,int limit) {
 if(pilot_keyboard_pending || pilot_keyboard_open || !target) return;
 const AeronInputSnapshot* input=Aeron_InputSnapshot();
 bool handheld=false;
 if(input) for(int i=0;i<AERON_CONTROLLER_MAX;i++)
  handheld|=input->controllers[i].connected && !strcmp(input->controllers[i].name,"H700 Gamepad");
 if(!handheld) return;
 pilot_keyboard_target=target; pilot_keyboard_limit=limit;
 snprintf(pilot_keyboard_text,sizeof pilot_keyboard_text,"%s",target->name);
 pilot_keyboard_selected=0; pilot_keyboard_pending=true;
}

static void PilotKeyboard_Close(bool accept) {
 if(accept && pilot_keyboard_target) {
  register_Set_Reg_String_Button_Name(pilot_keyboard_target,pilot_keyboard_text);
  xinpattr_Refresh_Input(&pilot_keyboard_target->input);
  xinpattr_Selected_Input(&pilot_keyboard_target->input);
 }
 pilot_keyboard_target=NULL; pilot_keyboard_open=false;
 XwSettingsMenu_RequestClose();
}

static void PilotKeyboard_Choose(int key) {
 static const char letters[]="QWERTYUIOPASDFGHJKL#ZXCVBNM_-@1234567890";
 size_t length=strlen(pilot_keyboard_text);
 if(key!=19 && key!=29) {
  if(length<(size_t)pilot_keyboard_limit) { pilot_keyboard_text[length]=letters[key]; pilot_keyboard_text[length+1]=0; }
 } else if(key==19) {
  if(length) pilot_keyboard_text[length-1]=0;
 } else PilotKeyboard_Close(true);
}

static void PilotKeyboard_Frame(const AeronInputSnapshot* input,float seconds) {
 unsigned pressed=0;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++) if(input->controllers[i].connected && !strcmp(input->controllers[i].name,"H700 Gamepad"))
  pressed|=input->controllers[i].gamepad_pressed_buttons;
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_UP)) pilot_keyboard_selected=(pilot_keyboard_selected+30)%40;
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_DOWN)) pilot_keyboard_selected=(pilot_keyboard_selected+10)%40;
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_LEFT)) pilot_keyboard_selected=(pilot_keyboard_selected/10)*10+(pilot_keyboard_selected+9)%10;
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_RIGHT)) pilot_keyboard_selected=(pilot_keyboard_selected/10)*10+(pilot_keyboard_selected+1)%10;
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_EAST)) { PilotKeyboard_Close(false); return; }
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_WEST)) PilotKeyboard_Choose(19);
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH)) { PilotKeyboard_Choose(pilot_keyboard_selected); if(!pilot_keyboard_open) return; }
 if(pressed&(1u<<AERON_GAMEPAD_BUTTON_START)) { PilotKeyboard_Close(true); return; }
 static AeronInputSnapshot filtered; filtered=*input;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++) memset(&filtered.controllers[i],0,sizeof filtered.controllers[i]);
 AeronUiTheme saved=*AeronUi_GetTheme(menu.ui),theme=saved;
 theme.text_px=54; theme.title_px=55; theme.help_px=39; theme.row_height=118;
 theme.title_height=76; theme.window_pad=20; theme.item_spacing=14; theme.column_spacing=12;
 memcpy(theme.text,(float[4]){1,1,1,1},sizeof theme.text);
 memcpy(theme.text_dim,(float[4]){0.90f,0.94f,1,1},sizeof theme.text_dim);
 memcpy(theme.surface,(float[4]){0.014f,0.022f,0.040f,1},sizeof theme.surface);
 memcpy(theme.widget_bg,(float[4]){0.06f,0.11f,0.19f,1},sizeof theme.widget_bg);
 AeronUi_SetTheme(menu.ui,&theme);
 AeronUi_BeginFrame(menu.ui,&(AeronUiFrameDesc){.input=&filtered,.dt_seconds=seconds});
 if(AeronUi_BeginWindow(menu.ui,"PILOOTNAAM",&(AeronUiWindowDesc){.width_ref=1560,.height_ref=1050,.centered=1,.flags=(1u<<8)})) {
  char line[80]; snprintf(line,sizeof line,"Naam: %s_  (%zu/%d)",pilot_keyboard_text,strlen(pilot_keyboard_text),pilot_keyboard_limit);
  AeronUi_Label(menu.ui,line);
  AeronUi_Help(menu.ui,"D-pad: kiezen   A: letter   X: wissen   Start: klaar   B: annuleren");
  static const char letters[]="QWERTYUIOPASDFGHJKL#ZXCVBNM_-@1234567890";
  for(int row=0;row<4;row++) {
   AeronUi_BeginColumns(menu.ui,10,NULL);
   for(int col=0;col<10;col++) {
    int key=row*10+col; char label[24];
    if(key!=19 && key!=29) snprintf(label,sizeof label,"%c##key%d",letters[key],key);
    else snprintf(label,sizeof label,"%s",key==19 ? "<" : "OK");
    if(XwQuick_Choice(menu.ui,label,key==pilot_keyboard_selected,&theme)) PilotKeyboard_Choose(key);
    if(col<9) AeronUi_NextColumn(menu.ui);
   }
   AeronUi_EndColumns(menu.ui);
  }
  AeronUi_EndWindow(menu.ui);
 }
 AeronUi_EndFrame(menu.ui); AeronUi_Submit(menu.ui); AeronUi_SetTheme(menu.ui,&saved);
}

#include "weapons_menu.inc"
