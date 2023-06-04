#include "stdafx.h"
#include "pde.h"

// string representation of keywords
char* keyword[NUM_KEYWORDS] = {
    "#library", "#addin", "#addinresid", "#include",
    "if", "else", "for", "while", "do", "return", "break",
    "continue", "static", "string", "int", "float", "char", "pointer",
    "#define", "#ifdef", "#ifndef", "#else", "#endif", "library", "include",
    "switch", "case", "default", "#undef"
};

// corresponding tokens
TokenID keytoken[NUM_KEYWORDS] = {
    tiLibrary, tiAddin, tiAddinResId, tiInclude,
    tiIf, tiElse, tiFor, tiWhile, tiDo, tiReturn, tiBreak,
    tiContinue, tiStatic, tiString, tiInt, tiFloat, tiChar, tiInt,
    tiDefine, tiIfDef, tiIfNDef, tiPElse, tiEndIf, tiLibrary, tiInclude,
    tiSwitch, tiCase, tiDefault, tiUndef
};

char *source, *sourceStart;
unsigned char* code;
Token tok;

GlobalInfo* globalI;
GlobalInit* globalInit;
LocalInfo* localI;
FuncInfo* funcI; //, *funcTable;
Macro* macro;
char* macrod;
unsigned short nLocals=0, nGlobals=0, nGlobalInits=0, nFuncs=0, libNum, libFuncNum, libName[10]; //, nBIFuncs=0;

unsigned long* cFloats;
char* cStrings;
unsigned short* reloc;
unsigned short nLabels;
long codePtr, peepLoc[4], funcStart, codeEnd;
VMsym peepInstr[4];
VarType peepType[4];
short incLevel;
unsigned short l_funcEnd, nStrChars, nFloats, localOff, globalOff, nMacros, nMacrod;

vector<caseEntry>* pCases;
vector<LibraryInfo> * pLibs;
PalmApp app;

char BCArgs[vmNumInstr] = {
   4, 1, 2, 2,
   0, 0, 0, 0,
   2, 2, 2, 2, 0,
   2, 2, 2, 2,
   2, 2, 2, 2,
   0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   2, 2, 2,
   4, 1, 1, 0, 0, 0, 2,
   1, 0, 0, 0, 2,

   // Version 3.0 instructions
   2, 2, 0,
   0, 0,
   0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   0, 2, 4, 4,

   5, 0, 0,
   7
};

bool wasArray, bAddSource=true;
