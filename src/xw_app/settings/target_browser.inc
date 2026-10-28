
/* Shared handheld list, opened only from the paused command menu. Engine adapters
 * copy visible data and validate slot identity again before selecting a target. */
static TargetEntry target_all[TARGET_CAP];
static int target_count,target_view_count,target_indices[TARGET_CAP],target_filter,target_sort,target_selected;
static char target_notice[180];
static void TargetRebuild(bool current){
 target_view_count=TargetView(target_all,target_count,target_filter,target_sort,target_indices);
 target_selected=0;
 if(current)for(int i=0;i<target_view_count;i++)if(target_all[target_indices[i]].ref==TargetCurrent())target_selected=i;
}
static void TargetsOpen(void){
 target_count=TargetCapture(target_all,TARGET_CAP);target_notice[0]=0;
 TargetRebuild(true);targets_open=true;
}
static void TargetCommit(void){
 if(target_selected<0||target_selected>=target_view_count)return;
 TargetEntry chosen=target_all[target_indices[target_selected]];
 if(!TargetSelect(&chosen)){
  target_count=TargetCapture(target_all,TARGET_CAP);TargetRebuild(true);
  snprintf(target_notice,sizeof target_notice,"Target unavailable or targeting computer damaged.");return;
 }
 TB_CLEAR();
 if(TB_CLOSE()){targets_open=false;quick_leaving=true;}
 else snprintf(target_notice,sizeof target_notice,"Could not close menu. Please try again.");
}
static void TargetFilter(int delta){target_filter=(target_filter+delta+10)%10;TargetRebuild(false);}
static void TargetSort(void){target_sort=(target_sort+1)%4;TargetRebuild(true);}
static void TargetPage(int delta){
 if(!target_view_count)return;
 int next=target_selected+delta*TARGET_PAGE;
 target_selected=next<0?0:next>=target_view_count?target_view_count-1:next;
}
static void TargetSubsystems(void){
 #ifdef TB_SUBSYSTEM_NODE
 if(target_view_count>0&&TargetSelect(&target_all[target_indices[target_selected]])){
  targets_open=false;quick_node=TB_SUBSYSTEM_NODE;quick_selected=0;
 }
 #endif
}
static void TargetsFrame(const AeronInputSnapshot*input,float seconds){
 unsigned pressed=0;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++)if(input->controllers[i].connected&&!strcmp(input->controllers[i].name,"H700 Gamepad"))pressed|=input->controllers[i].gamepad_pressed_buttons;
 if(input->has_focus){
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_EAST)){targets_open=false;return;}
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_LEFT_SHOULDER))TargetFilter(-1);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_RIGHT_SHOULDER))TargetFilter(1);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_WEST))TargetSort();
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_NORTH)){
 #ifdef TB_SUBSYSTEM_NODE
   if(target_view_count>0 && TargetSelect(&target_all[target_indices[target_selected]])){
    targets_open=false;quick_node=TB_SUBSYSTEM_NODE;quick_selected=0;return;
   }
 #else
   TargetFilter(1);
 #endif
  }
  if((pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_UP))&&target_selected>0)--target_selected;
  if((pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_DOWN))&&target_selected+1<target_view_count)++target_selected;
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_LEFT))TargetPage(-1);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_RIGHT))TargetPage(1);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH)){TargetCommit();if(!targets_open)return;}
 }
 static AeronInputSnapshot filtered;filtered=*input;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++)if(!strcmp(filtered.controllers[i].name,"H700 Gamepad")){
  filtered.controllers[i].gamepad_buttons=filtered.controllers[i].gamepad_pressed_buttons=filtered.controllers[i].gamepad_released_buttons=0;
  memset(filtered.controllers[i].gamepad_axes,0,sizeof filtered.controllers[i].gamepad_axes);
 }
 AeronUiContext*ui=TB_UI;
 AeronUiTheme original=*AeronUi_GetTheme(ui),theme=original;
 theme.text_px=32;theme.title_px=48;theme.help_px=32;theme.row_height=74;theme.title_height=68;
 theme.window_pad=22;theme.item_spacing=10;theme.widget_border_px=3;
 CommandCockpitTheme(&theme);
 AeronUi_SetTheme(ui,&theme);AeronUi_BeginFrame(ui,&(AeronUiFrameDesc){.input=&filtered,.dt_seconds=seconds});
 int page=target_selected/TARGET_PAGE,start=page*TARGET_PAGE;
 char title[180];snprintf(title,sizeof title,"TARGET LIST | %s (%d) %d/%d",target_filters[target_filter],target_view_count,page+1,target_view_count?(target_view_count+TARGET_PAGE-1)/TARGET_PAGE:1);
 AeronUiWindowDesc window={.width_ref=1560,.height_ref=1050,.centered=1,.flags=(1u<<8)};
 bool choose=false,back=false;int filter=0,pages=0;bool sort=false,subsystems=false;
 if(AeronUi_TacticalBegin(ui)){
  char line[180];snprintf(line,sizeof line,"%02d CONTACTS",target_view_count);
  AeronUi_CockpitText(ui,line,1080,34,405,65,42,1,0);
  snprintf(line,sizeof line,"FILTER: %s",target_filters[target_filter]);
  if(AeronUi_CockpitButton(ui,line,25,130,490,62,0,0))filter=1;
  snprintf(line,sizeof line,"%s  |  PAGE %d/%d",target_sorts[target_sort],page+1,target_view_count?(target_view_count+3)/4:1);
  AeronUi_CockpitText(ui,line,40,874,470,40,27,0,1);
  for(int row=0;row<TARGET_PAGE;row++){
   int item=start+row;if(item>=target_view_count)break;
   const TargetEntry*e=&target_all[target_indices[item]];float y=200+row*163;int selected=item==target_selected;
   AeronUi_TacticalPanel(ui,25,y,490,151,selected?2:0);
   char id[48];snprintf(id,sizeof id,"##contact-%d",item);
   if(AeronUi_CockpitButton(ui,id,25,y,490,151,0,0))target_selected=item;
   AeronUi_CockpitBlueprint(ui,e->type,35,y+20,160,110);
   snprintf(line,sizeof line,"%s%.22s",e->ref==TargetCurrent()?"* ":"",e->name);
   AeronUi_CockpitText(ui,line,205,y+10,295,46,32,4,1);
   snprintf(line,sizeof line,"%.2f km%s",e->distance_km,e->flags&TARGET_ATTACKING?"  >>":"");
   AeronUi_CockpitText(ui,line,205,y+56,290,43,35,e->flags&TARGET_ATTACKING?2:1,1);
   AeronUi_TacticalMeter(ui,"HULL",e->hull,205,y+94,290,0);
  }
  AeronUi_TacticalPanel(ui,535,130,975,780,0);
  if(target_view_count){
   const TargetEntry*e=&target_all[target_indices[target_selected]];
   AeronUi_CockpitText(ui,e->name,560,144,700,65,50,4,1);
   AeronUi_CockpitText(ui,e->type,560,207,650,44,32,0,1);
   AeronUi_CockpitText(ui,target_relations[e->relation],1250,156,230,52,28,e->relation==0?2:3,0);
   const char*status=e->flags&TARGET_ATTACKING?"! ATTACKING YOU":e->flags&TARGET_INCOMING?"! INCOMING WARHEAD":e->flags&TARGET_OBJECTIVE?"MISSION OBJECTIVE":e->flags&TARGET_INSPECTED?"TARGET IDENTIFIED":"TARGET CONTACT";
   AeronUi_CockpitText(ui,status,560,260,900,52,36,e->flags&(TARGET_ATTACKING|TARGET_INCOMING)?2:1,1);
   AeronUi_TacticalScope(ui,580,330,875,345);
   AeronUi_CockpitBlueprint(ui,e->type,605,335,820,330);
   AeronUi_TacticalPanel(ui,555,695,460,88,0);AeronUi_TacticalPanel(ui,1030,695,460,88,0);
   AeronUi_TacticalMeter(ui,"HULL",e->hull,570,711,430,0);
   AeronUi_TacticalMeter(ui,"SHIELDS",e->shields,1045,711,430,1);
   AeronUi_CockpitText(ui,e->systems_known?(e->systems_damaged?"DAMAGED SYSTEMS":"SYSTEM STATUS"):"SYSTEM STATUS UNAVAILABLE",560,795,680,40,28,e->systems_damaged?1:0,1);
   char damage[180]="";
   const struct{unsigned bit;const char*name;} labels[]={{0x40,"ENGINE"},{0x10,"LASERS"},{0x20,"CONTROLS"},{4,"TARGETING"},{1,"SHIELDS"},{8,"LAUNCHER"},{0x80,"HYPERDRIVE"},{0x100,"BEAM"},{0x200,"COMMS"},{2,"EJECTION"}};
   for(unsigned i=0;i<sizeof labels/sizeof *labels;i++)if(e->systems_damaged&labels[i].bit){size_t used=strlen(damage);snprintf(damage+used,sizeof damage-used,"%s%s",used?" / ":"",labels[i].name);}
   AeronUi_CockpitText(ui,e->systems_known?(damage[0]?damage:"MONITORED SYSTEMS OK"):"--",560,837,910,56,30,damage[0]?1:3,1);
  }else AeronUi_CockpitText(ui,"NO TARGETS MATCH THIS FILTER",580,350,865,190,42,4,0);
  if(AeronUi_CockpitButton(ui,"A SELECT TARGET",25,938,430,66,0,0))choose=true;
  if(AeronUi_CockpitButton(ui,"B BACK",460,938,230,66,0,0))back=true;
  if(AeronUi_CockpitButton(ui,"L1/R1 FILTER",700,938,350,66,0,0))filter=1;
  if(AeronUi_CockpitButton(ui,"X SORT",1060,938,230,66,0,0))sort=true;
 #ifdef TB_SUBSYSTEM_NODE
  if(AeronUi_CockpitButton(ui,"Y SYSTEMS",1285,938,230,66,0,0))subsystems=true;
 #endif
  if(target_notice[0])AeronUi_CockpitText(ui,target_notice,40,905,1450,32,25,2,0);
 }else if(AeronUi_BeginWindow(ui,title,&window)){
  char line[220];snprintf(line,sizeof line,"SORT: %s   |   L1/R1 FILTER   |   X SORT",target_sorts[target_sort]);AeronUi_Help(ui,line);
  AeronUi_Help(ui,"UP/DOWN SELECT   |   LEFT/RIGHT PAGE   |   A LOCK TARGET");
  for(int row=0;row<TARGET_PAGE;row++){
   int item=start+row;
   if(item<target_view_count){
    const TargetEntry*e=&target_all[target_indices[item]];
    char label[200];snprintf(label,sizeof label,"%s%.20s | %.13s | %.2f km | %s%s",e->ref==TargetCurrent()?"* ":"",e->name,e->type,e->distance_km,target_relations[e->relation],e->flags&TARGET_INCOMING?" !":"");
    if(TB_CHOICE(ui,label,target_selected==item,&theme)){target_selected=item;choose=true;}
   }else if(!row){AeronUi_Label(ui,"NO TARGETS MATCH THIS FILTER");}
   else AeronUi_Spacer(ui,theme.row_height);
  }
  if(target_view_count){
   const TargetEntry*e=&target_all[target_indices[target_selected]];char hull[24],shield[24];
   if(e->hull<0)snprintf(hull,sizeof hull,"--");else snprintf(hull,sizeof hull,"%d%%",e->hull);
   if(e->shields<0)snprintf(shield,sizeof shield,"--");else snprintf(shield,sizeof shield,"%d%%",e->shields);
   snprintf(line,sizeof line,"HULL: %s   |   SHIELDS: %s   |   %s",hull,shield,e->flags&TARGET_INSPECTABLE?(e->flags&TARGET_INSPECTED?"INSPECTED":"NOT INSPECTED"):"NOT INSPECTABLE");AeronUi_Help(ui,line);
   snprintf(line,sizeof line,"%s%s%s%s",e->flags&TARGET_ATTACKING?"ATTACKING YOU   ":"",e->flags&TARGET_OBJECTIVE?"OBJECTIVE   ":"",e->flags&TARGET_DAMAGED?"DAMAGED   ":"",e->flags&TARGET_INCOMING?"INCOMING WARHEAD":"");AeronUi_Help(ui,line[0]?line:"* CURRENT TARGET");
  }else{AeronUi_Help(ui,"L1 / R1 CHANGE FILTER");AeronUi_Help(ui,"MISSION PAUSED");}
  if(target_view_count){
   const TargetEntry*e=&target_all[target_indices[target_selected]];
   AeronUiTheme meters=theme;meters.row_height=40;meters.text_px=28;AeronUi_SetTheme(ui,&meters);
   AeronUi_BeginColumns(ui,2,NULL);
   if(e->hull>=0)AeronUi_PercentageMeter(ui,"HULL",(float)e->hull/100);
   AeronUi_NextColumn(ui);
   if(e->shields>=0)AeronUi_PercentageMeter(ui,"SHIELDS",(float)e->shields/100);
   AeronUi_EndColumns(ui);AeronUi_SetTheme(ui,&theme);
  }
 #ifdef TB_SUBSYSTEM_NODE
  AeronUi_Help(ui,"Y SUBSYSTEMS: select target and open subsystem commands");
 #endif
  if(target_notice[0])AeronUi_Error(ui,target_notice);
  AeronUiTheme compact=theme;compact.row_height=64;AeronUi_SetTheme(ui,&compact);
  AeronUi_BeginColumns(ui,3,NULL);
  if(AeronUi_Button(ui,"L1 FILTER -"))filter=-1;
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"R1 FILTER +"))filter=1;
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"X SORT"))sort=true;
  AeronUi_EndColumns(ui);
  AeronUi_BeginColumns(ui,3,NULL);
  if(AeronUi_Button(ui,"< PREVIOUS"))pages=-1;
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"> NEXT"))pages=1;
  AeronUi_NextColumn(ui);
  if(AeronUi_Button(ui,"B BACK"))back=true;
  AeronUi_EndColumns(ui);
  AeronUi_EndWindow(ui);
 }
 AeronUiOutput output=AeronUi_EndFrame(ui);TB_UNCAPTURE();AeronUi_Submit(ui);AeronUi_SetTheme(ui,&original);
 if(back||output.cancel_pressed)targets_open=false;
 else if(subsystems)TargetSubsystems();
 else if(choose)TargetCommit();
 else if(filter)TargetFilter(filter);
 else if(sort)TargetSort();
 else if(pages)TargetPage(pages);
}
