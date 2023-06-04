#include "PocketC.h"

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

// variables used at compile time
Token tok;         // current token
char* source;      // current position in source code
char* sourceStart; // beginning of the current file
GlobalInfo* globalI;
GlobalInit* globalInit;
LocalInfo* localI;
FuncInfo* funcI;
Macro* macro;
char* macrod;
unsigned short nLocals=0, nGlobals=0, nGlobalInits=0, nFuncs=0;
unsigned short* reloc;       // array of label addresses
unsigned short nLabels, _Labels, _Floats, _StrChars;    // leading underscore represents
unsigned short l_funcEnd, nStrChars, nFloats, localOff; // size of the currently allocated
unsigned short nMacros, nMacrod;                        // buffer
long _codeSize, codeOff, codePtr, dbgLoc, funcStart;
unsigned short _global, _globalInits, _func, _local, _macro, _macrod;
short incLevel, libNum, libFuncNum; 
bool wasArray;    // was the last variable looked up an array?
DmOpenRef codeDB; // reference to database being written

// variables used at compile and runtime
unsigned char* code;  // code
unsigned char* codesegs[MAX_CODE_SEGS];
float* cFloats;   // applet float resources
char* cStrings;   // applet string resources
unsigned short globalOff;    // offset of current global (compile) / size of global heap (runtime)

// virtual machine
AddIn addIns[10];
PocketCInterface pci;
Value retVal;     // return value for current function/built-in function
Value* globals;   // global memory "heap"
unsigned short st, fp, pc, fb;   // virtual machine registers
unsigned short stackSize;    // current size of stack buffer (st is the stack pointer)
Value* stack;     // stack

// runtime library
Handle output;    // output form buffer 
UInt MathLibRef, SerLibRef;
bool mathLibLoaded;

// handles (so that the memory is moveable), each handle is associated with a pointer elsewhere
VoidHand stack_h, code_h, floats_h, reloc_h, local_h, global_h, globalInits_h, func_h, macro_h, macrod_h;
Handle strings_h;

hvector<CaseEntry>* pCases;
hvector<LibraryInfo>* pLibs;
bool bUsesPocketCAddin;

// number of argument bytes for each instruction
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
    0, 2, 0, 0,
    
    5, 0, 0,
    7
};
