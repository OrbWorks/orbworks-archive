#include "OrbFormsRT.h"

vector<OAddIn> addIns;

bool LoadAddIn(char* name, int id) {
    MemHandle hAddIn = NULL;
    NATIVE_ENTRY_FUNC func = NULL;
    LocalID lid = NULL;
    DmOpenRef dbRef = NULL;
    
    if (*name == '\0') {
        // unused library reference
        OAddIn addIn = {0,};
        addIns.add(addIn);
        return true;
    }
    
    if (id == 0) {
        // load addin database by name
        word card = 0;
        lid = MyFindDatabase(name, byref card);
        if (!lid) return false;
        dbRef = DmOpenDatabase(card, lid, dmModeReadOnly);
        if (!dbRef) return false;
        hAddIn = DmGetResource('ofNA', 0);
        if (!hAddIn) DmCloseDatabase(dbRef);
    } else {
        // load addin resource by id
        hAddIn = DmGetResource('ofNA', id);
    }
    
    if (hAddIn) {
        OAddIn addIn = {0,};
        addIn.func = (NATIVE_ENTRY_FUNC)MemHandleLock(hAddIn);
        addIn.hAddIn = hAddIn;
        addIn.dbRef = dbRef;
        addIn.bInit = false;
        addIns.add(addIn);
        return true;
    } else {
        return false;
    }
}

void* current_addin_data;
OrbFormsInterface ofi;

Value* vm_pop() { return vm->pop(); }
void vm_push(Value* v) { vm->push(v); }
void vm_error(char* err) { vm->vm_error(err); }
Value* vm_deref(long ptr) { return vm->deref(ptr); }
void vm_call(long ptr) { vm->Call(ptr, true, false); }

void vm_callBI(long index) {
    assert(index < nbifuncs);
    vm->retVal.type = vtInt;
    vm->retVal.iVal = 0;
    (*bifuncs[index].func)(vm, index);
    if (bifuncs[index].hasRet)
        vm->push(&vm->retVal);
}

void val_release(Value* v) { v->release(); }
void val_acquire(Value* d, Value* s) { d->acquire(s); }
char* val_lock(Value* v) { return v->lock(); }
void val_unlock(Value* v) { v->unlock(); }
void val_unlockRelease(Value* v) { v->unlockRelease(); }
char* val_newString(Value* v, int len) { return v->newString(len); }
char* val_newConstString(Value* v, const char* s) { return v->newConstString(s); }
bool val_copyString(Value* v, const char* s) { return v->copyString(s); }

void* nobj_pop() {
    AddInObject* ao = (AddInObject*)PopNO(vm);
    return ao->data;
}

void* nobj_get_data(long id) {
    AddInObject* ao = (AddInObject*)ppile->Find(id);
    return ao->data;
}

long nobj_create(void* obj, NOBJ_DELETE_FUNC delfunc) {
    AddInObject* ao = new AddInObject;
    ao->delFunc = delfunc;
    ao->data = obj;
    ao->aidata = current_addin_data;
    nobject_init(vm, ao);
    return 0;
}

long nobj_addref(long id) {
    AddInObject* ao = (AddInObject*)ppile->Find(id);
    return ao->AddRef(true);
}

long nobj_delref(long id) {
    AddInObject* ao = (AddInObject*)ppile->Find(id);
    return ao->Release(true);
}

void* gadget_get_data(long id) {
    int index = FindCtrl(id);
    if (index == -1)
        vm->vm_error("gadget not found");
    assert(ctrls[index].type == rtGadget);
    return ctrls[index].pdata;
}

void gadget_set_data(long id, void* data) {
    int index = FindCtrl(id);
    if (index == -1)
        vm->vm_error("gadget not found");
    assert(ctrls[index].type == rtGadget);
    ctrls[index].pdata = data;
}

void gadget_get_bounds(long id, int* pX, int* pY, int* pW, int* pH) {
    int index = FindCtrl(id);
    if (index == -1)
        vm->vm_error("gadget not found");
    assert(ctrls[index].type == rtGadget);
    *pX = ctrls[index].x;
    *pY = ctrls[index].y;
    *pW = ctrls[index].w;
    *pH = ctrls[index].h;
}

bool reg_event_func(NATIVE_EVENT_FUNC func) {
    OEventFunc oefunc;
    oefunc.func = func;
    oefunc.data = current_addin_data;
    eventFuncs.add(oefunc);
    return true;
}

bool unreg_event_func(NATIVE_EVENT_FUNC func) {
    for (int i=0;i<eventFuncs.size();i++) {
        if (eventFuncs[i].func == func) {
            eventFuncs.erase(i);
            return true;
        }
    }
    return false;
}

struct AIContext {
    NATIVE_TS_ITER func;
    void* context;
};

bool AddinIterator(Value* val, VarType type, int len, void* context) {
    AIContext* c = (AIContext*)context;
    return (*c->func)(&ofi, val, type, len, c->context);
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

AddInObject::~AddInObject() {
    if (delFunc)
        (*delFunc)(&ofi, aidata, data);
}

void InitOFI() {
    ofi.version = appVersionNum;
    ofi.vm_pop = vm_pop;
    ofi.vm_push = vm_push;
    ofi.vm_retVal = &vm->retVal;
    ofi.vm_error = vm_error;
    ofi.vm_deref = vm_deref;
    ofi.vm_call = vm_call;
    ofi.vm_callBI = vm_callBI;
    
    ofi.val_release = val_release;
    ofi.val_acquire = val_acquire;
    ofi.val_lock = val_lock;
    ofi.val_unlock = val_unlock;
    ofi.val_unlockRelease = val_unlockRelease;
    ofi.val_newString = val_newString;
    ofi.val_newConstString = val_newConstString;
    ofi.val_copyString = val_copyString;
    
    ofi.nobj_pop = nobj_pop;
    ofi.nobj_get_data = nobj_get_data;
    ofi.nobj_create = nobj_create;
    ofi.nobj_addref = nobj_addref;
    ofi.nobj_delref = nobj_delref;
    
    ofi.gadget_get_data = gadget_get_data;
    ofi.gadget_set_data = gadget_set_data;
    ofi.gadget_get_bounds = gadget_get_bounds;
    
    ofi.event_reg_func = reg_event_func;
    ofi.event_unreg_func = unreg_event_func;
    
    ofi.ts_iterate = ts_iterate;
    ofi.ts_data_size = ts_data_size;
}
