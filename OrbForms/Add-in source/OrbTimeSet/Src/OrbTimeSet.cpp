#include <PalmOS.h>
#include <Extensions\ExpansionMgr\VFSMgr.h>
#include "OrbNative.h"

typedef unsigned char byte;

extern "C" {
void* __Startup__(OrbFormsInterface*, void*, UInt16);
}

// The following five functions would normally be defined
// in the runtime library that an application links with.
// However, since this is just a code resource, we cannot
// link to it, so must define these ourselves
void* operator new(unsigned long size) {
    return MemPtrNew(size);
}

void operator delete(void* p) {
    MemPtrFree(p);
}

void* operator new[](unsigned long size) {
    return MemPtrNew(size);
}

void operator delete[](void* p) {
    MemPtrFree(p);
}
// the compiler generates a call to this to copy a structure
extern "C" 
void *__copy(char *to,char *from,unsigned long size) {
    char *f,*t;

    if(to) for(f=(char *)from,t=(char *)to; size>0; size--) *t++=*f++;
    return to;
}

extern "C"
asm long __lmul__(long left:__D0,long right:__D1):__D0;
asm long __lmul__(long left:__D0,long right:__D1):__D0
{				/* d0: operand a ; d1: operand b */
#if __CFM68K__
    move.l	(a1), a5
#endif

    movem.l	d2-d3,-(sp)
    move.l	d0,d2
    swap	d2
    mulu.w	d1,d2		/* HiWord(a)*LoWord(b)->d2 */
    move.l	d1,d3
    swap	d3
    mulu.w	d0,d3		/* LoWord(a)*HiWord(b)->d3 */
    add.w	d3,d2		/* add the two results */
    swap	d2		/* and put LoWord of result in HiWord(d2) */
    clr.w	d2		/* clr LoWord(d2) */
    mulu	d1,d0		/* LoWord(a)*LoWord(b)->d0 */
    add.l	d2,d0		/* result:=d0+d2 */
    movem.l	(sp)+,d2-d3
    rts
}

void Date_setTime(OrbFormsInterface* ofi) {
    Value* vPtr = ofi->vm_pop();
    Value* pVal = ofi->vm_deref(vPtr->iVal);
    TimSetSeconds(pVal->iVal);
}

void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
{
    if (iFunc == NATIVE_STARTUP) {
        return NULL;
    } else if (iFunc == NATIVE_SHUTDOWN) {
        return NULL;
    }

    switch (iFunc) {
        case 0: Date_setTime(ofi);
    }
    return NULL;
}
