// pcc.h
#pragma warning (disable:4786)
#include <commdlg.h>
#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include "database.h"
#include <vector>
#include <string>
using namespace std;

#define FUNC_CHOP

#define MAX_CODE 0x40000
#define MAX_FLOAT 4096
#define MAX_STRING 0x10000
#define MAX_RELOC 0x10000
#define MAX_GLOBAL 4096
#define MAX_GLOBINIT 4096
#define MAX_LOCAL 512
#define MAX_FUNC 4096
#define MAX_MACRO 1024
#define MAX_MACROD 0x10000

BOOL CALLBACK MainDlg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool InitNonMFC();
void SavePrefs();
void SetDocText(const char* title, const char* body);

extern char g_pszVersion[32];
extern DWORD bUploadOnBuild;
extern DWORD g_bProjectSorted;

struct PalmBitmap {
    int id;
    string images[10];
};

struct PalmResource {
    string type;
    int id;
    string file;
};

struct PalmApp {
    string dbname;
    string name;
    string ver;
    string l_icon[10];
    string s_icon[10];
    string cid;
    string category;
    string file;
    vector<PalmBitmap> bmps;
    vector<PalmResource> res;
};

#define NUM_KEYWORDS 29
#define MAX_CODE_SEGS 4
#define HEADER_SIZE 25
#define CODE_SEG_SIZE (63 * 1024)

enum TokenID {
    // syntactic elements
    tiLibrary, tiAddin, tiAddinResId, tiInclude,
    tiIdent, tiConstString, tiConstInt, tiConstFloat, tiConstChar,
    // variable types and declarators
    tiStatic, tiInt, tiChar, tiFloat, tiString,
    // keywords
    tiIf, tiElse, tiFor, tiWhile, tiDo, tiReturn, tiBreak, tiContinue,
    tiSwitch, tiCase, tiDefault,
    // punctuation
    tiLParen, tiRParen, tiLBrace, tiRBrace, tiLBrack, tiRBrack,
    tiSemiColon, tiComma, tiAt, tiColon,
    tiMinus, tiPlus, tiMult, tiDiv, tiMod, tiAddr, tiAnd, tiOr, tiNot,
    // Comparison
    tiEq, tiNEq, tiLT, tiLTE, tiGT, tiGTE,
    // Assignment
    tiAssign, tiMultAssign, tiPlusAssign, tiMinusAssign, tiDivAssign, tiModAssign,
    tiInc, tiDec,
    // Bitwise
    tiBNot, tiXor, tiBOr, tiSL, tiSR,
    // Preprocessor
    tiDefine, tiIfDef, tiIfNDef, tiPElse, tiEndIf, tiUndef,
    tiEnd
};

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

    vmAddExtI, vmGetC, vmSetC,
    vmCase,

    vmNumInstr
};

struct SymbolInfo {
    string name;
    string decl;
    string docs;
    string file;
    int line;
};

struct Token {
    TokenID id;
    char* value;
    long intVal;
    float floatVal;
    short line;
};

struct Macro {
    char name[32];
    char len;
    char nTokens;
    unsigned short data;
};

struct IncludeStack {
    int line;
    char* src;
    char* isrc;
    char* name;
};

enum VarType {
    vtInt=0, vtChar, vtFloat, vtString, vtVoid, vtAddr, vtStackIndex, vtNone, vtMethodPtr, vtSavedFB
};

struct GlobalInfo {
    char name[32];
    VarType type;
    short arraySize;
    unsigned short globalOffset;
};

struct GlobalInit {
    unsigned short offset;
    VarType type;
    unsigned long val;
};

struct LocalInfo {
    char name[32];
    VarType type;
    short arraySize; // 0 for non arrays
    short stackOffset;
};

struct FuncInfo {
    char name[32];
    // VarType ret; return value not currently used
    char nArgs;
    char lib;
    long loc;
    VarType argType[10];
    long end;
    bool marked;
};

struct LibraryInfo {
    bool bLib; // false for add-in
    unsigned short resid;
    unsigned short name;
    string fullPath;
    bool marked;
};

struct caseEntry {
    VarType type;
    long val;
    unsigned short loc;
};

#include "data.h"

void c_error(char*, short); // compiler error
void oom(); // out of memory

// symbol functions
short addLocal(VarType type, char* name, short size);
short findLocal(char* name, VarType* vt=NULL);
short addGlobal(VarType type, char* name, short size);
void  addGlobalInit(VarType, unsigned short, int);
short findGlobal(char* name, VarType* vt=NULL);
short addFunc(char* name, short nArgs, long loc);
short findFunc(char* name, bool builtIn);
short addMacro(char* name, char nTokens, unsigned short data);
short findMacro(char* name, short& nTokens, unsigned short& data);
bool  delMacro(char* name);
unsigned short addMacroByte(char b);
unsigned short addMacroInt(long l);
unsigned short addMacroFloat(float f);
unsigned short addMacroString(char* s);
short addLibFunc(char* name, short nArgs, VarType arg1, VarType arg2, VarType arg3, VarType arg4, VarType arg5, VarType arg6, VarType arg7, VarType arg8, VarType arg9, VarType arg10);

// lex functions
void nextToken();
VarType varType(TokenID);
VarType constVarType(TokenID);

// parser
void parse();
void match(TokenID, char*);
short matchInt();

void dodecl(char*, VarType, bool);
void dofunc(char*, VarType);
void relocate();
void run();

// compiler
void doBlock();
void doStmts();
void doStmt();
VarType doExpr();
VarType expr0();
VarType expr0a();
VarType expr1();
VarType expr1a();
VarType expr1b();
VarType expr2();
VarType expr3();
VarType expr4();
VarType expr5();
VarType expr6();
VarType expr7();
void doPeep();

// code handler
void addByte(unsigned char);
void addInstr(VMsym, VarType vt = vtNone);
void addWord(short);
void addInt(long);
unsigned short newLabel();
unsigned short setLabel(unsigned short lab);
unsigned char endByte(short); // 0 = last byte added
void removeBytes(unsigned char* buff, short); // removes last bytes added
unsigned short addFloat(float);
unsigned short addString(char*);

#define sysAppLaunchCmdStartApplet sysAppLaunchCmdCustomBase

void SymAddFile(LPCTSTR tzFileName, bool bIncludes);
void SymAddBuffer(LPCTSTR tzFileName, LPCTSTR tzBuffer, bool bIncludes);
void SymRemoveFile(LPCTSTR tzFileName);
void SymClear();
bool SymIsFileInProject(LPCTSTR tzFileName);
LPTSTR SymFindDef(LPCTSTR tzName, int& line);

extern vector<string> post_build;
extern bool bNoErrorPopup;
