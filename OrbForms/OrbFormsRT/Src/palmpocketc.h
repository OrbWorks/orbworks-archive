//
// Basic I/O
//
#define FInputDialog	12100
#define FInputField		12101
#define FInputOK		12102

extern DIAtype diaType;

void lib_getsm(VM* vm, int) {
    Value* vDef = vm->pop();
    word nLines = vm->pop()->iVal;
    short w = vm->pop()->iVal;
    short y = vm->pop()->iVal;
    short x = vm->pop()->iVal;

    // create the form
    FormPtr pForm = FrmNewForm(FInputDialog, NULL, x, y, w + 22, nLines * 12 + 2,
        true, FInputOK, 0, 0);
    FieldPtr pField = FldNewField((void**)&pForm, FInputField, 2, 1, w, nLines * 12,
        stdFont, maxTextInput - 1, true, true, nLines == 1, false,
        leftAlign, true, false, false);
    ControlPtr pOK = CtlNewControl((void**)&pForm, FInputOK, buttonCtl, "OK", w + 4, 2, 16, 10,
        stdFont, 0, true);
        
    // set default text
    pField = (FieldPtr)FrmGetObjectPtr(pForm, FrmGetObjectIndex(pForm, FInputField));
    MemHandle hText = h_malloc(maxTextInput);
    char* def = (char*)MemHandleLock(hText);
    strncpy(def, vDef->lock(), maxTextInput - 1);
    def[maxTextInput - 1] = '\0';
    MemHandleUnlock(hText);
    vDef->unlockRelease();
    FldSetTextHandle(pField, hText);

    FormActiveStateType	curFrmState;
    MenuEraseStatus(0);
    FrmSaveActiveState(&curFrmState);
    FrmSetActiveForm(pForm);
    FrmSetFocus(pForm, FrmGetObjectIndex (pForm, FInputField));
    FrmDrawForm(pForm);
    FrmDoDialog(pForm);
    
    char* str = FldGetTextPtr(pField);
    vm->retVal.copyString(str);
    
    FrmDeleteForm(pForm);
    FrmRestoreActiveState(&curFrmState);
    vm->killVM = false;
}

//
// Graphics
//

void lib_graph_on_internal(VM* vm, NativeDraw* draw) {
    FrmGotoForm(GraphForm);
    UIYield();
    draw->begin(false, true);
}

void lib_graph_on(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    lib_graph_on_internal(vm, draw);
}

void lib_graph_off(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->end();
    FrmGotoForm(OutputForm);
    UIYield();
}

void lib_clearg(VM* vm, int) {
    if (FrmGetActiveFormID()==GraphForm) {
        RectangleType rect;
        // todo: high res
        RctSetRectangle(&rect, 0, 0, 160, 160);
        WinEraseRectangle(&rect, 0);
        FrmDrawForm(FrmGetActiveForm());
    }
}

void lib_title(VM* vm, int) {
    static char tit[80];
    Value* vTitle = vm->pop();
    strncpy(tit, vTitle->lock(), 79);
    tit[79] = '\0';
    vTitle->unlockRelease();
    FrmSetTitle(FrmGetActiveForm(), tit);
}

#define MAX_SAVE_BITS 3
WinHandle saveBits[MAX_SAVE_BITS];
short nSaveBits;
void lib_saveg(VM* vm, int) {
    Err err;
    RectangleType rect;
    
    if (nSaveBits == MAX_SAVE_BITS) {
        WinDeleteWindow(saveBits[0], 0);
        for (short i=0;i<MAX_SAVE_BITS-1;i++)
            saveBits[i] = saveBits[i+1];
        nSaveBits--;
    }

    RctSetRectangle(&rect, 0, 0, 160, 160);

    saveBits[nSaveBits] = WinSaveBits(&rect, &err);
    if (saveBits[nSaveBits]) vm->retVal.iVal = 1;
    nSaveBits++;
}

void lib_restoreg(VM* vm, int) {
    if (!nSaveBits) return;
    WinRestoreBits(saveBits[--nSaveBits], 0, 0);
}

int nPushDraw = 0;
void lib_pushdraw(VM* vm, int) {
    if (romVersion >= ver35) WinPushDrawState();
    nPushDraw++;
}

void lib_popdraw(VM* vm, int) {
    if (romVersion >= ver35) WinPopDrawState();
    nPushDraw--;
}

void lib_pctext(VM* vm, int) {
    Value* str = vm->pop();
    Value* y = vm->pop();
    Value* x = vm->pop();
    Value* c = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->text(c->iVal, x->iVal, y->iVal, str->lock(), true);
    str->unlockRelease();
}

char hval(char c) {
    if (isdigit(c)) return c-'0';
    if (c >='A' && c <= 'F') c+=32;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

typedef struct MyBitmapFlagsType
{
    UInt16 	compressed:1;  				// Data format:  0=raw; 1=compressed
    UInt16 	hasColorTable:1;				// if true, color table stored before bits[]
    UInt16 	hasTransparency:1;			// true if transparency is used
    UInt16 	indirect:1;						// true if bits are stored indirectly
    UInt16 	forScreen:1;					// system use only
    UInt16	directColor:1;					// direct color bitmap
    UInt16 	reserved:10;
}
MyBitmapFlagsType;
    
// this definition correspond to the 'Tbmp' and 'tAIB' resource types
typedef struct MyBitmapType
{
    Int16				width;
    Int16				height;
    UInt16				rowBytes;
    MyBitmapFlagsType	flags;
    UInt8				pixelSize;			// bits/pixel
    UInt8				version;			// version of bitmap. This is vers 2
    UInt16				nextDepthOffset;	// # of DWords to next BitmapType
                                            //  from beginnning of this one
    UInt8				transparentIndex;	// v2 only, if flags.hasTransparency is true,
                                            // index number of transparent color
    UInt8				compressionType;	// v2 only, if flags.compressed is true, this is
                                            // the type, see BitmapCompressionType
                                                        
    UInt16	 			reserved;			// for future use, must be zero!
    
    // if (flags.hasColorTable)
    //	  ColorTableType	colorTable		// NOTE: Could have 0 entries (2 bytes long)
    //
    // if (flags.directColor)
    //	  BitmapDirectInfoType	directInfo;
    // 
    // if (flags.indirect)
    //	  void*	  bitsP;						// pointer to actual bits
    // else
    //    UInt8	  bits[];					// or actual bits
    //
}
MyBitmapType;

#define BitmapType MyBitmapType
#define BitmapPtr MyBitmapType*

void lib_bitmap(VM* vm, int) {
    Value* hbits = vm->pop();
    Value* y = vm->pop();
    Value* x = vm->pop();
    
    char* bits = hbits->lock();
    char* data;
    short w,h,ci;
    
    if (strlen(bits) < 3) goto error;
    w = (hval(*bits) * 16 + hval(bits[1])); // Bits
    if (!w) goto error;
    h = (strlen(bits) - 2) / ((w+3)/4);
    
    short rowBytes = (((w+15) >> 4) << 1);
    short size = sizeof(BitmapType) + (h+2) * rowBytes;
    
    BitmapPtr bp = (BitmapPtr)malloc(size);
    if (!bp) {
        vm->vm_error("Not enough memory to draw bitmap");
        goto cleanup;
    }
    MemSet(bp, size, 0);
    bp->width = w;
    bp->height = h;
    bp->rowBytes = rowBytes;
    bp->flags.compressed = false;
    bp->pixelSize = 1;
    bp->version = 1;
    data = (char*)bp + sizeof(BitmapType);
    
    bits+=2; ci=0;
    for (short i=0;i<bp->rowBytes * h;i++) {
        data[i] = hval(*bits++) << 4; ci++;
        if (ci < (w+3)/4) data[i] |= hval(*bits++); ci++;
        if (ci >= (w+3)/4) {
            ci=0;
            if (!(i%2)) i++; // word-aligning
        }
    }
#undef BitmapPtr
    WinDrawBitmap((BitmapPtr)bp, x->iVal, y->iVal);
    free(bp);
cleanup:
    hbits->unlockRelease();
    return;
error:
    vm->vm_error("Invalid bitmap");
    goto cleanup;
}

void lib_drawnative(VM* vm, int) {
    bool bNative = vm->pop()->iVal;
    if (bHighDensity) {
        WinSetCoordinateSystem(bNative ? 0 : kCoordinatesStandard);
    }
}

//
// offscreen buffer support
//

#define MAX_OFFSCREEN 8
WinHandle offScreen[MAX_OFFSCREEN];

// buffer 0 implies screen
// buffer -1 returned on error
// buffer 1-MAX_OFFSCREEN is offScreen[i-1]
void lib_bucreate(VM* vm, int) {
    word h = vm->pop()->iVal;
    word w = vm->pop()->iVal;
    
    // find an empty slot
    short i;
    for (i=0;i<MAX_OFFSCREEN;i++) {
        if (offScreen[i] == NULL) break;
    }
    if (i < MAX_OFFSCREEN) {
        UInt16 err;
        offScreen[i] = WinCreateOffscreenWindow(w, h, nativeFormat, &err);
        if (offScreen[i]) {
            if (romVersion < ver35) {
                RectangleType rect;
                RctSetRectangle(&rect, 0, 0, w, h);
                WinHandle hPrevWnd = WinSetDrawWindow(offScreen[i]);
                WinEraseRectangle(&rect, 0);
                WinSetDrawWindow(hPrevWnd);
            }
            vm->retVal.iVal = i + 1;
        } else
            vm->retVal.iVal = 0;
        
    } else {
        // no free buffers
        vm->retVal.iVal = 0;
    }
}

WinHandle getBufferHandle(long id, bool allowDisplay) {
    if (id > 0 && id <= MAX_OFFSCREEN) {
        return offScreen[id - 1];
    } else if (id == 0 && allowDisplay) {
        return WinGetDisplayWindow();
    }
    return NULL;
}

void lib_budelete(VM* vm, int) {
    word id = vm->pop()->iVal;
    
    WinHandle win = getBufferHandle(id, false);
    if (win) {
        WinDeleteWindow(win, false);
        offScreen[id - 1] = NULL;
    }
}

void lib_buset(VM* vm, int) {
    word id = vm->pop()->iVal;
    
    WinHandle win = getBufferHandle(id, false);
    if (win == NULL) win = WinGetDisplayWindow();
    WinSetDrawWindow(win);
}

void lib_bucopyrect(VM* vm, int) {
    word mode = vm->pop()->iVal;
    word yd = vm->pop()->iVal;
    word xd = vm->pop()->iVal;
    word did = vm->pop()->iVal;
    word h = vm->pop()->iVal;
    word w = vm->pop()->iVal;
    word ys = vm->pop()->iVal;
    word xs = vm->pop()->iVal;
    word sid = vm->pop()->iVal;
    
    WinHandle src = getBufferHandle(sid, true);
    WinHandle dst = getBufferHandle(did, true);
    if (src == NULL || dst == NULL) return;
    
    RectangleType rect;
    RctSetRectangle(&rect, xs, ys, w, h);
    WinCopyRectangle(src, dst, &rect, xd, yd, (WinDrawOperation)mode);
}

void lib_bucopy(VM* vm, int) {
    word mode = vm->pop()->iVal;
    word y = vm->pop()->iVal;
    word x = vm->pop()->iVal;
    word did = vm->pop()->iVal;
    word sid = vm->pop()->iVal;
    
    WinHandle src = getBufferHandle(sid, true);
    WinHandle dst = getBufferHandle(did, true);
    if (src == NULL || dst == NULL) return;
    
    Coord w, h;
    WinHandle prev = WinSetDrawWindow(dst);
    WinGetWindowExtent(&w, &h);
    RectangleType rect;
    RctSetRectangle(&rect, 0, 0, w, h);
    WinCopyRectangle(src, dst, &rect, x, y, (WinDrawOperation)mode);
}

void lib_bitmaprm(VM* vm, int) {
    word mode = vm->pop()->iVal;
    word y = vm->pop()->iVal;
    word x = vm->pop()->iVal;
    word id = vm->pop()->iVal;
    if (romVersion >= ver35) {
        MemHandle hMem = DmGetResource('Tbmp', id);
        if (hMem) {
            WinDrawOperation dop = WinSetDrawMode((WinDrawOperation)mode);
            WinPaintBitmap((BitmapPtr)MemHandleLock(hMem), x, y);
            WinSetDrawMode(dop);
            MemHandleUnlock(hMem);
        }
    }
}

//
// Eventing
//

void EventQueue::AddEvent(short type, short v1, short v2) {
    // don't add duplicates for some events
    if (nEvents > 0) {
        if (events[nEvents-1].type == eqNone) {
            nEvents--; // get rid of the nil event if another event is coming in
        }
    }
    
    // if this is resize, remove any pending resize or update events
    if (type == eqResize && nEvents) {
        for (short i=0; i<nEvents; i++) {
            if (events[i].type == eqResize || events[i].type == eqUpdate) {
                if (i != nEvents-1) {
                    memcpy(&events[i], &events[i+1], (nEvents - i - 1) * sizeof(EventItem));
                }
                nEvents--;
                break;
            }
        }
    }
    
    // queue is full
    if (nEvents == maxEvents) {
        short iEntry = 0;
        // if the top event is the same as the new event, but not a key event
        // just replace it
        if (events[nEvents-1].type == type) {
            if (type != eqKey) {
                events[nEvents-1].v1 = v1;
                events[nEvents-1].v2 = v2;
                return;
            }
        }
        
        if (type == eqPenMove) return;
        
        for (short i=0; i<nEvents-1; i++) {
            if (events[i].type == events[i+1].type && events[i].type != eqKey) {
                iEntry = i;
                break;
            }
            if (events[i].type == eqPenMove) iEntry = i; // remove a pen move if nothing else is found
        }

        if (iEntry < nEvents-1) {		
            memcpy(&events[iEntry], &events[iEntry+1], (nEvents - iEntry - 1) * sizeof(EventItem));
        }
    } else {
        nEvents++;
    }
        
    events[nEvents-1].type = type;
    events[nEvents-1].v1 = v1;
    events[nEvents-1].v2 = v2;
}

bool EventQueue::PopEvent() {
    if (nEvents) {
        type = events[0].type;
        v1 = events[0].v1;
        v2 = events[0].v2;
        if (type == eqResize) {
            // replace with a redraw event
            events[0].type = eqUpdate;
            events[0].v1 = events[0].v2 = 0;
        } else {
            memcpy(&events[0], &events[1], (nEvents - 1) * sizeof(EventItem));
            nEvents--;
        }
        return true;
    }
    return false;
}

void EventQueue::Clear() {
    nEvents = 0;
    type = v1 = v2 = 0;
}

void EventQueue::RemoveInput() {
    short i = nEvents;
    while (i--) {
        if (events[i].type == eqPenUp || events[i].type == eqPenDown || events[i].type == eqKey) {
            if (i != nEvents - 1)
                memcpy(&events[i], &events[i+1], (nEvents - i - 1) * sizeof(events[0]));
            nEvents--;
        }
    }
}

short penx=0, peny=0, npenx=0,npeny=0;
extern UInt32 density;
extern EventQueue eventQueue;
extern EventType lastEvent;
#define RetInt(x) vm->retVal.iVal = x

void lib_wait(VM* vm, int) {
    // Wait for either a pen or key message
    eventQueue.RemoveInput();
tryAgain:
    do {
        while (!eventQueue.PopEvent())
            PocketCYield(true);
    } while (eventQueue.type != EventQueue::eqPenUp && eventQueue.type != EventQueue::eqKey);
    if (eventQueue.type == EventQueue::eqPenUp) {
        RetInt(-1);
        if (lastEvent.screenY > 159) goto tryAgain;
        penx = eventQueue.v1;
        peny = eventQueue.v2;
        npenx = penx * density / 72;
        npeny = peny * density / 72;
    } else {
        vm->retVal.type = vtChar;
        vm->retVal.cVal = eventQueue.v1;
    }
}

void lib_waitp(VM* vm, int) {
    // Wait for a pen message
    eventQueue.RemoveInput();
    do {
        while (!eventQueue.PopEvent())
            PocketCYield(true);
    } while (eventQueue.type != EventQueue::eqPenUp || eventQueue.v2 > 159);
    penx = eventQueue.v1;
    peny = eventQueue.v2;
    npenx = penx * density / 72;
    npeny = peny * density / 72;
}

void lib_getc(VM* vm, int) {
    // Wait for a key message
    do {
        while (!eventQueue.PopEvent())
            PocketCYield(true);
    } while (eventQueue.type != EventQueue::eqKey);
    vm->retVal.type = vtChar;
    vm->retVal.cVal = eventQueue.v1;
}

void lib_pstate(VM* vm, int) {
    short x, y;
    Boolean b;
    EvtGetPen(&x, &y, &b);
    penx=x; peny=y;
    npenx = penx * density / 72;
    npeny = peny * density / 72;
    RetInt(b);
}

short lastkey;
void lib_pcevent(VM* vm, int) {
    Value* time = vm->pop();
    if (!eventQueue.PopEvent()) {
        PocketCYield(time->iVal);
        if (!eventQueue.PopEvent()) {
            return;
        }
    }
    if (eventQueue.type == EventQueue::eqKey) lastkey = eventQueue.v1;
    else if (eventQueue.type >= EventQueue::eqPenDown && eventQueue.type <= EventQueue::eqPenMove) {
        penx = eventQueue.v1;
        peny = eventQueue.v2;
        npenx = penx * density / 72;
        npeny = peny * density / 72;
    } else if (eventQueue.type == EventQueue::eqResize) {
        penx = eventQueue.v1;
        peny = eventQueue.v2;
    }
    RetInt(eventQueue.type);
}

extern bool bHookHard, bHookMenu, bHookSync, bHookSilk;
void lib_hookmenu(VM* vm, int) {
    bool hook = vm->pop()->iVal;
    bHookMenu = hook;
}

void lib_hooksilk(VM* vm, int) {
    bool hook = vm->pop()->iVal;
    bHookSilk = hook;
}

void lib_hooksync(VM* vm, int) {
    bool hook = vm->pop()->iVal;
    bHookSync = hook;
}

void lib_key(VM* vm, int) {
    vm->retVal.type = vtChar;
    vm->retVal.cVal = lastkey;
}

void lib_penx(VM* vm, int) {
    RetInt(penx); 
}

void lib_peny(VM* vm, int) {
    RetInt(peny);
}

void lib_npenx(VM* vm, int) {
    RetInt(npenx); 
}

void lib_npeny(VM* vm, int) {
    RetInt(npeny);
}

void lib_enableresize(VM* vm, int) {
    enableResize = true;
}


//
// System
//

long atexit_func;
void lib_atexit(VM* vm, int) {
    long ptr = vm->pop()->iVal;
    
    if (ptr < 0 || ptr > CODE_SEGMENT_SIZE * MAX_CODE_SEGMENTS)
        vm->vm_error("Invalid function pointer in atexit()");
    atexit_func = ptr;
}


//
// Native libraries
//
#define VMCTRL_ENABLE_EVENTS 0

// Library routines
enum PCStrType { pcstClassic, pcstIndirect, pcstConst };

struct PCString {
    PCStrType sType;
    unsigned short nRef;
    char data[1];
};

struct PocketCLibraryApiExt {
    char* (* lockString)(Value* v);
    void  (* unlockString)(Value* v);
    void  (* acquireString)(Value* d, Value* s);
    void  (* releaseString)(Value* v);
    void  (* unlockReleaseString)(Value* v);
    char* (* newString)(Value* v, unsigned short len);
    bool  (* newStringFromConst)(Value* v, const char* data);
    void  (* newConstString)(Value* v, const char* data);
};

struct PocketCLibGlobalsType {
    void   (* push)(Value&);
    void   (* pop)(Value&);
    void   (* cleanup)(Value&);
    void   (* typeCast)(Value&, VarType);
    void   (* typeMatch)(Value&, Value&);
    bool   (* UIYield)(bool);
    short  (* addLibFunc)(char* name, short nArgs, VarType arg1, VarType arg2, VarType arg3, VarType arg4, VarType arg5, VarType arg6, VarType arg7, VarType arg8, VarType arg9, VarType arg10);
    void   (* callFunc_deprecated)(unsigned short loc);
    Value* retVal;
    Value* (* deref)(short ptr);
    bool   (* callBI)(char* name);
    char*  (* lockString_deprecated)(MemHandle sVal);
    void   (* unlockString_deprecated)(MemHandle sVal);
    UInt32 (* vmCtrl)(UInt32 id, UInt32 val);
    void   (* callFunc32)(long loc);
    PocketCLibraryApiExt* apiExt;
    UInt32 reserved;
};
typedef PocketCLibGlobalsType* PocketCLibGlobalsPtr;

PocketCLibGlobalsPtr PocketCLibOpen(word refNum, void*) SYS_TRAP(sysLibTrapOpen);
Err PocketCLibClose(word refNum, dword) SYS_TRAP(sysLibTrapClose);
Err PocketCLibExecute(word refNum, short funcNum) SYS_TRAP(sysLibTrapCustom+1);

void pc_valOldPCToOrb(Value& val) {
    if (val.type == vtChar) {
        val.cVal = val._.pad[0]; // move the character to the new location
    } else if (val.type == vtString) {
        MemHandle h = (MemHandle)val.sVal;
        val.sVal = NULL;
        char* str = (char*)MemHandleLock(h);
        val.copyString(str);
        MemHandleUnlock((MemHandle)h);
    }
}

void pc_valOrbToOldPC(Value& val) {
    if (val.type == vtChar) {
        val._.pad[0] = val.cVal;
    } else if (val.type == vtString) {
        char* str = val.lock();
        MemHandle hcstr = MemHandleNew(strlen(str) + 1);
        char* cstr = (char*)MemHandleLock(hcstr);
        strcpy(cstr, str);
        MemHandleUnlock(hcstr);
        val.unlockRelease();
        val.sVal = (void*)hcstr;
    }	
}

void pc_valNewPCToOrb(Value& val) {
    if (val.type == vtString) {
        //if (val.stype == pcstConst) {
        if (((dword)val.sVal & 0x80000000) == 0) { // horrible!!!
            val.stype = stConst;
        } else {
            MemHandle hStr = (MemHandle)val.sVal;
            PCString* pStr = (PCString*)MemHandleLock(hStr);
            val.sVal = NULL;
            val.copyString(pStr->data);
            if (--pStr->nRef == 0) {
                MemHandleUnlock(hStr);
                MemHandleFree(hStr);
            } else {
                MemHandleUnlock(hStr);
            }
        }
    }
}

void pc_valOrbToNewPC(Value& val) {
    if (val.type == vtString) {
        if (val.stype == stConst) {
            val.stype = (StringType)pcstConst;
        } else {
            char* str = val.lock();
            MemHandle hStr = MemHandleNew(sizeof(PCString) + strlen(str));
            PCString* pStr = (PCString*)MemHandleLock(hStr);
            pStr->nRef = 1;
            pStr->sType = pcstIndirect;
            strcpy(str, pStr->data);
            MemHandleUnlock(hStr);
            val.unlockRelease();
            val.sVal = hStr;
            val.stype = (StringType)pcstIndirect;
        }
    }
}

void pc_push(Value& val) {
    Value lval = val;
    pc_valOldPCToOrb(lval);
    vm->push(&lval);
}

void pc_pop(Value& val) {
    Value* pVal = vm->pop();
    val = *pVal;
    pc_valOrbToOldPC(val);
}

void pc_cleanup(Value& val) {
    if (val.type == vtString)
        MemHandleFree((MemHandle)val.sVal);
    val.sVal = NULL;
}

void pc_typeCast(Value& val, VarType vt) {
    pc_valOldPCToOrb(val);
    vm->typeCast(&val, vt);
    pc_valOrbToOldPC(val);
}

void pc_typeMatch(Value& l, Value& r) {
    pc_valOldPCToOrb(l);
    pc_valOldPCToOrb(r);
    vm->typeMatch(&l, &r);
    pc_valOrbToOldPC(l);
    pc_valOrbToOldPC(r);
}

bool pc_UIYield(bool b) {
    return PocketCYield(b);
}

void pc_callFunc32(long addr) {
    vm->Call(addr, true, true);
    if (vm->retVal.type == vtVoid) {
        Value zero;
        zero.type = vtInt;
        zero.iVal = 0;
        vm->push(&zero);
    } else {
        vm->push(&vm->retVal);
    }
}

void pc_callFunc16(word addr) {
    pc_callFunc32(addr);
}

extern NativeDraw* pDefaultDraw;
bool pc_callBI(char* name) {
    Value zero;
    vm->retVal.type = vtInt;
    vm->retVal.iVal = 0;
    if (0 == strcmp(name, "graph_on")) {
        lib_graph_on_internal(vm, pDefaultDraw);
    } else if (0 == strcmp(name, "hookmenu")) {
        lib_hookmenu(vm, 0);
    } else if (0 == strcmp(name, "hooksilk")) {
        lib_hooksilk(vm, 0);
    } else if (0 == strcmp(name, "event")) {
        lib_pcevent(vm, 0);
    } else if (0 == strcmp(name, "malloc")) {
        word size = vm->pop()->iVal;
        vm->retVal.iVal = vm->globs.alloc(size, "i");
        vm->gsize = vm->globs.size();
    } else if (0 == strcmp(name, "textattr")) {
        int underline = vm->pop()->iVal;
        int color = vm->pop()->iVal;
        int font = vm->pop()->iVal;
        pDefaultDraw->underline(underline);
        pDefaultDraw->font(font);
    } else if (0 == strcmp(name, "graph_off")) {
        lib_graph_off(vm, 0);
    } else {
        vm->vm_error("PocketC Native Library method 'callBI' not supported");
        return false;
    }
    return true;
}

vector<dword> tweakedAddrs;
bool g_bNewPocketC;

Value* pc_deref(short addr) {
    // decompress the address
    dword daddr = (addr & 0x3fff);
    daddr |= (((long)addr << 16) & 0xc0000000);
    Value* res = vm->deref(daddr);
    if (res->type == vtString && !g_bNewPocketC) {
        // if the address was already tweaked, don't tweak again
        for (int i = 0; i < tweakedAddrs.size(); i++) {
            if (tweakedAddrs[i] == daddr)
                return res;
        }
        tweakedAddrs.add(daddr);
        pc_valOrbToNewPC(*res);
    }
    return res;
}

char* pc_oldLockString(MemHandle hStr) {
    assert(!g_bNewPocketC);
    if (0x80000000 & (dword)hStr) {
        PCString* pString = (PCString*)MemHandleLock(hStr);
        assert(pString->sType == pcstIndirect);
        return &pString->data[0];		
    } else
        return (char*)hStr; //it's a pointer
}

void pc_oldUnlockString(MemHandle hStr) {
    assert(!g_bNewPocketC);
    if (0x80000000 & (dword)hStr) {
        MemHandleUnlock(hStr);
    }
}

UInt32 pc_vmCtrl(UInt32 id, UInt32 val) {
    if (id == VMCTRL_ENABLE_EVENTS) {
        nativeEnableEvents = !!val;
        return nativeEnableEvents;
    }
    return 0;
}

char* pc_lockString(Value* v) {
    return v->lock();
}

void pc_unlockString(Value* v) {
    v->unlock();
}

void pc_acquireString(Value* d, Value* s) {
    d->acquire(s);
}

void pc_releaseString(Value* v) {
    v->release();
}

void pc_unlockReleaseString(Value* v) {
    v->unlockRelease();
}

char* pc_newString(Value* v, unsigned short len) {
    return v->newString(len);
}

bool pc_newStringFromConst(Value* v, const char* data) {
    return v->copyString(data);
}

void pc_newConstString(Value* v, const char* data) {
    v->newConstString(data);
}

PocketCLibraryApiExt pcae;

word LoadPocketCLib(char* name, bool& bNewPocketC) {
    UInt16 card	= 0;
    LocalID	lid	= MyFindDatabase(name, byref card);
    if (!lid) {
        return 0;
    }
    
    dword creator, type;
    if (DmDatabaseInfo(card, lid, 0, 0,	0, 0, 0, 0,	0, 0, 0, &type,	&creator)) {
        return 0;
    }
    word ref;
    Err	err;
    if ((err = SysLibLoad(type,	creator, &ref))	!= 0) {
        return 0;
    }
    
    PocketCLibGlobalsPtr gP	= PocketCLibOpen(ref, NULL);
    if (!gP) {
        SysLibRemove(ref);
        return 0;
    }
    gP->pop	= pc_pop;
    gP->push = pc_push;
    gP->cleanup	= pc_cleanup;
    gP->typeCast = pc_typeCast;
    gP->typeMatch =	pc_typeMatch;
    gP->UIYield	= pc_UIYield;
    gP->addLibFunc = 0;
    gP->callFunc_deprecated = pc_callFunc16;
    gP->callFunc32 = pc_callFunc32;
    gP->callBI = pc_callBI;
    gP->deref =	pc_deref;
    gP->lockString_deprecated = pc_oldLockString;
    gP->unlockString_deprecated = pc_oldUnlockString;
    gP->retVal = &vm->retVal;
    gP->vmCtrl = pc_vmCtrl;

    pcae.lockString = pc_lockString;
    pcae.unlockString = pc_unlockString;
    pcae.acquireString = pc_acquireString;
    pcae.releaseString = pc_releaseString;
    pcae.unlockReleaseString = pc_unlockReleaseString;
    pcae.newString = pc_newString;
    pcae.newStringFromConst = pc_newStringFromConst;
    pcae.newConstString = pc_newConstString;
    
    if (gP->reserved == 0x12345678) {
        gP->apiExt = &pcae;
        gP->lockString_deprecated = NULL;
        gP->unlockString_deprecated = NULL;
        bNewPocketC = true;
    } else {
        gP->apiExt = NULL;
        bNewPocketC = false;
    }
    
    MemPtrUnlock(gP);

    return ref;
}

void ClosePocketCLib(word libRef) {
    PocketCLibClose(libRef, 0);
    SysLibRemove(libRef);
}

void ExecutePocketCLibFunc(bool bNewPocketC, word libRef, word iFunc) {
    g_bNewPocketC = bNewPocketC;
    PocketCLibExecute(libRef, (short)iFunc);
    if (!g_bNewPocketC) {
        for (int i=0; i<tweakedAddrs.size(); i++) {
            Value* val = vm->deref(tweakedAddrs[i]);
            pc_valNewPCToOrb(*val);
        }
    }
    tweakedAddrs.clear();
}
