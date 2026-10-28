#define EQUIP_UI menu.ui
#define EQUIP_NOTICE quick_notice
#define EQUIP_IMPERIAL 0
#define EQUIP_SPECIAL XW_INPUT_ACTION_TOGGLE_S_FOILS
#define EQUIP_SPECIAL_LABEL "TOGGLE S-FOILS"
#define EQUIP_CLEAR() XwCombos_Clear()
#define EQUIP_QUEUE(a) XwCombos_Queue(a)
#define EQUIP_CLOSE() XwQuick_Close()
#define EQUIP_UNCAPTURE() do{menu.captures=false;}while(0)
#define EQUIP_ACTION(a) XW_INPUT_ACTION_##a
static void WeaponsOpen(void){weapon_choices=EquipmentWeapons();weapon_row=1;weapons_open=true;}
static void WeaponsApply(int action){
 if(!weapon_choices.active)return;
 EQUIP_CLEAR();
 if(!EQUIP_QUEUE(action)){snprintf(EQUIP_NOTICE,sizeof EQUIP_NOTICE,"Could not queue weapon setting.");return;}
 if(EQUIP_CLOSE()){weapons_open=false;quick_leaving=true;}else{EQUIP_CLEAR();snprintf(EQUIP_NOTICE,sizeof EQUIP_NOTICE,"Could not resume flight.");}
}
static void WeaponsActivate(void){
 if(weapon_row==0)WeaponsApply(EQUIP_ACTION(CYCLE_WEAPON_GROUP));
 else if(weapon_row==1&&weapon_choices.count)WeaponsApply(EQUIP_ACTION(FIRING_SINGLE)+weapon_choices.values[weapon_choices.selected]-1);
 else if(weapon_row==2)WeaponsApply(EQUIP_SPECIAL);
 else if(weapon_row==3)WeaponsApply(EQUIP_ACTION(WEAPON_GROUP_PREVIOUS));
 else if(weapon_row==4)WeaponsApply(EQUIP_ACTION(FIRE_WEAPON));
}
static void WeaponsFrame(const AeronInputSnapshot*input,float seconds){
 unsigned pressed=0;for(int i=0;i<AERON_CONTROLLER_MAX;i++)if(input->controllers[i].connected&&!strcmp(input->controllers[i].name,"H700 Gamepad"))pressed|=input->controllers[i].gamepad_pressed_buttons;
 if(input->has_focus){
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_EAST)){weapons_open=false;return;}
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_UP))weapon_row=(weapon_row+4)%5;
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_DOWN))weapon_row=(weapon_row+1)%5;
  int delta=(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_RIGHT)?1:0)-(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_LEFT)?1:0);
  if(weapon_row==1&&weapon_choices.count)weapon_choices.selected=(weapon_choices.selected+delta+weapon_choices.count)%weapon_choices.count;
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH)){WeaponsActivate();if(!weapons_open)return;}
 }
 static AeronInputSnapshot filtered;filtered=*input;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++)if(!strcmp(filtered.controllers[i].name,"H700 Gamepad")){
  filtered.controllers[i].gamepad_buttons=filtered.controllers[i].gamepad_pressed_buttons=filtered.controllers[i].gamepad_released_buttons=0;
  memset(filtered.controllers[i].gamepad_axes,0,sizeof filtered.controllers[i].gamepad_axes);
 }
 AeronUiContext*ui=EQUIP_UI;AeronUiTheme original=*AeronUi_GetTheme(ui),theme=original;CommandCockpitTheme(&theme);
 AeronUi_SetTheme(ui,&theme);AeronUi_BeginFrame(ui,&(AeronUiFrameDesc){.input=&filtered,.dt_seconds=seconds});
 bool activate=false,back=false;
 if(AeronUi_CockpitBegin(ui,"WEAPONS",EQUIP_IMPERIAL,1)){
  AeronUi_CockpitText(ui,"UP / DOWN SELECT  |  LEFT / RIGHT MODE",100,175,1320,65,33,0,0);
  /* One section with exclusive choices; native values remain live on reopen. */
  if(AeronUi_CockpitButton(ui,"SELECT WEAPON",95,285,420,90,weapon_row==0,1)){weapon_row=0;activate=true;}
  AeronUi_CockpitText(ui,weapon_choices.active?(weapon_choices.warhead?"WARHEADS":"CANNONS"):"NO ACTIVE FLIGHT",560,290,800,75,38,0,0);
  AeronUi_CockpitText(ui,"FIRING MODE",110,385,1300,55,38,weapon_row==1?1:0,1);
  for(int i=0;i<weapon_choices.count;i++){
   float w=1280.f/weapon_choices.count;
   const char*label=WeaponChoiceLabel(&weapon_choices,i);
   if(AeronUi_CockpitButton(ui,label,110+i*w,450,w-15,105,i==weapon_choices.selected,1)){weapon_row=1;weapon_choices.selected=i;}
  }
  if(!weapon_choices.count)AeronUi_CockpitText(ui,"NO WEAPON AVAILABLE",110,450,1280,100,38,0,0);
  AeronUi_CockpitBlueprint(ui,EquipmentCraftName(),110,615,260,210);
  if(AeronUi_CockpitButton(ui,EQUIP_SPECIAL_LABEL,405,610,480,92,weapon_row==2,1)){weapon_row=2;activate=true;}
  if(AeronUi_CockpitButton(ui,"PREVIOUS WEAPON",910,610,495,92,weapon_row==3,1)){weapon_row=3;activate=true;}
  if(AeronUi_CockpitButton(ui,"FIRE ONCE",405,723,480,92,weapon_row==4,1)){weapon_row=4;activate=true;}
  AeronUi_CockpitText(ui,"A APPLIES & RESUMES",920,730,475,70,29,0,0);
  if(AeronUi_CockpitButton(ui,"A APPLY",55,900,430,85,0,0))activate=true;
  if(AeronUi_CockpitButton(ui,"B BACK",565,900,400,85,0,0))back=true;
  AeronUi_CockpitText(ui,"LEFT / RIGHT MODE",1040,900,440,85,30,0,0);
 }else{
  AeronUiWindowDesc window={.width_ref=1300,.height_ref=1000,.centered=1};
  if(AeronUi_BeginWindow(ui,"WEAPONS",&window)){
   if(AeronUi_Button(ui,"SELECT WEAPON")){weapon_row=0;activate=true;}
   AeronUi_Label(ui,"FIRING MODE");
   for(int i=0;i<weapon_choices.count;i++)if(AeronUi_Button(ui,WeaponChoiceLabel(&weapon_choices,i))){weapon_choices.selected=i;weapon_row=1;}
   if(AeronUi_Button(ui,EQUIP_SPECIAL_LABEL)){weapon_row=2;activate=true;}
   if(AeronUi_Button(ui,"PREVIOUS WEAPON")){weapon_row=3;activate=true;}
   if(AeronUi_Button(ui,"A APPLY"))activate=true;
   if(AeronUi_Button(ui,"B BACK"))back=true;
   AeronUi_EndWindow(ui);
  }
 }
 AeronUiOutput out=AeronUi_EndFrame(ui);EQUIP_UNCAPTURE();AeronUi_Submit(ui);AeronUi_SetTheme(ui,&original);
 if(back||out.cancel_pressed)weapons_open=false;else if(activate)WeaponsActivate();
}
