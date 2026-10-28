/* One flat function list, with direct recording of controller chords. */
#include "xw_runtime/input/keyboard_mapping.h"
static int fb_selected;
static bool fb_capture,fb_armed,fb_restore;
static unsigned fb_chord;
static char fb_notice[180];
static unsigned FbRaw(const AeronInputSnapshot* input) {
 static const int buttons[]={AERON_GAMEPAD_BUTTON_SOUTH,AERON_GAMEPAD_BUTTON_EAST,AERON_GAMEPAD_BUTTON_WEST,AERON_GAMEPAD_BUTTON_NORTH,AERON_GAMEPAD_BUTTON_LEFT_SHOULDER,AERON_GAMEPAD_BUTTON_RIGHT_SHOULDER,-1,-1,AERON_GAMEPAD_BUTTON_DPAD_UP,AERON_GAMEPAD_BUTTON_DPAD_DOWN,AERON_GAMEPAD_BUTTON_DPAD_LEFT,AERON_GAMEPAD_BUTTON_DPAD_RIGHT};
 unsigned raw=0;
 for(int slot=0;slot<AERON_CONTROLLER_MAX;slot++) {
  const AeronControllerSnapshot* d=&input->controllers[slot];
  if(!d->connected || strcmp(d->name,"H700 Gamepad")) continue;
  for(int b=0;b<12;b++) {
   bool down=b==6 ? d->gamepad_axes[AERON_GAMEPAD_AXIS_LEFT_TRIGGER]>16383 : b==7 ? d->gamepad_axes[AERON_GAMEPAD_AXIS_RIGHT_TRIGGER]>16383 : (d->gamepad_buttons&(1u<<buttons[b]))!=0;
   if(down) raw|=1u<<b;
  }
 }
 return raw;
}
static void FbMask(unsigned mask,char* out,int size) {
 static const char* keys[]={"A","B","X","Y","L1","R1","L2","R2","Up","Down","Left","Right"};
 out[0]=0;
 for(int b=0;b<12;b++) if(mask&(1u<<b)) { int len=strlen(out); snprintf(out+len,size-len,"%s%s",len?"+":"",keys[b]); }
 if(!mask) snprintf(out,size,"--");
}
static void FbAssigned(int action,char* out,int size) {
 out[0]=0; const FlightBinding* bindings=XwCombos_Bindings();
 for(int i=0;i<XwCombos_Count();i++) if(bindings[i].action==action && bindings[i].mask) {
  char mask[100]; FbMask(bindings[i].mask,mask,sizeof mask); int len=strlen(out);
  snprintf(out+len,size-len,"%s%s",len?", ":"",mask);
 }
 if(!out[0]) snprintf(out,size,"--");
}
static void FbKeyboard(int action,char* out,int size) {
 const XwKeyboardBindings* keys=&menu.draft.keyboard;
 out[0]=0;
 for(size_t i=0;i<keys->count;i++) if(keys->bindings[i].action==action) {
  char key[90]; XwKeyboardMapping_FormatSource(key,sizeof key,keys->bindings[i].source);
  int len=strlen(out); snprintf(out+len,size-len,"%s%s",len?", ":"",key);
 }
 if(!out[0]) snprintf(out,size,"--");
}
static void FbStartCapture(void) { fb_capture=true; fb_armed=false; fb_chord=0; fb_notice[0]=0; }
static void FbCaptureFrame(unsigned raw,bool focus) {
 if(!focus) { fb_armed=false; fb_chord=0; return; }
 if(!fb_armed) { if(!raw) fb_armed=true; return; }
 if(raw) fb_chord|=raw;
 else if(fb_chord) {
  if(XwCombos_SetActionBinding(fb_selected+1,fb_chord)) { fb_capture=false; fb_notice[0]=0; }
  else { snprintf(fb_notice,sizeof fb_notice,"Combinatie niet opgeslagen; probeer opnieuw."); fb_chord=0; fb_armed=false; }
 }
}
static void XwBindings_Frame(const AeronInputSnapshot* input,float seconds) {
 unsigned pressed=0;
 bool havepad=false;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++) if(input->controllers[i].connected && !strcmp(input->controllers[i].name,"H700 Gamepad")) { pressed|=input->controllers[i].gamepad_pressed_buttons; havepad=true; }
 int count=XW_INPUT_ACTION_COUNT-1;
 if(fb_selected<0 || fb_selected>=count) fb_selected=0;
 if(fb_capture) {
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_BACK)) { fb_capture=false; fb_chord=0; }
  else FbCaptureFrame(FbRaw(input),input->has_focus && havepad);
 } else if(fb_restore) {
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_EAST)) fb_restore=false;
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH)) { XwCombos_RestoreDefaults(); fb_restore=false; }
 } else if(input->has_focus) {
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_BACK)) { menu.page=0; return; }
  if(pressed&((1u<<AERON_GAMEPAD_BUTTON_EAST)|(1u<<AERON_GAMEPAD_BUTTON_START))) { (void)XwSettingsMenu_RequestClose(); return; }
  if((pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_UP)) && fb_selected>0) fb_selected--;
  if((pressed&(1u<<AERON_GAMEPAD_BUTTON_DPAD_DOWN)) && fb_selected+1<count) fb_selected++;
  int pages=(count+2)/3;
  if(pressed&((1u<<AERON_GAMEPAD_BUTTON_DPAD_LEFT)|(1u<<AERON_GAMEPAD_BUTTON_LEFT_SHOULDER))) fb_selected=((fb_selected/3+pages-1)%pages)*3;
  if(pressed&((1u<<AERON_GAMEPAD_BUTTON_DPAD_RIGHT)|(1u<<AERON_GAMEPAD_BUTTON_RIGHT_SHOULDER))) fb_selected=((fb_selected/3+1)%pages)*3;
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_SOUTH)) FbStartCapture();
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_WEST)) XwCombos_SetActionBinding(fb_selected+1,0);
  if(pressed&(1u<<AERON_GAMEPAD_BUTTON_NORTH)) fb_restore=true;
 }
 static AeronInputSnapshot filtered; filtered=*input;
 for(int i=0;i<AERON_CONTROLLER_MAX;i++) if(!strcmp(filtered.controllers[i].name,"H700 Gamepad")) memset(&filtered.controllers[i],0,sizeof filtered.controllers[i]);
 AeronUiContext* ui=menu.ui;
 AeronUiTheme saved=*AeronUi_GetTheme(ui),large=saved;
 large.text_px=46; large.title_px=51; large.help_px=40; large.row_height=174;
 large.title_height=76; large.window_pad=20; large.item_spacing=14; large.column_spacing=18; large.widget_border_px=3;
 memcpy(large.text,(float[4]){1,1,1,1},sizeof large.text);
 memcpy(large.text_dim,(float[4]){.90f,.94f,1,1},sizeof large.text_dim);
 memcpy(large.surface,(float[4]){.014f,.022f,.040f,1},sizeof large.surface);
 memcpy(large.widget_bg,(float[4]){.06f,.11f,.19f,1},sizeof large.widget_bg);
 memcpy(large.widget_border,(float[4]){.30f,.49f,.70f,1},sizeof large.widget_border);
 AeronUi_SetTheme(ui,&large);
 AeronUi_BeginFrame(ui,&(AeronUiFrameDesc){.input=&filtered,.dt_seconds=seconds});
 if(AeronUi_BeginWindow(ui,fb_capture ? "DRUK DE GEWENSTE COMBINATIE" : "FUNCTIES EN CONTROLLERKNOPPEN",&(AeronUiWindowDesc){.width_ref=1560,.height_ref=1050,.centered=1})) {
  if(fb_capture) {
   AeronUi_Help(ui,XwInputActions_DisplayName((XwInputAction)(fb_selected+1)));
   AeronUi_Help(ui,fb_armed ? "Druk je knoppen in; laat alles los om op te slaan." : "Laat eerst de bevestigingsknop los.");
   char chord[120]; FbMask(fb_chord,chord,sizeof chord); AeronUi_Label(ui,chord);
   AeronUi_Help(ui,"Select: annuleren");
  } else if(fb_restore) {
   AeronUi_Help(ui,"Standaardindeling herstellen?");
   if(AeronUi_Button(ui,"A  Ja, herstellen")) { XwCombos_RestoreDefaults(); fb_restore=false; }
   if(AeronUi_Button(ui,"B  Nee, behouden")) fb_restore=false;
  } else {
   AeronUi_Help(ui,"D-pad: kiezen  |  A: toewijzen  |  X: wissen");
   AeronUiTheme heading=large; heading.row_height=54; heading.text_px=40; AeronUi_SetTheme(ui,&heading);
   const float weights[]={.66f,.34f};
   AeronUi_BeginColumns(ui,2,weights); AeronUi_Label(ui,"Functie / toetsenbord"); AeronUi_NextColumn(ui); AeronUi_Label(ui,"Controller"); AeronUi_EndColumns(ui);
   int start=fb_selected/3*3;
   for(int j=0;j<3;j++) {
    int index=start+j,action=index+1;
    if(index>=count) { AeronUi_Spacer(ui,large.row_height); continue; }
    char key[180],assigned[200],function[300],button[220];
    FbKeyboard(action,key,sizeof key); FbAssigned(action,assigned,sizeof assigned);
    snprintf(function,sizeof function,"%s\n%s",XwInputActions_DisplayName((XwInputAction)action),key);
    AeronUi_SetTheme(ui,&large); AeronUi_BeginColumns(ui,2,weights);
    AeronUiTheme text=large; text.help_px=43; if(index==fb_selected) memcpy(text.text_dim,(float[4]){.45f,.85f,1,1},sizeof text.text_dim);
    AeronUi_SetTheme(ui,&text); AeronUi_Help(ui,function);
    AeronUi_NextColumn(ui); AeronUi_SetTheme(ui,&large);
    if(strlen(assigned)>21) snprintf(button,sizeof button,"%.18s...##fb%d",assigned,index); else snprintf(button,sizeof button,"%s##fb%d",assigned,index);
    if(XwQuick_Choice(ui,button,index==fb_selected,&large)) { fb_selected=index; FbStartCapture(); }
    AeronUi_EndColumns(ui);
   }
   AeronUiTheme compact=large; compact.row_height=76; compact.text_px=40; AeronUi_SetTheme(ui,&compact);
   char full[240],current[200]; FbAssigned(fb_selected+1,current,sizeof current); snprintf(full,sizeof full,"Controller: %s",current); AeronUi_Help(ui,full);
   char line[180]; snprintf(line,sizeof line,"%d/%d  |  Links/rechts of L1/R1: pagina  |  B: terug",start/3+1,(count+2)/3); AeronUi_Help(ui,line);
   if(AeronUi_Button(ui,"Y  Standaard herstellen")) fb_restore=true;
  }
  if(fb_notice[0]) AeronUi_Error(ui,fb_notice);
  if(XwCombos_Error()[0]) AeronUi_Error(ui,XwCombos_Error());
  AeronUi_EndWindow(ui);
 }
 AeronUi_EndFrame(ui); AeronUi_Submit(ui); AeronUi_SetTheme(ui,&saved);
}
