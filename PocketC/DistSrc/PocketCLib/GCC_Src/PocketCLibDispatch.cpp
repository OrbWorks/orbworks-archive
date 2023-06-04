// Include Pilot headers
#include "PocketCLib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dispatch table declarations */
UInt32 install_dispatcher(UInt16,SysLibTblEntryPtr);
MemPtr *gettable(void);

// The first piece of code must be the code which sets
// up the dispatch table. For any library, it looks like this: 

UInt32 start (UInt16 refnum, SysLibTblEntryPtr entryP)
{
    UInt32 ret;

    asm("
    movel %%fp@(10),%%sp@-
    movew %%fp@(8),%%sp@-
    jsr install_dispatcher(%%pc)
    movel %%d0,%0
    jmp out(%%pc)
gettable:
    lea jmptable(%%pc),%%a0
    rts
jmptable:
    dc.w 38
    dc.w 14
    dc.w 18
    dc.w 22
    dc.w 26
    dc.w 30
    dc.w 34
    jmp PocketCLibOpen(%%pc)
    jmp PocketCLibClose(%%pc)
    jmp PocketCLibSleep(%%pc)
    jmp PocketCLibWake(%%pc)
    jmp PocketCLibAddFunctions(%%pc)
    jmp PocketCLibExecuteFunction(%%pc)
    .asciz \"PocketCLib\"
.even
out:
    " : "=r" (ret) :);
    return ret;
}

    UInt32 install_dispatcher(UInt16 refnum, SysLibTblEntryPtr entryP)
    {
        MemPtr *table = gettable();
        entryP->dispatchTblP = table;
        entryP->globalsP = NULL;
        return 0;
    }

#ifdef __cplusplus 
}
#endif


