#include "PocketCLib.h"

/********************************************************************
 * Internally used routines
 ********************************************************************/
static PocketCLibGlobalsPtr PrvMakeGlobals(UInt16 refNum);
static PocketCLibGlobalsPtr PrvLockGlobals(UInt16 refNum);

/********************************************************************
 * Internally used macros
 ********************************************************************/

// Unlock globals
#define PrvUnlockGlobals(gP)	MemPtrUnlock(gP)

/********************************************************************
 * Open routine - add global initialization code
 ********************************************************************/

PocketCLibGlobalsPtr PocketCLibOpen(UInt16 refNum, UInt32*) {
    PocketCLibGlobalsPtr gP;
    Err err = 0;

    gP = PrvMakeGlobals(refNum);
    
    // NOTE: You CANNOT call the global routines (e.g. pop() ) at this point!
    //
    // TODO: Initialize globals
    //
    
    // NOTE: PocketC will fill the structure and then _unlock_ it
    return gP;
}

/********************************************************************
 * Close routine - add global cleanup code 
 ********************************************************************/

Err PocketCLibClose(UInt16 refNum, UInt32) {
    MemHandle gH;
    SysLibTblEntryPtr libEntryP;

    libEntryP = SysLibTblEntry(refNum);
    ErrFatalDisplayIf(libEntryP == NULL, "invalid PocketC lib refNum");

    gH = (MemHandle)(libEntryP->globalsP);
    if (gH) {
        
        //
        // TODO: Insert cleanup routines here (if necessary)
        //
        
        libEntryP->globalsP = NULL;
        MemHandleFree(gH);
    }
    
    return 0;
}

/********************************************************************
 * AddFunctions routine - add your libraries custom functions here
 ********************************************************************/

Err PocketCLibAddFunctions(UInt16 refNum) {
    PocketCLibGlobalsPtr gP;
    
    gP = PrvLockGlobals(refNum);
    
    //
    // TODO: Add library functions here
    //
    gP->addLibFunc("times5", 1, vtInt);
    gP->addLibFunc("reverse", 1, vtString);
    gP->addLibFunc("volume", 3, vtFloat, vtFloat, vtFloat);
    gP->addLibFunc("makeStr", 0);

    PrvUnlockGlobals(gP);
    return 0;
}

/********************************************************************
 * Functions exposed to PocketC applets
 ********************************************************************/

void times5(PocketCLibGlobalsPtr gP) {
    Value x;
    
    gP->pop(x);
    gP->retVal->iVal = x.iVal * 5;
}

void reverse(PocketCLibGlobalsPtr gP) {
    Value str;
    
    gP->pop(str);
    
    char* b = (char*)MemHandleLock(str.sVal);
    char* e = b + StrLen(b) - 1;
    while (b < e) {
        char t = *b;
        *b = *e;
        *e = t;
        b++; e--;
    }
    
    MemHandleUnlock(str.sVal);
    
    gP->retVal->type = vtString;
    gP->retVal->sVal = str.sVal;
    // we don't need to call cleanup on str since the string MemHandle
    // is still in use
}

void volume(PocketCLibGlobalsPtr gP) {
    Value x, y, z;
    
    gP->pop(z); // pop them off in reverse order
    gP->pop(y);
    gP->pop(x);
    
    gP->retVal->type = vtFloat;
    gP->retVal->fVal = x.fVal * y.fVal * z.fVal;
}

/********************************************************************
 * ExecuteFunction routine - call your library functions from here
 ********************************************************************/
Err PocketCLibExecuteFunction(UInt16 refNum, int funcNum) {
    PocketCLibGlobalsPtr gP;
    
    gP = PrvLockGlobals(refNum);
    
    switch (funcNum) {
        case 0: times5(gP); break;
        case 1: reverse(gP); break;
        case 2: volume(gP); break;
    };
    
    PrvUnlockGlobals(gP);
    return 0;
}

/********************************************************************
 * Sleep/Wake routines - don't modify these
 ********************************************************************/

Err PocketCLibSleep(UInt16 refNum) {
    return 0;
}

Err PocketCLibWake(UInt16 refNum) {
    return 0;
}

/********************************************************************
 * MakeGlobals - no modificcations needed
 ********************************************************************/

static PocketCLibGlobalsPtr PrvMakeGlobals(UInt16 refNum)
{
    PocketCLibGlobalsPtr gP = NULL;
    MemHandle	 gH;
    SysLibTblEntryPtr libEntryP;

    // Get library globals
    libEntryP = SysLibTblEntry(refNum);
    ErrFatalDisplayIf(libEntryP == NULL, "invalid PocketC lib refNum");

    // Allocate and initialize our library globals.
    gH = MemHandleNew(sizeof(PocketCLibGlobalsType));
    if (!gH) return NULL;

    libEntryP->globalsP = (void*)gH;
    gP = PrvLockGlobals(refNum);
    
    // Initialize our library globals
    MemSet(gP, sizeof(PocketCLibGlobalsType), 0);

    return gP;
}

/********************************************************************
 * LockGlobals - no modificcations needed
 ********************************************************************/

static PocketCLibGlobalsPtr PrvLockGlobals(UInt16 refNum) {
    PocketCLibGlobalsPtr gP = NULL;
    MemHandle gH;
    SysLibTblEntryPtr libEntryP;

    libEntryP = SysLibTblEntry(refNum);
    if (libEntryP) gH = (MemHandle)(libEntryP->globalsP);
    if (gH) gP = (PocketCLibGlobalsPtr)MemHandleLock(gH);

    return gP;
}
