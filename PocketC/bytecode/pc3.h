#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CODE_SEGS 4
#define HEADER_SIZE 25
#define CODE_SEG_SIZE (63 * 1024)

enum VMsym {
   // constants
   vmCInt=0, vmCChar, vmCFloat, vmCString,
   // typecasts
   vmToInt, vmToChar, vmToFloat, vmToString,
   // variables
   vmGetG, vmSetG, vmGetL, vmSetL, vmArray,
   vmIncGA, vmDecGA, vmIncLA, vmDecLA,
   vmIncGB, vmDecGB, vmIncLB, vmDecLB,
   // operators
   vmAdd, vmSub, vmMult, vmDiv, vmMod, vmNeg, vmNot, vmAnd, vmOr,
   // relational operators
   vmEq, vmNEq, vmLT, vmLTE, vmGT, vmGTE,
   // flow control
   vmJmp, vmJmpZ, vmJmpNZ,
   vmOldCall, vmCallBI, vmRet, vmSetRet, vmSetRet0, vmPop, vmPopN,
   vmLink, vmUnLink, vmHalt, vmSwap, vmLibCall,

   // Version 3.0 stuff
   vmCWord, vmCWordPFP, vmCall,
   vmLoad, vmSave,
   vmIncA, vmDecA, vmIncB, vmDecB,
   // bitwise
   vmBAnd, vmBOr, vmBNot, vmSL, vmSR, vmXor,
   vmNoOp, vmNoOp2, vmCallF, vmFuncA,

   vmAddExtI, vmGetC, vmSetC, vmCase,

   vmNumInstr
};

enum VarType {
   vtInt=0, vtChar, vtFloat, vtString, vtVoid, vtAddr, vtStackIndex, vtNone
};

struct Value {
   VarType type;
   union {
      int iVal;
      float fVal;
      char cVal;
      char* sVal; // should be a handle
   };
};
