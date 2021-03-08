// ESP32 forth computer
// PHREDA 2021
// * Use FabGL Library with 1 change (http://www.fabglib.org)W

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

void cprint(char *s) {
byte c;
while ((c=*s++)!=0) { *cpos++=cattrib|c;if (cpos>=&screen[SCREEN_SIZE]) { cscroll();cpos-=40; } }
}

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
};

enum {
iLIT1,iLIT2,iLITs,iCOM,iJMPR,iJMP,iCALL,iADR,iVAR, // no 
 
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
iMOV,iMOVA,iFILL,iCMOV,iCMOVA,iCFILL
};

#define STACKSIZE 512
#define DICCMEM 0x3ff
#define CODEMEM 0xfff
#define DATAMEM 0xffff

int32_t stack[STACKSIZE];

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

uint32_t boot;

void r3init() {
ndicc=memc=memd=0;
}

int char6bit(int c) { return ((((c-0x1f)&0x40)>>1)|(c-0x1f))&0x3f;}

int64_t word2code(char *str) {
int c;
int64_t v=0;
while ((c=*str++)>32) { v=(v<<6)|char6bit(c); }
return v;
}


void runr3(int32_t boot) {
stack[STACKSIZE-1]=0;  
int32_t TOS=0;
int32_t *NOS=&stack[0];
int32_t *RTOS=&stack[STACKSIZE-1];
int32_t REGA=0;
int32_t REGB=0;
int32_t op=0;
int32_t ip=boot;
while(1) { 
  op=memcode[ip++]; //  printcode(op);
  switch(op&0x7f){
    case iLIT1:NOS++;*NOS=TOS;TOS=op>>7;continue;
    case iLIT2:NOS++;*NOS=TOS;TOS=op&0xffffff80;continue;
    case iLITs:NOS++;*NOS=TOS;TOS=op>>7;continue;
    case iCOM:continue;
    case iJMPR:ip+=op>>7;continue;
    case iJMP:ip=op>>7;continue;
    case iCALL:RTOS--;*RTOS=ip;ip=op>>7;continue;
    case iADR:NOS++;*NOS=TOS;TOS=op>>7;continue;
    case iVAR:NOS++;*NOS=TOS;TOS=memdata[op>>7];continue;
    case iEND:ip=*RTOS;RTOS++;if (ip==0) { return; } continue;  
    case OPEB:continue;
    case CLOB:ip+=op>>7;continue;
    case OPEA:ip+=op>>7;continue;
    case CLOA:NOS++;*NOS=TOS;TOS=op>>7;continue;
    case iEX:RTOS--;*RTOS=ip;ip=TOS;TOS=*NOS;NOS--;continue;  
    case ZIF:if (TOS!=0) {ip+=op>>7;} continue;
    case UIF:if (TOS==0) {ip+=op>>7;} continue;
    case PIF:if (TOS<0) {ip+=op>>7;} continue;
    case NIF:if (TOS>=0) {ip+=op>>7;} continue;
	case IFL:if (TOS<=*NOS) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFL
	case IFG:if (TOS>=*NOS) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFG
	case IFE:if (TOS!=*NOS) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFN	
	case IFGE:if (TOS>*NOS) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFGE
	case IFLE:if (TOS<*NOS) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFLE
	case IFNE:if (TOS==*NOS) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFNO
	case IFAND:if (!(TOS&*NOS)) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFNA
	case IFNAND:if (TOS&*NOS) {ip+=(op>>7);} TOS=*NOS;NOS--;continue;//IFAN
	case IFBT:if (*(NOS-1)>TOS||*(NOS-1)<*NOS) {ip+=(op>>7);} TOS=*(NOS-1);NOS-=2;continue;//BTW 
    case iDUP:NOS++;*NOS=TOS;continue;
    case iDROP:TOS=*NOS;NOS--;continue;
    case iOVER:NOS++;*NOS=TOS;TOS=*(NOS-1);continue;
    case iPICK2:NOS++;*NOS=TOS;TOS=*(NOS-2);continue;
    case iPICK3:NOS++;*NOS=TOS;TOS=*(NOS-3);continue;
    case iPICK4:NOS++;*NOS=TOS;TOS=*(NOS-4);continue;
    case iSWAP:op=*NOS;*NOS=TOS;TOS=op;continue;
    case iNIP:NOS--;continue;
    case iROT:op=TOS;TOS=*(NOS-1);*(NOS-1)=*NOS;*NOS=op;continue;
    case i2DUP:op=*NOS;NOS++;*NOS=TOS;NOS++;*NOS=op;continue;
    case i2DROP:NOS--;TOS=*NOS;NOS--;continue;
    case i3DROP:NOS-=2;TOS=*NOS;NOS--;continue;
    case i4DROP:NOS-=3;TOS=*NOS;NOS--;continue;
    case i2OVER:NOS++;*NOS=TOS;TOS=*(NOS-3);NOS++;*NOS=TOS;TOS=*(NOS-3);continue;
    case i2SWAP:op=*NOS;*NOS=*(NOS-2);*(NOS-2)=op;op=TOS;TOS=*(NOS-1);*(NOS-1)=op;continue;  
    case iFE:TOS=*(int*)&memdata[TOS];continue;
    case iCFE:TOS=*(char*)&memdata[TOS];continue;
    case iFEP:NOS++;*NOS=TOS+4;TOS=*(int*)&memdata[TOS];continue;
    case iCFEP:NOS++;*NOS=TOS+1;TOS=*(char*)&memdata[TOS];continue;
    case iST:*(int*)&memdata[TOS]=(int)*NOS;NOS--;TOS=*NOS;NOS--;continue;
    case iCST:memdata[TOS]=(char)*NOS;NOS--;TOS=*NOS;NOS--;continue;
    case iSTP:*(int*)&memdata[TOS]=*NOS;NOS--;TOS+=4;continue;
    case iCSTP:memdata[TOS]=(char)*NOS;NOS--;TOS+=1;continue;
    case iPST:*(int*)&memdata[TOS]+=*NOS;NOS--;TOS=*NOS;NOS--;continue;
    case iCPST:memdata[TOS]+=*NOS;NOS--;TOS=*NOS;NOS--;continue;
    case iTA:REGA=TOS;TOS=*NOS;NOS--;continue;
    case iAT:NOS++;*NOS=TOS;TOS=REGA;continue;
    case iAFE:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGA];continue;
    case iAST:*(int32_t*)&memdata[REGA]=TOS;TOS=*NOS;NOS--;continue;
    case iAP:REGA+=TOS;TOS=*NOS;NOS--;continue;
    case iAFEP:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGA];REGA+=4;continue;
    case iASTP:*(int32_t*)&memdata[REGA]=TOS;TOS=*NOS;NOS--;REGA+=4;continue;
    case iTB:REGB=TOS;TOS=*NOS;NOS--;continue;
    case iBT:NOS++;*NOS=TOS;TOS=REGB;continue;
    case iBFE:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGB];continue;
    case iBST:*(int32_t*)&memdata[REGB]=TOS;TOS=*NOS;NOS--;continue;
    case iBP:REGB+=TOS;TOS=*NOS;NOS--;continue;
    case iBFEP:NOS++;*NOS=TOS;TOS=*(int32_t*)&memdata[REGB];REGB+=4;continue;
    case iBSTP:*(int32_t*)&memdata[REGB]=TOS;TOS=*NOS;NOS--;REGB+=4;continue;
    case iNOT:TOS=~TOS;continue;
    case iNEG:TOS=-TOS;continue;
    case iABS:op=(TOS>>63);TOS=(TOS+op)^op;continue;
    case iSQRT:TOS=(int)sqrt(TOS);continue;
    case iCLZ:TOS=__builtin_clz(TOS);continue;
    case iAND:TOS&=*NOS;NOS--;continue;
    case iOR:TOS|=*NOS;NOS--;continue;
    case iXOR:TOS^=*NOS;NOS--;continue;
    case iADD:TOS=*NOS+TOS;NOS--;continue;
    case iSUB:TOS=*NOS-TOS;NOS--;continue;
    case iMUL:TOS=*NOS*TOS;NOS--;continue;
    case iDIV:TOS=(*NOS/TOS);NOS--;continue;
    case iMOD:TOS=*NOS%TOS;NOS--;continue;
    case iSHL:TOS=*NOS<<TOS;NOS--;continue;
    case iSAR:TOS=*NOS>>TOS;NOS--;continue;
    case iSHR:TOS=((uint32_t)*NOS)>>TOS;NOS--;continue;
    case iDMOD:op=*NOS;*NOS=op/TOS;TOS=op%TOS;continue;
    case iMULDIV:TOS=((int64_t)(*(NOS-1))*(*NOS)/TOS);NOS-=2;continue;
    case iMULSH:TOS=((int64_t)(*(NOS-1)*(*NOS))>>TOS);NOS-=2;continue;
    case iSHDIV:TOS=(int64_t)((*(NOS-1)<<TOS)/(*NOS));NOS-=2;continue;
    case iMOV:continue;
    case iMOVA:continue;
    case iFILL:continue;
    case iCMOV:continue;
    case iCMOVA:continue;
    case iCFILL:continue;

    }
  }
}

//------------------ TOKENIZER

int32_t nro; // for parse
int level;
int modo;

int cntstacka;
int stacka[32];
int lastblock;

void iniA(void) { cntstacka=0; }
void pushA(int n) { stacka[cntstacka++]=n; }
int popA(void) { return stacka[--cntstacka]; }


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
char toupp(char c) { return c&0xdf; }

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
  /*
int i=cntdicc;
while (--i>-1) { 
	if (strequal(dicc[i].nombre,p) && ((dicc[i].info&1)==1 || i>=dicclocal)) return i;
	}
 */
return -1;
};

void closevar() {
if (ndicc==0) return;
if (!dicc[ndicc-1].mem&0x80000000) return; // prev is var
if ((dicc[ndicc-1].mem&0xfffff)<memd) return;  		// have val
memdata[memd]=0;memd+=4;	
}

void compilaCODE(char *str) {
	cprint("CODE ");ccr();
int ex=0;
closevar();
if (*(str+1)==':') { ex=1;str++; } // exported
if (*(str+1)<33) { boot=memc; }
dicc[ndicc].name=word2code(str);
dicc[ndicc].mem=(ex<<31)|memc;
ndicc++;
modo=1;
}

void compilaDATA(char *str) {
	cprint("DATA ");ccr();
int ex=0;
closevar();
if (*(str+1)=='#') { ex=1;str++; } // exported
dicc[ndicc].name=word2code(str);
dicc[ndicc].mem=0x80000000|(ex<<31)|memd;
ndicc++;
modo=2;
}

// compile a token (int)
void codetok(int32_t nro) { memcode[memc++]=nro; }

char *nextcom(char *) { }
void compilaSTR(char*) {
	cprint("STR ");
}

void compilaADDR(int) {
	cprint("ADR ");
}

void datanro(int32_t n) { 
int8_t *p=&memdata[memd];	
switch(modo){
	case 2:*(int*)p=(int)n;memd+=4;break;
	case 3:	for(int i=0;i<n;i++) { *p++=0; };memd+=n;break;
	case 4:*p=(char)n;memd+=1;break;
	}
}

void compilaLIT(int32_t n) {
	//cprint("LIT ");cprint(dec(n));
if (modo>1) { datanro(n);return; }
codetok((n<<7)|iLIT1); 
//int32_t v=n;
//if ((token<<7>>7)==n) return;
//token=n>>;
//codetok((token<<8)+LIT2); 
}

// dicc base in data definition
void dataMAC(int n)
{
if (n==1) modo=4; // (	bytes
if (n==2) modo=2; // )
if (n==iMUL) modo=3; // * reserva bytes 
}


// Start block code (
void blockIn(void)
{
pushA(memc);
level++;
}

// solve conditional void
int solvejmp(int from,int to) 
{
int whi=false;
for (int i=from;i<to;i++) {
	int op=memcode[i]&0x7f;
	if (op>=ZIF && op<=IFBT && (memcode[i]>>7)==0) { // patch while 
		memcode[i]|=(memc-i)<<7;	// full dir
		whi=true;
		}
	}
return whi;
}

// end block )
void blockOut(void)
{
int from=popA();
int dist=memc-from;
if (solvejmp(from,memc)) { // salta
	codetok((-(dist+1)<<7)+iJMPR); 	// jmpr
} else { // patch if	
	memcode[from-1]|=(dist<<7);		// full dir
	}
level--;	
lastblock=memc;
}

// start anonymous definition (adress only word)
void anonIn(void)
{
pushA(memc);
codetok(iJMP);	
level++;	
}

// end anonymous definition, save adress in stack
void anonOut(void)
{
int from=popA();
memcode[from]|=(memc<<7);  // patch jmp
codetok((from+1)<<7|iLIT1);
level--;  
}

// compile word from core diccionary
void compilaCORE(int n) {
	//cprint("CORE ");
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

codetok(n+9);		
}
void compilaWORD(int) {
	cprint("WORDS ");
}

void seterror(char *,char *) {}

// tokeniza string
int r3token(char *str) {
level=0;
while(*str!=0) {
	str=trim(str);if (*str==0) return -1;
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
				seterror(str,"adr not found");return 0; 
				}
			compilaADDR(nro);str=nextw(str);break;		
		default:
			if (isNro(str)||isNrof(str)) 
				{ compilaLIT(nro);str=nextw(str);break; }
			if (isCore(str)) 
				{ compilaCORE(nro);str=nextw(str);break; }
			nro=isWord(str);
			//if (nro<0) { seterror(str,"word not found");return 0; }
			if (modo==1) 
				compilaWORD(nro); 
			else 
				compilaADDR(nro);
			str=nextw(str);break;
		}
	}
return -1;
}


void dump() {
cprint("DUMP");ccr();
for(int i=0;i<memc;i++) {
	cprint(hex(memcode[i]));cprint(" ");
	}
ccr();
for(int i=0;i<memd;i++) {
	cprint(hex(memdata[i]));cprint(" ");
	}
ccr();
}

void interpreter() {
ccr();cprint("Parsing...");ccr();
int error=r3token(inputpad);


ccr();dump();ccr();
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
cink(12);cprint("Computer ");
cink(63);ccr();

cprompt();
}

void loop()
{
cpad();

ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for vertical sync
}