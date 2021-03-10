// ESP32 forth computer
// PHREDA 2021
// * Use FabGL Library with 1 change (http://www.fabglib.org)W

#include "SPIFFS.h" 

#include "fabgl.h"
#include "fontjrom.h"

TaskHandle_t  mainTaskHandle;

fabgl::VGADirectController DisplayController;
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
for (byte x=SCREEN_W;x!=0;x--)	{ // 80 40
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
char inputpad[256];
uint32_t *cposinp;
uint32_t cmax,ccur,cfin;

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
//if (!keyboard.virtualKeyAvailable()) { return 0; }
auto vk = keyboard->getNextVirtualKey(&down);
if (vk==fabgl::VK_NONE) return 0;
return (vk<<8)|keyboard->virtualKeyToASCII(vk);
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
//  if (vk==fabgl::VK_PAGEDOWN) { kbac(); }  
		}
	cpos=cposinp;
	cprint(inputpad);cprint(" ");
	cpos=cposinp+ccur;
	while (cpos+1>=&screen[SCREEN_SIZE]) { cpos-=40;cposinp-=40; }	// scroll??
	ccursorb=*cpos;*(cpos)|=0x800000; // blink in cursor
	}
}

//------------------- INTERPRETER

const char *wcoredicc[]={
";","(",")","[","]",
"EX","0?","1?","+?","-?",
"<?",">?","=?",">=?","<=?","<>?","AND?","NAND?","BT?",
"DUP","DROP","OVER","PICK2","PICK3","PICK4","SWAP","NIP",
"ROT","2DUP","2DROP","3DROP","4DROP","2OVER","2SWAP",
"@","C@","@+","C@+","!","C!","!+","C!+","+!","C+!",
">A","A>","A@","A!","A+","A@+","A!+",
">B","B>","B@","B!","B+","B@+","B!+",
"NOT","NEG","ABS","SQRT","CLZ",
"AND","OR","XOR","+","-","*","/","MOD",
"<<",">>",">>>","/MOD","* /","*>>","<</",
"MOVE","MOVE>","FILL","CMOVE","CMOVE>","CFILL",

"WORDS",".S",
"INK","PAPER","CLS","ATXY","EMIT","PRINT","CR",
""
};

#define INTERNALWORDS 9
enum {
iLIT1,iLIT2,iLITs,iCOM,iJMPR,iJMP,iCALL,iVAR,iADR, // internal
 
iEND,OPEB,CLOB,OPEA,CLOA,iEX,ZIF,UIF,PIF,NIF,
IFL,IFG,IFE,IFGE,IFLE,IFNE,IFAND,IFNAND,IFBT,
iDUP,iDROP,iOVER,iPICK2,iPICK3,iPICK4,iSWAP,iNIP,	
iROT,i2DUP,i2DROP,i3DROP,i4DROP,i2OVER,i2SWAP,	
iFE,iCFE,iFEP,iCFEP,iST,iCST,iSTP,iCSTP,iPST,iCPST,		
iTA,iAT,iAFE,iAST,iAP,iAFEP,iASTP,					
iTB,iBT,iBFE,iBST,iBP,iBFEP,iBSTP,					
iNOT,iNEG,iABS,iSQRT,iCLZ,						
iAND,iOR,iXOR,iADD,iSUB,iMUL,iDIV,iMOD,
iSHL,iSAR,iSHR,iDMOD,iMULDIV,iMULSH,iSHDIV,
iMOV,iMOVA,iFILL,iCMOV,iCMOVA,iCFILL,

iWORDS,iSTACK,
iINK,iPAPER,iCLS,iATXY,iEMIT,iPRINT,iCR,

iiii, // last real
};

#define STACKSIZE 512
#define DICCMEM 0x3ff
#define CODEMEM 0xfff
#define DATAMEM 0xffff

int32_t stack[STACKSIZE];
int32_t ATOS,*ANOS;

struct dicctionary {
  uint64_t name;
  uint32_t mem;
};

dicctionary dicc[DICCMEM];
uint32_t ndicc=0;

int32_t memcode[CODEMEM];
uint32_t memc=0;

int8_t  memdata[DATAMEM];
uint32_t memd=0;

inline int char6bit(int c) { return ((((c-0x1f)&0x40)>>1)|(c-0x1f))&0x3f;}

int64_t word2code(char *str) {
int c;
int64_t v=0;
while ((c=*str++)>32) { v=(v<<6)|char6bit(c); }
return v;
}

char wname[16];

inline int bit6char(int c) { return (c&0x3f)+0x1f; }

char *code2word(int64_t vname) {
wname[15]=0;
int i=14;
while (vname!=0) {
  wname[i]=bit6char((int)vname);
  i--;vname>>=6;
  }
return (char*)&wname[i+1];
}

void r3init() {
ndicc=memc=memd=0;
resetr3();
}

void resetr3() {
ATOS=0;
ANOS=&stack[-1];
}

void runr3(int32_t boot) {
stack[STACKSIZE-1]=0;  
register int32_t TOS=ATOS;
register int32_t *NOS=ANOS;
register int32_t *RTOS=&stack[STACKSIZE-1];
register int32_t REGA=0;
register int32_t REGB=0;
register int32_t op=0;
register int32_t ip=boot;

next:
  op=memcode[ip++]; //  printcode(op);
  switch(op&0x7f){
    case iLIT1:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iLIT2:NOS++;*NOS=TOS;TOS=op&0xffffff80;goto next;
    case iLITs:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iCOM:goto next;
    case iJMPR:ip+=op>>8;goto next;
    case iJMP:ip=op>>8;goto next;
    case iCALL:RTOS--;*RTOS=ip;ip=op>>8;goto next;
    case iVAR:NOS++;*NOS=TOS;TOS=memdata[op>>8];goto next;
    case iADR:NOS++;*NOS=TOS;TOS=op>>8;goto next;
	
    case iEND:ip=*RTOS;RTOS++;if (ip==0) { ATOS=TOS;ANOS=NOS;return; } goto next;  
    case OPEB:goto next;
    case CLOB:ip+=op>>8;goto next;
    case OPEA:ip+=op>>8;goto next;
    case CLOA:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iEX:RTOS--;*RTOS=ip;ip=TOS;TOS=*NOS;NOS--;goto next;  
	case ZIF:if (TOS!=0) {ip+=op>>8;} goto next;
	case UIF:if (TOS==0) {ip+=op>>8;} goto next;
	case PIF:if (TOS<0) {ip+=op>>8;} goto next;
	case NIF:if (TOS>=0) {ip+=op>>8;} goto next;
	case IFL:if (TOS<=*NOS) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFL
	case IFG:if (TOS>=*NOS) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFG
	case IFE:if (TOS!=*NOS) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFN	
	case IFGE:if (TOS>*NOS) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFGE
	case IFLE:if (TOS<*NOS) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFLE
	case IFNE:if (TOS==*NOS) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFNO
	case IFAND:if (!(TOS&*NOS)) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFNA
	case IFNAND:if (TOS&*NOS) {ip+=(op>>8);} TOS=*NOS;NOS--;goto next;//IFAN
	case IFBT:if (*(NOS-1)>TOS||*(NOS-1)<*NOS) {ip+=(op>>8);} TOS=*(NOS-1);NOS-=2;goto next;//BTW 
    case iDUP:NOS++;*NOS=TOS;goto next;
    case iDROP:TOS=*NOS;NOS--;goto next;
    case iOVER:NOS++;*NOS=TOS;TOS=*(NOS-1);goto next;
    case iPICK2:NOS++;*NOS=TOS;TOS=*(NOS-2);goto next;
    case iPICK3:NOS++;*NOS=TOS;TOS=*(NOS-3);goto next;
    case iPICK4:NOS++;*NOS=TOS;TOS=*(NOS-4);goto next;
    case iSWAP:op=*NOS;*NOS=TOS;TOS=op;goto next;
    case iNIP:NOS--;goto next;
    case iROT:op=TOS;TOS=*(NOS-1);*(NOS-1)=*NOS;*NOS=op;goto next;
    case i2DUP:op=*NOS;NOS++;*NOS=TOS;NOS++;*NOS=op;goto next;
    case i2DROP:NOS--;TOS=*NOS;NOS--;goto next;
    case i3DROP:NOS-=2;TOS=*NOS;NOS--;goto next;
    case i4DROP:NOS-=3;TOS=*NOS;NOS--;goto next;
    case i2OVER:NOS++;*NOS=TOS;TOS=*(NOS-3);NOS++;*NOS=TOS;TOS=*(NOS-3);goto next;
    case i2SWAP:op=*NOS;*NOS=*(NOS-2);*(NOS-2)=op;op=TOS;TOS=*(NOS-1);*(NOS-1)=op;goto next;  
    case iFE:TOS=*(int*)&memdata[TOS];goto next;
    case iCFE:TOS=*(char*)&memdata[TOS];goto next;
    case iFEP:NOS++;*NOS=TOS+4;TOS=*(int*)&memdata[TOS];goto next;
    case iCFEP:NOS++;*NOS=TOS+1;TOS=*(char*)&memdata[TOS];goto next;
    case iST:*(int*)&memdata[TOS]=(int)*NOS;NOS--;TOS=*NOS;NOS--;goto next;
    case iCST:memdata[TOS]=(char)*NOS;NOS--;TOS=*NOS;NOS--;goto next;
    case iSTP:*(int*)&memdata[TOS]=*NOS;NOS--;TOS+=4;goto next;
    case iCSTP:memdata[TOS]=(char)*NOS;NOS--;TOS+=1;goto next;
    case iPST:*(int*)&memdata[TOS]+=*NOS;NOS--;TOS=*NOS;NOS--;goto next;
    case iCPST:memdata[TOS]+=*NOS;NOS--;TOS=*NOS;NOS--;goto next;
    case iTA:REGA=TOS;TOS=*NOS;NOS--;goto next;
    case iAT:NOS++;*NOS=TOS;TOS=REGA;goto next;
    case iAFE:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGA];goto next;
    case iAST:*(int32_t*)&memdata[REGA]=TOS;TOS=*NOS;NOS--;goto next;
    case iAP:REGA+=TOS;TOS=*NOS;NOS--;goto next;
    case iAFEP:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGA];REGA+=4;goto next;
    case iASTP:*(int32_t*)&memdata[REGA]=TOS;TOS=*NOS;NOS--;REGA+=4;goto next;
    case iTB:REGB=TOS;TOS=*NOS;NOS--;goto next;
    case iBT:NOS++;*NOS=TOS;TOS=REGB;goto next;
    case iBFE:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGB];goto next;
    case iBST:*(int32_t*)&memdata[REGB]=TOS;TOS=*NOS;NOS--;goto next;
    case iBP:REGB+=TOS;TOS=*NOS;NOS--;goto next;
    case iBFEP:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGB];REGB+=4;goto next;
    case iBSTP:*(int32_t*)&memdata[REGB]=TOS;TOS=*NOS;NOS--;REGB+=4;goto next;
    case iNOT:TOS=~TOS;goto next;
    case iNEG:TOS=-TOS;goto next;
    case iABS:op=(TOS>>63);TOS=(TOS+op)^op;goto next;
    case iSQRT:TOS=(int)sqrt(TOS);goto next;
    case iCLZ:TOS=__builtin_clz(TOS);goto next;
    case iAND:TOS&=*NOS;NOS--;goto next;
    case iOR:TOS|=*NOS;NOS--;goto next;
    case iXOR:TOS^=*NOS;NOS--;goto next;
    case iADD:TOS=*NOS+TOS;NOS--;goto next;
    case iSUB:TOS=*NOS-TOS;NOS--;goto next;
    case iMUL:TOS=*NOS*TOS;NOS--;goto next;
    case iDIV:TOS=(*NOS/TOS);NOS--;goto next;
    case iMOD:TOS=*NOS%TOS;NOS--;goto next;
    case iSHL:TOS=*NOS<<TOS;NOS--;goto next;
    case iSAR:TOS=*NOS>>TOS;NOS--;goto next;
    case iSHR:TOS=((uint32_t)*NOS)>>TOS;NOS--;goto next;
    case iDMOD:op=*NOS;*NOS=op/TOS;TOS=op%TOS;goto next;
    case iMULDIV:TOS=((int64_t)(*(NOS-1))*(*NOS)/TOS);NOS-=2;goto next;
    case iMULSH:TOS=((int64_t)(*(NOS-1)*(*NOS))>>TOS);NOS-=2;goto next;
    case iSHDIV:TOS=(int64_t)((*(NOS-1)<<TOS)/(*NOS));NOS-=2;goto next;
    case iMOV:goto next;
    case iMOVA:goto next;
    case iFILL:goto next;
    case iCMOV:goto next;
    case iCMOVA:goto next;
    case iCFILL:goto next;
	
	case iWORDS:xwords();goto next;
  case iSTACK:xstack();goto next;
	// case iLIST
	// case iEDIT
	// case iFORGET
	// case iCSAVE
	// case iCLOAD
	// case iCNEW
	// case iMEM
	// case iMEMFONT
	// case iMEMCONS
	
	// case iSLEEP
	// case iMSEC
	// case iTIME
	// case iDATE
 
	case iINK:cink(TOS);TOS=*NOS;NOS--;goto next;
	case iPAPER:cpaper(TOS);TOS=*NOS;NOS--;goto next;
	case iCLS:ccls();goto next;
	case iATXY:catxy(TOS,*NOS);NOS--;TOS=*NOS;NOS--;goto next;
	case iEMIT:cemit(TOS);TOS=*NOS;NOS--;goto next;
	case iPRINT:cprint((char*)&memdata[TOS]);TOS=*NOS;NOS--;goto next;
	case iCR:ccr();goto next;
  
	// case KEY
	// case CHAR
	// case XYPEN
	// case BPEN
/*
// FILES
  File dir = SPIFFS.open("/");
  File entry =  dir.openNextFile();
  entry.name()
  entry.size()
  entry.isDirectory()
  entry.close()
  
  file = SPIFFS.open("/test.txt", "w");
    file.println("Hello From ESP32 :-)");
  file.close();
  
  SPIFFS.remove(path)
  
   while(file.available()){
    Serial.write(file.read());
  }
// TIME
time_t now;
char strftime_buf[64];
struct tm timeinfo;

time(&now);
// Set timezone to China Standard Time
setenv("TZ", "CST-8", 1);
tzset();

localtime_r(&now, &timeinfo);
strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);

// MSEC
struct timeval tv_now;
gettimeofday(&tv_now, NULL);
int64_t time_us = (int64_t)tv_now.tv_sec * 1000000L + (int64_t)tv_now.tv_usec;

*/	
    }
}

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
}

void xstack() {
cink(63);
for (int32_t *i=stack+1;i<=ANOS;i++) { cpaper(0);cemit(32);cpaper(52);cprint(dec(*i)); }
if (stack<=ANOS) { cpaper(0);cemit(32);cpaper(52);cprint(dec(ATOS)); }
cpaper(0);ccr();
} 


//------------------ TOKENIZER

int32_t nro; // for parse

// scan for a valid number begin in *p char
// return number in global var "nro"
int isNro(char *p)
{
//if (*p=='&') { p++;nro=*p;return -1;} // codigo ascii
int dig=0,signo=0,base;
if (*p=='-') { p++;signo=1; } else if (*p=='+') p++;
if ((unsigned char)*p<33) return 0;// no es numero
switch(*p) {
  case '$': base=16;p++;break;// hexa
  case '%': base=2;p++;break;// binario
  default:  base=10;break; 
  }; 
nro=0;if ((unsigned char)*p<33) return 0;// no es numero
while ((unsigned char)*p>32) {
  if (base!=10 && *p=='.') dig=0;  
  else if (*p<='9') dig=*p-'0'; 
  else if (*p>='a') dig=*p-'a'+10;  
  else if (*p>='A') dig=*p-'A'+10;  
  else return 0;
  if (dig<0 || dig>=base) return 0;
  nro*=base;nro+=dig;
  p++;
  };
if (signo==1) nro=-nro;  
return -1; 
};

// scan for a valid fixed point number begin in *p char
// return number in global var "nro"
int isNrof(char *p)         // decimal punto fijo 16.16
{
int64_t parte0;
int dig=0,signo=0;
if (*p=='-') { p++;signo=1; } else if (*p=='+') p++;
if ((unsigned char)*p<33) return 0;// no es numero
nro=0;
while ((unsigned char)*p>32) {
  if (*p=='.') { parte0=nro;nro=1;if (*(p+1)<33) return 0; } 
  else  {
  	if (*p<='9') dig=*p-'0'; else dig=-1;
  	if (dig<0 || dig>9) return 0;
  	nro=(nro*10)+dig;
  	}
  p++;
  };  
int decim=1;
int64_t num=nro;
while (num>1) { decim*=10;num/=10; }
num=0x10000*nro/decim;
nro=(num&0xffff)|(parte0<<16);
if (signo==1) nro=-nro;
return -1; 
};

// uppercase a char
inline char toupp(char c) { return c&0xdf; }

// compare two words, until space	
int strequal(char *s1,char *s2)
{
while ((unsigned char)*s1>32) {
	if (toupp(*s2++)!=toupp(*s1++)) return 0;
	}
if (((unsigned char)*s2)>32) return 0;
return -1;
}
	
// advance pointer with space	
char *trim(char *s)	{ while (((unsigned char)*s)<33&&*s!=0) s++;return s; }

// advance to next word
char *nextw(char *s) { while (((unsigned char)*s)>32) s++;return s; }

// advance to next line
char *nextcr(char *s) { while (((unsigned char)*s)>31||*s==9) s++;return s; }

// advance to next string ("), admit "" for insert " in a string, multiline is ok too
char *nextstr(char *s) { s++; while (*s!=0) { if (*s==34) { s++;if (*s!=34) { s++;break; } } s++;}
return s;
}

// ask for a word in the core dicc
int isCore(char *p) { nro=0;
char **m=(char**)wcoredicc;
while (**m!=0) {
  if (strequal(*m,p)) return -1;
  *m++;nro++; }
return 0;  
};

// ask for a word in the dicc, calculate local or exported too
int isWord(char *p) { 
int64_t w=word2code(p);
int i=ndicc;
while (--i>-1) { 
	if (dicc[i].name==w) return i; //&& ((dicc[i].info&1)==1 || i>=dicclocal)) return i;
	}
return -1;
};

void closevar() {
if (ndicc==0) return;
if (!dicc[ndicc-1].mem&0x80000000) return; // prev is var
if ((dicc[ndicc-1].mem&0xfffff)<memd) return;  		// have val
memdata[memd]=0;memd+=4;	
}

void compilaCODE(char *str) {
int ex=0;
closevar();
if (*(str+1)==':') { ex=1;str++; } // exported
//if (*(str+1)<33) { boot=memc; }
dicc[ndicc].name=word2code(str+1);
dicc[ndicc].mem=(ex<<31)|memc;
ndicc++;
modo=1;
}

void compilaDATA(char *str) {
int ex=0;
closevar();
if (*(str+1)=='#') { ex=1;str++; } // exported
dicc[ndicc].name=word2code(str+1);
dicc[ndicc].mem=0x80000000|(ex<<31)|memd;
ndicc++;
modo=2;
}

// compile a token (int)
void codetok(int32_t nro) { memcode[memc++]=nro; }

char *nextcom(char *) { }

// store in datamemory a string
int datasave(char *str) 
{
int r=memd;
for(;*str!=0;str++) { 
	if (*str==34) { str++;if (*str!=34) break; }
	memdata[memd++]=*str;
	}
memdata[memd++]=0;	
return r;
}

// compile a string, in code save the token to retrieve too.
void compilaSTR(char *str) 
{
str++;
int ini=datasave(str);	
if (modo<2) codetok((ini<<8)+iLITs); // lit data
}

// Store in datamemory a number or reserve mem
void datanro(int32_t n) { 
int8_t *p=&memdata[memd];	
switch(modo){
	case 2:*(int*)p=(int)n;memd+=4;break;
	case 3:	for(int i=0;i<n;i++) { *p++=0; };memd+=n;break;
	case 4:*p=(char)n;memd+=1;break;
	}
}


// Compile adress of var
void compilaADDR(int n) 
{
if (modo>1) { 
//	if ((dicc[n].mem&0x80000000)==0) // code
		datanro(dicc[n].mem&0xfffff);
//	else							// data
//		datanro((int32_t)&memdata[dicc[n].mem&0xfffff]);	
	return; 
	}
codetok(((dicc[n].mem&0xfffff)<<8)+iADR);  //1 code 2 data***
}

// Compile literal
void compilaLIT(int32_t n) {
if (modo>1) { datanro(n);return; }
codetok((n<<8)|iLIT1); 
//int32_t v=n;
//if ((token<<8>>8)==n) return;
//token=n>>;
//codetok((token<<8)+LIT2); 
}

// dicc base in data definition
void dataMAC(int n) {
if (n==1) modo=4; // (	bytes
if (n==2) modo=2; // )
if (n==iMUL) modo=3; // * reserva bytes 
}


// Start block code (
void blockIn(void) {
pushA(memc);
level++;
}

// solve conditional void
int solvejmp(int from,int to) {
int whi=false;
for (int i=from;i<to;i++) {
	int op=memcode[i]&0x7f;
	if (op>=ZIF && op<=IFBT && (memcode[i]>>8)==0) { // patch while 
		memcode[i]|=(memc-i)<<8;	// full dir
		whi=true;
		}
	}
return whi;
}

// end block )
void blockOut(void) {
int from=popA();
int dist=memc-from;
if (solvejmp(from,memc)) { // salta
	codetok((-(dist+1)<<8)+iJMPR); 	// jmpr
} else { // patch if	
	memcode[from-1]|=(dist<<8);		// full dir
	}
level--;	
lastblock=memc;
}

// start anonymous definition (adress only word)
void anonIn(void) {
pushA(memc);
codetok(iJMP);
level++;	
}

// end anonymous definition, save adress in stack
void anonOut(void) {
int from=popA();
memcode[from]|=(memc<<8);  // patch jmp
codetok((from+1)<<8|iLIT1);
level--;  
}

// compile word from core diccionary
void compilaCORE(int n) {
if (modo>1) { dataMAC(n);return; }
if (n==0) { 					// ;
	if (level==0) modo=0; 
//	if ((memcode[memc-1]&0xff)==iCALL && lastblock!=memc) { // avoid jmp to block
//		memcode[memc-1]=(memcode[memc-1]^iCALL)|iJMP; // call->jmp avoid ret
//		return;
//		}
	}
if (n==1) { blockIn();return; }		//(	etiqueta
if (n==2) { blockOut();return; }	//)	salto
if (n==3) { anonIn();return; }		//[	salto:etiqueta
if (n==4) { anonOut();return; }		//]	etiqueta;push
//int token=memcode[memc-1];

codetok(n+INTERNALWORDS);		
}

// compile WORD,or VAR
void compilaWORD(int n) {
if (modo>1) { datanro(n);return; }
codetok(((dicc[n].mem&0xfffff)<<8)+iCALL+((dicc[n].mem>>31)&1));
}

int cerror;

// tokeniza string
int r3token(char *str) {
char *istr=str;
level=0; //

while(*str!=0) {
	str=trim(str);if (*str==0) return 0;
	switch (*str) {
		case '^':str=nextcr(str);break;		// include
		case '|':str=nextcom(str);break;	// comments	
		case '"':compilaSTR(str);str=nextstr(str);break;	// strings		
		case ':':compilaCODE(str);str=nextw(str);break;		// $3a :  Definicion	// :CODE
		case '#':compilaDATA(str);str=nextw(str);break;		// $23 #  Variable	// #DATA
		case 0x27:	// $27 ' Direccion	// 'ADR
			nro=isWord(str+1);
			if (nro<0) { 
				if (isCore(str)) // 'ink allow compile replace
					{ compilaCORE(nro);str=nextw(str);break; }
				cerror=str-istr;return 2; 
				}
			compilaADDR(nro);str=nextw(str);break;		
		default:
			if (isNro(str)||isNrof(str)) 
				{ compilaLIT(nro);str=nextw(str);break; }
			if (isCore(str)) 
				{ compilaCORE(nro);str=nextw(str);break; }
			nro=isWord(str);
			if (nro<0) { cerror=str-istr;return 1; }
			if (modo<2) 
				compilaWORD(nro); 
			else 
				compilaADDR(nro);
			str=nextw(str);break;
		}
	}
return 0;
}

void dump() {
cprint("DUMP");ccr();
for(int i=0;i<memc;i++) { cprint(hex(memcode[i]));cprint(" "); }
ccr();
for(int i=0;i<memd;i++) { cprint(hex(memdata[i]));cprint(" "); }
ccr();
}

void dumpd() {
cprint("DICC");ccr();	
for(int i=0;i<ndicc;i++) {
	cprint(code2word(dicc[i].name));
	cprint(" ");
	cprint(hex(dicc[i].mem));
	ccr();
	}
}

const char *prompt[]={">",":>","#>","#>","#>"};

void cprompt() {
ccr();cprint((char*)prompt[modo]);  
cposinp=cpos;
inputpad[0]=0;cmax=255;ccur=0;cfin=0;
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

void interpreter() {
int32_t andicc=ndicc,amemd=memd,amemc=memc;

ccr();
if (inputpad[0]==0) { modo=0;cprompt();return; }

int error=r3token(inputpad);

if (modo==0&&andicc==ndicc&&error==0) { // only imm, not new definitions 
  codetok(iEND);
  runr3(amemc);
  memd=amemd;memc=amemc; 
  }

//printstack();ccr();
//dump();ccr();
//dumpd();ccr();
if (error==0) { cink(12);cpaper(0); } else { cink(63);cpaper(3); }
cprint((char*)msg[error]);
cink(63);cpaper(0);
if (error==0) { cprompt(); } else { cedit(); }
}

//------------------- MAIN
void setup()
{
//Serial.begin(115200);Serial.write("ESP32 Forth Computer\n");

if (!SPIFFS.begin()){ 
//	Serial.write("SPIFFS error!\n");
	return; }

mainTaskHandle = xTaskGetCurrentTaskHandle();
PS2Controller.begin(PS2Preset::KeyboardPort0); //PS2Preset::KeyboardPort0_MousePort1);

auto keyboard = PS2Controller.keyboard();
//keyboard->suspendVirtualKeyGeneration(false);

//        keyboard->setLayout(&fabgl::USLayout);
//        keyboard->setLayout(&fabgl::UKLayout);
//        keyboard->setLayout(&fabgl::GermanLayout);
//        keyboard->setLayout(&fabgl::ItalianLayout);

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
	DisplayController.setResolution(VGA_640x480_60HzD); // 80x60 << better view
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
}

void loop()
{
cpad();

ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for vertical sync
}
