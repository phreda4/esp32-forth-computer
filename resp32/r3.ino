// r32 tokenizer and vm
// PHREDA 2021
//------------------- INTERPRETER

void r3init() {
ndicc=0;
memc=memd=0;
modo=0;
resetr3();
}

void resetr3() {
ATOS=0;
ANOS=&stack[-1];
level=0; 
}

// DATE TIME
time_t now;
struct tm timeinfo;

void runr3(int32_t boot) {
stack[STACKSIZE-1]=0;  
register int32_t TOS=ATOS;
register int32_t *NOS=ANOS;
register int32_t *RTOS=&stack[STACKSIZE-1];
register int32_t REGA=0;
register int32_t REGB=0;
register int32_t op=0;
register int32_t ip=boot;
int W;

next:
  op=memcode[ip++]; //  printcode(op);
  switch(op&0x7f){
    case iLITd:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iLITh:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iLITb:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iLITf:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iLIT2:TOS^=(op<<16)&0xff000000;goto next;
    case iLITs:NOS++;*NOS=TOS;TOS=op>>8;goto next;
    case iCOM:goto next;
    case iJMPR:ip+=op>>8;goto next;
    case iJMP:ip=op>>8;goto next;
	case iCALL:RTOS--;*RTOS=ip;ip=op>>8;goto next;
	case iVAR:NOS++;*NOS=TOS;TOS=*(int*)&memdata[op>>8];goto next;
	case iADR:NOS++;*NOS=TOS;TOS=(op>>8);goto next;
	case iADRv:NOS++;*NOS=TOS;TOS=(op>>8);goto next;	
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
    case iMOV:
		W=(int)&memdata[*(NOS-1)];op=(int)&memdata[*NOS];
		while (TOS--) { *(int*)W=*(int*)op;W+=4;op+=4; }
		NOS-=2;TOS=*NOS;NOS--;goto next;
    case iMOVA:
		W=(int)&memdata[*(NOS-1)+(TOS<<2)];op=(int)&memdata[(*NOS)+(TOS<<2)];
		while (TOS--) { W-=4;op-=4;*(int*)W=*(int*)op; }  
		NOS-=2;TOS=*NOS;NOS--;goto next;
    case iFILL:
		W=(int)&memdata[*(NOS-1)];op=*NOS;while (TOS--) { *(int*)W=op;W+=4; }	
		NOS-=2;TOS=*NOS;NOS--;goto next;
    case iCMOV:
		W=(int)&memdata[*(NOS-1)];op=(int)&memdata[*NOS];
		while (TOS--) { *(char*)W=*(char*)op;W++;op++; }
		NOS-=2;TOS=*NOS;NOS--;goto next;
    case iCMOVA:
		W=(int)&memdata[*(NOS-1)+TOS];op=(int)&memdata[*NOS+TOS];
		while (TOS--) { W--;op--;*(char*)W=*(char*)op; }
		NOS-=2;TOS=*NOS;NOS--;goto next;
    case iCFILL:
		W=(int)&memdata[*(NOS-1)];op=*NOS;while (TOS--) { *(char*)W=op;W++; }
		NOS-=2;TOS=*NOS;NOS--;goto next;
    case iREDRAW:redraw();goto next;
    
	case iINK:cink(TOS);TOS=*NOS;NOS--;goto next;
	case iPAPER:cpaper(TOS);TOS=*NOS;NOS--;goto next;
	case iCLS:ccls();goto next;
	case iATXY:catxy(*NOS,TOS);NOS--;TOS=*NOS;NOS--;goto next;
	case iEMIT:cemit(TOS);TOS=*NOS;NOS--;goto next;
	case iPRINT:cprint((char*)&memdata[TOS]);TOS=*NOS;NOS--;goto next;
	case iCR:ccr();goto next;

	case iKEY:NOS++;*NOS=TOS;TOS=getkey();goto next;
	case iMEM:NOS++;*NOS=TOS;TOS=memd;goto next;
	case iMEMSCR:NOS++;*NOS=TOS;TOS=(char*)screen-(char*)memdata;goto next; 
	case iMEMFNT:NOS++;*NOS=TOS;TOS=(char*)romfont-(char*)memdata;goto next;
	
	// case iSLEEP
	case iMSEC:
		NOS++;*NOS=TOS;TOS=millis();goto next;
	case iTIME://"TIME"
		time(&now);localtime_r(&now, &timeinfo);
		NOS++;*NOS=TOS;TOS=(timeinfo.tm_hour<<16)|(timeinfo.tm_min<<8)|timeinfo.tm_sec;goto next;
	case iIDATE://"DATE"
		time(&now);localtime_r(&now, &timeinfo);
		NOS++;*NOS=TOS;TOS=(timeinfo.tm_year+1900)<<16|(timeinfo.tm_mon+1)<<8|timeinfo.tm_mday;goto next;

  // case XYPEN
  // case BPEN
  
// m "filename" -- lm
	case iLOAD: TOS=iload((char*)&memdata[TOS],*NOS);NOS--;goto next;
// m cnt "filename" --	
	case iSAVE: isave((char*)&memdata[TOS],*(NOS-1),*NOS);NOS-=2;TOS=*NOS;NOS--;goto next; 
	case iAPPEND: iappend((char*)&memdata[TOS],*NOS,*(NOS-1));NOS-=2;TOS=*NOS;NOS--;goto next;

  case iRUN: xcload((char*)&memdata[TOS]);
          resetr3();
          stack[STACKSIZE-1]=0;  
          TOS=ATOS;NOS=ANOS;
          RTOS=&stack[STACKSIZE-1];
          REGA=REGB=0;
          op=0;          
          ip=dicc[ndicc-1].mem&0xfffff;
          goto next;
    }
}

void fastmode(){ esp_intr_disable(DisplayController.m_isr_handle); }
void slowmode(){ esp_intr_enable(DisplayController.m_isr_handle); }

int iload(char *fn,int m) {
int len;
char *pmem=(char*)&memdata[m];
fastmode();
if (SPIFFS.exists(fn)) {
  File entry=SPIFFS.open(fn,"r");
  while(entry.available()){ len=entry.readBytes(pmem,512);pmem+=len; }
  entry.close();
  }
slowmode();
return (int)pmem-(int)memdata;
}

void isave(char *fn,int m,int c) {
fastmode();
unsigned char *pmem=(unsigned char*)&memdata[m];
File entry=SPIFFS.open(fn,"w");
entry.write(pmem,c);
entry.close();
slowmode();
}

void iappend(char *fn,int m,int c){
fastmode();
unsigned char *pmem=(unsigned char*)&memdata[m];
File entry = SPIFFS.open(fn,"a");
entry.write(pmem,c);
entry.close();
slowmode();
}

//------------------ TOKENIZER

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
vname=&0xfffffffffffffff;
wname[15]=0;
int i=14;
while (vname!=0) {
  wname[i]=bit6char((int)vname);
  i--;vname>>=6;
  }
return (char*)&wname[i+1];
}

int32_t nro; // for parse
int32_t base; // for parse

// scan for a valid number begin in *p char
// return number in global var "nro"
int isNro(char *p)
{
//if (*p=='&') { p++;nro=*p;return -1;} // codigo ascii
int dig=0,signo=0;
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
base=32;
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


// ask for a word in the sys dicc
int isSysWord(char *p) { nro=0;
char **m=(char**)wsysdicc;
while (**m!=0) {
  if (strequal(*m,p)) return nro;
  *m++;nro++; }
return -1;  
};

void closevar() {
if (ndicc==0) return;
if (!(dicc[ndicc-1].mem&0x80000000)) return; // prev is var
if ((dicc[ndicc-1].mem&0xfffff)<memd) return;  		// have val
if (((dicc[ndicc-1].mem>>20)&0x3f)>0) return;
memdata[memd]=0;memd+=4;	
dicc[ndicc-1].mem+=0x400000;
}

void compilaCODE(char *str) {
int ex=0;
closevar();
if (*(str+1)==':') { ex=1;str++; } // exported
//if (*(str+1)<33) { boot=memc; }
dicc[ndicc].name=word2code(str+1);
dicc[ndicc].mem=(ex<<30)|memc;
ndicc++;
modo=1;
}

void compilaDATA(char *str) {
int ex=0;
closevar();
if (*(str+1)=='#') { ex=1;str++; } // exported
dicc[ndicc].name=word2code(str+1);
dicc[ndicc].mem=0x80000000|(ex<<30)|memd;
ndicc++;
modo=2;
}

// compile a token (int)
void codetok(int32_t nro) { memcode[memc++]=nro; }

// store in datamemory a comment
int comsave(char *str) 
{
int r=memd;
for(;*str!=0;str++) { 
	if (*str==13) { break; }
	memdata[memd++]=*str;
	}
memdata[memd++]=0;	
return r;
}

// skip coment
char *nextcom(char *str) { 
//str++;
//int ini=comsave(str);	
//codetok((ini<<8)+iCOM); // not save
return nextcr(str);  
}

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
	case 2:*(int*)p=(int)n;memd+=4;dicc[ndicc-1].mem+=0x400000;break;
	case 3:	for(int i=0;i<n;i++) { *p++=0; };memd+=n;dicc[ndicc-1].mem+=n<<20;break;
	case 4:*p=(char)n;memd+=1;dicc[ndicc-1].mem+=0x100000;break;
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
codetok(((dicc[n].mem&0xfffff)<<8)+iADR+((dicc[n].mem>>31)&1));
}

// Compile literal
void compilaLIT(int32_t n) {
if (modo>1) { datanro(n);return; }
if (base==10) {	base=iLITd;
} else if (base==16) { base=iLITh;
} else if (base==2) { base=iLITb;
} else if (base==32) { 	base=iLITf; }
codetok((n<<8)|base); 
int32_t v=n<<8>>8;
if (v==n) return;
v=((v^n)>>16)&0xff00;
codetok(v|iLIT2); 
}

// dicc base in data definition
void dataMAC(int n) {
if (n==1) modo=4; // (	bytes
if (n==2) modo=2; // )
if (n==iMUL) modo=3; // * reserva bytes 
}


// Start block code (
void blockIn(void) {
codetok(OPEB);
pushA(memc);
level++;
}

// solve conditional void
int solvejmp(int from,int to) {
int whi=false;
for (int i=from;i<to;i++) {
	int op=memcode[i]&0xff;
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
	codetok((-(dist+1)<<8)+CLOB); 	// )
} else { // patch if	
	memcode[from-2]|=(dist+2)<<8;		// full dir
  codetok(CLOB);   // )
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
codetok((from+1)<<8|iLITd);
level--;  
}

// compile word from core diccionary
void compilaCORE(int n) {
if (modo>1) { dataMAC(n+INTERNALWORDS);return; }
if (n==0) { 					// ;
	if (level==0) modo=0; 
	if ((memcode[memc-1]&0xff)==iCALL && lastblock!=memc) { // avoid jmp to block
		memcode[memc-1]=(memcode[memc-1]^iCALL)|iJMP; // call->jmp avoid ret
		dicc[ndicc-1].mem|=((memc-(dicc[ndicc-1].mem&0xfffff)))<<20;
		return;
		}
	dicc[ndicc-1].mem|=((memc-(dicc[ndicc-1].mem&0xfffff))+1)<<20; // length in tokens		
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

// tokeniza string
int r3token(char *str) {
char *istr=str;	// first position for calculate place of error
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
			if (nro>=0) {
				if (modo<2) compilaWORD(nro); else compilaADDR(nro);
				str=nextw(str);break;
				}
			nro=isSysWord(str);
			if (nro>=0) { 
			  str=trim(nextw(str));exSis(nro,str);return 0; }
			cerror=str-istr;
			return 1;
		}
	}
return 0;
}
