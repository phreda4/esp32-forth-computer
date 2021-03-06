// ESP32 forth computer
// PHREDA 2021
// * Use FabGL Library with 1 change (http://www.fabglib.org)W

#include "fabgl.h"
#include "fontjrom.h"

TaskHandle_t  mainTaskHandle;

fabgl::VGADirectController DisplayController;
fabgl::PS2Controller PS2Controller;

//------------------- CONSOLE

#define screen_size 40*30

uint32_t screen[screen_size];
byte blink;

uint32_t cattrib=0x3f000000;
uint32_t *cpos=screen;
uint32_t ccursorb;

// pixel order 2301
void IRAM_ATTR drawScanline(void *arg,uint8_t *dest,int scanLine) {
byte c[2],bf,*a=dest;
//uint32_t addrc=scanLine>>3;addrc=(addrc<<4)+(addrc<<6); //*80
uint32_t addrc=scanLine>>3;addrc=(addrc<<3)+(addrc<<5); //*40
uint32_t cc,*ac=&screen[addrc];
for (byte x=40;x!=0;x--)	{ // 80 40
	cc=*ac++;
  c[0]=((cc>>16)&0x3f)|DisplayController.m_HVSync;
  c[1]=((cc>>24)&0x3f)|DisplayController.m_HVSync;
  if (cc&0x80000000) {  // graphics x2
    bf=cc>>(((scanLine>>1)&3)<<2);  
    *a++=*a++=c[(bf>>1)&1];
    *a++=*a++=c[(bf>>0)&1];
    *a++=*a++=c[(bf>>3)&1];
    *a++=*a++=c[(bf>>2)&1];
    continue; }
  bf=romfont[((cc&0xff)<<3)+(scanLine&7)];    
  if (cc&0x40000000) { bf=~bf; }      // neg
  if (cc&0x800000) { bf=blink^bf; } // blink
  if (cc&0x400000) { if ((scanLine&7)==7) { bf=~bf; } } // underline?
  *a++=c[(bf>>5)&1];  
  *a++=c[(bf>>4)&1];  
  *a++=c[(bf>>7)&1];  
  *a++=c[(bf>>6)&1];
  *a++=c[(bf>>1)&1];      
  *a++=c[bf&1];  
  *a++=c[(bf>>3)&1];  
  *a++=c[(bf>>2)&1];
  }
if (scanLine==DisplayController.getScreenHeight()-1) { // signal end of screen
  vTaskNotifyGiveFromISR(mainTaskHandle, NULL); 
  }
}

// colors and atributes
inline void cink(byte c) { cattrib=(c&0x3f)<<24|(cattrib&0xc0ffffff); }
void cpaper(byte c) { cattrib=(c&0x3f)<<16|(cattrib&0xffc0ffff); }
void cninv() { cattrib=cattrib&0x40000000; }
void cinv() { cattrib=cattrib|0x40000000; }
void cnblk() { cattrib=cattrib&0x800000; }
void cblk() { cattrib=cattrib|0x800000; }
void cnund() { cattrib=cattrib&0x400000; }
void cund() { cattrib=cattrib|0x400000; }
void cblink() { cattrib=cattrib&0xc0ffffff; }

void ccls() {
uint32_t *p=cpos=screen;
for(int i=screen_size;i>0;i--) { *p++=cattrib; }
}

void cscroll() {
uint32_t *p=screen;
for(int i=screen_size-40;i>0;i--) { *p=*(p+40);p++; }
p=&screen[40*29];
for(int i=40;i>0;i--) { *p++=cattrib; }
}

void catxy(byte x,byte y) { if (x<40&&y<30) { cpos=&screen[x+y*40]; } }
void cemit(int c) { *cpos++=cattrib|c;if (cpos>=&screen[screen_size]) { cscroll();cpos-=40; } }

void cprint(char *s) {
byte c;
while ((c=*s++)!=0) { *cpos++=cattrib|c;if (cpos>=&screen[screen_size]) { cscroll();cpos-=40; } }
}

void ccr() {
cpos=&screen[((((cpos-screen))/40)+1)*40];
if (cpos>=&screen[screen_size]) { cscroll();cpos-=40; }
}

//------------------- CONVERSION
char strbuff[64];

char *dec(int v) {
char *p=&strbuff[62];*(p+1)=0;
int va=abs(v);
if (va<0) { return "-2147483648"; }
do { *p--=(va%10)+0x30;va/=10; } while (va!=0);
if (v<0) { *p=0x2d; } else { p++; }
return p;
}

char *bin(int v) {
char *p=&strbuff[62];*(p+1)=0;
int va=abs(v);
do { *p--=(va&1)+0x30;va>>=1; } while (va!=0);
if (v<0) { *p=0x2d; } else { p++; }
return p;
}

char *hex(int v) {
char *p=&strbuff[62];*(p+1)=0;
int va=abs(v);
do { 
  if ((va&0xf)>9) { *p--=(va&0xf)+0x37; } else { *p--=(va&0xf)+0x30; }
  va>>=4; } while (va!=0);
if (v<0) { *p=0x2d; } else { p++; }
return p;
}

//------------------- PAD
char inputpad[256];
uint32_t *cposinp;
uint32_t cmax,ccur,cfin;

void cprompt() {
ccr();cprint(" >");  
cposinp=cpos;
inputpad[0]=0;
cmax=255;ccur=0;cfin=0;
}

void modover(int c) {
  inputpad[ccur++]=(byte)c;
  if (ccur>cfin) { cfin=ccur; }
  inputpad[cfin]=0;
}
void modoins(int c) {
  memmove(&inputpad[ccur+1],&inputpad[ccur],cfin-ccur+1);
  inputpad[ccur++]=(byte)c;
  cfin++;
}
void kbac() {
  if (ccur==0) { return ; }
  memmove(&inputpad[ccur-1],&inputpad[ccur],cfin-ccur+1);  
  ccur--;cfin--;
}
void kdel() { ccur++;kbac();  }  
void khome() { ccur=0; }  
void kend() { ccur=cfin; }  
void klef() { if (ccur>0) { ccur--; } }  
void krig() { if (ccur<cfin) { ccur++; } }  

byte kmode=0;
bool down;

void cpad() {
cpos=cposinp;
cprint(inputpad);cprint(" ");
cpos=cposinp+ccur;
ccursorb=*cpos;*(cpos)|=0x800000; // blink in cursor

auto keyboard = PS2Controller.keyboard();
if (keyboard->virtualKeyAvailable()) {
  
  auto vk = keyboard->getNextVirtualKey(&down);
  int c = keyboard->virtualKeyToASCII(vk);
  if (down) {
    *cpos=ccursorb;// remove cursor
    if (c>=32) { 
      if (kmode==0) { modoins(c); } else { modover(c); }
  } else {
      if (vk==fabgl::VK_INSERT) { kmode^=1; }            
      if (vk==fabgl::VK_BACKSPACE) { kbac(); }  
      if (vk==fabgl::VK_DELETE) { kdel(); }  
      if (vk==fabgl::VK_HOME) { khome(); }  
      if (vk==fabgl::VK_END) { kend(); }  
      if (vk==fabgl::VK_LEFT) { klef(); }  
      if (vk==fabgl::VK_RIGHT) { krig(); }  

      if (vk==fabgl::VK_RETURN) { interpreter(); }  
      
//  if (vk==fabgl::VK_TAB) { kbac(); }            
//  if (vk==fabgl::VK_UP) { kbac(); }  
//  if (vk==fabgl::VK_DOWN) { kbac(); }  
//  if (vk==fabgl::VK_PAGEUP) { kbac(); }  
//  if (vk==fabgl::VK_PAGEDOWN) { kbac(); }  
    }
  }
 }  
}

//------------------- INTERPRETER

void interpreter() {
  ccr();
  cprint("ok");
  cprompt();
}
//------------------- MAIN
void setup()
{
//  Serial.begin(115200); delay(500); Serial.write("ESP32 Forth Computer\n"); // DEBUG ONLY

mainTaskHandle = xTaskGetCurrentTaskHandle();
PS2Controller.begin(PS2Preset::KeyboardPort0); //PS2Preset::KeyboardPort0_MousePort1);
//keyboard->suspendVirtualKeyGeneration(false);
DisplayController.begin();
DisplayController.setDrawScanlineCallback(drawScanline);
DisplayController.setResolution(QVGA_320x240_60Hz); // 40x30

//  DisplayController.setResolution(VGA_640x480_60HzD); // 80x60 << better view
  /*  
  VGA_640x480_60Hz,
  VGA_640x480_60HzAlt1,
  VGA_640x480_60HzD, << better
  VGA_640x480_73Hz,
  VESA_640x480_75Hz,
  */
 
ccls(); 
cink(60);cprint("ESP32 ");
cink(3);cprint("Forth ");
cink(12);cprint("Computer ");
cink(63);ccr();

cprompt();
}


void loop()
{
cpad();

if (millis()&0x100) { blink=0xff; } else { blink=0; } 
ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for vertical sync
}
