#include "PocketC.h"

void Alert(char*);
//#define ASSERT(x) { if (!(x)) Alert("Assert Failed"); }
#define ASSERT(x)

char* LockStringOld(Handle hStr) {
    if (0x80000000 & (ULong)hStr) {
        String* pString = (String*)MemHandleLock(hStr);
        ASSERT(pString->sType == stIndirect);
        return &pString->data[0];		
    } else
        return (char*)hStr; //it's a pointer
}

void UnlockStringOld(Handle hStr) {
    if (0x80000000 & (ULong)hStr) {
        MemHandleUnlock(hStr);
    }
}

char* LockString(Value* v) {
    if (v->stype == stIndirect) {
        String* pString = (String*)MemHandleLock(v->sVal);
        return &pString->data[0];		
    } else
        return (char*)v->sVal; //it's a pointer
}

void UnlockString(Value* v) {
    if (v->stype == stIndirect) {
        MemHandleUnlock(v->sVal);
    }
}
    
void AcquireString(Value* d, Value* s) {
    if (s->stype == stIndirect) {
        String* pString = (String*)MemHandleLock(s->sVal);
        pString->nRef++;
        MemHandleUnlock(s->sVal);
        d->stype = stIndirect;
    } else {
        d->stype = stConst;
    }
    d->sVal = s->sVal;
    d->type = vtString;
}
    
void ReleaseString(Value* v) {
    if (v->stype == stIndirect) {
        unsigned short count;
        String* pString = (String*)MemHandleLock(v->sVal);
        count = --(pString->nRef);
        
        MemHandleUnlock(v->sVal);
        if (!count)
            MemHandleFree(v->sVal);
    }
}

void UnlockReleaseString(Value* v) {
    if (v->stype == stIndirect) {
        unsigned short count;
        String* pString = (String*)MemHandleLock(v->sVal);
        
        count = --(pString->nRef);
        MemHandleUnlock(v->sVal); // twice
        MemHandleUnlock(v->sVal);
        if (!count)
            MemHandleFree(v->sVal);
    }
}

// size is the length of the string NOT including the '\0'
char* NewString(Value* v, unsigned short size) {
    String* pStr;
    Handle h = (Handle)MemHandleNew(sizeof(String) + size);
    if (h) {
        pStr = (String*)MemHandleLock(h);
        pStr->sType = stIndirect;
        pStr->nRef = 1;
        v->type = vtString;
        v->stype = stIndirect;
        v->sVal = h;
        return &pStr->data[0];
    }
    return NULL; // maybe do something here
}

bool NewStringFromConst(Value* v, const char* data) {
    char* pNew = NewString(v, strlen(data));
    if (pNew) {
        strcpy(pNew, data);
        MemHandleUnlock(v->sVal);
        return true;
    }
    return false;
}

void NewConstString(Value* v, const char* data) {
    v->type = vtString;
    v->stype = stConst;
    v->sVal = (Handle)data;
}

void EnsureIndirect(Value* v) {
    if (v->stype == stIndirect) {
        return;
    } else {
        if (!NewStringFromConst(v, (char*)v->sVal)) rt_oom();
        return;
    }
}

void EnsureMalleable(Value* v) {
    EnsureIndirect(v);
    Handle h = v->sVal;
    String* pStr = (String*)MemHandleLock(h);
    if (pStr->nRef > 1) {
        // need to make a new copy before we modify it
        char* data;
        data = NewString(v, strlen(pStr->data));
        if (!data) rt_oom();
        strcpy(data, pStr->data);
        MemHandleUnlock(v->sVal);
        pStr->nRef--;
        MemHandleUnlock(h);
    } else {
        MemHandleUnlock(h);
    }
}
    
