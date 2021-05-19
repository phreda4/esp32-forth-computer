// rESP32 r3 for esp32
// PHREDA 2021

//#include <WiFi.h>

#include <SPIFFS.h>

#include "r3.h"

//const char* ssid = "Adelaida";
//WiFiServer server(80);

TaskHandle_t  mainTaskHandle;

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

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

//////////////////////////////////////////
const char *wsysdicc[]={
"WORDS",".S","LIST","EDIT","DUMP",
"CD","DIR","CLOAD","CSAVE","CNEW",
"WIFI","CAT",
};

void exSis(int n,char *str) {
switch(n) {
case 0:xwords();break;
case 1:xstack();break;
case 2:xlist(str);break;
case 3:xedit(str);break;
case 4:dump();break;
case 5:xcd(str);break;
case 6:xdir();break;
case 7:xcload(str);break;
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
    Serial.write(lastdir+"\n");
    }
  return 0;
  } // not in path
return 1;
}

void xdir() {
fastmode();
File root = SPIFFS.open("/");
File file = root.openNextFile();
Serial.write(nowpath);
char *fn;
int di;
cpyldir((char *)file.name());
while(file){
	fn=(char*)file.name();
	di=indir(fn);
	if (di>0) {
		fn=filenameo(fn);
		Serial.write(fn+" "+dec(file.size())+"b\n");
		}
	file = root.openNextFile();
	}
file.close();
root.close();
slowmode();
//Serial.write(nowpath+"\n");
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
while (wcoredicc[i][0]!=0) {
  Serial.write((char*)wcoredicc[i]);cemit(32);
  i++;}
for (i=0;i<ndicc;i++) {
  //if (dicc[i].mem&0x80000000) cink(51); else cink(3);  
  Serial.write(code2word(dicc[i].name)+" ");
  }

modo=-1;
}

void xstack() {
for (int32_t *i=stack+1;i<=ANOS;i++) { cpaper(0);cemit(32);cpaper(52);Serial.write(dec(*i)); }
if (stack<=ANOS) { cpaper(0);cemit(32);cpaper(52);Serial.write(dec(ATOS)); }
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
		Serial.write("error line "+dec(lin)+"\n");
		Serial.write(inputpad+"\n");
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
char *fno=filename(fn);
strcpy(fname,fno);
// search includes, only one file
File entry = SPIFFS.open(fno,"r");
while(entry.available()){
  len=entry.readBytesUntil('\n',inputpad,CMAX);
  inputpad[len]=0;
  r3includes(inputpad);
  }
entry.close();

r3init();
for(int i=0;i<cincludes;i++) {
  fno=filename(includes[i]);
  Serial.write(fno+"\n");
  cload(fno);
	}
Serial.write(fname+"\n");
cload(fname);

slowmode();
}

void xcat(char *fn) {
fastmode();
int len;
char *fno=filename(fn);
if (SPIFFS.exists(fno)) {
  File entry = SPIFFS.open(fno,"r");
  while(entry.available()){
  len=entry.readBytesUntil('\n',inputpad,CMAX);
  inputpad[len]=0;lin++;
  Serial.write(inputpad+"\n");
    }
  entry.close();
  }
slowmode();
}
//--------------------------
void xcsave(char *fn) {
fastmode();

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
/*
Serial.write("Connecting to ");
Serial.write((char*)ssid);
  WiFi.begin(ssid, "33134253");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.write(".");
  }
  // Print local IP address and start web server

Serial.write("WiFi connected.");
Serial.write("IP address: ");
Serial.write((char*)WiFi.localIP().toString().c_str());
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
	Serial.write((char*)wcoredicc[(op&0xff)-INTERNALWORDS]);
} else {
	switch (op&0xff) {
    case iLITd:Serial.write(dec(getval(op,nop)));break;
    case iLITh:Serial.write("$");Serial.write(hex(getval(op,nop)));break;
    case iLITb:Serial.write("%");Serial.write(bin(getval(op,nop)));break;
    case iLITf:Serial.write(fix(getval(op,nop)));break;	
    case iLIT2:break;
    case iLITs:Serial.write("'");emitstr((char*)&memdata[op>>8]);Serial.write("'");break;
    case iCOM:Serial.write("|");emitcr((char*)&memdata[op>>8]+"\n");break;
    case iJMPR:Serial.write(dec(op>>8));break;
    case iJMP:Serial.write(code2word(dicc[a2dicc(op>>8,0)].name));cink(7);Serial.write(" ;");break;
    case iCALL:Serial.write(code2word(dicc[a2dicc(op>>8,0)].name));break;
    case iVAR:Serial.write(code2word(dicc[a2dicc(op>>8,0x80000000)].name));break;
    case iADR:Serial.write("'");Serial.write(code2word(dicc[a2dicc(op>>8,0)].name));break;
    case iADRv:Serial.write("'");Serial.write(code2word(dicc[a2dicc(op>>8,0x80000000)].name));break;
		}
	}
Serial.write(" ");	
}


void printword(int w) {
Serial.write(":"); 
Serial.write(code2word(dicc[w].name));cemit(32);
int it=dicc[w].mem&0xfffff;
for (int i=0;i<(dicc[w].mem>>20)&0x3ff;i++) {
  tokenprint(memcode[it++],memcode[it]); }

}

void printvar(int w) {
Serial.write("#"); 
Serial.write(code2word(dicc[w].name));cemit(32);
int *it=(int*)&memdata[dicc[w].mem&0xfffff];
for (int i=((dicc[w].mem>>20)&0x3ff)>>2;i>0;i--) {
  cemit(32);Serial.write(dec(*it++)); }

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
for (int i=0;i<ndicc;i++) {
	if (dicc[i].mem&0x80000000) printvar(i); else printword(i);
	}
}


void xedit(char *str) {
}

void dump() {
int d=ATOS;
unsigned char b;
char *mem=(char*)&memdata[ATOS];
for (int j=0;j<16;j++) {
  Serial.write(hex(d));Serial.write(":");
  for(int i=0;i<16;i++) { 
    b=(unsigned char)*mem;
    if (b<16) { Serial.write("0"); } 
    Serial.write(hex(b));
    mem++;d++; }
  
  }
}

const char *prompt[]={">",":>","#>","#>","#>"};

void cprompt() {
Serial.write((char*)prompt[modo]);  
cposinp=cpos;
inputpad[0]=0;ccur=0;cfin=0;
//cpos=cposinp;
//Serial.write(inputpad);
//Serial.write(" ");
cpos=cposinp+ccur;
ccursorb=*cpos;*(cpos)|=0x800000; // blink in cursor
}

void cedit() {
ccur=cerror;
Serial.write((char*)prompt[modo]);  
cposinp=cpos;
Serial.write(inputpad);
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
	Serial.write((char*)msg[error]);
	}
if (error==0) { cprompt(); } else { cedit(); }
}

void interpreter() {

if (inputpad[0]==0) { modo=0;cprompt();return; }

if (modo!=0) {	// in definition
	error=r3token(inputpad);
	promt();
	return;
	}

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

void inter() {
while (1) {
  cpad();
  }
}

void setup() {
Serial.begin(115200);
Serial.write("rESP32\n");

SPIFFS.begin(true);

mainTaskHandle = xTaskGetCurrentTaskHandle();

r3init();

cprompt();
strcpy(nowpath,"/");
cpath=1;
inter();
}

void loop() { }
