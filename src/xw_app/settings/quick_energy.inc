extern void AeronUi_CommandPowerMeter(AeronUiContext*,const char*,int,int,int,int);
static EnergyEditor energy_draft;
static int energy_shield_mode,energy_shield_original;
static void EnergyAdjust(int delta){if(energy_draft.row==2){if(energy_draft.has_shields)energy_shield_mode=(energy_shield_mode+delta+3)%3;}else EnergyEditor_Adjust(&energy_draft,delta);}
static void EnergyOpen(void){
 energy_shield_mode=energy_shield_original=EquipmentShieldMode();
 CraftData* craft=g_playerFlightState.craft;
 if(!craft){
  if(getenv("RGSP_ENERGY_EDITOR_PREVIEW")){EnergyEditor_Begin(&energy_draft,2,2,true);energy_open=true;return;}
  snprintf(quick_notice,sizeof quick_notice,"No active flight.");return;
 }
 EnergyEditor_Begin(&energy_draft,craft->laserRedirect,craft->shieldRedirect,
   (craft->workingSubsystems&XW_CRAFT_SUBSYSTEM_SHIELDS)!=0);
 energy_open=true;quick_notice[0]=0;
}
static void EnergyCommit(void){
 if(!energy_open)return;
 if(!g_playerFlightState.craft){energy_open=false;return;}
 XwCombos_Clear();
 bool ok=XwCombos_Queue(XW_INPUT_ACTION_LASER_RATE_0+energy_draft.lasers);
 if(energy_draft.has_shields)ok=ok&&XwCombos_Queue(XW_INPUT_ACTION_SHIELD_RATE_0+energy_draft.shields);
 if(energy_draft.has_shields&&energy_shield_mode!=energy_shield_original)ok=ok&&XwCombos_Queue((energy_shield_mode==0?XW_INPUT_ACTION_SHIELDS_FORWARD:energy_shield_mode==2?XW_INPUT_ACTION_SHIELDS_REAR:XW_INPUT_ACTION_SHIELDS_BALANCED));
 if(!ok){XwCombos_Clear();snprintf(quick_notice,sizeof quick_notice,"Could not apply power distribution.");return;}
 energy_open=false;quick_leaving=true;(void)XwQuick_Close();
}
static void EnergyFrame(const AeronInputSnapshot* input,float seconds){
 unsigned pressed=0;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++){
  const AeronControllerSnapshot* d=&input->controllers[i];
  if(d->connected&&!strcmp(d->name,"H700 Gamepad"))pressed|=d->gamepad_pressed_buttons;
 }
 if(input->has_focus){
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_EAST)){energy_open=false;return;}
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH)){EnergyCommit();return;}
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_UP))energy_draft.row=(energy_draft.row+2)%3;
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_DOWN))energy_draft.row=(energy_draft.row+1)%3;
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_LEFT))EnergyAdjust(-1);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_RIGHT))EnergyAdjust(1);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_NORTH))EnergyEditor_Begin(&energy_draft,4,1,energy_draft.has_shields);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_RIGHT_SHOULDER))EnergyEditor_Begin(&energy_draft,0,0,energy_draft.has_shields);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_WEST)){
   int row=energy_draft.row;EnergyEditor_Begin(&energy_draft,2,2,energy_draft.has_shields);energy_draft.row=row;
  }
 }
 static AeronInputSnapshot ui_input;ui_input=*input;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++)if(!strcmp(ui_input.controllers[i].name,"H700 Gamepad")){
  ui_input.controllers[i].gamepad_buttons=0;ui_input.controllers[i].gamepad_pressed_buttons=0;
  ui_input.controllers[i].gamepad_released_buttons=0;memset(ui_input.controllers[i].gamepad_axes,0,sizeof ui_input.controllers[i].gamepad_axes);
 }
 AeronUiContext* ui=menu.ui;AeronUiTheme original=*AeronUi_GetTheme(ui),theme=original;
 theme.text_px=38;theme.title_px=48;theme.help_px=30;theme.row_height=90;theme.title_height=76;
 theme.window_pad=24;theme.item_spacing=16;
 CommandCockpitTheme(&theme);
 AeronUi_SetTheme(ui,&theme);AeronUi_BeginFrame(ui,&(AeronUiFrameDesc){.input=&ui_input,.dt_seconds=seconds});
 bool apply=false,cancel=false,individual=false;
 AeronUiWindowDesc window={.width_ref=1560,.height_ref=1050,.centered=1,.flags=(1u<<8)};
 if(AeronUi_CockpitBegin(ui,"SHIP & POWER",0,1)){
  AeronUi_CockpitText(ui,"UP / DOWN SELECT  |  LEFT / RIGHT ADJUST",120,175,600,65,30,0,0);
  AeronUi_CockpitText(ui,"THROTTLE UNCHANGED",800,175,600,65,31,0,0);
  const char* names[]={"LASERS","SHIELDS","ENGINES"};
  for(int row=0;row<3;row++){
   int value=row==0?energy_draft.lasers:row==1?energy_draft.shields:EnergyEditor_Motors(&energy_draft);
   char id[32];snprintf(id,sizeof id,"##power-%d",row);
   if(AeronUi_CockpitButton(ui,id,95,285+row*90,1350,86,row<2&&row==energy_draft.row,0)&&row<2)energy_draft.row=row;
   if(row==1&&!energy_draft.has_shields)AeronUi_CockpitText(ui,"SHIELDS NOT FITTED",150,375,1180,78,38,0,1);
   else AeronUi_CockpitMeter(ui,names[row],value,row==2?8:4,140,295+row*90,1250,row);
  }
  if(AeronUi_CockpitButton(ui,"##shield-direction",95,610,1350,190,energy_draft.row==2,1))energy_draft.row=2;
  AeronUi_CockpitText(ui,"SHIELD DIRECTION",130,635,390,110,38,0,1);
  AeronUi_CockpitBlueprint(ui,EquipmentCraftName(),535,610,240,145);
  AeronUi_CockpitText(ui,"FWD",535,755,110,35,24,energy_draft.has_shields&&energy_shield_mode!=2?1:0,0);
  AeronUi_CockpitText(ui,"AFT",665,755,110,35,24,energy_draft.has_shields&&energy_shield_mode!=0?1:0,0);
  const char*modes[]={"FRONT","BALANCED","REAR"};
  if(energy_draft.has_shields){
   if(AeronUi_CockpitButton(ui,"<",805,655,100,95,0,0)){energy_draft.row=2;EnergyAdjust(-1);}
   AeronUi_CockpitText(ui,modes[energy_shield_mode],920,655,330,95,42,1,0);
   if(AeronUi_CockpitButton(ui,">",1280,655,100,95,0,0)){energy_draft.row=2;EnergyAdjust(1);}
  }else AeronUi_CockpitText(ui,"NOT FITTED",820,650,550,100,36,0,0);
  AeronUi_CockpitText(ui,"A APPLIES & RESUMES  |  B CANCELS",120,815,1290,40,27,0,0);
  if(AeronUi_CockpitButton(ui,"A APPLY",55,900,280,85,0,0))apply=true;
  if(AeronUi_CockpitButton(ui,"B CANCEL",365,900,240,85,0,0))cancel=true;
  if(AeronUi_CockpitButton(ui,"X BALANCED",635,900,240,85,0,0))EnergyEditor_Begin(&energy_draft,2,2,energy_draft.has_shields);
  if(AeronUi_CockpitButton(ui,"Y WEAPONS",910,900,265,85,0,0))EnergyEditor_Begin(&energy_draft,4,1,energy_draft.has_shields);
  if(AeronUi_CockpitButton(ui,"R1 ENGINES",1210,900,275,85,0,0))EnergyEditor_Begin(&energy_draft,0,0,energy_draft.has_shields);
  if(quick_notice[0])AeronUi_CockpitText(ui,quick_notice,120,815,1290,40,25,2,0);
 }else if(AeronUi_BeginWindow(ui,"POWER DISTRIBUTION | PAUSED",&window)){
  AeronUi_Help(ui,"UP / DOWN SELECT   |   LEFT / RIGHT ADJUST");
  const char* labels[]={"LASERS","SHIELDS","ENGINES"};
  for(int row=0;row<3;row++){
   int value=row==0?energy_draft.lasers:row==1?energy_draft.shields:EnergyEditor_Motors(&energy_draft);
   if(row==1&&!energy_draft.has_shields)AeronUi_Label(ui,"SHIELDS: NOT FITTED");
   else AeronUi_CommandPowerMeter(ui,labels[row],value,row==2?8:4,row,row==energy_draft.row);
  }
  char shield_line[80];snprintf(shield_line,sizeof shield_line,"SHIELD DIRECTION: %s",energy_draft.has_shields?(const char*[]){"FRONT","BALANCED","REAR"}[energy_shield_mode]:"NOT FITTED");
  if(AeronUi_Button(ui,shield_line)){energy_draft.row=2;EnergyAdjust(1);}
  AeronUi_SetTheme(ui,&theme);
  static const char* charge[]={"FAST DRAIN","SLOW DRAIN","MAINTAIN","RECHARGE","MAX RECHARGE"};
  char line[200];snprintf(line,sizeof line,"LASERS: %s | SHIELDS: %s",charge[energy_draft.lasers],energy_draft.has_shields?charge[energy_draft.shields]:"NOT FITTED");
  AeronUi_Help(ui,line);AeronUi_Help(ui,"NOT APPLIED   |   THROTTLE UNCHANGED");
  AeronUi_Help(ui,"A APPLY & RESUME   |   X BALANCED   |   B CANCEL");
  AeronUi_BeginColumns(ui,3,NULL);
  if(AeronUi_Button(ui,"X BALANCED"))EnergyEditor_Begin(&energy_draft,2,2,energy_draft.has_shields);
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"Y WEAPONS"))EnergyEditor_Begin(&energy_draft,4,energy_draft.has_shields?1:0,energy_draft.has_shields);
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"R1 FULL ENGINES"))EnergyEditor_Begin(&energy_draft,0,0,energy_draft.has_shields);
  AeronUi_EndColumns(ui);
  if(quick_notice[0])AeronUi_Error(ui,quick_notice);
  AeronUi_BeginColumns(ui,3,NULL);
  if(AeronUi_Button(ui,"B CANCEL"))cancel=true;AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"MORE SETTINGS"))individual=true;AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"A APPLY & RESUME"))apply=true;
  AeronUi_EndColumns(ui);AeronUi_EndWindow(ui);
 }
 AeronUiOutput out=AeronUi_EndFrame(ui);menu.captures=false;
 AeronUi_Submit(ui);AeronUi_SetTheme(ui,&original);
 if(apply)EnergyCommit();
 else if(cancel||out.cancel_pressed)energy_open=false;
 else if(individual){energy_open=false;quick_node=4;quick_selected=0;}
}
