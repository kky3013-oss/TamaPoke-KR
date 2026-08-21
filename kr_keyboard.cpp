#include "kr_keyboard.h"
#include "kr_text.h"
#include <string.h>

static char *gBuf = nullptr;
static size_t gCap = 0;
static int gL = -1, gV = -1, gT = 0;
static uint8_t gDot = 0;
static uint32_t gLastKeyMs = 0;
static int gLastKey = -1;
static int gCycle = 0;
static char gCycleBuf[64];
static int gCycleL=-1,gCycleV=-1,gCycleT=0;
static uint8_t gCycleDot=0;

static int compatToL(uint32_t c) {
  static const uint32_t cp[] = {0x3131,0x3132,0x3134,0x3137,0x3138,0x3139,0x3141,0x3142,0x3143,0x3145,0x3146,0x3147,0x3148,0x3149,0x314A,0x314B,0x314C,0x314D,0x314E};
  for (int i=0;i<19;i++) if (cp[i]==c) return i;
  return -1;
}
static int compatToV(uint32_t c) {
  static const uint32_t cp[] = {0x314F,0x3150,0x3151,0x3152,0x3153,0x3154,0x3155,0x3156,0x3157,0x3158,0x3159,0x315A,0x315B,0x315C,0x315D,0x315E,0x315F,0x3160,0x3161,0x3162,0x3163};
  for (int i=0;i<21;i++) if (cp[i]==c) return i;
  return -1;
}
static uint32_t lcp(int i) { static const uint32_t a[]={0x3131,0x3132,0x3134,0x3137,0x3138,0x3139,0x3141,0x3142,0x3143,0x3145,0x3146,0x3147,0x3148,0x3149,0x314A,0x314B,0x314C,0x314D,0x314E}; return a[i]; }
static uint32_t vcp(int i) { static const uint32_t a[]={0x314F,0x3150,0x3151,0x3152,0x3153,0x3154,0x3155,0x3156,0x3157,0x3158,0x3159,0x315A,0x315B,0x315C,0x315D,0x315E,0x315F,0x3160,0x3161,0x3162,0x3163}; return a[i]; }
static int combineV(int a,int b) { static const int c[][3]={{8,0,9},{8,1,10},{8,20,11},{13,4,14},{13,5,15},{13,20,16},{18,20,19},{0,20,1},{2,20,3},{4,20,5},{6,20,7}}; for(auto&r:c)if(r[0]==a&&r[1]==b)return r[2];return -1; }
static int combineT(int a,int b) { static const int c[][3]={{1,19,3},{4,12,5},{4,18,6},{8,0,9},{8,6,10},{8,7,11},{8,19,12},{8,25,13},{8,26,14},{8,15,15},{17,19,18}}; for(auto&r:c)if(r[0]==a&&r[1]==b)return r[2];return -1; }
static int tToL(int t) { static const int map[]={-1,0,1,-1,2,-1,-1,3,5,-1,-1,-1,9,16,17,18,6,7,-1,9,10,11,12,14,15,16,17,18}; return (t>=0&&t<28)?map[t]:-1; }
static int lToT(int L) { static const int map[]={1,2,4,7,-1,8,16,17,-1,19,20,21,22,-1,23,24,25,26,27}; return map[L]; }
static void putUtf8(uint32_t cp){if(!gBuf||gCap<2)return;size_t n=strlen(gBuf);size_t need=cp<=0x7F?1:cp<=0x7FF?2:3;if(n+need>=gCap)return;if(need==1)gBuf[n++]=(char)cp;else if(need==2){gBuf[n++]=(char)(0xC0|(cp>>6));gBuf[n++]=(char)(0x80|(cp&63));}else{gBuf[n++]=(char)(0xE0|(cp>>12));gBuf[n++]=(char)(0x80|((cp>>6)&63));gBuf[n++]=(char)(0x80|(cp&63));}gBuf[n]=0;}
static void eraseLastUtf8(){if(!gBuf)return;size_t n=strlen(gBuf);if(!n)return;do{n--;}while(n&&((gBuf[n]&0xC0)==0x80));gBuf[n]=0;}
static void emitActive(){if(gL<0)return;if(gV<0)putUtf8(lcp(gL));else putUtf8(0xAC00+((gL*21+gV)*28+gT));gL=gV=-1;gT=0;gDot=0;}
static void addVowel(int V){
  if(gDot){
    if(gV<0){ if(V==20) gV=(gDot==1?0:2); else if(V==18) gV=(gDot==1?8:12); else gV=V; gDot=0; return; }
    if(gV==20){gV=(gDot==1?4:6);gDot=0;return;}
    if(gV==18){gV=(gDot==1?13:17);gDot=0;return;}
    gDot=0;
  }
  if(gL<0){putUtf8(vcp(V));return;}
  if(gV<0){gV=V;return;}
  if(gT){int oldL=tToL(gT);if(oldL>=0){gT=0;emitActive();gL=oldL;gV=V;return;}}
  int nv=combineV(gV,V);if(nv>=0){gV=nv;return;}emitActive();putUtf8(vcp(V));
}
static void addJamo(uint32_t cp){int L=compatToL(cp),V=compatToV(cp);if(V>=0){addVowel(V);return;}if(L<0)return;if(gL<0){gL=L;return;}if(gV<0){emitActive();gL=L;return;}if(gT==0){int ti=lToT(L);if(ti>0){gT=ti;return;}emitActive();gL=L;return;}int nt=combineT(gT,L);if(nt>=0){gT=nt;return;}emitActive();gL=L;}
static void addDot(){if(gV>=0&&(gV==20||gV==18)){gDot=(gDot%2)+1;return;}if(gDot==0)gDot=1;else if(gDot==1)gDot=2;else gDot=1;}
static void saveCycle(){if(gBuf){strncpy(gCycleBuf,gBuf,sizeof(gCycleBuf)-1);gCycleBuf[sizeof(gCycleBuf)-1]=0;}gCycleL=gL;gCycleV=gV;gCycleT=gT;gCycleDot=gDot;}
static void restoreCycle(){if(gBuf){strncpy(gBuf,gCycleBuf,gCap-1);gBuf[gCap-1]=0;}gL=gCycleL;gV=gCycleV;gT=gCycleT;gDot=gCycleDot;}

void krKeyboardBegin() { krTextBegin(); }
void krKeyboardOpen(const char *existing, char *buf, size_t cap) {
  gBuf=buf; gCap=cap; gL=gV=-1; gT=0; gDot=0; gLastKey=-1; gCycle=0; gLastKeyMs=0;
  if(existing && buf && cap){strncpy(buf,existing,cap-1);buf[cap-1]=0;}
}
static void keyText(Arduino_Canvas *gfx,int x,int y,const char *s){krSetTextColor(gfx,0x0000);krSetCursor(gfx,x,y);krPrint(gfx,s);}
static void key(Arduino_Canvas *gfx,int x,int y,int w,int h,const char *s,bool special){gfx->fillRoundRect(x,y,w,h,8,special?0xFD20:0xFFFF);gfx->drawRoundRect(x,y,w,h,8,0x0000);keyText(gfx,x+(w-(int)strlen(s)*6)/2,y+15,s);}
void krKeyboardRender(Arduino_Canvas *gfx,const char *buf,bool koreanMode){
  gfx->fillScreen(RGB565_BLACK);gfx->fillCircle(233,233,231,0xE73F);
  krSetTextColor(gfx,0x0000);krSetCursor(gfx,35,38);krPrint(gfx,koreanMode?"한글 입력":"영문 입력");
  key(gfx,330,22,105,36,koreanMode?"한글":"EN",true);
  gfx->fillRoundRect(48,70,370,48,8,0xFFFF);gfx->drawRoundRect(48,70,370,48,8,0x0000);
  krSetCursor(gfx,62,101);krPrint(gfx,(buf&&*buf)?buf:"_");
  if(!koreanMode){
    static const char *keys[]={"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"};
    for(int i=0;i<26;i++){int col=i%6,row=i/6;key(gfx,28+col*69,135+row*50,63,44,keys[i],false);}
    key(gfx,28,385,132,44,"<--",true);key(gfx,170,385,132,44,"SPACE",false);key(gfx,312,385,63,44,"OK",true);
  }else{
    static const char *num[]={"1 ㄱㅋ","2 ㄴㄹ","3 ㄷㅌ","4 ㅂㅍ","5 ㅅㅎ","6 ㅈㅊ","7 ㅇㅁ","8 ㅡ","9 ㅣ","* ㆍ","0 ㅏㅑㅓㅕ"};
    for(int i=0;i<11;i++){int col=i%3,row=i/3;key(gfx,32+col*137,135+row*50,128,44,num[i],false);}
    key(gfx,32,385,128,44,"삭제",true);key(gfx,169,385,128,44,"공백",false);key(gfx,306,385,128,44,"확인",true);
  }
  gfx->flush();
}
void krKeyboardTap(int16_t x,int16_t y,bool &koreanMode,char *buf,size_t cap,bool &closeKeyboard){
  closeKeyboard=false;gBuf=buf;gCap=cap;
  if(y>=22&&y<58&&x>=330){emitActive();koreanMode=!koreanMode;gLastKey=-1;gCycle=0;return;}
  if(y>=70&&y<118)return;
  if(!koreanMode){
    if(y>=135&&y<375){int col=(x-28)/69,row=(y-135)/50,i=row*6+col;if(i>=0&&i<26){emitActive();if(strlen(buf)<cap-1){size_t n=strlen(buf);buf[n]=char('A'+i);buf[n+1]=0;}return;}}
    if(y>=385&&y<429){if(x<160){emitActive();eraseLastUtf8();}else if(x<305){emitActive();putUtf8(' ');}else{emitActive();closeKeyboard=true;}return;}
    return;
  }
  if(y>=135&&y<385){int col=(x-32)/137,row=(y-135)/50,i=row*3+col;if(i>=0&&i<11){
      static const uint32_t seq[11][4]={{0x3131,0x314B,0,0},{0x3134,0x3139,0,0},{0x3137,0x314C,0,0},{0x3142,0x314D,0,0},{0x3145,0x314E,0,0},{0x3148,0x314A,0,0},{0x3147,0x3141,0,0},{0x3161,0,0,0},{0x3163,0,0,0},{0,0,0,0},{0x314F,0x3151,0x3153,0x3155}};
      uint32_t now=millis();int count=(i==9)?2:1;while(count<4&&seq[i][count])count++;
      if(gLastKey==i&&now-gLastKeyMs<800){gCycle=(gCycle+1)%count;restoreCycle();}else{gCycle=0;saveCycle();}
      gLastKey=i;gLastKeyMs=now;
      if(i==9){addDot();return;}addJamo(seq[i][gCycle]);return;
    }}
  if(y>=385&&y<429){if(x<160){emitActive();eraseLastUtf8();}else if(x<305){emitActive();putUtf8(' ');}else{emitActive();closeKeyboard=true;}}
}
