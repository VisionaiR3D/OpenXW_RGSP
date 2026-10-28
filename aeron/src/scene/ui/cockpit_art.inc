/* Textured, immediate-mode cockpit presentation. Layout uses the original
 * 1536x1024 artwork coordinates, scaled uniformly with the game viewport. */
#include "stb_image.h"
#include <stdio.h>
#include "cockpit_art.h"

static UiRect art_rect(AeronUiContext*c,float x,float y,float w,float h){
 float sx=(float)c->out_w/1536,sy=(float)c->out_h/1024;
 return (UiRect){x*sx,y*sy,w*sx,h*sy};
}
static AeronTexture* art_load(const char*path){
 int w,h,n;unsigned char*p=stbi_load(path,&w,&h,&n,4);
 if(!p)return NULL;
 if(strstr(path,"target-portraits.png"))for(int i=0;i<w*h;i++)if(!p[i*4]&&!p[i*4+1]&&!p[i*4+2])p[i*4+3]=0;
 AeronTexture*t=Aeron_CreateTexture(&(AeronTextureDesc){.width=w,.height=h,.mip_count=1,
  .format=AERON_TEXTURE_FORMAT_RGBA8_SRGB,.usage=AERON_TEXTURE_USAGE_SAMPLED|AERON_TEXTURE_USAGE_TRANSFER_DST,.debug_name=path});
 if(t&&!Aeron_UploadTextureData(&(AeronTextureUploadDesc){.texture=t,.width=w,.height=h,
  .raw_data=p,.raw_size=(uint32_t)(w*h*4)})){Aeron_DestroyTexture(t);t=NULL;}
 stbi_image_free(p);return t;
}
static void art_sprite(AeronUiContext*c,int texture,float sx,float sy,float sw,float sh,UiRect r){
 if(!c->cockpit_textures[texture])return;
 float tw=texture==3?3072.f:1536.f,th=texture==3?2048.f:1024.f;
 AeronDrawList2DSprite s={.texture=c->cockpit_textures[texture],.src_u0=sx/tw,.src_v0=sy/th,
  .src_u1=(sx+sw)/tw,.src_v1=(sy+sh)/th,.dst_x=r.x,.dst_y=r.y,.dst_w=r.w,.dst_h=r.h,
  .tint={1,1,1,1},.blend=AERON_BLIT2D_BLEND_PMA,.filter=texture==3?AERON_BLIT2D_FILTER_NEAREST:AERON_BLIT2D_FILTER_LINEAR};
 AeronDrawList_AddSprite(c->list,&s);
}
static void art_frame(AeronUiContext*c,UiRect r,int selected,int fill){
 /* Nine-slice a real mockup panel. The text-bearing center of the reference
  * selection image is never sampled. Neutral centers use a blank skin patch. */
 int tex=selected?1:0;float sx=selected?492:(c->cockpit_imperial?526:492),sy=selected?307:(c->cockpit_imperial?320:307);
 float sw=selected?496:(c->cockpit_imperial?478:496),sh=selected?174:(c->cockpit_imperial?168:174),b=28;
 float bx=b*c->out_w/1536.f,by=b*c->out_h/1024.f;
 if(bx>r.w/3)bx=r.w/3;if(by>r.h/6)by=r.h/6;
 float xs[]={sx,sx+b,sx+sw-b,sx+sw},ys[]={sy,sy+b,sy+sh-b,sy+sh};
 float dx[]={r.x,r.x+bx,r.x+r.w-bx,r.x+r.w},dy[]={r.y,r.y+by,r.y+r.h-by,r.y+r.h};
 for(int y=0;y<3;y++)for(int x=0;x<3;x++){
  UiRect part={dx[x],dy[y],dx[x+1]-dx[x],dy[y+1]-dy[y]};
  if(x==1&&y==1){if(fill)art_sprite(c,0,760,365,160,60,part);}
  else art_sprite(c,tex,xs[x],ys[y],xs[x+1]-xs[x],ys[y+1]-ys[y],part);
 }
}
void AeronUi_CockpitText(AeronUiContext*c,const char*text,float x,float y,float w,float h,float size,int color,int left){
 UiRect r=art_rect(c,x,y,w,h);float px=size*c->out_h/1024.f;
 const float colors[][4]={{.25f,.57f,.95f,1},{1,.55f,.12f,1},{.9f,.055f,.025f,1},{.12f,.8f,.22f,1},{.9f,.88f,.79f,1}};
 const AeronFontAtlas*font=ui_font_title(c);if(!font||!font->cell_h)return;
 AeronTextLine lines[4];int count=0;AeronTextStyle st={.font=font};
 for(int attempt=0;attempt<10;attempt++){
  st.scale=px/font->cell_h;st.tracking_px=ui_font_tracking_atlas(c,font)*st.scale;
  count=AeronText_Wrap(&st,text,(int)strlen(text),r.w,lines,4);
  if(count<=2&&AeronText_LineHeight(&st)*count<=r.h)break;
  px*=.91f;
 }
 float lineh=AeronText_LineHeight(&st),top=r.y+(r.h-lineh*count)*.5f;
 AeronRectI clip={(int)r.x,(int)r.y,(int)r.w,(int)r.h};
 for(int i=0;i<count&&i<4;i++){
  float tx=left?r.x:r.x+r.w*.5f,ty=top+i*lineh;
  ui_draw_text(c,font,tx,ty,left?AERON_TEXT_LEFT:AERON_TEXT_CENTER,px,colors[color],text+lines[i].start,lines[i].length,&clip);
  /* A sub-pixel overstrike gives the compact pixel font the reference's weight. */
  ui_draw_text(c,font,tx+.55f*c->out_h/480.f,ty,left?AERON_TEXT_LEFT:AERON_TEXT_CENTER,px,colors[color],text+lines[i].start,lines[i].length,&clip);
 }
}
int AeronUi_CockpitBegin(AeronUiContext*c,const char*title,int imperial,int console){
 if(!c||c->offline||!c->frame_active)return 0;
 if(!c->cockpit_attempted){
  c->cockpit_attempted=1;
  c->cockpit_textures[0]=art_load("resources/cockpit/main.png");
  c->cockpit_textures[1]=art_load("resources/cockpit/selection.png");
  c->cockpit_textures[2]=art_load("resources/cockpit/console.png");
  c->cockpit_textures[3]=art_load("resources/cockpit/target-portraits.png");
 }
 if(!c->cockpit_textures[0]||!c->cockpit_textures[1]||(console&&!c->cockpit_textures[2]))return 0;
 c->cockpit_imperial=imperial;c->any_window=1;
 art_sprite(c,console?2:0,0,0,1536,1024,(UiRect){0,0,(float)c->out_w,(float)c->out_h});
 AeronUi_CockpitText(c,title,390,20,750,64,54,imperial?0:1,0);
 AeronUi_CockpitText(c,"PAUSED",590,82,350,35,29,imperial?2:0,0);
 return 1;
}
int AeronUi_CockpitButton(AeronUiContext*c,const char*text,float x,float y,float w,float h,int selected,int frame){
 UiRect r=art_rect(c,x,y,w,h);int clicked=ui_widget_behavior(c,ui_make_id(c,text),&r,1);
 if(frame||selected)art_frame(c,r,selected,frame);
 if(strncmp(text,"##",2))AeronUi_CockpitText(c,text,x+22,y+10,w-44,h-20,36,selected?1:0,0);
 return clicked;
}
int AeronUi_CockpitMenuButton(AeronUiContext*c,const char*text,int slot,int selected,int root){
 int col=slot%2,row=slot/2;
 float x=c->cockpit_imperial?(col?1012:526):(col?999:492);
 float y=c->cockpit_imperial?320+row*180:307+row*187;
 float w=c->cockpit_imperial?(col?496:478):(col?502:496),h=c->cockpit_imperial?168:174;
 UiRect r=art_rect(c,x,y,w,h);
 if(!*text){art_frame(c,r,0,1);return 0;}
 int clicked=ui_widget_behavior(c,ui_make_id(c,text),&r,1);
 if(!root||selected)art_frame(c,r,selected,!root);
 AeronUi_CockpitText(c,text,x+(root?178:30),y+24,w-(root?200:60),h-48,root?43:38,selected?1:0,0);
 return clicked;
}
void AeronUi_CockpitMeter(AeronUiContext*c,const char*label,int value,int maximum,float x,float y,float w,int kind){
 char title[80];if(value<0)snprintf(title,sizeof title,"%s  --",label);
 else if(maximum==100)snprintf(title,sizeof title,"%s  %d%%",label,value);
 else snprintf(title,sizeof title,"%s  %d/%d",label,value,maximum);
 AeronUi_CockpitText(c,title,x,y,w*.40f,70,36,0,1);
 const float colors[4][4]={{1,.20f,.025f,1},{.025f,.4f,.95f,1},{1,.55f,.10f,1},{.06f,.8f,.15f,1}};
 const float off[4]={.006f,.018f,.035f,1};
 int blocks=maximum<=8?maximum:10;
 for(int i=0;i<blocks;i++){
  float bw=(w*.59f)/blocks-10;UiRect b=art_rect(c,x+w*.41f+i*(bw+10),y+15,bw,40);
  int lit=value>=0 && i*maximum<value*blocks;
  ui_draw_fill(c,&b,lit?colors[kind]:off,NULL);
 }
}
void AeronUi_CockpitStatus(AeronUiContext*c,int hull,int shields,int page,int pages){
 int imperial=c->cockpit_imperial;
 AeronUi_CockpitText(c,imperial?"IMPERIAL":"REBEL",imperial?75:170,imperial?270:264,imperial?365:270,52,34,0,0);
 /* The blueprint is faction artwork; numbers below come only from the live craft. */
 const float green[4]={.06f,.8f,.15f,1},blue[4]={.025f,.4f,.95f,1},off[4]={.006f,.018f,.035f,1};
 int v[]={hull,shields};
 for(int row=0;row<2;row++){
  float y=imperial?706+row*62:677+row*62;char s[48];
  if(v[row]<0)snprintf(s,sizeof s,"%s --",row?"SHD":"HULL");else snprintf(s,sizeof s,"%s %d%%",row?"SHD":"HULL",v[row]);
  AeronUi_CockpitText(c,s,imperial?78:68,y,170,50,25,0,1);
  for(int i=0;i<8;i++){UiRect b=art_rect(c,245+i*22,y+10,17,32);ui_draw_fill(c,&b,v[row]>=0&&i*100<v[row]*8?(row?blue:green):off,NULL);}
 }
 if(pages>1){char s[70];snprintf(s,sizeof s,"L1 / R1  PAGE %d / %d",page,pages);AeronUi_CockpitText(c,s,550,240,875,44,29,0,0);}
}

static const struct {float x,y,w,h;} portrait_rects[]={
 {0,0,99,64},
 {256,0,102,58},
 {512,0,75,47},
 {768,60,65,49},
 {1024,55,59,57},
 {1280,59,95,51},
 {1536,0,101,52},
 {1792,0,256,256},
 {2048,0,256,256},
 {2304,0,256,256},
 {2560,0,256,256},
 {2816,0,256,256},
 {0,256,256,256},
 {256,256,256,256},
 {512,256,256,256},
 {768,256,256,256},
 {1024,256,256,256},
 {1280,256,256,256},
 {1536,256,256,256},
 {1792,256,256,256},
 {2048,256,256,256},
 {2304,256,256,256},
 {2560,256,256,256},
 {2816,256,256,256},
 {0,512,256,256},
 {256,512,256,256},
 {512,512,256,256},
 {768,512,256,256},
 {1024,512,256,256},
 {1280,512,256,256},
 {1536,512,256,256},
 {1792,512,256,256},
 {2048,512,256,256},
 {2304,512,256,256},
 {2560,512,256,256},
 {2816,512,256,256},
 {0,768,256,256},
 {256,768,256,256},
 {512,768,256,256},
 {768,768,256,256},
 {1024,768,256,256},
 {1280,768,256,256},
 {1536,768,256,256},
 {1792,768,256,256},
 {2048,768,256,256},
 {2304,768,256,256},
 {2560,768,256,256},
 {2816,768,256,256},
 {0,1024,256,256},
 {256,1024,256,256},
 {512,1024,256,256},
 {768,1024,256,256},
 {1024,1024,256,256},
 {1280,1024,256,256},
 {1536,1024,256,256},
 {1792,1024,256,256},
 {2048,1024,256,256},
 {2304,1024,256,256},
 {2560,1024,256,256},
 {2816,1024,256,256},
 {0,1280,256,256},
 {256,1280,256,256},
 {512,1280,256,256},
 {768,1280,256,256},
 {1024,1280,256,256},
 {1280,1280,256,256},
 {1536,1280,256,256},
 {1792,1280,256,256},
 {2048,1280,256,256},
 {2304,1280,256,256},
};
static const struct {const char*name;int id;} portrait_names[]={
 {"xwing",0},
 {"ywing",1},
 {"awing",2},
 {"tieinterceptor",3},
 {"tiefighter",4},
 {"tiebomber",5},
 {"assaultgunboat",6},
 {"bwing",10},
 {"tieadvanced",14},
 {"tiedefender",15},
 {"missileboat",16},
 {"twing",17},
 {"z95headhunter",18},
 {"r41starchaser",19},
 {"shuttle",21},
 {"escortshuttle",22},
 {"patrolcraft",23},
 {"scoutcraft",24},
 {"transport",25},
 {"assaulttransport",26},
 {"escorttransport",27},
 {"tug",28},
 {"utilitytug",29},
 {"containera",30},
 {"containerb",31},
 {"containerc",32},
 {"containerd",33},
 {"heavylifter",34},
 {"freighter",35},
 {"cargoferry",36},
 {"modularconveyor",37},
 {"containertransport",38},
 {"muuriantransport",39},
 {"corelliantransport",40},
 {"corelliancorvette",41},
 {"modcorvette",42},
 {"nebulonbfrigate",43},
 {"nebulonb2frigate",44},
 {"c3passengerliner",45},
 {"carrackcruiser",46},
 {"strikecruiser",47},
 {"escortcarrier",48},
 {"dreadnaught",49},
 {"calamaricruiser",50},
 {"ltcalamaricruiser",51},
 {"interdictor",52},
 {"victorystardest",53},
 {"imperialstardest",54},
 {"containere",55},
 {"containerf",56},
 {"containerg",57},
 {"containerh",58},
 {"containeri",59},
 {"platformxq1",60},
 {"platformxq2",61},
 {"platformxq3",62},
 {"platformxq4",63},
 {"platformxq5",64},
 {"platformxq6",65},
 {"platform",66},
 {"unused",69},
 {"gunboat",6},
 {"container",30},
 {"stardestroyer",54},
 {"imperialstardestroyer",54},
 {"advancedtie",14},
};

void AeronUi_CockpitPortrait(AeronUiContext*c,int id,float x,float y,float w,float h){
 if(id<0||id>=(int)(sizeof portrait_rects/sizeof *portrait_rects)||!c->cockpit_textures[3])return;
 float sw=portrait_rects[id].w,sh=portrait_rects[id].h;
 /* Fit in output pixels, preserving the source view's aspect ratio. */
 UiRect dst=art_rect(c,x,y,w,h);float scale=fminf(dst.w/sw,dst.h/sh);
 dst.x+=(dst.w-sw*scale)*.5f;dst.y+=(dst.h-sh*scale)*.5f;dst.w=sw*scale;dst.h=sh*scale;
 art_sprite(c,3,portrait_rects[id].x,portrait_rects[id].y,sw,sh,dst);
}
void AeronUi_CockpitBlueprint(AeronUiContext*c,const char*name,float x,float y,float w,float h){
 char key[80];int n=0;for(const char*p=name;p&&*p&&n<79;p++){char a=*p;if(a>='A'&&a<='Z')a+=32;if((a>='a'&&a<='z')||(a>='0'&&a<='9'))key[n++]=a;}key[n]=0;
 for(unsigned i=0;i<sizeof portrait_names/sizeof *portrait_names;i++)if(!strcmp(key,portrait_names[i].name)){AeronUi_CockpitPortrait(c,portrait_names[i].id,x,y,w,h);return;}
}

void AeronUi_TacticalPanel(AeronUiContext*c,float x,float y,float w,float h,int color){
 const float bg[]={.008f,.018f,.027f,1};const float colors[][4]={{.08f,.25f,.32f,1},{.85f,.40f,.08f,1},{.95f,.08f,.04f,1},{.1f,.7f,.4f,1}};
 UiRect r=art_rect(c,x,y,w,h);ui_draw_fill(c,&r,bg,NULL);
 UiRect edges[]={art_rect(c,x,y,w,2),art_rect(c,x,y+h-2,w,2),art_rect(c,x,y,2,h),art_rect(c,x+w-2,y,2,h)};
 for(int i=0;i<4;i++)ui_draw_fill(c,&edges[i],colors[color],NULL);
}
int AeronUi_TacticalBegin(AeronUiContext*c){
 if(!AeronUi_CockpitBegin(c,"",1,1))return 0;
 const float bg[]={.004f,.009f,.014f,1};UiRect r={0,0,c->out_w,c->out_h};ui_draw_fill(c,&r,bg,NULL);
 AeronUi_TacticalPanel(c,18,16,1500,105,0);
 AeronUi_CockpitText(c,"TARGET LIST",45,28,850,72,65,4,1);
 return 1;
}
void AeronUi_TacticalScope(AeronUiContext*c,float x,float y,float w,float h){
 const float grid[]={.018f,.065f,.085f,1},cyan[]={.08f,.38f,.48f,1};
 for(int i=0;i<9;i++){UiRect v=art_rect(c,x+w*i/8,y,1,h);ui_draw_fill(c,&v,grid,NULL);}
 for(int i=0;i<7;i++){UiRect v=art_rect(c,x,y+h*i/6,w,1);ui_draw_fill(c,&v,grid,NULL);}
 for(int i=0;i<100;i++){float a=i*6.2831853f/100;UiRect v=art_rect(c,x+w*.5f+cosf(a)*w*.43f,y+h*.5f+sinf(a)*h*.46f,3,3);ui_draw_fill(c,&v,cyan,NULL);}
 for(int i=0;i<4;i++){float bx=x+(i%2)*(w-48),by=y+(i/2)*(h-48);UiRect a=art_rect(c,bx,by+(i/2?46:0),48,2),b=art_rect(c,bx+(i%2?46:0),by,2,48);ui_draw_fill(c,&a,cyan,NULL);ui_draw_fill(c,&b,cyan,NULL);}
}

void AeronUi_TacticalMeter(AeronUiContext*c,const char*label,int value,float x,float y,float w,int shield){
 char text[64];if(value<0)snprintf(text,sizeof text,"%s --",label);else snprintf(text,sizeof text,"%s %d%%",label,value);
 AeronUi_CockpitText(c,text,x,y,w,32,29,shield?0:1,1);
 const float colors[][4]={{1,.6f,.13f,1},{.1f,.6f,.95f,1},{.04f,.10f,.14f,1}};
 for(int i=0;i<10;i++){UiRect r=art_rect(c,x+i*w/10,y+37,w/10-5,15);ui_draw_fill(c,&r,value>=0&&i*10<value?colors[shield?1:0]:colors[2],NULL);}
}
