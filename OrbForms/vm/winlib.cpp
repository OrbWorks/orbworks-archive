#include "vm.h"
#include <windows.h>

void lib_NYI(VM* vm, int) {
    vm->vm_error("NYI");
}

void lib_vector_call(VM* vm, int) {
    Value *f, *a, *s, *c;
    c = vm->pop();
    s = vm->pop();
    f = vm->pop();
    a = vm->pop();

    int addr = a->iVal;
    int count = c->iVal;
    int size = s->iVal;
    int func = f->iVal;
    
    int pc = vm->pc;
    int fb = vm->fb;
    int fp = vm->fp;

    if (addr == 0) {
        // this was not allocated to begin with.
        // @TODO: must do this is compiler, too!
        return;
    }
    
    if (count == 0xffff) {
        // get the count from heap info
        word bsize = vm->globs.getsize(addr);
        if (bsize == 0xffff)
            vm->vm_error("attempt to delete bad pointer");
        else if (bsize % size != 0)
            vm->vm_error("heap looks broken");
        if (vm->killVM)
            return;
        count = bsize / size;
    }
    
    while (count--) {
        Value t;
        t.type = vtInt;
        t.iVal = addr;
        vm->push(&t);
        vm->Call(func, true);
        addr += size;
    }
    
    vm->killVM = false;
    vm->pc = pc;
    vm->fp = fp;
    vm->fb = fb;
}

void lib_vector_bicall(VM* vm, int) {
}

void lib_vector_libcall(VM* vm, int) {
}

void lib_vector_virtcall(VM* vm, int) {
    Value *f, *a, *s, *c;
    c = vm->pop();
    s = vm->pop();
    f = vm->pop();
    a = vm->pop();

    int addr = a->iVal;
    int count = c->iVal;
    int size = s->iVal;
    int func = f->iVal;
    
    int pc = vm->pc;
    int fb = vm->fb;
    int fp = vm->fp;

    if (addr == 0) {
        // this was not allocated to begin with.
        // @TODO: must do this is compiler, too!
        return;
    }

    if (size == 0xffff) {
        // size will only be unknown for an array of the same type of
        // object - no mixed objects in the array
        int objID = vm->deref(addr)->iVal;
        size = vm->getObjectSize(objID);
    }
    
    if (count == 0xffff) {
        // get the count from heap info
        word bsize = vm->globs.getsize(addr);
        if (bsize == 0xffff)
            vm->vm_error("attempt to delete bad pointer");
        else if (bsize % size != 0)
            vm->vm_error("heap looks broken");
        if (vm->killVM)
            return;
        count = bsize / size;
    }
    
    while (count--) {
        Value t;
        t.type = vtInt;
        t.iVal = addr;
        vm->push(&t);

        int objID = vm->deref(addr)->iVal;
        int funcloc = vm->getVirtFunc(objID, func);
        vm->Call(funcloc, true);
        addr += size;
    }
    
    vm->killVM = false;
    vm->pc = pc;
    vm->fp = fp;
    vm->fb = fb;
}

void lib_malloct(VM* vm, int) {
    Value* size = vm->pop();
    Value* type = vm->pop();
    vm->retVal.iVal = vm->globs.alloc(size->iVal, type->sVal);
    //vm->gsize = vm->globs.size();
    vm->cleanUp(type);
}

void lib_memcpy(VM* vm, int) {
    Value* vLen = vm->pop();
    Value* vpSrc = vm->pop();
    Value* vpDst = vm->pop();
    
    long dst = vpDst->iVal;
    long src = vpSrc->iVal;
    long len = vLen->iVal;
    
    for (long i=0;i<len;i++) {
        Value* vSrc = vm->deref(src++);
        Value* vDst = vm->deref(dst++);
        //if (vSrc->type != vDst->type) {
        //	vm->vm_error("memcpy to a location of different type");
        //}
        vm->cleanUp(vDst);
        vm->copyVal(vDst,vSrc);
    }
}

void lib_puts(VM* vm, int) {
    Value* p = vm->pop();
    assert(p->type == vtString);
    printf("%s", p->sVal);
    vm->cleanUp(p);
}

void lib_alert(VM* vm, int) {
    Value* p = vm->pop();
    assert(p->type == vtString);
    MessageBox(NULL, p->sVal, "OrbForms", 0);
    vm->cleanUp(p);
}

void lib_ticks(VM* vm, int) {
    vm->retVal.type = vtInt;
    vm->retVal.iVal = GetTickCount();
}

void get_TestOnlyDouble_dbl(VM* vm, int) {
    Value* st = vm->pop();
    Value* single = vm->deref(st->iVal+1);
    assert(single->type == vtInt);
    vm->retVal.type = vtInt;
    vm->retVal.iVal = single->iVal * 2;
    vm->cleanUp(st);
}

void set_TestOnlyDouble_dbl(VM* vm, int) {
    Value* val = vm->pop();
    Value* st = vm->pop();
    Value* single = vm->deref(st->iVal+1);
    assert(single->type == vtInt);
    single->iVal = val->iVal / 2;
    vm->retVal.type = vtInt;
    vm->retVal.iVal = single->iVal * 2;
    vm->cleanUp(st);
}

int popID(VM* vm) {
    Value* vObjref = vm->pop();
    assert(vObjref->type == vtInt);
    Value* vID = vm->deref(vObjref->iVal);
    assert(vID->type == vtInt);
    return vID->iVal;
}



BuiltinFunc bifuncs[] = {
    { lib_NYI, false },
    { lib_vector_call, false },
    { lib_vector_bicall, false },
    { lib_vector_libcall, false },
    { get_TestOnlyDouble_dbl, true },
    { set_TestOnlyDouble_dbl, true },
    { lib_puts, false },
    { lib_alert, false },
    { lib_ticks, true },
};

int nbifuncs = sizeof(bifuncs) / sizeof(bifuncs[0]);
