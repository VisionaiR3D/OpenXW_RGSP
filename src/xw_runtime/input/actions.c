/* Registry shape follows OpenXvT; bytes come from X-Wing's recovered input consumers. */
#include "xw_runtime/input/actions.h"
#include <string.h>
#include "xw_runtime/input/energy_distribution.h"
#include "aeron/log.h"
#include "xw/flight/player/user.h"

static const struct {
	const char *name, *label;
	XwInputActionCategory category;
	uint16_t key;
} g_actions[] = {
	{ "none", "None", XW_INPUT_ACTION_CATEGORY_SYSTEM, 0 },
	{ "fire_weapon", "Fire weapon", XW_INPUT_ACTION_CATEGORY_WEAPONS, 156 },
	{ "target_roll_modifier", "Roll / target modifier", XW_INPUT_ACTION_CATEGORY_TARGETING, 157 },
	{ "cycle_weapon_group", "Cycle weapon group", XW_INPUT_ACTION_CATEGORY_WEAPONS, 119 },
	{ "cycle_weapon_firing_mode", "Cycle firing mode", XW_INPUT_ACTION_CATEGORY_WEAPONS, 120 },
	{ "toggle_s_foils", "Toggle S-foils", XW_INPUT_ACTION_CATEGORY_WEAPONS, 102 },
	{ "cycle_cannon_recharge_rate", "Cycle cannon recharge", XW_INPUT_ACTION_CATEGORY_WEAPONS, 203 },
	{ "cycle_shield_recharge_rate", "Cycle shield recharge", XW_INPUT_ACTION_CATEGORY_WEAPONS, 204 },
	{ "cycle_shield_mode", "Cycle shield distribution", XW_INPUT_ACTION_CATEGORY_WEAPONS, 115 },
	{ "xfer_shields_to_cannon", "Transfer shields to cannons", XW_INPUT_ACTION_CATEGORY_WEAPONS, 215 },
	{ "xfer_cannon_to_shields", "Transfer cannons to shields", XW_INPUT_ACTION_CATEGORY_WEAPONS, 216 },
	{ "target_next", "Next target", XW_INPUT_ACTION_CATEGORY_TARGETING, 116 },
	{ "target_prev", "Previous target", XW_INPUT_ACTION_CATEGORY_TARGETING, 121 },
	{ "target_nearest_fighter", "Nearest enemy fighter", XW_INPUT_ACTION_CATEGORY_TARGETING, 114 },
	{ "target_my_attacker", "Nearest attacker", XW_INPUT_ACTION_CATEGORY_TARGETING, 101 },
	{ "target_clear", "Clear target", XW_INPUT_ACTION_CATEGORY_TARGETING, 111 },
	{ "identify", "Toggle target identification", XW_INPUT_ACTION_CATEGORY_TARGETING, 105 },
	{ "auto_target", "Target in crosshairs", XW_INPUT_ACTION_CATEGORY_TARGETING, 155 },
	{ "target_preset_recall_1", "Recall target 1", XW_INPUT_ACTION_CATEGORY_TARGETING, 199 },
	{ "target_preset_store_1", "Store target 1", XW_INPUT_ACTION_CATEGORY_TARGETING, 211 },
	{ "target_preset_recall_2", "Recall target 2", XW_INPUT_ACTION_CATEGORY_TARGETING, 200 },
	{ "target_preset_store_2", "Store target 2", XW_INPUT_ACTION_CATEGORY_TARGETING, 212 },
	{ "target_preset_recall_3", "Recall target 3", XW_INPUT_ACTION_CATEGORY_TARGETING, 201 },
	{ "target_preset_store_3", "Store target 3", XW_INPUT_ACTION_CATEGORY_TARGETING, 213 },
	{ "target_preset_recall_4", "Recall target 4", XW_INPUT_ACTION_CATEGORY_TARGETING, 202 },
	{ "target_preset_store_4", "Store target 4", XW_INPUT_ACTION_CATEGORY_TARGETING, 214 },
	{ "throttle_up", "Increase throttle", XW_INPUT_ACTION_CATEGORY_THROTTLE, 61 },
	{ "throttle_down", "Decrease throttle", XW_INPUT_ACTION_CATEGORY_THROTTLE, 45 },
	{ "throttle_zero", "Stop engines", XW_INPUT_ACTION_CATEGORY_THROTTLE, 92 },
	{ "throttle_one_third", "One-third throttle", XW_INPUT_ACTION_CATEGORY_THROTTLE, 91 },
	{ "throttle_two_thirds", "Two-thirds throttle", XW_INPUT_ACTION_CATEGORY_THROTTLE, 93 },
	{ "throttle_full", "Full throttle", XW_INPUT_ACTION_CATEGORY_THROTTLE, 8 },
	{ "match_speed", "Match target speed", XW_INPUT_ACTION_CATEGORY_THROTTLE, 13 },
	{ "view_toggle_elevated", "Toggle elevated view", XW_INPUT_ACTION_CATEGORY_VIEW, 48 },
	{ "hold_view_elevated", "Hold elevated view", XW_INPUT_ACTION_CATEGORY_VIEW, 178 },
	{ "view_left_shoulder", "View left shoulder", XW_INPUT_ACTION_CATEGORY_VIEW, 49 },
	{ "hold_view_left_shoulder", "Hold view left shoulder", XW_INPUT_ACTION_CATEGORY_VIEW, 179 },
	{ "view_rear", "View rear", XW_INPUT_ACTION_CATEGORY_VIEW, 50 },
	{ "hold_view_rear", "Hold view rear", XW_INPUT_ACTION_CATEGORY_VIEW, 180 },
	{ "view_right_shoulder", "View right shoulder", XW_INPUT_ACTION_CATEGORY_VIEW, 51 },
	{ "hold_view_right_shoulder", "Hold view right shoulder", XW_INPUT_ACTION_CATEGORY_VIEW, 181 },
	{ "view_left_wing", "View left wing", XW_INPUT_ACTION_CATEGORY_VIEW, 52 },
	{ "hold_view_left_wing", "Hold view left wing", XW_INPUT_ACTION_CATEGORY_VIEW, 182 },
	{ "view_straight_up", "View straight up", XW_INPUT_ACTION_CATEGORY_VIEW, 53 },
	{ "hold_view_straight_up", "Hold view straight up", XW_INPUT_ACTION_CATEGORY_VIEW, 183 },
	{ "view_right_wing", "View right wing", XW_INPUT_ACTION_CATEGORY_VIEW, 54 },
	{ "hold_view_right_wing", "Hold view right wing", XW_INPUT_ACTION_CATEGORY_VIEW, 184 },
	{ "view_left_forward", "View left forward", XW_INPUT_ACTION_CATEGORY_VIEW, 55 },
	{ "hold_view_left_forward", "Hold view left forward", XW_INPUT_ACTION_CATEGORY_VIEW, 185 },
	{ "view_forward", "View forward", XW_INPUT_ACTION_CATEGORY_VIEW, 56 },
	{ "hold_view_forward", "Hold view forward", XW_INPUT_ACTION_CATEGORY_VIEW, 186 },
	{ "view_right_forward", "View right forward", XW_INPUT_ACTION_CATEGORY_VIEW, 57 },
	{ "hold_view_right_forward", "Hold view right forward", XW_INPUT_ACTION_CATEGORY_VIEW, 187 },
	{ "view_toggle_cockpit", "Toggle cockpit", XW_INPUT_ACTION_CATEGORY_VIEW, 46 },
	{ "view_player", "Return to player craft", XW_INPUT_ACTION_CATEGORY_VIEW, 195 },
	{ "view_warhead", "Follow fired warhead", XW_INPUT_ACTION_CATEGORY_VIEW, 196 },
	{ "view_external", "Toggle external view", XW_INPUT_ACTION_CATEGORY_VIEW, 47 },
	{ "view_manual_camera", "Toggle camera control", XW_INPUT_ACTION_CATEGORY_VIEW, 63 },
	{ "info_map", "Map", XW_INPUT_ACTION_CATEGORY_INFORMATION, 109 },
	{ "info_damage", "Damage report", XW_INPUT_ACTION_CATEGORY_INFORMATION, 100 },
	{ "info_briefing", "Mission briefing", XW_INPUT_ACTION_CATEGORY_INFORMATION, 98 },
	{ "replay_record", "Toggle film recording", XW_INPUT_ACTION_CATEGORY_INFORMATION, 99 },
	{ "replay_view", "View recorded film", XW_INPUT_ACTION_CATEGORY_INFORMATION, 118 },
	{ "comm_head_home", "Order target home", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 72 },
	{ "comm_stop_and_wait", "Stop and wait", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 87 },
	{ "comm_continue_mission", "Continue mission", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 71 },
	{ "comm_evade", "Evade", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 69 },
	{ "comm_assign_target", "Attack my target", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 65 },
	{ "comm_cover_me", "Cover me", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 67 },
	{ "comm_ignore_target", "Ignore target", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 73 },
	{ "comm_report_orders", "Report orders", XW_INPUT_ACTION_CATEGORY_COMMUNICATIONS, 82 },
	{ "hyperspace", "Hyperspace / leave proving grounds", XW_INPUT_ACTION_CATEGORY_SYSTEM, 104 },
	{ "eject", "Eject", XW_INPUT_ACTION_CATEGORY_SYSTEM, 132 },
	{ "confirm_order", "Target incoming warhead / acknowledge mission end", XW_INPUT_ACTION_CATEGORY_SYSTEM,
	  32 },
	{ "pause", "Pause", XW_INPUT_ACTION_CATEGORY_SYSTEM, 112 },
	{ "escape", "Open settings", XW_INPUT_ACTION_CATEGORY_SYSTEM, 27 },
	{ "cycle_graphics_detail", "Cycle graphics detail", XW_INPUT_ACTION_CATEGORY_SYSTEM, 0 },
	{ "rebuild_cockpit", "Rebuild cockpit", XW_INPUT_ACTION_CATEGORY_SYSTEM, 0 },
	{ "toggle_interlace", "Toggle interlace", XW_INPUT_ACTION_CATEGORY_SYSTEM, 0 },
	{ "show_version", "Show version", XW_INPUT_ACTION_CATEGORY_SYSTEM, 149 },
	{ "frame_rate", "Frame rate display", XW_INPUT_ACTION_CATEGORY_SYSTEM, 0 },
	{ "toggle_music", "Toggle music", XW_INPUT_ACTION_CATEGORY_SYSTEM, 140 },
	{ "toggle_sound", "Toggle sound effects", XW_INPUT_ACTION_CATEGORY_SYSTEM, 146 },
 {"engine_more","Meer motorenergie",XW_INPUT_ACTION_CATEGORY_WEAPONS,248},
 {"engine_less","Minder motorenergie",XW_INPUT_ACTION_CATEGORY_WEAPONS,249},
 {"energy_normalize","Energie normaal",XW_INPUT_ACTION_CATEGORY_WEAPONS,514},
 {"cannon_recharge_increase","Lasers sneller opladen",XW_INPUT_ACTION_CATEGORY_WEAPONS,515},
 {"cannon_recharge_decrease","Lasers langzamer opladen",XW_INPUT_ACTION_CATEGORY_WEAPONS,516},
 {"shields_forward","Shields naar voren",XW_INPUT_ACTION_CATEGORY_WEAPONS,517},
 {"shields_rear","Shields naar achteren",XW_INPUT_ACTION_CATEGORY_WEAPONS,518},
 {"shields_balanced","Shields gelijk verdelen",XW_INPUT_ACTION_CATEGORY_WEAPONS,519},
 {"weapon_group_previous","Vorig wapen",XW_INPUT_ACTION_CATEGORY_WEAPONS,520},
 {"laser_rate_0","Laser laadstand 0",XW_INPUT_ACTION_CATEGORY_WEAPONS,530},
 {"laser_rate_1","Laser laadstand 1",XW_INPUT_ACTION_CATEGORY_WEAPONS,531},
 {"laser_rate_2","Laser laadstand 2",XW_INPUT_ACTION_CATEGORY_WEAPONS,532},
 {"laser_rate_3","Laser laadstand 3",XW_INPUT_ACTION_CATEGORY_WEAPONS,533},
 {"laser_rate_4","Laser laadstand 4",XW_INPUT_ACTION_CATEGORY_WEAPONS,534},
 {"shield_rate_0","Shield laadstand 0",XW_INPUT_ACTION_CATEGORY_WEAPONS,535},
 {"shield_rate_1","Shield laadstand 1",XW_INPUT_ACTION_CATEGORY_WEAPONS,536},
 {"shield_rate_2","Shield laadstand 2",XW_INPUT_ACTION_CATEGORY_WEAPONS,537},
 {"shield_rate_3","Shield laadstand 3",XW_INPUT_ACTION_CATEGORY_WEAPONS,538},
 {"shield_rate_4","Shield laadstand 4",XW_INPUT_ACTION_CATEGORY_WEAPONS,539},
 { "roll_left", "Roll links (vasthouden)", XW_INPUT_ACTION_CATEGORY_VIEW, 0 },
 { "roll_right", "Roll rechts (vasthouden)", XW_INPUT_ACTION_CATEGORY_VIEW, 0 },
 {"engine_level_0","Motorenergie 0",XW_INPUT_ACTION_CATEGORY_WEAPONS,560},
 {"engine_level_1","Motorenergie 1",XW_INPUT_ACTION_CATEGORY_WEAPONS,561},
 {"engine_level_2","Motorenergie 2",XW_INPUT_ACTION_CATEGORY_WEAPONS,562},
 {"engine_level_3","Motorenergie 3",XW_INPUT_ACTION_CATEGORY_WEAPONS,563},
 {"engine_level_4","Motorenergie 4",XW_INPUT_ACTION_CATEGORY_WEAPONS,564},
 {"firing_single","Firing single",XW_INPUT_ACTION_CATEGORY_WEAPONS,570},
 {"firing_pair","Firing pair",XW_INPUT_ACTION_CATEGORY_WEAPONS,571},
 {"firing_linked","Firing linked",XW_INPUT_ACTION_CATEGORY_WEAPONS,572},
 {"target_list_open","Open target list",XW_INPUT_ACTION_CATEGORY_TARGETING,0},
};

XwInputAction XwInputActions_FromName(const char* name) {
	if (name)
		for (int i = 0; i < XW_INPUT_ACTION_COUNT; ++i)
			if (!strcmp(name, g_actions[i].name))
				return (XwInputAction)i;
	return XW_INPUT_ACTION_NONE;
}

const char* XwInputActions_ToName(XwInputAction action) {
	return (unsigned)action < XW_INPUT_ACTION_COUNT ? g_actions[action].name : "none";
}

const char* XwInputActions_DisplayName(XwInputAction action) {
	return (unsigned)action < XW_INPUT_ACTION_COUNT ? g_actions[action].label : "None";
}

XwInputActionCategory XwInputActions_Category(XwInputAction action) {
	return (unsigned)action < XW_INPUT_ACTION_COUNT ? g_actions[action].category
													: XW_INPUT_ACTION_CATEGORY_SYSTEM;
}

const char* XwInputActions_CategoryName(XwInputActionCategory category) {
	static const char* names[] = { "Weapons",     "Targeting", "Throttle",      "View",
								   "Information", "System",    "Communications" };
	return (unsigned)category < XW_INPUT_ACTION_CATEGORY_COUNT ? names[category] : "";
}

uint16_t XwInputActions_Key(XwInputAction action) {
	return (unsigned)action < XW_INPUT_ACTION_COUNT ? g_actions[action].key : 0;
}

bool XwInputActions_Visible(XwInputAction action) {
	return action != XW_INPUT_ACTION_CYCLE_GRAPHICS_DETAIL && action != XW_INPUT_ACTION_TOGGLE_INTERLACE &&
		   action != XW_INPUT_ACTION_REBUILD_COCKPIT && action != XW_INPUT_ACTION_FRAME_RATE;
}

bool XwInputActions_KeyboardBindable(XwInputAction action) {
	return action > XW_INPUT_ACTION_NONE && action < XW_INPUT_ACTION_ENGINE_MORE;
}

uint16_t XwInputActions_ReleaseKey(XwInputAction action) {
	uint16_t key = XwInputActions_Key(action);
	return key == 0xB2 ? 0xB2 : key > 0xB2 && key <= 0xBB ? 0xBA : 0;
}

void XwInputActions_EngineEnergy(XwInputAction action) {
 if(!g_playerFlightState.craft) return;
 CraftData* craft=g_playerFlightState.craft;
 EnergyDistribution next=EnergyRedistribute(craft->laserRedirect,craft->shieldRedirect,
   (craft->workingSubsystems & XW_CRAFT_SUBSYSTEM_SHIELDS)!=0,action==XW_INPUT_ACTION_ENGINE_MORE);
 craft->laserRedirect=next.lasers; craft->shieldRedirect=next.shields;
 Aeron_LogInfo("xw.energy","%s: lasers=%d shields=%d",XwInputActions_ToName(action),next.lasers,next.shields);
}

void XwInputActions_EnginePreset(int level) {
 CraftData* craft=g_playerFlightState.craft;
 if(!craft) return;
 EnergyDistribution next=EnergyEnginePreset(level,(craft->workingSubsystems & XW_CRAFT_SUBSYSTEM_SHIELDS)!=0);
 craft->laserRedirect=next.lasers;
 craft->shieldRedirect=next.shields;
 Aeron_LogInfo("xw.energy","engine preset %d: lasers=%d shields=%d",level,next.lasers,next.shields);
}
