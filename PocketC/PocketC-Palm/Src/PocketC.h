#ifndef __PILOT_H__
#include <PalmOS.h>
#include <PalmCompatibility.h>
#endif
#include <FloatMgr.h>
#include <SonyCLIE.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

#define CURRENT_VERSION 0x714
#define NUM_POCKETC_ADDINS 2

#define ver30 sysMakeROMVersion(3,0,0,sysROMStageRelease,0)
#define ver31 sysMakeROMVersion(3,1,0,sysROMStageRelease,0)
#define ver35 sysMakeROMVersion(3,5,0,sysROMStageRelease,0)
#define ver40 sysMakeROMVersion(4,0,0,sysROMStageRelease,0)
#define ver50 sysMakeROMVersion(5,0,0,sysROMStageRelease,0)

#define EVIL_CHECK
#ifdef EVIL_CHECK
extern ULong evilCode;
volatile extern ULong modEvil;
extern ULong hasEvil;
#endif

#undef ERROR_CHECK_LEVEL
#define ERROR_CHECK_LEVEL 1
#define assert(x) { ErrFatalDisplayIf(!(x), #x); }
//#define assert(x)

#define byref

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
    
    vmAddExtI, vmGetC, vmSetC, vmCase,
        
    vmNumInstr
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
    short data;
};

enum VarType {
    vtInt=0, vtChar, vtFloat, vtString, vtVoid, vtAddr, vtStackIndex, vtNone, vtMethodPtr, vtSavedFB
};

enum StrType { stClassic, stIndirect, stConst };

struct Value {
    VarType type;
    StrType stype;
    union {
        long iVal;
        float fVal;
        char cVal;
        Handle sVal; // handle to a struct String
    };
};

struct String {
    StrType sType;
    unsigned short nRef;
    char data[1];
};

// for old native libraries
char* LockStringOld(Handle hStr);
void  UnlockStringOld(Handle hStr);

char* LockString(Value*);
void  UnlockString(Value*);
void  AcquireString(Value* d, Value* s); // ref++
void  ReleaseString(Value*); // ref--
void  UnlockReleaseString(Value*); // when you are done with the contents for good
char* NewString(Value* v, unsigned short size); // create a locked, empty String
bool  NewStringFromConst(Value* v, const char* data); // create an unlocked, initialized String
void  NewConstString(Value* v, const char*);
void  EnsureMalleable(Value* v);

struct GlobalInfo {
    char name[32];
    VarType type;
    short arraySize;
    unsigned short globalOffset;
};

struct GlobalInit {
    unsigned short offset;
    VarType type;
    long val;
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
    void (* loc)(short);
    //long loc;
    VarType argType[10];
};

struct LibraryInfo {
    bool bLib; // false for add-in
    unsigned short resid;
    unsigned short name;
};

struct CaseEntry {
    VarType type;
    unsigned short loc;
    long val;
};

struct EventItem {
    short type;
    short v1, v2;
};

class EventQueue {
public:
    enum { eqNone, eqKey, eqPenDown, eqPenUp, eqPenMove, eqUp, eqDown,
        eqHK1, eqHK2, eqHK3, eqHK4, eqMenu, eqHome, eqFind, eqCalc,
        eqHotSync, eqLeft, eqRight, eqSelect, eqUndefined,
        eqScan, eqLowBatt, eqTrigger, eqUpdate, eqResize };
    void AddEvent(short type, short v1 = 0, short v2 = 0);
    bool PopEvent();
    void Clear();
    void RemoveInput();
    short type, v1, v2;
private:
    enum { maxEvents = 10 };
    EventItem events[maxEvents];
    short nEvents;
};

// Native add-ins
struct PocketCInterface;

#define NATIVE_STARTUP 0xffff
#define NATIVE_SHUTDOWN 0xfffe
typedef void* (*NATIVE_ENTRY_FUNC)(PocketCInterface*, void* pData, UInt16 iFunc);
typedef bool (*NATIVE_TS_ITER)(PocketCInterface*, Value*, VarType, int, void*); // return true to continue iterating

struct PocketCInterface {
    UInt16 version;
    // VM interface
    Value* (*vm_pop)();
    void   (*vm_push)(Value*); // moves the value - ownership is stack
    Value* vm_retVal;
    void   (*vm_error)(char*); // throws
    Value* (*vm_deref)(long ptr); // may throw
    void   (*vm_call)(long ptr); // modifies stack
    bool   (*vm_callBI)(char* name); // modifies stack
    
    // Value interface
    void  (*val_release)(Value*); // cleanup a string
    void  (*val_acquire)(Value*, Value*);
    char* (*val_lock)(Value*);
    void  (*val_unlock)(Value*);
    void  (*val_unlockRelease)(Value*);
    char* (*val_newString)(Value*, int len);
    char* (*val_newConstString)(Value*, const char*);
    bool  (*val_copyString)(Value*, const char*);

    // Type string processing
    long  (*ts_iterate)(long addr, char* strFormat, int count, NATIVE_TS_ITER func, void* pContext);
    long  (*ts_data_size)(long addr, char* strFormat, int count);
};

struct AddIn {
    UInt16 libRef; // 0 - not loaded, 0xffff - addin
    NATIVE_ENTRY_FUNC func;
    void* data;
    MemHandle hAddIn;
    DmOpenRef dbRef;
    bool bInit;
};

void InitPCI();

class Format {
public:
    Format(char* str, short count, bool pack, char* err);
    bool Next(VarType& type, long& len);
private:
    Format();

    char* format, *orig, *err;
    short count, scount;
    short len;
    VarType type;
    bool bPack;
};


#include "stdcover.h"
#include "vector.h" // my own vector class
#include "MathLib.h"
#include "data.h"

void c_error(char*, short); // compiler error
void oom(); // out of memory
void rt_oom(); // runtime out of memory
void vm_error(char*, unsigned short); // VM error
void vm_warn(char*, unsigned short); // VM warning

// symbol functions
short addLocal(VarType type, char* name, short size);
short findLocal(char* name);
short addGlobal(VarType type, char* name, short size);
void addGlobalInit(VarType, short, long);
short findGlobal(char* name);
short addFunc(char* name, short nArgs, long loc);
short findFunc(char* name, bool builtIn);
short addMacro(char* name, char nTokens, short data);
short findMacro(char* name, short& nTokens, unsigned short& data);
bool delMacro(char* name);
short addMacroByte(char b);
short addMacroInt(long l);
short addMacroFloat(float f);
short addMacroString(char* s);

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
bool doExpr();
void expr0();
void expr0a();
void expr1();
void expr1a();
void expr1b();
void expr2();
void expr3();
void expr4();
void expr5();
void expr6();
void expr7();

// code handler
void addByte(unsigned char);
void addWord(short);
void addInt(long);
unsigned short newLabel();
unsigned short setLabel(unsigned short lab);
unsigned char endByte(short); // 0 = last byte added
void removeBytes(unsigned char* buff, short); // removes last bytes added
unsigned short addFloat(float);
unsigned short addString(char*);
void flushCode();

// virtual machine
void push(Value&);
void pop(Value&);
void typeMatch(Value&, Value&);
void typeCast(Value&, VarType);
void cleanup(Value&);
short  compare(Value&, Value&);
void truthVal(Value&);
bool loadByteCode(short);
bool exec();
bool UIYield(long);
Value* deref(short ptr);
extern bool killEvents, killVM, nativeEnableEvents, enableResize;
extern bool inputDone;
extern Handle inputRet;

// for native lib compatibility
void oldpush(Value&);
void oldpop(Value&);
void oldTypeMatch(Value&, Value&);
void oldTypeCast(Value&, VarType);
void oldCleanup(Value&);
void ValueToOld(Value&);

// Machine dependent library initialization
void initLib();
void closeLib();

#define VMCTRL_ENABLE_EVENTS 0

// Library routines
struct PocketCLibraryApiExt {
    char* (* lockString)(Value* v);
    void  (* unlockString)(Value* v);
    void  (* acquireString)(Value* d, Value* s);
    void  (* releaseString)(Value* v);
    void  (* unlockReleaseString)(Value* v);
    char* (* newString)(Value* v, unsigned short len);
    bool  (* newStringFromConst)(Value* v, const char* data);
    void  (* newConstString)(Value* v, const char* data);
};

struct PocketCLibGlobalsType {
    void (* push)(Value&);
    void (* pop)(Value&);
    void (* cleanup)(Value&);
    void (* typeCast)(Value&, VarType);
    void (* typeMatch)(Value&, Value&);
    bool (* UIYield)(bool);
    short  (* addLibFunc)(char* name, short nArgs, VarType arg1, VarType arg2, VarType arg3, VarType arg4, VarType arg5, VarType arg6, VarType arg7, VarType arg8, VarType arg9, VarType arg10);
    void (* callFunc_deprecated)(unsigned short loc);
    Value* retVal;
    Value* (* deref)(short ptr);
    bool (* callBI)(char* name);
    char* (* lockString_deprecated)(Handle sVal);
    void (* unlockString_deprecated)(Handle sVal);
    UInt32 (* vmCtrl)(UInt32 id, UInt32 val);
    void (* callFunc32)(long loc);
    PocketCLibraryApiExt* apiExt;
    UInt32 reserved;
};
typedef PocketCLibGlobalsType* PocketCLibGlobalsPtr;

short addLibFunc(char* name, short nArgs, VarType arg1, VarType arg2, VarType arg3, VarType arg4, VarType arg5, VarType arg6, VarType arg7, VarType arg8, VarType arg9, VarType arg10);
extern PocketCLibGlobalsPtr PocketCLibOpen(UInt refNum, DWordPtr) SYS_TRAP(sysLibTrapOpen);
extern Err PocketCLibClose(UInt refNum, DWord) SYS_TRAP(sysLibTrapClose);
extern Err PocketCLibAddFunctions(UInt refNum) SYS_TRAP(sysLibTrapCustom);
extern Err PocketCLibExecute(UInt refNum, short funcNum) SYS_TRAP(sysLibTrapCustom+1);

#define sysAppLaunchCmdStartApplet sysAppLaunchCmdCustomBase
#define sysAppLaunchCmdCompile (sysAppLaunchCmdCustomBase + 1)

//
// resize support
//

typedef enum {
    DIA_TYPE_NONE = 0,
    DIA_TYPE_SONY2,
    DIA_TYPE_PALM10,
    DIA_TYPE_PALM11,
    DIA_TYPE_UNKNOWN
} DIAtype;

typedef enum {
    DIA_STATE_MAX = 0,
    DIA_STATE_MIN,
    DIA_STATE_NO_STATUS_BAR,
    DIA_STATE_USER,
    DIA_STATE_UNDEFINED
} DIAState;

DIAtype InitDIA(void);
void CloseDIA(void);
void SetFormDIAPolicy(UInt16 formID);
bool HandleResizeNotification(UInt32 notificationType);
bool LayoutForm(word formID);
void InitFormDIA(word formID);

void SetDIAState(DIAState state);
DIAState GetDIAState();
bool GetDIARotation();
void SetDIARotation(bool portrait);
bool HandleResizeEvent(word formID, EventPtr pEvent);

#define RESIZE_X_MASK 0x0003
#define RESIZE_X_ATTACH_LEFT 0x0000
#define RESIZE_X_ATTACH_RIGHT 0x0001
#define RESIZE_X_STRETCH 0x0002
#define RESIZE_Y_MASK 0x000C
#define RESIZE_Y_ATTACH_TOP 0x0000
#define RESIZE_Y_ATTACH_BOTTOM 0x0004
#define RESIZE_Y_STRETCH 0x0008
#define RESIZE_STRETCH (RESIZE_X_STRETCH | RESIZE_Y_STRETCH)

struct ResizeData {
    word id;
    word flags;
    int nCtrls;
    int x, y, w, h;
    int ox, oy, ow, oh;
};

extern ResizeData resizeData[];
extern int cResizeData;
