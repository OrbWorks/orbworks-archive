#include <PalmOS.h>
#include "OrbNative.h"

extern "C" {
void* __Startup__(OrbFormsInterface*, void*, UInt16);
}

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

// called by compiler
extern "C" 
void *__copy(char *to,char *from,unsigned long size) {
    char *f,*t;

    if(to) for(f=(char *)from,t=(char *)to; size>0; size--) *t++=*f++;
    return to;
}

struct TargetGadget {
    int x, y, w, h;
    int last_x, last_y;
    bool isPenDown;
};

void TG_onopen(OrbFormsInterface* ofi) {
    // the only parameter to onopen is the object pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve the bounds for future use
    TargetGadget* tg = new TargetGadget;
    ofi->gadget_get_bounds(obj->iVal, &tg->x, &tg->y, &tg->w, &tg->h);
    tg->last_x = -1;
    tg->isPenDown = false;
    ofi->gadget_set_data(obj->iVal, tg);
}

void TG_onclose(OrbFormsInterface* ofi) {
    // the only parameter to onclose is the object pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // cleanup our data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);
    delete tg;
    ofi->gadget_set_data(obj->iVal, NULL);
}

void TG_draw(TargetGadget* tg) {
    // draw the outside box
    RectangleType rect;
    RctSetRectangle(&rect, tg->x+1, tg->y+1, tg->w-1, tg->h-1);

    FrameBitsType fb;
    fb.word = 0;
    fb.bits.cornerDiam = 0;
    fb.bits.width = 1;

    WinDrawRectangleFrame(fb.word, &rect);
    
    // cleanup the inside
    WinEraseRectangle(&rect, 0);
    
    // if pen is down, draw the lines inside
    if (tg->isPenDown) {
        WinDrawLine(tg->x + 0,     tg->y + 0,     tg->last_x, tg->last_y);
        WinDrawLine(tg->x + tg->w, tg->y + 0,     tg->last_x, tg->last_y);
        WinDrawLine(tg->x + 0,     tg->y + tg->h, tg->last_x, tg->last_y);
        WinDrawLine(tg->x + tg->w, tg->y + tg->h, tg->last_x, tg->last_y);
    }
}

void TG_ondraw(OrbFormsInterface* ofi) {
    // the only parameter to ondraw is the object pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve our stored data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);
    
    TG_draw(tg);
}

void TG_onpendown(OrbFormsInterface* ofi) {
    // the only parameter to ondraw is the object pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve our stored data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);

    tg->isPenDown = true;
    
    // since Event has not members, retrieving the properties does not require
    // a pointer to a real object. So, just push 0
    Value val;
    val.type = vtInt;
    val.iVal = 0;
    
    // get event.x
    ofi->vm_push(&val);
    ofi->vm_callBI(46);
    tg->last_x = ofi->vm_pop()->iVal + tg->x; // relative to gadget

    // get event.y
    ofi->vm_push(&val);
    ofi->vm_callBI(47);
    tg->last_y = ofi->vm_pop()->iVal + tg->y; // relative to gadget
    
    TG_draw(tg);
    
    if (tg->last_x < tg->x + 5) {
        // call the onleft handler if it was specified
        // the TargetGadget has the following layout:
        //   UIGadget gadget;
        //   handler onleft;
        // since the UIGadget has only one field (id), we know that the onleft handler
        // is at offset 1 from the beginning of the object
        Value* onleft = ofi->vm_deref(pobj + 1);
        if (onleft->iVal) {
            // onleft was specified, so call it
            // first, push the object address
            Value addr;
            addr.type = vtInt;
            addr.iVal = pobj;
            ofi->vm_push(&addr);
            ofi->vm_call(onleft->iVal);
            // there is no return value, so nothing to pop
        }
    }
}

void TG_onpenup(OrbFormsInterface* ofi) {
    // the only parameter to ondraw is the object pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve our stored data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);

    tg->isPenDown = false;
    
    TG_draw(tg);
}

void TG_onpenmove(OrbFormsInterface* ofi) {
    // the only parameter to ondraw is the object pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve our stored data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);

    // since Event has not members, retrieving the properties does not require
    // a pointer to a real object. So, just push 0
    Value val;
    val.type = vtInt;
    val.iVal = 0;
    
    // get event.x
    ofi->vm_push(&val);
    ofi->vm_callBI(46);
    tg->last_x = ofi->vm_pop()->iVal + tg->x; // relative to gadget

    // get event.y
    ofi->vm_push(&val);
    ofi->vm_callBI(47);
    tg->last_y = ofi->vm_pop()->iVal + tg->y; // relative to gadget
    
    TG_draw(tg);
}

void add(OrbFormsInterface* ofi) {
    Value* x = ofi->vm_pop();
    Value* y = ofi->vm_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = x->iVal + y->iVal;
}

void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
{
    if (iFunc == NATIVE_STARTUP)
        return NULL;
    else if (iFunc == NATIVE_SHUTDOWN)
        return NULL;
    else if (iFunc == 0)
        add(ofi);
    else if (iFunc == 1)
        TG_onopen(ofi);
    else if (iFunc == 2)
        TG_onclose(ofi);
    else if (iFunc == 3)
        TG_ondraw(ofi);
    else if (iFunc == 4)
        TG_onpendown(ofi);
    else if (iFunc == 5)
        TG_onpenup(ofi);
    else if (iFunc == 6)
        TG_onpenmove(ofi);

    return NULL;
}
