// see data.cpp for descriptions

// Global storage of identifiers
extern GlobalInfo* globalI;
extern GlobalInit* globalInit;
extern LocalInfo* localI;
extern FuncInfo* funcI;
extern const FuncInfo funcTable[];
extern unsigned short nLocals, nGlobals, nGlobalInits, nFuncs, nBIFuncs, localOff, globalOff;

// Input/ Output streams
extern char *source, *sourceStart;
extern unsigned char* code;
extern unsigned char* codesegs[MAX_CODE_SEGS];
extern Handle output;

// Lexical storage
extern short currLine;
extern Token tok;
extern char* keyword[NUM_KEYWORDS];
extern TokenID keytoken[NUM_KEYWORDS];
extern Macro* macro;
extern char* macrod;

// VM/compiler storage
extern Value retVal;
extern float* cFloats;
extern char* cStrings;
extern Handle strings_h;
extern VoidHand floats_h, reloc_h;
extern VoidHand local_h, global_h, globalInits_h, func_h, macro_h, macrod_h;
extern unsigned short _func, _local, _global, _globalInits, _macro, _macrod;
extern unsigned short nFloats, nStrChars, _Floats, _StrChars, nMacros, nMacrod;
extern short incLevel, libNum, libFuncNum;
extern AddIn addIns[];
extern PocketCInterface pci;
extern bool wasArray;

extern Value* globals;
extern char BCArgs[vmNumInstr];
extern unsigned short* reloc;
extern unsigned short st,fp,stackSize,pc,fb, nCodeSegs;
extern Value* stack;
extern VoidHand stack_h;
extern unsigned short l_funcEnd;
extern long funcStart;
extern long codePtr, _codeSize, codeOff;
extern DmOpenRef codeDB;
extern VoidHand code_h;
extern unsigned short nLabels, _Labels;
extern long dbgLoc;

extern EventQueue eventQueue;

extern UInt MathLibRef, SerLibRef;
extern bool mathLibLoaded;
extern bool bHasColor, bHighDensity;

extern hvector<CaseEntry> * pCases;
extern hvector<LibraryInfo>* pLibs;
extern bool bUsesPocketCAddin;
