static int slot_selected,slot_mode,slot_confirm=-1;
static bool slot_exists[5];
static char slot_labels[5][150],slot_notice[300];
static void SlotsRefresh(void){for(int i=0;i<5;i++)slot_exists[i]=XwSaveSlots_Info(i+1,slot_labels[i],sizeof slot_labels[i]);}
static void SlotsOpen(void){
 slots_open=true;slot_selected=0;slot_confirm=-1;slot_notice[0]=0;
 slot_mode=XwFlightSim_IsPlayerControl()?0:1;SlotsRefresh();
}
static void SlotsChoose(void){
 if(slot_mode==0&&!XwFlightSim_IsPlayerControl()){snprintf(slot_notice,sizeof slot_notice,"Opslaan kan alleen tijdens een missie.");return;}
 if(slot_mode==1&&!slot_exists[slot_selected]){snprintf(slot_notice,sizeof slot_notice,"Dit slot is leeg.");return;}
 if(slot_confirm!=slot_selected){
  slot_confirm=slot_selected;
  snprintf(slot_notice,sizeof slot_notice,slot_mode?"Slot laden? A: bevestigen. Huidige missie wordt vervangen.":slot_exists[slot_selected]?"Slot overschrijven? A: bevestigen.":"Missie opslaan in dit slot? A: bevestigen.");return;
 }
 slot_confirm=-1;
 if(slot_mode==0){
  if(XwSaveSlots_Save(slot_selected+1)){SlotsRefresh();snprintf(slot_notice,sizeof slot_notice,"Slot %d opgeslagen.",slot_selected+1);}
  else snprintf(slot_notice,sizeof slot_notice,"%s",XwSaveSlots_Error());
 }else{
  if(XwSaveSlots_RequestLoad(slot_selected+1)){snprintf(slot_notice,sizeof slot_notice,"Slot wordt geladen...");Aeron_RequestQuit();}
  else snprintf(slot_notice,sizeof slot_notice,"%s",XwSaveSlots_Error());
 }
}
static void SlotsBack(void){slots_open=false;slot_confirm=-1;if(!quick_enabled)menu.page=XW_SETTINGS_GAME;}
static void SlotsFrame(const AeronInputSnapshot* input,float seconds){
 unsigned pressed=0;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++)if(input->controllers[i].connected&&!strcmp(input->controllers[i].name,"H700 Gamepad"))pressed|=input->controllers[i].gamepad_pressed_buttons;
 if(input->has_focus){
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_EAST)){SlotsBack();return;}
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_UP)){slot_selected=(slot_selected+4)%5;slot_confirm=-1;}
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_DOWN)){slot_selected=(slot_selected+1)%5;slot_confirm=-1;}
  if(pressed&((1u<<AERON_GAMEPAD_BUTTON_LEFT_SHOULDER)|(1u<<AERON_GAMEPAD_BUTTON_DPAD_LEFT))){slot_mode=0;slot_confirm=-1;slot_notice[0]=0;}
  if(pressed&((1u<<AERON_GAMEPAD_BUTTON_RIGHT_SHOULDER)|(1u<<AERON_GAMEPAD_BUTTON_DPAD_RIGHT))){slot_mode=1;slot_confirm=-1;slot_notice[0]=0;}
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH))SlotsChoose();
 }
 static AeronInputSnapshot filtered;filtered=*input;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++)if(!strcmp(filtered.controllers[i].name,"H700 Gamepad")){
  filtered.controllers[i].gamepad_buttons=filtered.controllers[i].gamepad_pressed_buttons=filtered.controllers[i].gamepad_released_buttons=0;
  memset(filtered.controllers[i].gamepad_axes,0,sizeof filtered.controllers[i].gamepad_axes);
 }
 AeronUiContext* ui=menu.ui;AeronUiTheme original=*AeronUi_GetTheme(ui),theme=original;
 theme.text_px=44;theme.title_px=56;theme.help_px=36;theme.row_height=85;theme.title_height=72;theme.window_pad=24;theme.item_spacing=10;
 AeronUi_SetTheme(ui,&theme);AeronUi_BeginFrame(ui,&(AeronUiFrameDesc){.input=&filtered,.dt_seconds=seconds});
 bool back=false,settings=false;
 if(AeronUi_BeginWindow(ui,"MISSIE OPSLAAN / LADEN",&(AeronUiWindowDesc){.width_ref=1560,.height_ref=1050,.centered=1})){
  AeronUi_BeginColumns(ui,2,NULL);
  if(XwQuick_Choice(ui,"L1  Opslaan",slot_mode==0,&theme)){slot_mode=0;slot_confirm=-1;}
  AeronUi_NextColumn(ui);
  if(XwQuick_Choice(ui,"R1  Laden",slot_mode==1,&theme)){slot_mode=1;slot_confirm=-1;}
  AeronUi_EndColumns(ui);
  AeronUi_Help(ui,"Omhoog/omlaag: slot | A: kiezen en bevestigen | B: terug");
  for(int i=0;i<5;i++)if(XwQuick_Choice(ui,slot_labels[i],slot_selected==i,&theme)){
   if(slot_selected!=i){slot_selected=i;slot_confirm=-1;}
   SlotsChoose();
  }
  if(slot_notice[0])AeronUi_Help(ui,slot_notice);
  else AeronUi_Help(ui,slot_mode?"Laden hervat de opgeslagen missie. Het spel start kort opnieuw.":"Bewaar de huidige missie, inclusief schepen en energieverdeling.");
  AeronUi_BeginColumns(ui,2,NULL);
  if(AeronUi_Button(ui,"B  Terug"))back=true;AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"Instellingen"))settings=true;
  AeronUi_EndColumns(ui);AeronUi_EndWindow(ui);
 }
 AeronUiOutput out=AeronUi_EndFrame(ui);menu.captures=false;AeronUi_Submit(ui);AeronUi_SetTheme(ui,&original);
 if(back||out.cancel_pressed)SlotsBack();
 if(settings){slots_open=false;quick_enabled=quick_return=false;menu.page=XW_SETTINGS_GAME;}
}
