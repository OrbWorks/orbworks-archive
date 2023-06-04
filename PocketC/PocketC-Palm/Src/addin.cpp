#include "PocketC.h"

void CallFunc(long loc);
bool CallBI(char* name);

Value* vm_pop() {
    return &stack[--st];
}

void vm_push(Value* v) {
    push(*v);
}

void vm_error(char* msg) {
    vm_error(msg, pc);
}

Value* vm_deref(long ptr) {
    return deref(ptr);
}

void vm_call(long ptr) {
    CallFunc(ptr);
}

bool vm_callBI(char* name) {
    return CallBI(name);
}

void val_release(Value* v) {
    cleanup(*v);
}

void val_acquire(Value* d, Value* s) {
    AcquireString(d, s);
}

char* val_lock(Value* v) {
    return LockString(v);
}

void val_unlock(Value* v) {
    UnlockString(v);
}

void val_unlockRelease(Value* v) {
    UnlockReleaseString(v);
}

char* val_newString(Value* v, int len) {
    return NewString(v, len);
}

char* val_newConstString(Value* v, const char* c) {
    NewConstString(v, (char*)c);
    return (char*)c;
}

bool val_copyString(Value* v, const char* c) {
    return NewStringFromConst(v, (char*)c);
}

typedef bool (*IterFunc)(Value*, VarType, int, void*); // return true to continue iterating

long IterateValues(long addr, char* strFormat, int count, IterFunc func, void* pContext) {
    long res = 0;
    Format fmt(strFormat, count, true, "Type format error in add-in");
    long len;
    VarType type;
    
    while (fmt.Next(byref type, byref len)) {
        Value* pVal = deref(addr);
        if (type != pVal->type) {
            if (type == vtChar && pVal->type == vtInt) {
                // it's ok
            } else {
                vm_error("Type mismatch");
            }
        }
        if (!(*func)(pVal, type, len, pContext)) {
            goto done;
        } else {
            res++;
        }
        addr++;
    }
done:
    return res;
}

bool TypeSizeIterFunc(Value* pVal, VarType type, int len, void* pContext) {
    long* pRes = (long*)pContext;
    if (len) {
        (*pRes) += len;
    } else {
        assert(type == vtString);
        char* pStr = LockString(pVal);
        (*pRes) += strlen(pStr) + 1;
        UnlockString(pVal);
    }
    return true;
}

long TypeDataSize(long addr, char* strFormat, int count) {
    long res = 0;
    IterateValues(addr, strFormat, count, &TypeSizeIterFunc, &res);
    return res;
}

struct AIContext {
    NATIVE_TS_ITER func;
    void* context;
};

bool AddinIterator(Value* val, VarType type, int len, void* context) {
    AIContext* c = (AIContext*)context;
    return (*c->func)(&pci, val, type, len, c->context);
}

long ts_iterate(long addr, char* strFormat, int count, NATIVE_TS_ITER func, void* pContext) {
    AIContext c;
    c.context = pContext;
    c.func = func;
    return IterateValues(addr, strFormat, count, AddinIterator, &c);
}

long ts_data_size(long addr, char* strFormat, int count) {
    return TypeDataSize(addr, strFormat, count);
}

void InitPCI() {
    pci.version = CURRENT_VERSION;
    pci.vm_pop = vm_pop;
    pci.vm_push = vm_push;
    pci.vm_retVal = &retVal;
    pci.vm_error = vm_error;
    pci.vm_deref = vm_deref;
    pci.vm_call = vm_call;
    pci.vm_callBI = vm_callBI;
    pci.val_release = val_release;
    pci.val_acquire = val_acquire;
    pci.val_lock = val_lock;
    pci.val_unlock = val_unlock;
    pci.val_unlockRelease = val_unlockRelease;
    pci.val_newString = val_newString;
    pci.val_newConstString = val_newConstString;
    pci.val_copyString = val_copyString;
    pci.ts_iterate = ts_iterate;
    pci.ts_data_size = ts_data_size;
}
