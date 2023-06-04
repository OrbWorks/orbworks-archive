
// Global storage of identifiers
extern GlobalInfo* globalI;
extern GlobalInit* globalInit;
extern LocalInfo* localI;
extern FuncInfo* funcI;
extern FuncInfo* funcTable;
extern unsigned short nLocals, nGlobals, nGlobalInits, nFuncs, nBIFuncs, localOff, globalOff;

// Input/ Output streams
extern char *source, *sourceStart;
extern unsigned char* code;

// Lexical storage
extern short currLine;
extern Token tok;
extern char* keyword[NUM_KEYWORDS];
extern TokenID keytoken[NUM_KEYWORDS];
extern Macro* macro;
extern char* macrod;

// VM/compiler storage
extern unsigned long* cFloats;
extern char* cStrings;
extern unsigned short nFloats, nStrChars, nMacros, nMacrod;
extern unsigned short libNum, libFuncNum;
extern short incLevel;
extern bool wasArray;

extern char BCArgs[vmNumInstr];
extern unsigned short* reloc;
extern unsigned short l_funcEnd;
extern long codePtr, peepLoc[4], funcStart, codeEnd;
extern unsigned short nLabels;
extern VMsym peepInstr[4];
extern VarType peepType[4];

extern bool bAddSource;
extern vector<string> g_libFiles;
extern vector<string> g_funcNames;
extern vector<string> g_funcTips, g_funcDocs;
extern vector<SymbolInfo> g_syms;

extern vector<caseEntry> * pCases;
extern vector<LibraryInfo> * pLibs;

extern PalmApp app;