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

//
// TargetGadget
//
// A native gadget
//

struct TargetGadget {
    int x, y, w, h;
    int last_x, last_y;
    bool isPenDown;
};

void TG_onopen(OrbFormsInterface* ofi) {
    // the only parameter to onopen is the struct pointer
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
    // the only parameter to onclose is the struct pointer
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
    // the only parameter to ondraw is the struct pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve our stored data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);
    
    TG_draw(tg);
}

void TG_onpendown(OrbFormsInterface* ofi) {
    // the only parameter to ondraw is the struct pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve our stored data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);

    tg->isPenDown = true;
    
    // since Event has not members, retrieving the properties does not require
    // a pointer to a real struct. So, just push 0
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
        // is at offset 1 from the beginning of the struct
        Value* onleft = ofi->vm_deref(pobj + 1);
        if (onleft->iVal) {
            // onleft was specified, so call it
            // first, push the struct address
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
    // the only parameter to ondraw is the struct pointer
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
    // the only parameter to ondraw is the struct pointer
    // pop the pointer
    long pobj = ofi->vm_pop()->iVal;
    // dereference the pointer to access the first field (the id)
    Value* obj = ofi->vm_deref(pobj);
    
    // retrieve our stored data
    TargetGadget* tg = (TargetGadget*)ofi->gadget_get_data(obj->iVal);

    // since Event has not members, retrieving the properties does not require
    // a pointer to a real struct. So, just push 0
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

bool my_event_func(OrbFormsInterface* ofi, void* data, EventType* event) {
    // this custom event function intercepts the contrast button
    if (event->eType == keyDownEvent &&
        event->data.keyDown.chr == vchrContrast && 
        (event->data.keyDown.modifiers & commandKeyMask))
    {
        // call alert("You pressed contrast button");
        Value str;
        ofi->val_newConstString(&str, "You pressed the contrast button");
        ofi->vm_push(&str);
        ofi->vm_callBI(7);
        return true; // we handled the event
    }
    return false; // we did not handle the event
}

// bool hookcontrast(bool)
void hook_contrast(OrbFormsInterface* ofi) {
    Value* v = ofi->vm_pop();
    ofi->vm_retVal->type = vtInt;
    if (v->iVal) {
        // register our custom event function
        ofi->vm_retVal->iVal = ofi->event_reg_func(my_event_func);
    } else {
        // unregister our custom event function
        ofi->vm_retVal->iVal = ofi->event_unreg_func(my_event_func);
    }
}

// int add(int x, int y)
void add(OrbFormsInterface* ofi) {
    Value* y = ofi->vm_pop();
    Value* x = ofi->vm_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = x->iVal + y->iVal;
}

// struct SampleSerial {
//   int id;
//   void _init() @ ...;
//   bool open(int baud) @ ...;
//   void close() @ ...;
//   void send(char byte) @ ...;
//   byte recv() @ ...;
// };

struct SerialData {
    bool bOpen;
    UInt16 portId;
};

void Serial_destroy(OrbFormsInterface* ofi, void* data, void* obj) {
    SerialData* sd = (SerialData*)obj;
    // make sure it is already closed
    if (sd->bOpen) {
        SrmClose(sd->portId);
    }
    delete sd;
}

void Serial_init(OrbFormsInterface* ofi) {
    SerialData* sd = new SerialData;
    sd->bOpen = false;
    sd->portId = 0;
    ofi->nobj_create(sd, Serial_destroy);
}

void Serial_open(OrbFormsInterface* ofi) {
    UInt32 baud = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;

    if (!sd->bOpen) {
        Err err = SrmOpen(serPortCradlePort, baud, &sd->portId);
        if (err == errNone) {
            sd->bOpen = true;
            ofi->vm_retVal->iVal = 1;
        }
    }
}

void Serial_close(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();

    if (sd->bOpen) {
        SrmClose(sd->portId);
        sd->bOpen = false;
    }
}

void Serial_send(OrbFormsInterface* ofi) {
    char byte = ofi->vm_pop()->cVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();

    if (sd->bOpen) {
        Err err;
        SrmSend(sd->portId, &byte, 1, &err);
    }
}

void Serial_recv(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtChar;
    ofi->vm_retVal->cVal = 0;

    if (sd->bOpen) {
        Err err;
        char byte = 0;
        SrmReceive(sd->portId, &byte, 1, 100, &err);
        ofi->vm_retVal->cVal = byte;
    }
}

void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
{
    // we don't need to setup any global state
    if (iFunc == NATIVE_STARTUP) return NULL;
    if (iFunc == NATIVE_SHUTDOWN) return NULL;

    switch (iFunc) {
        case 0: add(ofi); break;
        case 1: TG_onopen(ofi); break;
        case 2: TG_onclose(ofi); break;
        case 3: TG_ondraw(ofi); break;
        case 4: TG_onpendown(ofi); break;
        case 5: TG_onpenup(ofi); break;
        case 6: TG_onpenmove(ofi); break;
        
        case 7: Serial_init(ofi); break;
        case 8: Serial_open(ofi); break;
        case 9: Serial_close(ofi); break;
        case 10: Serial_send(ofi); break;
        case 11: Serial_recv(ofi); break;
        
        case 12: hook_contrast(ofi); break;
    }

    return NULL;
}
