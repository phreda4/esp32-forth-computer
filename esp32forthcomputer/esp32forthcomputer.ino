// ESP32 forth computer
// PHREDA 2021
// * Use FabGL Library with 2 changes (http://www.fabglib.org)

//#include <WiFi.h>

#include <SPIFFS.h>

#include "fabgl.h"
#include "fontjrom.h"

#include "r3.h"

//const char* ssid = "";
//WiFiServer server(80);

TaskHandle_t  mainTaskHandle;

class VGADirectControllerPublic : public fabgl::VGADirectController
{
 public:
  intr_handle_t GetISRHandle() 
  {
   return m_isr_handle;
  }
  uint8_t GetHVSync()
  {
    return m_HVSync;
  }
};

//fabgl::VGADirectController DisplayController;
VGADirectControllerPublic DisplayController;
fabgl::PS2Controller PS2Controller;

//------------------- CONSOLE
#define SCREEN_W 40
#define SCREEN_H 30

//#define SCREEN_W 80
//#define SCREEN_H 60

#define SCREEN_SIZE (SCREEN_W*SCREEN_H)

uint32_t screen[SCREEN_SIZE];
byte blink;

uint32_t cattrib=0x3f000000;
uint32_t *cpos=screen;
uint32_t ccursorb;

// pixel order 2301
void IRAM_ATTR drawScanline(void *arg,uint8_t *dest,int scanLine) {
byte c[2],bf,*a=dest;
#if SCREEN_W == 80
uint32_t addrc=scanLine>>3;addrc=(addrc<<4)+(addrc<<6); //*80
#else
uint32_t addrc=scanLine>>3;addrc=(addrc<<3)+(addrc<<5); //*40
#endif
uint32_t cc,*ac=&screen[addrc];
//Modificacion JJ
//uint8_t jj_m_HVSync = DisplayController.createBlankRawPixel();
uint8_t jj_m_HVSync = DisplayController.GetHVSync();
for (byte x=SCREEN_W;x!=0;x--)	
{ // 80 40
  cc=*ac++;
  //JJ c[0]=((cc>>16)&0x3f)|DisplayController.m_HVSync;
  //JJ c[1]=((cc>>24)&0x3f)|DisplayController.m_HVSync;
  c[0]=((cc>>16)&0x3f)|jj_m_HVSync;
  c[1]=((cc>>24)&0x3f)|jj_m_HVSync;
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
  if (millis()&0x100) { blink=0xff; } else { blink=0; } 
  }
}

// colors and atributes
inline void cink(byte c) { cattrib=(c&0x3f)<<24|(cattrib&0xc0ffffff); }
inline void cpaper(byte c) { cattrib=(c&0x3f)<<16|(cattrib&0xffc0ffff); }
inline void cninv() { cattrib=cattrib&0x40000000; }
inline void cinv() { cattrib=cattrib|0x40000000; }
inline void cnblk() { cattrib=cattrib&0x800000; }
inline void cblk() { cattrib=cattrib|0x800000; }
inline void cnund() { cattrib=cattrib&0x400000; }
inline void cund() { cattrib=cattrib|0x400000; }
inline void cblink() { cattrib=cattrib&0xc0ffffff; }

void ccls() {
uint32_t *p=cpos=screen;
for(int i=SCREEN_SIZE;i>0;i--) { *p++=cattrib; }
}

void cscroll() {
uint32_t *p=screen;
for(int i=SCREEN_SIZE-SCREEN_W;i>0;i--) { *p=*(p+SCREEN_W);p++; }
p=&screen[SCREEN_W*(SCREEN_H-1)];
for(int i=SCREEN_W;i>0;i--) { *p++=cattrib; }
}

void catxy(byte x,byte y) { if (x<SCREEN_W&&y<SCREEN_H) { cpos=&screen[x+y*40]; } }
void cemit(int c) { *cpos++=cattrib|c;if (cpos>=&screen[SCREEN_SIZE]) { cscroll();cpos-=40; } }
void cprint(char *s) { int c;while ((c=(int)*s++)!=0) { cemit(c); } } 
void ccr() {
cpos=&screen[((((cpos-screen))/SCREEN_W)+1)*SCREEN_W];
if (cpos>=&screen[SCREEN_SIZE]) { cscroll();cpos-=SCREEN_W; }
}

//------------------- CONVERSION
char strbuff[33];

char *dec(int v) {
char *p=&strbuff[31];*(p+1)=0;
int va=abs(v);
if (va<0) { return "-2147483648"; }
do { *p--=(va%10)+0x30;va/=10; } while (va!=0);
if (v<0) { *p=0x2d; } else { p++; }
return p;
}

char *bin(int v) {
char *p=&strbuff[31];*(p+1)=0;
int va=abs(v);
do { *p--=(va&1)+0x30;va>>=1; } while (va!=0);
//if (v<0) { *p=0x2d; } else { p++; }
return p;
}

char *hex(int v) {
char *p=&strbuff[31];*(p+1)=0;
int va=abs(v);
do { 
  if ((va&0xf)>9) { *p--=(va&0xf)+0x37; } else { *p--=(va&0xf)+0x30; }
  va>>=4; } while (va!=0);
//if (v<0) { *p=0x2d; } else { p++; }
return p;
}

char *fix(int v) {
char *p;
int va=abs(v);
p=dec((((va&0xffff)*10000)>>16)+10000);
*p--=0x2e;va>>=16; // .
do { *p--=(va%10)+0x30;va/=10; } while (va!=0);
if (v<0) { *p=0x2d; } else { p++; }
return p;
}

//------------------- PAD
#define CMAX 255
char inputpad[CMAX+1];

uint32_t *cposinp;
uint32_t ccur,cfin;

int level;
int modo;
int cntstacka;
int stacka[8];
int lastblock;

inline void iniA(void) { cntstacka=0; }
inline void pushA(int n) { stacka[cntstacka++]=n; }
inline int popA(void) { return stacka[--cntstacka]; }

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
void kdel() { if (ccur<cfin) { ccur++;kbac(); } }  
void khome() { ccur=0; }  
void kend() { ccur=cfin; }  
void klef() { if (ccur>0) { ccur--; } }  
void krig() { if (ccur<cfin) { ccur++; } }  

byte kmode=0;
bool down;

int getkey() {
auto keyboard = PS2Controller.keyboard();
auto vk=keyboard->getNextVirtualKey(&down,1);
if (vk==fabgl::VK_NONE) return 0;
int scanc=keyboard->virtualKeyToASCII(vk);
if (scanc==-1) scanc=vk<<8;
return scanc|((down==false)?0x10000:0);
}

void cpad() {
auto keyboard = PS2Controller.keyboard();
VirtualKey vk = keyboard->getNextVirtualKey(&down);
if (vk==fabgl::VK_NONE) return;
int c=keyboard->virtualKeyToASCII(vk);
if (down) {
  *cpos=ccursorb;// remove cursor
  if (c>31&&c<127) { 
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
// 	if (vk==fabgl::VK_PAGEDOWN) { kbac(); }  
	}
  cpos=cposinp;
  cprint(inputpad);cprint(" ");
  cpos=cposinp+ccur;
  while (cpos+1>=&screen[SCREEN_SIZE]) { cpos-=40;cposinp-=40; }	// scroll??
  ccursorb=*cpos;*(cpos)|=0x800000; // blink in cursor
  }
}


//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

//////////////////////////////////////////
const char *wsysdicc[]={
"WORDS",".S","LIST","EDIT","DUMP",
"CD","DIR","CLOAD","CSAVE","CNEW",
"WIFI","CAT",
};

char filenow[32];

void exSis(int n,char *str) {
switch(n) {
case 0:xwords();break;
case 1:xstack();break;
case 2:xlist(str);break;
case 3:xedit();break;
case 4:dump();break;
case 5:xcd(str);break;
case 6:xdir();break;
case 7:strcpy(filenow,filename(str));filenamer3(filenow);xcload(filenow);break;
case 8:xcsave(str);break;
case 9:xcnew();break;
case 10:xwifi();break;
case 11:xcat(str);break;
  }
}

char fullfn[32];
char nowpath[32];
char lastdir[32];
int cpath;

char *filename(char *n) {
char *pn=(char*)fullfn;
char *pp=(char*)nowpath;
while(*pp!=0) { *pn++=*pp++; }
while(*n!=0) { *pn++=*n++; }
*pn=0;
return (char *)fullfn;
}

char *filenameo(char *n) {
char *pp=(char*)nowpath;
while (*pp!=0) { 
	if (*pp!=*n) { return 0;}
	pp++;n++;
	}
return n;
}

void cpyldir(char *n) {
char *l,*p=(char *)lastdir;
while (*n!=0) { 
	if (*n==0x2f) { l=p; }
	*p++=*n++; }
*(l+1)=0;
}

int indir(char *n) {
char *l,*nl=n;
char *lp=0,*np=nowpath;
char *ll=0,*ld=lastdir;
while (*nl!=0) { 
	if (*nl==0x2f) { l=nl; } // last /
	if (*nl!=*np) { if (lp==0) { lp=nl; } } // first diferent from nowpad
	if (*nl!=*ld) { if (ll==0) { ll=nl; } } // first diferent from lastdir
	nl++;np++;ld++; }
if ((int)(lp-n)<cpath) { 
  return 0;
  }
cpyldir(n);
if (lp<l) { 
  if (ll<l) {
    cprint(lastdir);
    ccr();
    }
  return 0;
  } // not in path
return 1;
}

void xdir() {
cink(63);cpaper(0);  
fastmode();
File root = SPIFFS.open("/");
File file = root.openNextFile();
cprint(nowpath);ccr();
char *fn;
int di;
cpyldir((char *)file.name());
while(file){
	fn=(char*)file.name();
	di=indir(fn);
	if (di>0) {
		fn=filenameo(fn);
		cprint(fn);
		cemit(32);cprint(dec(file.size()));
		cprint("b");  
		ccr();
		}
	file = root.openNextFile();
	}
file.close();
root.close();
slowmode();
//cprint(nowpath);ccr();
modo=-1;
}

void xcd(char *n) {
char *p=nowpath;
while (*p!=0) { p++; }
if (*n==0x2e && *(n+1)==0x2e) {
  if (p>nowpath+1) { p-=2; }
  while (p>nowpath && *p!=0x2f) { p--; }
  p++;*p=0;
  cpath=(int)p-(int)nowpath;
  return;  
  }
while (*n!=0) { *p++=*n++; }
if (*(p-1)!=0x2f) { *p++=0x2f; }
*p=0;
cpath=(int)p-(int)nowpath;
modo=-1;
}
//////////////////////////////////////////////////////////////

void xwords() {
int i=0;
cpaper(0);
cink(7);
while (wcoredicc[i][0]!=0) {
  cprint((char*)wcoredicc[i]);cemit(32);
  i++;}
for (i=0;i<ndicc;i++) {
  if (dicc[i].mem&0x80000000) cink(51); else cink(3);  
  cprint(code2word(dicc[i].name));cemit(32);    
  }
ccr();
modo=-1;
}

void xstack() {
cink(63);
for (int32_t *i=stack+1;i<=ANOS;i++) { cpaper(0);cemit(32);cpaper(52);cprint(dec(*i)); }
if (stack<=ANOS) { cpaper(0);cemit(32);cpaper(52);cprint(dec(ATOS)); }
cpaper(0);ccr();
} 

////////////////////////////////////////////////
// includes

char inames[1024];
char *pinames;
char *includes[32];
int cincludes;
int cnttok,cntdef;

void isinclude(char *str)
{
includes[cincludes]=pinames;
while ((unsigned char)*str>31) { *pinames++=*str++; }
*pinames++=0;
//printf("[%s]",filename);
cincludes++;
}

// resolve includes, recursive definition
void r3includes(char *str) 
{
while(*str!=0) {
	str=trim(str);
	switch (*str) {
		case '^':isinclude(str+1);str=nextcr(str);break;// include
		case '|':str=nextcom(str);break;				// comments	
		case ':':cntdef++;modo=1;str=nextw(str);break;	// code
		case '#':cntdef++;modo=0;str=nextw(str);break;	// data	
		case '"':cnttok+=modo;str=nextstr(str);break;	// strings		
		default: cnttok+=modo;str=nextw(str);break; 	// other
		}
	}
}

void cload(char *fn) {
int len,error,lin=0;

if (SPIFFS.exists(fn)) {
  File entry = SPIFFS.open(fn,"r");
  while(entry.available()){
	len=entry.readBytesUntil('\n',inputpad,CMAX);
	inputpad[len]=0;lin++;
    error=r3token(inputpad);
	if (error!=0) { 
		cprint("error line ");cprint(dec(lin));ccr();
		cprint(inputpad); ccr();
		return; }
    }
  entry.close();
  }
}

void xcload(char *fn) {
char fname[32];

pinames=inames;	// clear nameincludes
cincludes=0;	// cnt includes
cntdef=0;		// cnt definitions
cnttok=0;		// cnt tokens

fastmode();
int len;
//char *fno=filename(fn);
strcpy(fname,fn);
// search includes, only one file
File entry = SPIFFS.open(fn,"r");
while(entry.available()){
  len=entry.readBytesUntil('\n',inputpad,CMAX);
  inputpad[len]=0;
  r3includes(inputpad);
  }
entry.close();

r3init();
for(int i=0;i<cincludes;i++) {
  fn=filename(includes[i]);
  cprint(fn);ccr();
  cload(fn);
	}
cprint(fname);ccr();
cload(fname);

slowmode();
}

void xcat(char *fn) {
fastmode();
int len,lin=0;
char *fno=filename(fn);
if (SPIFFS.exists(fno)) {
  File entry = SPIFFS.open(fno,"r");
  while(entry.available()){
  len=entry.readBytesUntil('\n',inputpad,CMAX);
  inputpad[len]=0;lin++;
  cprint(inputpad);ccr();
    }
  entry.close();
  }
slowmode();
}
//--------------------------
void xcsave(char *fn) {
fastmode();
// 
if (*fn!=0) { fn=filenow; } else { strcpy(filenow,filename(fn)); }

File entry = SPIFFS.open(filename(fn),"w");
for (int i=0;i<ndicc;i++) {
	entry.print("\n");
	}
entry.close();
slowmode();
}


//--------------------------
void xcnew() {
r3init();
}


void xwifi() {
cink(63);cpaper(0);  
/*
cprint("Connecting to ");
cprint((char*)ssid);
  WiFi.begin(ssid, "33134253");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    cprint(".");
  }
  // Print local IP address and start web server
ccr();
cprint("WiFi connected.");
cprint("IP address: ");
cprint((char*)WiFi.localIP().toString().c_str());
server.begin();	
*/
}

////////////////////////////////////////////////////////////////
// ----
void emitcr(char *s) { 
while (((unsigned char)*s)>31||*s==9) { cemit(*s++); } 
}

void emitstr(char *s) { 
while (*s!=0) { cemit(*s++); }
}

int getval(int op,int nop) {
if ((nop&0xff)==iLIT2) { return (op>>8)^((nop^0xff00)<<16); }
return op>>8;
}

// search word by address
int a2dicc(int32_t mem,int flag) {
for (int i=0;i<ndicc;i++) {
  if ((dicc[i].mem&0xfffff)==mem && (dicc[i].mem&0x80000000)==flag) return i;
  }
return -1;
}

void tokenprint(int op,int nop) {
if ((op&0xff)>=INTERNALWORDS) {
	cink(7);
	cprint((char*)wcoredicc[(op&0xff)-INTERNALWORDS]);
} else {
	switch (op&0xff) {
    case iLITd:cink(15);cprint(dec(getval(op,nop)));break;
    case iLITh:cink(15);cprint("$");cprint(hex(getval(op,nop)));break;
    case iLITb:cink(15);cprint("%");cprint(bin(getval(op,nop)));break;
    case iLITf:cink(15);cprint(fix(getval(op,nop)));break;	
    case iLIT2:break;
    case iLITs:cink(63);cemit(34);emitstr((char*)&memdata[op>>8]);cemit(34);break;
    case iCOM:cink(21);cprint("|");emitcr((char*)&memdata[op>>8]);ccr();break;
    case iJMPR:cprint(dec(op>>8));break;
    case iJMP:cink(12);cprint(code2word(dicc[a2dicc(op>>8,0)].name));cink(7);cprint(" ;");break;
    case iCALL:cink(12);cprint(code2word(dicc[a2dicc(op>>8,0)].name));break;
    case iVAR:cink(12);cprint(code2word(dicc[a2dicc(op>>8,0x80000000)].name));break;
    case iADR:cink(60);cprint("'");cprint(code2word(dicc[a2dicc(op>>8,0)].name));break;
    case iADRv:cink(60);cprint("'");cprint(code2word(dicc[a2dicc(op>>8,0x80000000)].name));break;
		}
	}
cemit(32);
}


void printword(int w) {
cink(3);cprint(":"); 
cprint(code2word(dicc[w].name));cemit(32);
int it=dicc[w].mem&0xfffff;
for (int i=0;i<(dicc[w].mem>>20)&0x3ff;i++) {
  tokenprint(memcode[it++],memcode[it]); }
ccr();
}

void printvar(int w) {
cink(51);cprint("#"); 
cprint(code2word(dicc[w].name));cemit(32);
int *it=(int*)&memdata[dicc[w].mem&0xfffff];
for (int i=((dicc[w].mem>>20)&0x3ff)>>2;i>0;i--) {
  cemit(32);cprint(dec(*it++)); }
ccr();
}

void emptykey() {
while (getkey()!=0) ;
}

void waitkey() {
redraw();
while (getkey()==0) ;
}

void xlist(char *str) {
/*	
int w;
if ((token&0xff)==iADR) {
	w=a2dicc(token>>8,0x00000000);
	printword(w);
	return;
} else if ((token&0xff)==iADRv) {
	w=a2dicc(token>>8,0x80000000);	
	printvar(w);
	return;
	} 
*/	
emptykey();	
ccls();
for (int i=0;i<ndicc;i++) {
	if (dicc[i].mem&0x80000000) printvar(i); else printword(i);
	if ((i&7)==7) waitkey();
	}
}


void xedit() {
xcload("/edit.r3");  
filenamer3(filenow);
runr3(dicc[ndicc-1].mem&0xfffff);
xcload(filenow);
}

void dump() {
int d=ATOS;
unsigned char b;
char *mem=(char*)&memdata[ATOS];
for (int j=0;j<16;j++) {
  cprint(hex(d));cprint(":");
  for(int i=0;i<16;i++) { 
    b=(unsigned char)*mem;
    if (i&1) { cink(63); } else { cink(42); }
    if (b<16) { cprint("0"); } 
    cprint(hex(b));
    mem++;d++; }
  ccr(); 
  }
}

const char *prompt[]={">",":>","#>","#>","#>"};

void cprompt() {
ccr();cprint((char*)prompt[modo]);  
cposinp=cpos;
inputpad[0]=0;ccur=0;cfin=0;
//cpos=cposinp;
//cprint(inputpad);
//cprint(" ");
cpos=cposinp+ccur;
ccursorb=*cpos;*(cpos)|=0x800000; // blink in cursor
}

void cedit() {
ccur=cerror;
ccr();cprint((char*)prompt[modo]);  
cposinp=cpos;
cprint(inputpad);
cpos=cposinp+ccur;
while (cpos+1>=&screen[SCREEN_SIZE]) { cpos-=40;cposinp-=40; }	// scroll??
ccursorb=*cpos;*(cpos)|=0x800000; // blink in cursor
}

const char *msg[]={
  "Ok.",
  "Word not found",
  "Addr not found",
};
int error;

void promt() {
if (modo==0||error!=0) {
	if (error==0) { cink(12);cpaper(0); } else { cink(63);cpaper(3); }
	cprint((char*)msg[error]);
	}
cink(63);cpaper(0);
if (error==0) { cprompt(); } else { cedit(); }
}

void interpreter() {

if (inputpad[0]==0) { modo=0;cprompt();return; }

if (modo!=0) {	// in definition
	error=r3token(inputpad);
	promt();
	return;
	}

ccr();
int andicc=ndicc,amemd=memd,amemc=memc;
	
error=r3token(inputpad);
if (modo==0&&andicc==ndicc&&error==0) { // only imm, not new definitions 
  codetok(iEND);
  runr3(amemc);
  memd=amemd;memc=amemc;
  }
if (modo==-1) { modo=0; }
promt();
}

//------------------- MAIN
void redraw() {
ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for vertical sync
}

void inter() {
while (1) {
  cpad();
  redraw();
  }
}

void setup() {
Serial.begin(115200);Serial.write("ESP32 Forth Computer\n");

SPIFFS.begin(true);

mainTaskHandle = xTaskGetCurrentTaskHandle();
PS2Controller.begin(PS2Preset::KeyboardPort0); //PS2Preset::KeyboardPort0_MousePort1);

auto keyboard = PS2Controller.keyboard();
//keyboard->suspendVirtualKeyGeneration(false);

// keyboard->setLayout(&fabgl::USLayout);
// keyboard->setLayout(&fabgl::UKLayout);
// keyboard->setLayout(&fabgl::GermanLayout);
// keyboard->setLayout(&fabgl::ItalianLayout);
keyboard->setLayout(&fabgl::SpanishLayout);
        
DisplayController.begin();
DisplayController.setDrawScanlineCallback(drawScanline);

#if SCREEN_W == 80
  /*  
  VGA_640x480_60Hz,
  VGA_640x480_60HzAlt1,
  VGA_640x480_60HzD, << better
  VGA_640x480_73Hz,
  VESA_640x480_75Hz,
  */
	DisplayController.setResolution(VGA_640x400_60Hz); // 80x60 << better view
#else
	DisplayController.setResolution(QVGA_320x240_60Hz); // 40x30
#endif

r3init();
ccls(); 
cink(60);cprint("ESP32 ");
cink(3);cprint("Forth ");
cink(12);cprint("Computer ");ccr();
cink(52);cprint("PHREDA 2021");ccr();
cink(12);
cprint("Mem:");
//cprint(dec(heap_caps_get_free_size(MALLOC_CAP_DMA)>>10));
cprint(dec(heap_caps_get_free_size(MALLOC_CAP_32BIT)>>10));
cprint("Kb - ");
cprint(" FS:");
cprint(dec(SPIFFS.usedBytes()>>10));cprint("/");
cprint(dec(SPIFFS.totalBytes()>>10));cprint("Kb used");   
ccr();

cink(63);
cprompt();
strcpy(nowpath,"/");
cpath=1;
strcpy(filenow,"/new.r3");
inter();
}

void loop() { }
