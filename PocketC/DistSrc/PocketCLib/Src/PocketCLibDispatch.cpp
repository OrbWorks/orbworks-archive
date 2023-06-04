// Define this so we don't use the PalmPilot pre-compiled headers.
// We can't use them here because we re-define some build options
#ifndef PILOT_PRECOMPILED_HEADERS_OFF
#define	PILOT_PRECOMPILED_HEADERS_OFF
#endif

// Define EMULATION_LEVEL.  __PALMOS_TRAPS__ was pre-defined by MW C Compiler
// Test it before we blow it away so we can properly define EMULATION_LEVEL.
#if __PALMOS_TRAPS__
    #define EMULATION_LEVEL		EMULATION_NONE		// building Pilot executable
#endif

// Now clear __PALMOS_TRAPS__ and define USE_TRAPS so headers do the right thing
// That is, we need to be able to get the addresses of SampleLibrary routines.
// Other modules will access the routines through traps.
#undef __PALMOS_TRAPS__
#define __PALMOS_TRAPS__ 	0
#define	USE_TRAPS 	0


// Include Pilot headers
#include "PocketCLib.h"

#if EMULATION_LEVEL == EMULATION_NONE
    #define PocketCLibInstall   __Startup__
#endif


// Local prototypes
extern "C" Err PocketCLibInstall(UInt16 refNum, SysLibTblEntryPtr entryP);
static asm void*  PocketCLibDispatchTable(void);


Err PocketCLibInstall(UInt16 refNum, SysLibTblEntryPtr entryP) {
    // Install pointer to our dispatch table
    entryP->dispatchTblP = (void**)PocketCLibDispatchTable();
    
    // Initialize globals pointer to zero (we will set up our library
    // globals in the library "open" call).
    entryP->globalsP = 0;

    return 0;
}

// First, define the size of the jump instruction
#if EMULATION_LEVEL == EMULATION_NONE
    #define prvJmpSize		4
#elif EMULATION_LEVEL == EMULATION_MAC
    #define prvJmpSize		6
#else
    #error unsupported emulation mode
#endif	// EMULATION_LEVEL...

// Now, define a macro for offset list entries
#define libDispatchEntry(index)		(kOffset+((index)*prvJmpSize))


// Finally, define the size of the dispatch table's offset list --
// it is equal to the size of each entry (which is presently 2 bytes) times
// the number of entries in the offset list (***including the @Name entry***).
//
#define	kOffset		(2*7)						// NOTE: This is NOT empirical for PocketC libs!!!!!!

static asm void* PocketCLibDispatchTable(void)
{
    LEA		@Table, A0
    RTS

@Table:
    // Offset to library name
    DC.W		@Name

    // Standard traps
    DC.W		libDispatchEntry(0)					// Open
    DC.W		libDispatchEntry(1)					// Close
    DC.W		libDispatchEntry(2)					// Sleep
    DC.W		libDispatchEntry(3)					// Wake
    
    // Start of the Custom traps
    DC.W		libDispatchEntry(4)					// AddFunctions
    DC.W		libDispatchEntry(5)					// ExecuteFunction

// Standard library function handlers
@GotoOpen:
    JMP 		PocketCLibOpen
@GotoClose:
    JMP 		PocketCLibClose
@GotoSleep:
    JMP 		PocketCLibSleep
@GotoWake:
    JMP 		PocketCLibWake
    
// Customer library function handlers
@GotoAddFuncs:
    JMP 		PocketCLibAddFunctions
@GotoExecFuncs:
    JMP 		PocketCLibExecuteFunction
    
@Name:

    //
    // TODO: Change the library name
    //
    
    DC.B		"PocketClib"
}
