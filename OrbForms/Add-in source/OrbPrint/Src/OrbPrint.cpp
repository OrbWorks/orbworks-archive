#include <PalmOS.h>
#include "OrbNative.h"

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

// Print command defines
#define cmdPrintChars     32768
#define cmdXmitChars      32769
#define cmdStartPrint     32770
#define cmdStartXmit      32771
#define cmdPrintLine      32772
#define cmdXmitLine       32773
#define cmdEndPrint       32774
#define cmdEndXmit        32775
#define cmdPrintLinePassThru 32776
#define cmdXmitLinePassThru  32777
#define cmdDoFF           32800
#define cmdSetPlain       32802
#define cmdSetBold        32804
#define cmdSetStdFont     32810
#define cmdSetNewFont     32812
#define cmdSetPtSize      32814
#define cmdSetLeftMargin  32820
#define cmdSetTopMargin   32822
#define cmdSetChars       32824
#define cmdSetLines       32826
#define cmdSetPortrait    32828
#define cmdSetLandscape   32830
#define cmdSetIndent      32832
#define cmdSetNumCopies   32834
#define cmdGetChars       33000
#define cmdGetLines       33001
#define sysAppLaunchPrintSetup 40000

// Font defines
#define Courier               0
#define Times                 1
#define Helvetica             2
#define fontMask              7
#define pt12                  0
#define pt10                  8
#define pt9                  16
#define pointMask            56

/*
object PalmPrint {
    int id;
    void _init() @ library("OrbPrint", 0);
    void _destroy() @ 80;
    void _copy(PalmPrint) @ 89;
    
    int charPerLine @ library("OrbPrint", 12) : library("OrbPrint", 13);
    int linesPerPage @ library("OrbPrint", 14) : library("OrbPrint", 15);
    int font @ :library("OrbPrint", 16);
    int ptSize @ :library("OrbPrint", 17);
    int leftMargin @ :library("OrbPrint", 18);
    int topMargin @ :library("OrbPrint", 19);
    int style @ :library("OrbPrint", 20);
    int indent @ :library("OrbPrint", 21);
    int numCopies @ :library("OrbPrint", 22);

    bool open() @ library("OrbPrint", 1);
    
    // block mode
    void printString(string str) @ library("OrbPrint", 2);
    void xmitString(string str) @ library("OrbPrint", 3);
    
    // line mode
    void beginPrint() @ library("OrbPrint", 4);
    void endPrint() @ library("OrbPrint", 5);
    void beginXmit() @ library("OrbPrint", 6);
    void endXmit() @ library("OrbPrint", 7);
    void printLine(string str) @ library("OrbPrint", 8);
    void xmitLine(string str) @ library("OrbPrint", 9);
    void printLinePassThru(string str) @ library("OrbPrint", 10);
    void xmitLinePassThru(string str) @ library("OrbPrint", 11);
};
*/

struct PrintData {
    LocalID dbID;
};

void Print_destroy(OrbFormsInterface* ofi, void* data, void* obj) {
    PrintData* pd = (PrintData*)obj;
    delete pd;
}

void Print_init(OrbFormsInterface* ofi) {
    PrintData* pd = new PrintData;
    pd->dbID = NULL;
    ofi->nobj_create(pd, Print_destroy);
}

void Print_open(OrbFormsInterface* ofi) {
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;

    pd->dbID = DmFindDatabase(0,"PalmPrint");
    if (pd->dbID) ofi->vm_retVal->iVal = 1;
}

void Print_printString(OrbFormsInterface* ofi) {
    Value* str = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    char* pstr = ofi->val_lock(str);

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdPrintChars, pstr, &result);
    }
    ofi->val_unlockRelease(str);
}

void Print_xmitString(OrbFormsInterface* ofi) {
    Value* str = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    char* pstr = ofi->val_lock(str);

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdXmitChars, pstr, &result);
    }
    ofi->val_unlockRelease(str);
}

void Print_beginPrint(OrbFormsInterface* ofi) {
    PrintData* pd = (PrintData*)ofi->nobj_pop();

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdStartPrint, 0, &result);
    }
}

void Print_endPrint(OrbFormsInterface* ofi) {
    PrintData* pd = (PrintData*)ofi->nobj_pop();

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdEndPrint, 0, &result);
    }
}

void Print_beginXmit(OrbFormsInterface* ofi) {
    PrintData* pd = (PrintData*)ofi->nobj_pop();

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdStartXmit, 0, &result);
    }
}

void Print_endXmit(OrbFormsInterface* ofi) {
    PrintData* pd = (PrintData*)ofi->nobj_pop();

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdEndXmit, 0, &result);
    }
}

void Print_printLine(OrbFormsInterface* ofi) {
    Value* str = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    char* pstr = ofi->val_lock(str);

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdPrintLine, pstr, &result);
    }
    ofi->val_unlockRelease(str);
}

void Print_printLinePass(OrbFormsInterface* ofi) {
    Value* str = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    char* pstr = ofi->val_lock(str);

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdPrintLinePassThru, pstr, &result);
    }
    ofi->val_unlockRelease(str);
}

void Print_xmitLine(OrbFormsInterface* ofi) {
    Value* str = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    char* pstr = ofi->val_lock(str);

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdXmitLine, pstr, &result);
    }
    ofi->val_unlockRelease(str);
}

void Print_xmitLinePass(OrbFormsInterface* ofi) {
    Value* str = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    char* pstr = ofi->val_lock(str);

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdXmitLinePassThru, pstr, &result);
    }
    ofi->val_unlockRelease(str);
}

void Print_getChars(OrbFormsInterface* ofi) {
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    short chars = 0;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdGetChars, (char*)&chars, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = chars;
}

void Print_setChars(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long chars = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdSetChars, (char*)chars, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = chars;
}

void Print_getLines(OrbFormsInterface* ofi) {
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    short val = 0;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdGetLines, (char*)&val, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setLines(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdSetLines, (char*)val, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setFont(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        if (val == 8)
            SysAppLaunch(0,pd->dbID,0,cmdSetStdFont, 0, &result);
        else {
            val &= fontMask;			
            SysAppLaunch(0,pd->dbID,0,cmdSetNewFont, (char*)val, &result);
        }
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setSize(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        val &= pointMask;			
        SysAppLaunch(0,pd->dbID,0,cmdSetPtSize, (char*)val, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setLeft(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdSetLeftMargin, (char*)val, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setTop(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdSetTopMargin, (char*)val, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setStyle(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        if (val == 0)
            SysAppLaunch(0,pd->dbID,0,cmdSetPlain, 0, &result);
        else
            SysAppLaunch(0,pd->dbID,0,cmdSetBold, 0, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setIndent(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdSetIndent, (char*)val, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setCopies(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        SysAppLaunch(0,pd->dbID,0,cmdSetNumCopies, (char*)val, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}

void Print_setOrient(OrbFormsInterface* ofi) {
    Value* vc = ofi->vm_pop();
    PrintData* pd = (PrintData*)ofi->nobj_pop();
    long val = vc->iVal;

    if (pd->dbID) {
        UInt32 result;
        if (val == 0)
            SysAppLaunch(0,pd->dbID,0,cmdSetLandscape, 0, &result);
        else
            SysAppLaunch(0,pd->dbID,0,cmdSetPortrait, 0, &result);
    }
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = val;
}


void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
{
    if (iFunc == NATIVE_STARTUP || iFunc == NATIVE_SHUTDOWN) return NULL;

    switch (iFunc) {
        case NATIVE_STARTUP:
            return NULL; // we do not need to setup any global state
        case NATIVE_SHUTDOWN:
            return NULL;
        case 0: Print_init(ofi); break;
        case 1: Print_open(ofi); break;
        case 2: Print_printString(ofi); break;
        case 3: Print_xmitString(ofi); break;
        case 4: Print_beginPrint(ofi); break;
        case 5: Print_endPrint(ofi); break;
        case 6: Print_beginXmit(ofi); break;
        case 7: Print_endXmit(ofi); break;
        case 8: Print_printLine(ofi); break;
        case 9: Print_xmitLine(ofi); break;
        case 10: Print_printLinePass(ofi); break;
        case 11: Print_xmitLinePass(ofi); break;
        case 12: Print_getChars(ofi); break;
        case 13: Print_setChars(ofi); break;
        case 14: Print_getLines(ofi); break;
        case 15: Print_setLines(ofi); break;
        case 16: Print_setFont(ofi); break;
        case 17: Print_setSize(ofi); break;
        case 18: Print_setLeft(ofi); break;
        case 19: Print_setTop(ofi); break;
        case 20: Print_setStyle(ofi); break;
        case 21: Print_setIndent(ofi); break;
        case 22: Print_setCopies(ofi); break;
        case 23: Print_setOrient(ofi); break;
    }
    return NULL;
}
