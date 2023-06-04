#ifndef POCKETC_LIB_H
#define POCKETC_LIB_H

#include <PalmOS.h>

enum VarType { vtInt, vtChar, vtFloat, vtString, vtVoid };
enum StrType { stClassic, stIndirect, stConst };

struct Value {
    VarType type;
    StrType stype;
    union {
        long iVal;
        float fVal;
        char cVal;
        MemHandle sVal;
    };
};

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
    short  (* addLibFunc)(char* name, short nArgs, VarType arg1=vtInt, VarType arg2=vtInt, VarType arg3=vtInt, VarType arg4=vtInt, VarType arg5=vtInt, VarType arg6=vtInt, VarType arg7=vtInt, VarType arg8=vtInt, VarType arg9=vtInt, VarType arg10=vtInt);
    void (* callFunc_deprecated)(unsigned short loc);
    Value* retVal;
    Value* (* deref)(short ptr);
    bool (* callBI)(char* name);
    char* (* lockString_deprecated)(MemHandle sVal);
    void (* unlockString_deprecated)(MemHandle sVal);
    UInt32 (* vmCtrl)(UInt32 id, UInt32 val);
    void (* callFunc32)(long loc);
    PocketCLibraryApiExt* apiExt;
    UInt32 reserved;
    
    //
    // TODO: Add your global variables here
    //
    
};
typedef PocketCLibGlobalsType* PocketCLibGlobalsPtr;

#ifdef __cplusplus
extern "C" {
#endif


/********************************************************************
 * Standard library open, close, sleep and wake functions
 ********************************************************************/

extern PocketCLibGlobalsPtr PocketCLibOpen(UInt16 refNum, UInt32*);
extern Err PocketCLibClose(UInt16 refNum, UInt32);
extern Err PocketCLibSleep(UInt16 refNum);
extern Err PocketCLibWake(UInt16 refNum);

/********************************************************************
 * Custom library API functions
 ********************************************************************/
    
// Add the PocketC library function information
extern Err PocketCLibAddFunctions(UInt16 refNum);

// Execute a PocketC function
extern Err PocketCLibExecuteFunction(UInt16 refNum, int funcNum);

// For loading the library in PalmPilot Mac emulation mode
extern Err PocketCLibInstall(UInt16 refNum, SysLibTblEntryPtr entryP);

#ifdef __cplusplus 
}
#endif


#endif
