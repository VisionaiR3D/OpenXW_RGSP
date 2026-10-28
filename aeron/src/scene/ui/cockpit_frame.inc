/* Lightweight native geometry; only command windows opt into this frame. */
static void ui_cockpit_frame(AeronUiContext* ctx, UiRect r) {
 const float u=ctx->scale;
 const float steel[4]={.12f,.17f,.21f,1}, dark[4]={.006f,.012f,.022f,1};
 const float light[4]={.35f,.43f,.49f,1};
 const float lamp[4]={1,.34f,.055f,1};
 for(int side=0;side<2;side++) {
  float x=side?r.x+r.w-12*u:r.x;
  UiRect rail={x,r.y,12*u,r.h};ui_draw_fill(ctx,&rail,steel,NULL);
  for(int n=0;n<12;n++){
   UiRect groove={x+3*u,r.y+(90+n*76)*u,6*u,28*u};ui_draw_fill(ctx,&groove,dark,NULL);
  }
 }
 UiRect bottom={r.x,r.y+r.h-22*u,r.w,22*u};ui_draw_fill(ctx,&bottom,steel,NULL);
 for(int side=0;side<2;side++)for(int end=0;end<2;end++){
  float x=side?r.x+r.w-30*u:r.x+14*u;
  float y=end?r.y+r.h-18*u:r.y+12*u;
  UiRect screw={x,y,12*u,6*u};ui_draw_fill(ctx,&screw,light,NULL);
  UiRect slot={x+2*u,y+2*u,8*u,2*u};ui_draw_fill(ctx,&slot,dark,NULL);
 }
 for(int n=0;n<5;n++){
  UiRect led={r.x+r.w*.5f+(n-2)*18*u,r.y+r.h-15*u,10*u,6*u};
  ui_draw_fill(ctx,&led,n==2?lamp:dark,NULL);
 }
}

static void ui_cockpit_icon(AeronUiContext*ctx,const UiRect*r,const char*label,const AeronRectI*clip){
 int icon=-1;
 if(!strncmp(label,"TARGETS",7))icon=0;
 else if(!strncmp(label,"SHIP & POWER",12))icon=1;
 else if(!strncmp(label,"WEAPONS",7))icon=2;
 else if(!strncmp(label,"WINGMEN",7))icon=3;
 else if(!strncmp(label,"MISSION",7))icon=4;
 else if(!strncmp(label,"VIEW & SETTINGS",15))icon=5;
 if(icon<0)return;
 /* Pixel-grid symbols: targeting reticle, energy, cannon, formation, map, display. */
 static const unsigned char bits[6][9]={
 {16,16,60,66,255,66,60,16,16},
 {8,24,56,112,126,28,24,16,16},
 {24,24,24,24,60,60,126,66,66},
 {16,56,16,0,68,238,68,0,0},
 {254,130,146,186,146,130,254,0,0},
 {254,130,130,130,254,16,56,0,0}};
 float px=ui_ref(ctx,4),x=r->x+ui_ref(ctx,22),y=r->y+r->h*.5f-px*4.5f;
 for(int row=0;row<9;row++)for(int col=0;col<8;col++)if(bits[icon][row]&(1u<<col)){
  UiRect dot={x+col*px,y+row*px,px,px};ui_draw_fill(ctx,&dot,ctx->theme.text,clip);
 }
}

/* Gamepad-driven power allocation, rendered without a misleading speed percentage. */
void AeronUi_CommandPowerMeter(AeronUiContext*ctx,const char*label,int value,int maximum,int kind,int selected){
 UiRect r;if(!ctx||!ctx->frame_active||!ui_layout_row(ctx,ui_ref(ctx,100),&r))return;
 float colors[3][4]={{1,.22f,.025f,1},{.035f,.45f,.95f,1},{1,.55f,.09f,1}};
 float off[4]={.012f,.022f,.036f,1};
 float size=ui_ref(ctx,ctx->theme.text_px),u=ctx->scale;
 char text[100];snprintf(text,sizeof text,"%s  %d/%d",label,value,maximum);
 ui_draw_text(ctx,ui_font_regular(ctx),r.x+8*u,r.y+(r.h-size)*.5f,AERON_TEXT_LEFT,size,
  selected?ctx->theme.accent:ctx->theme.text,text,(int)strlen(text),NULL);
 float start=r.x+r.w*.48f,gap=12*u,w=(r.w*.52f-gap*maximum)/maximum;
 for(int i=0;i<maximum;i++){
  UiRect block={start+i*(w+gap),r.y+25*u,w,50*u};
  ui_draw_surface(ctx,&block,i<value?colors[kind]:off,NULL,0,ctx->theme.widget_border,2,1,NULL);
 }
}
