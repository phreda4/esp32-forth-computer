//
// PHREDA 2021
//
#ifndef R3_H
#define R3_H

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

"REDRAW","INK","PAPER","CLS","ATXY","EMIT","PRINT","CR",
"KEY","MEM","MEMSCR","MEMFNT",
"MSEC","TIME","DATE",
"LOAD","SAVE","APPEND",

""
};

#define INTERNALWORDS 13

enum {
iLITd,iLITh,iLITb,iLITf,iLIT2,iLITs,iCOM,iJMPR,iJMP,iCALL,iVAR,iADR,iADRv, // internal
 
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

iREDRAW,iINK,iPAPER,iCLS,iATXY,iEMIT,iPRINT,iCR,
iKEY,iMEM,iMEMSCR,iMEMFNT,

  // iSLEEP
iMSEC,iTIME,iIDATE,

iLOAD,iSAVE,iAPPEND,

iiii, // last real
};

#define STACKSIZE 256
#define DICCMEM 0x1ff
#define CODEMEM 0x3ff
#define DATAMEM 0x3fff

int32_t stack[STACKSIZE];
int32_t ATOS,*ANOS;

struct dicctionary {
  uint64_t name;
  uint32_t mem; // $c info 3ff len fffff adr
};

dicctionary dicc[DICCMEM];
uint32_t ndicc=0;

int32_t memcode[CODEMEM];
uint32_t memc=0;

int8_t  memdata[DATAMEM];
uint32_t memd=0;

int cerror;

#endif
