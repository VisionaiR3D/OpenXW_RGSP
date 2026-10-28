#include "xw/flight/object/craft.h"
#define EQUIPMENT_XW
/* Same model as the native cannon/launcher x-key handler. */
typedef struct {int count,values[3],selected,cannons;bool active,warhead;const char*craft;} WeaponChoices;
static WeaponChoices weapon_choices;
static int weapon_row;
static const char* WeaponChoiceLabel(const WeaponChoices*c,int i){
 if(c->values[i]==1)return "SINGLE";
 if(c->values[i]==2||c->warhead||c->cannons==2)return "DUAL";
 return c->cannons==4?"QUAD":"LINKED";
}
static WeaponChoices EquipmentWeapons(void){
 WeaponChoices c={0};
#ifdef EQUIPMENT_XW
 CraftData*p=g_playerFlightState.craft;if(!p)return c;
 int bank=g_playerFlightState.selectedWeaponBank;if(bank>=4||g_playerFlightState.craftTypeIndex>=XW_CRAFT_TYPE_COUNT)return c;
 c.warhead=g_playerFlightState.selectedWeaponMode!=0;if(c.warhead&&bank>=2)return c;
 c.craft=(const char*)g_craftTypeDefs[g_playerFlightState.craftTypeIndex].name;
 c.cannons=c.warhead?2:g_craftTypeDefs[g_playerFlightState.craftTypeIndex].laserGroupSlotCount[bank];
 int mode=c.warhead?((p->warheadLauncherFlags[bank]&2)?3:1):p->laserState.linkMode[bank];
#else
 CraftData*p=pstate.player_craft;if(!p)return c;
 int bank=pstate.player_weapon_group;if(bank>=2||pstate.player_spec_num>=NUM_SPEC_DATA)return c;
 c.warhead=pstate.player_weapon_mode!=0;c.craft=spec_name_ptrs[p->species_idx];
 c.cannons=c.warhead?2:spec_data[pstate.player_spec_num].laser_count[bank];
 int mode=c.warhead?((p->missile_armed[bank]&2)?3:1):p->laser_owner_player[bank];
#endif
 c.active=true;if(c.cannons<=0)return c;
 c.values[c.count++]=1;
 if(!c.warhead&&c.cannons==4)c.values[c.count++]=2;
 if(c.cannons>1)c.values[c.count++]=3;
 for(int i=0;i<c.count;i++)if(c.values[i]==mode)c.selected=i;
 return c;
}
static int EquipmentShieldMode(void){
#ifdef EQUIPMENT_XW
 return g_playerFlightState.craft?g_playerFlightState.craft->shieldDistribMode:1;
#else
 return pstate.player_craft?pstate.player_craft->is_player_craft:1;
#endif
}
static const char* EquipmentCraftName(void){WeaponChoices c=EquipmentWeapons();return c.craft?c.craft:"NO ACTIVE CRAFT";}
