#include "OrbFormsRT.h"
#include "OrbFormsRTRsc.h"

//#define VM_OUTPUT(x) x
#define VM_OUTPUT(x)

// string routines
char* Value::lock() {
    if (stype == stConst)
        return (char*)sVal;
    OrbString* pOS = (OrbString*)MemHandleLock((MemHandle)sVal);
    return pOS->str;
}

void Value::unlock() {
    if (stype == stShared)
        MemHandleUnlock((MemHandle)sVal);
}

void Value::acquire(Value* src) {
    stype = src->stype;
    sVal = src->sVal;
    if (stype == stShared) {
        OrbString* pOS = (OrbString*)MemHandleLock((MemHandle)sVal);
        pOS->ref++;
        MemHandleUnlock((MemHandle)sVal);
    }
}

void Value::release() {
    int ref = 0;
    if (stype == stShared && sVal) {
        OrbString* pOS = (OrbString*)MemHandleLock((MemHandle)sVal);
        ref = --(pOS->ref);
        MemHandleUnlock((MemHandle)sVal);
        if (ref == 0) {
            MemHandleFree((MemHandle)sVal);
        }
        sVal = 0;
    }
}

void Value::unlockRelease() {
    unlock();
    release();
}

bool Value::makeEditable() {
    if (stype == stConst) {
        char* str = (char*)sVal;
        return copyString(str);
    } else {
        if (sVal) {
            OrbString* pStr = (OrbString*)MemHandleLock((MemHandle)sVal);
            if (pStr->ref == 1) {
                MemHandleUnlock((MemHandle)sVal);
                return true;
            }
            pStr->ref--;
            MemHandle oldStr = (MemHandle)sVal;
            bool res = copyString(pStr->str);
            MemHandleUnlock(oldStr);
            return res;
        }
        return false;
    }
}

char* Value::newString(int len) {
    sVal = h_malloc(sizeof(OrbString) + len);
    stype = stShared;
    type = vtString;
    if (sVal) {
        OrbString* pStr = (OrbString*)MemHandleLock((MemHandle)sVal);
        pStr->ref = 1;
        return pStr->str;
    }
    return NULL;
}

bool Value::copyString(const char* str) {
    if (str == NULL) str = "";
    int len = strlen(str);
    char* p = newString(len);
    if (p) {
        strcpy(p, str);
        unlock();
        return true;
    }
    return false;
}

char* Value::newConstString(const char* str) {
    stype = stConst;
    type = vtString;
    sVal = (char*)str;
    return (char*)str;
}

//#undef assert
//#define assert(x) { if (!(x)) { HostFPrintF(hostFile, "ASSERT: @%d :%s\n", __LINE__, #x); ErrDisplayFileLineMsg(__FILE__, __LINE__, #x); } }

// virtual machine
void lib_vector_virtcall(VM* vm, int);


VM::VM() : stack(20), code(NULL), str(NULL), objinfo(NULL), /*globs(NULL),*/
    pc(0), st(0), fb(0), fp(0), segmentStart(0), hString(NULL),
    callDepth(0), oisize(0)
{
    for (int i=0;i<MAX_CODE_SEGMENTS;i++) {
        hCodeSegments[i] = NULL;
        codeSegments[i] = NULL;
    }
    hostFile = NULL;
    VM_OUTPUT(hostFile = HostFOpen("D:\\Development\\OrbForms\\vm_out.txt", "wt");)
    VM_OUTPUT(if (hostFile) HostFPrintF(hostFile, "New log\n");)
}

VM::~VM() {
    if (hostFile)
        HostFClose(hostFile);
}

void VM::vm_error(const char* err, bool bAssert) {
    killApp = true;
    killVM = true;
    static char buff[512];
    long fb = segmentStart + this->fb;
    unsigned int strLoc = GetCodeByte(fb-1) * 256 + GetCodeByte(fb-2);
    char* name = &str[strLoc];
    long pc = GetFullPC();
    if (bAssert) {
        StrPrintF(buff, "assert(%s)\n---- stack [%ld] ----\n%s", err, pc, stackDump(8));
    } else {
        StrPrintF(buff, "Addr: %ld\nFunc:%s\n%s", pc, name, err);
    }
    FrmCustomAlert(VMErrorAlert, buff, NULL, NULL);
    //printf("%s @ %d\n", err, pc);
    ErrThrow(2);
}

char* VM::stackDump(int depth) {
    long currFB = segmentStart+fb;
    long currFP = fp;
    g_buff[0] = 0;
    int nRemaining = 255;
    while (currFB != -1 && depth--) {
        unsigned int strLoc = GetCodeByte(currFB-1) * 256 + GetCodeByte(currFB-2);
        char* name = &str[strLoc];
        int len = strlen(name) + 1;
        if (len < nRemaining) {
        	strcat(g_buff, name);
    		strcat(g_buff, "\n");
    		nRemaining -= len;
    		
        	long i = currFP;
        	if (currFP == 2 && !hasUI) break; // non UI apps
        	while (stack[i].type != vtSavedFB && i < st) i++;
        	if (i == st) break; // bad stack
        	currFB = stack[i].iVal;
        	if (stack[i+1].type != vtRetAddr) break; // bad stack
        	currFB |= stack[i+1].iVal & 0xffffe000; // get full address
        	if (stack[i+2].type != vtSavedFP) break; // bad stack
        	currFP = stack[i+2].iVal;
        }
        
    }
    return g_buff;
}

#define BINARY(op, _type, _val) \
    temp.type = _type; \
    temp._val = (op1->_val op op2->_val); \
    beforePush2(op1, op2, ot1, ot2); \
    push(&temp)
    
#define COMP(op, _type, _val) \
    temp.type = vtInt; \
    temp.iVal = (op1->_val op op2->_val); \
    beforePush2(op1, op2, ot1, ot2); \
    push(&temp)
    
#define SAFE_DIV(_type, _val) \
    if (op2->_val == 0) { vm_error("divide by zero error"); } \
    else { \
    temp.type = _type; \
    temp._val = (op1->_val / op2->_val); \
    beforePush2(op1, op2, ot1, ot2); \
    push(&temp); }
    
#define UNARY(op, _type, _val) \
    temp.type = _type; \
    temp._val = op op2->_val; \
    beforePush1(op2, ot2); \
    push(&temp)
    
#define CONVERT(_type, _vald, _vals) \
    temp.type = _type; \
    temp._vald = op2->_vals; \
    beforePush1(op2, ot2); \
    push(&temp)

#define COMP_STR(op) \
    temp.type = vtInt; \
    temp.iVal = (strcmp(op1->lock(), op2->lock()) op 0); \
    op1->unlock(); op2->unlock(); \
    beforePushS2(op1, op2, ot1, ot2); \
    push(&temp)

#define POST_OP(op, _val) \
    op1 = deref(op2->iVal); \
    beforePush1(op2, ot2); \
    push(op1); \
    op1->_val op

#define PRE_OP(op, _val) \
    op1 = deref(op2->iVal); \
    op1->_val op; \
    beforePush1(op2, ot2); \
    push(op1)


Value* VM::deref(long addr) {
    if ((addr & 0xC0000000) == 0xC0000000) {
        long off = fp + (addr & 0x3FFFFFFF);
        if (off >= st)	{
            //printf("Invalid stack ref (st=%d): 0x%x (%d)\n", st, addr, off);
            vm_error("invalid stack reference");
        }
        return &stack[off];
    } else if (addr & 0x80000000) {
        long off = addr & 0x3FFFFFFF;
        if (off >= st) {
            //printf("Invalid stack ref (st=%d): 0x%x (%d)\n", st, addr, off);
            vm_error("invalid stack reference");
        }
        return &stack[off];
    } else {
        if (addr >= gsize) {
            //printf("Invalid global ref (gsize=%d): %d\n", gsize, addr);
            vm_error("invalid global reference");
        }
        return &globs[addr];
    }
}

long VM::decodeWord(int addr) {
    long off;
    if ((addr & 0xC000) == 0xC000) {
        off = fp + (addr & 0x3FFF) + 0x80000000;
    } else if (addr & 0x8000) {
        off = (addr & 0x3FFF) + 0x80000000;
    } else {
        off = addr;
    }
    return off;
}

void VM::push(Value* v) {
#ifdef FAST_STACK
    stack.addfast();
    st++;
    moveVal(stack_top++, v);
#else
    stack.add();
    moveVal(&stack[st++], v);
#endif
}

Value* VM::pop() {
#ifdef FAST_STACK
    --st;
    Value* v = --stack_top;
    stack.removefast();
#else
    Value* v = &stack[--st];
    stack.remove(); // remove will never actually remove the top
#endif
    return v;
}

inline int VM::getWord() {
    //long ret = code[pc] + (long)code[pc+1] * 256;
    int ret;
    char* p = (char*)&ret;
    char* pcode = (char*)&code[pc];
    *p = *(pcode+1);
    *(p+1) = *(pcode);
    pc += 2;
    return ret;
    
    //int ret = (word)(code[pc]) + ((word)(code[pc+1]) << 8);
    //pc+=2;
    //return ret;
}

long VM::getLong() {
    //long ret = code[pc] + ((long)code[pc+1] << 8) + ((long)code[pc+2] << 16) + ((long)code[pc+3] << 24);
    long ret;
    char* p = (char*)&ret;
    char* pcode = (char*)&code[pc];
    *p = *(pcode+3);
    *(p+1) = *(pcode+2);
    *(p+2) = *(pcode+1);
    *(p+3) = *(pcode);
    
    pc += 4;
    return ret;
}

#ifdef DEBUG
void VM::moveVal(Value* dst, Value* src) {
    long* pd = (long*)dst;
    long* ps = (long*)src;
    *pd++ = *ps++;
    *(short*)pd = *(short*)ps;
    //memcpy(dst, src, sizeof(Value));
    if (src->type == vtString)
        src->sVal = NULL;
}
#endif

void VM::copyVal(Value* dst, Value* src) {
    long* pd = (long*)dst;
    long* ps = (long*)src;
    *pd++ = *ps++;
    *(short*)pd = *(short*)ps;
    //memcpy(dst, src, sizeof(Value));
    if (dst->type == vtString)
        dst->acquire(src);
        //dst->sVal = strdup(src->sVal);
}

long VM::GetFullPC() {
    return segmentStart + pc;
}

void VM::SetPC(long addr) {
    if (addr == -1) {
        pc = -1;
    } else {
        assert(addr < CODE_SEGMENT_SIZE * MAX_CODE_SEGMENTS);
        assert(codeSegments[addr >> CODE_SEGMENT_SHIFT]);
        code = codeSegments[addr >> CODE_SEGMENT_SHIFT];
        pc = addr % CODE_SEGMENT_SIZE;
        segmentStart = (addr >> CODE_SEGMENT_SHIFT) * CODE_SEGMENT_SIZE;
    }
}

byte VM::GetCodeByte(long addr) {
    assert(addr < CODE_SEGMENT_SIZE * MAX_CODE_SEGMENTS);
    assert(codeSegments[addr >> CODE_SEGMENT_SHIFT]);
    return codeSegments[addr >> CODE_SEGMENT_SHIFT][addr % CODE_SEGMENT_SIZE];
}

bool VM::Load() {
    VarType* gtypes;
    GlobalInit* globis;
    long gisize;
    MemHandle hGlobTypes = 0, hGlobInits = 0, hHeader = 0, hLibs = 0;
    for (int i=0;i<MAX_CODE_SEGMENTS;i++) {
        hCodeSegments[i] = DmGet1Resource('ofCD', i);
        if (hCodeSegments[i]) {
            codeSegments[i] = (byte*)MemHandleLock(hCodeSegments[i]);
        } else if (i == 0) {
            Alert("unable to get code segment\n");
            goto error;
        }
    }
    hGlobTypes = DmGet1Resource('ofGT', 0);
    if (hGlobTypes) {
        gsize = MemHandleSize(hGlobTypes) / sizeof(VarType);
        gtypes = (VarType*)MemHandleLock(hGlobTypes);
    } else {
        gsize = 0;
    }
    hGlobInits = DmGet1Resource('ofGI', 0);
    if (hGlobInits) {
        gisize = MemHandleSize(hGlobInits) / sizeof(GlobalInit);
        globis = (GlobalInit*)MemHandleLock(hGlobInits);
    } else {
        gisize = 0;
    }
    hString = DmGet1Resource('ofST', 0);
    if (hString) {
        ssize = MemHandleSize(hString);
        str = (char*)MemHandleLock(hString);
    } else {
        Alert("unable to get string segment\n");
        goto error;
    }
    hObjInfo = DmGet1Resource('ofOI', 0);
    if (hObjInfo) {
        oisize = MemHandleSize(hObjInfo);
        objinfo = (byte*)MemHandleLock(hObjInfo);
    }

    hLibs = DmGet1Resource('ofNA', 0);
    if (hLibs) {
        InfoStream info;
        info.init((const byte*)MemHandleLock(hLibs));
        byte nLibs = info.getByte();
        for (int i=0;i<nLibs;i++) {
            char* name = &str[info.getWord()];
            word id = info.getWord();
            char buff[128];
            if (id == 0xffff) {
                // PocketC Native Library
                OAddIn ai = {0};
                ai.bPocketC = true;
                ai.libRef = LoadPocketCLib(name, byref ai.bNewPocketC);
                if (!ai.libRef) {
                    sprintf(buff, "Unable to load PocketC native library '%s'", name);
                    Alert(buff);
                    goto error;
                }
                // TODO: clean this mess up
                extern vector<OAddIn> addIns;
                addIns.add(ai);
            } else if (!LoadAddIn(name, id)) {
                sprintf(buff, "Unable to load native add-in '%s'", name);
                Alert(buff);
                goto error;				
            }
        }
        
        MemHandleUnlock(hLibs);
        DmReleaseResource(hLibs);
        hLibs = NULL;
    }

    globs.init(gsize);
    //globs.fill(gsize);
    //globs.lock();
    long i;
    for (i=0;i<gsize;i++) {
        globs[i].type = gtypes[i];
        if (gtypes[i] == vtString)
            globs[i].newConstString("");
    }

    for (i=0;i<gisize;i++) {
        assert(globis[i].val.type == globs[globis[i].offset].type);
        globs[globis[i].offset].iVal = globis[i].val.iVal;
        if (globis[i].val.type == vtString)
            globs[globis[i].offset].newConstString(&str[globs[globis[i].offset].iVal]);
    }

    if (hGlobInits) {
        MemHandleUnlock(hGlobInits);
        DmReleaseResource(hGlobInits);
    }
    if (hGlobTypes) {
        MemHandleUnlock(hGlobTypes);
        DmReleaseResource(hGlobTypes);
    }
    
    stack.init(100);
    stack.lock();
    stack.addfast();
    stack_top = &stack[0];
    stack.removefast();

    // init the native addins
    InitOFI();
    for (int i=0;i<addIns.size();i++) {
        if (addIns[i].func) {
            addIns[i].data = (*addIns[i].func)(&ofi, NULL, NATIVE_STARTUP);
            addIns[i].bInit = true;
        }
    }
    return true;

error:
    if (hLibs) {
        MemHandleUnlock(hLibs);
        DmReleaseResource(hLibs);
    }
    if (hGlobInits) {
        MemHandleUnlock(hGlobInits);
        DmReleaseResource(hGlobInits);
    }
    if (hGlobTypes) {
        MemHandleUnlock(hGlobTypes);
        DmReleaseResource(hGlobTypes);
    }
    Unload();
    return false;
}

void VM::Unload() {
    int i;
    for (i=0;i<globs.size();i++) {
        cleanUp(&globs[i]);
    }
    for (i=0;i<stack.size();i++) {
        cleanUp(&stack[i]);
    }
    //globs.unlock();
    stack.unlock();
    for (i=0;i<MAX_CODE_SEGMENTS;i++) {
        if (!hCodeSegments[i])
            break;
        MemHandleUnlock(hCodeSegments[i]);
        DmReleaseResource(hCodeSegments[i]);
    }
        
    if (hString) {
        MemHandleUnlock(hString);
        DmReleaseResource(hString);
    }
    if (hObjInfo) {
        MemHandleUnlock(hObjInfo);
        DmReleaseResource(hObjInfo);
    }
    
    for (i=0;i<addIns.size();i++) {
        OAddIn& addIn = addIns[i];
        if (!addIn.bPocketC) {
            if (addIn.bInit)
                (*addIn.func)(&ofi, addIn.data, NATIVE_SHUTDOWN);
            if (addIn.hAddIn) {
                MemHandleUnlock(addIn.hAddIn);
                DmReleaseResource(addIn.hAddIn);
            }
            if (addIn.dbRef) {
                DmCloseDatabase(addIn.dbRef);
            }
        } else {
            if (addIn.libRef) {
                ClosePocketCLib(addIn.libRef);
            }
        }
    }
    
    eventFuncs.clear();

}

#ifdef DEBUG
void VM::beforePush1(Value* a, OperandType ota) {
    if (ota == otStack) {
        if (a != pop())
            vm_error("stack busted");
    }
}

void VM::beforePushS1(Value* a, OperandType ota) {
    if (ota == otStack) {
        if (a != pop())
            vm_error("stack busted");
        cleanUp(a);
    }
}

void VM::beforePush2(Value* a, Value* b, OperandType ota, OperandType otb) {
    if (otb == otStack) {
        if (b != pop())
            vm_error("stack busted");
    }
    if (ota == otStack) {
        if (a != pop())
            vm_error("stack busted");
    }
}

void VM::beforePushS2(Value* a, Value* b, OperandType ota, OperandType otb) {
    if (otb == otStack) {
        if (b != pop())
            vm_error("stack busted");
        cleanUp(b);
    }
    if (ota == otStack) {
        if (a != pop())
            vm_error("stack busted");
        cleanUp(a);
    }
}
#endif

long VM::getVirtFunc(long objID, long funcID) {
    assert(funcID < objinfo[objID + 4]);
    funcID = objID + 5 + 4 * funcID;
    return objinfo[funcID] + ((long)objinfo[funcID + 1] << 8) + ((long)objinfo[funcID + 2] << 16) + ((long)objinfo[funcID + 3] << 24);
}

long VM::getIfaceFunc(long objID, long ifaceID, long funcID) {
    long ifaceBase = objID + 5 + objinfo[objID + 4] * 4;
    long nIfaces = objinfo[ifaceBase];
    ifaceBase += 3;
    bool found = false;
    for (int i=0;i<nIfaces;i++) {
        long index = objinfo[ifaceBase] + ((long)objinfo[ifaceBase + 1] << 8);
        if (index == ifaceID) {
            found = true;
            break;
        }
        ifaceBase += 6;
    }
    if (!found) vm_error("object does not implement required interface");
    ifaceBase += 2;
    long index = objinfo[ifaceBase] + ((long)objinfo[ifaceBase + 1] << 8) + ((long)objinfo[ifaceBase + 2] << 16) + ((long)objinfo[ifaceBase + 3] << 24);
    funcID = index + funcID * 4;
    return objinfo[funcID] + ((long)objinfo[funcID + 1] << 8) + ((long)objinfo[funcID + 2] << 16) + ((long)objinfo[funcID + 3] << 24);
}

long VM::getObjectSize(long objID) {
    return objinfo[objID + 2] + ((long)objinfo[objID + 3] << 8);
}

void VM::setObjinfo(long addr, long objID) {
    Value* v = deref(addr);
    assert(v->type == vtInt);
    v->iVal = objID;
    
    // get subobject count
    long ifaceBase = objID + 5 + objinfo[objID + 4] * 4;
    long nIfaces = objinfo[ifaceBase];
    long nIfaceMethods = objinfo[ifaceBase+1] + ((long)objinfo[ifaceBase+2] << 8);

    long iSubObjectBase = ifaceBase + 3 + nIfaces * 6 + nIfaceMethods * 4 + 2;
    long nSubObjects = objinfo[iSubObjectBase-2] + ((long)objinfo[iSubObjectBase-1] << 8);

    for (long i=0;i<nSubObjects;i++) {
        long offset = objinfo[iSubObjectBase++];
        offset += ((long)objinfo[iSubObjectBase++] << 8);
        long subobjID = objinfo[iSubObjectBase++];
        subobjID += ((long)objinfo[iSubObjectBase++] << 8);
        setObjinfo(addr + offset, subobjID);
    }
}

#define STACK_MINUS_1 (stack_top-1)
#define STACK_MINUS_2 (stack_top-2)

#include "..\..\compiler\inst.h"
void VM::typeMatch(Value* a, Value* b) {
    if (a->type == b->type) return;
    if (b->type == vtString) {
        typeCast(a, vtString);
    } else if (a->type == vtString) {
        typeCast(b, vtString);
    } else if (b->type == vtFloat) {
        typeCast(a, vtFloat);
    } else if (a->type == vtFloat) {
        typeCast(b, vtFloat);
    } else if (b->type == vtInt) {
        typeCast(a, vtInt);
    } else {
        typeCast(b, vtInt);
    }
}

void VM::typeCast(Value* v, VarType type) {
    if (v->type == type) return;
    if (type == vtString) {
        char* str;
        if (v->type == vtChar) {
            char c = v->cVal;
            str = v->newString(1);
            str[0] = c;
            str[1] = 0;
            v->unlock();
        } else if (v->type == vtInt) {
            long l = v->iVal;
            str = v->newString(16);
            itoa(l, str, 10);
            v->unlock();
        } else {
            float f = v->fVal;
            str = v->newString(16);
            ftoa(f, str);
            v->unlock();
        }
        v->type = vtString;
        return;
    }
    if (type == vtFloat) {
        if (v->type == vtString) {
            float f;
            f = (float)atof(v->lock());
            v->unlockRelease();
            v->fVal = f;
        }
        else if (v->type == vtInt) v->fVal = (float)v->iVal;
        else v->fVal = (float)(short)v->cVal;
        v->type = vtFloat;
        return;
    }
    if (type == vtInt) {
        if (v->type == vtString) {
            long l;
            l = atoi(v->lock());
            v->unlockRelease();
            v->iVal = l;
        }
        else if (v->type == vtFloat) v->iVal = (long)v->fVal;
        else v->iVal = (long)v->cVal;
        v->type = vtInt;
        return;
    }
    if (v->type == vtString) {
        char c;
        c = v->lock()[0];
        v->unlockRelease();
        v->cVal = c;
    }
    else if (v->type == vtInt) v->cVal = (char)v->iVal;
    else v->cVal = (char)(long)v->fVal;
    v->type = vtChar;
}


bool VM::Call(long addr, bool pushRet, bool yield) {
    long prev_pc = GetFullPC(), prev_fb = fb;
    SetPC(addr);
    fb = pc;
    killVM = false;
    long nArray = 1, index, objID, funcID, tempLong;
    OperandType ot1, ot2;
    Value* op1, *op2, *op3;
    Value* src, *dst;
    char* srcStr;
    char* op1Str, *op2Str;
    Value immed1, immed2;
    Value temp;
    char* tempStr;
    byte inst, num;
    short count = 50;
    callDepth++;
    
    if (pushRet) {
        temp.type = vtSavedFB;
        temp.iVal = -1;
        push(&temp);
        temp.type = vtRetAddr;
        push(&temp);
    }

    while (!killVM) {
        //if (nativeEnableEvents && !killEvents && ++count > 100) {
        if (yield && nativeEnableEvents && ++count > 100) {
             count=0;
             if (PocketCYield(false)) {
             	return false;
             }
        }
        inst = code[pc++];
        ot1 = ot2 = otNone;
        //if ((inst & 0xC0) == 0xC0) {
        if (inst >= (byte)0xC0) {
            // handle extended instructions
            byte ext = inst;
            long stoff = 1, val1, val2;
            inst = code[pc++];

            if (ext & 0x20)
                val1 = getWord();
            if (ext & 0x08)
                val2 = getWord();

            if (ext & 0x0C) {
                ot2 = (OperandType)((ext & 0x0C) >> 2);
                if (ot2 == otImmed) {
                    immed2.iVal = decodeWord(val2);
                    op2 = &immed2;
                } else if (ot2 == otAddr) {
                    op2 = deref(decodeWord(val2));
                } else if (ot2 == otStack) {
                    op2 = &stack[st-stoff];
                    stoff++;
                }
            }
            if (ext & 0x30) {
                ot1 = (OperandType)((ext & 0x30) >> 4);
                if (ot1 == otImmed) {
                    immed1.iVal = decodeWord(val1);
                    op1 = &immed1;
                } else if (ot1 == otAddr) {
                    op1 = deref(decodeWord(val1));
                } else if (ot1 == otStack) {
                    op1 = &stack[st-stoff];
                    stoff++;
                }
                if (ot2 == otNone) {
                    op2 = op1; // just one arg, so put it in the right place
                    ot2 = ot1;
                }
            }
            assert((ext & 0x02) == 0);
        } else {
            op2 = STACK_MINUS_1;
            ot2 = otStack;
            op1 = STACK_MINUS_2;
            ot1 = otStack;
        }

        VM_OUTPUT(
        if (hostFile) {
            char instBuff[7] = "      "; //{ ' ', ' ', ' ', ' ', ' ', ' ', 0 };
            strncpy(instBuff + (6-strlen(instNames[inst])), instNames[inst], strlen(instNames[inst]));
            HostFPrintF(hostFile, "%2ld * %5ld ", callDepth, GetFullPC());
            HostFPutS(instBuff, hostFile);
            HostFPrintF(hostFile, " sp=%ld\n", st);
            //HostFPrintF(hostFile, "* %5ld %6.6s sp=%ld\n", pc, instNames[inst], st);
        }
        )

reeval:
        switch (inst) {
            case vmCInt:
                temp.type = vtInt;
                temp.iVal = getLong();
                while (nArray--)
                    push(&temp);
                break;
            case vmSCInt:
                index = decodeWord(getWord());
                dst = deref(index);
                assert(dst->type == vtInt);
                dst->iVal = getLong();
                break;
            case vmCChar:
                temp.type = vtChar;
                temp.cVal = code[pc++];
                while (nArray--)
                    push(&temp);
                break;
            case vmSCChar:
                index = decodeWord(getWord());
                dst = deref(index);
                assert(dst->type == vtChar);
                dst->cVal = code[pc++];
                break;
            case vmCFloat:
                temp.type = vtFloat;
                index = getLong();
                temp.fVal = *((float*)&index);
                while (nArray--)
                    push(&temp);
                break;
            case vmSCFloat:
                index = decodeWord(getWord());
                dst = deref(index);
                assert(dst->type == vtFloat);
                index = getLong();
                dst->fVal = *((float*)&index);
                break;
            case vmCString:
                index = getWord();
                temp.type = vtString;
                while (nArray--) {
                    temp.newConstString(&str[index]);
                    push(&temp);
                }
                break;
            case vmSCString:
                index = decodeWord(getWord());
                dst = deref(index);
                assert(dst->type == vtString);
                cleanUp(dst);
                dst->newConstString(&str[getWord()]);
                break;
            case vmCWord:
                temp.type = vtInt;
                temp.iVal = getWord();
                while (nArray--)
                    push(&temp);
                break;
            case vmSCWord:
                index = decodeWord(getWord());
                dst = deref(index);
                assert(dst->type == vtInt);
                dst->iVal = getWord();
                break;
            case vmCWordPFP:
                temp.type = vtInt;
                temp.iVal = getWord() + fp + 0x80000000;
                while (nArray--)
                    push(&temp);
                break;

            case vmMove:
                src = deref(decodeWord(getWord()));
                dst = deref(decodeWord(getWord()));
                //if (src->type != dst->type) {
                //	vm_error("moving to a location of different type");
                //}
                
                cleanUp(dst);
                copyVal(dst, src);
                break;

            case vmStackInit: {
                #define count index
                tempStr = &str[getWord()];
                count = 1;
                while (*tempStr) {
stackLoop:
                    temp.iVal = 0;
                    switch (*tempStr) {
                        case 'i': temp.type = vtInt; break;
                        case 'c': temp.type = vtChar; break;
                        case 'f': temp.type = vtFloat; break;
                        case 's': temp.type = vtString; temp.newConstString(""); break;
                        default:
                            count = 0;
                            while (*tempStr >= '0' && *tempStr <= '9') {
                                count *=10;
                                count += *tempStr - '0';
                                tempStr++;
                            }
                            goto stackLoop;
                    }
#ifdef FAST_STACK
                    stack.reserve(count);
                    stack_top = &stack[st-1] + 1;
#endif
                    while (count--) {
                        push(&temp);
                        if (temp.type == vtString && count)
                            temp.newConstString("");
                    }
                    count = 1;
                    tempStr++;
                }
#ifdef FAST_STACK
                stack.reserve(100); // ensure that stack has 100 values to spare
                stack_top = &stack[st-1] + 1;
#endif
                break;
                #undef count
            }

            case vmGetReg:
                index = code[pc++];
                assert(index < 10 && index >= 0);
                temp.type = vtInt;
                temp.iVal = reg[index];
                push(&temp);
                break;

            case vmSetReg:
                index = code[pc++];
                assert(index < 10 && index >= 0);
                assert(stack[st-1].type == vtInt);
                reg[index] = stack[st-1].iVal;
                break;

            //
            // conversion ops
            //
            case vmItoC: CONVERT(vtChar, cVal, iVal); break;
            case vmItoF: CONVERT(vtFloat, fVal, iVal); break;
            case vmItoS:
                tempStr = temp.newString(16);
                itoa(op2->iVal, tempStr, 10);
                temp.unlock();
                beforePushS1(op2, ot2);
                push(&temp);
                break;
            case vmFtoI: CONVERT(vtInt, iVal, fVal); break;
            case vmFtoC: CONVERT(vtChar, cVal, fVal); break;
            case vmFtoS:
                tempStr = temp.newString(16);
                ftoa(op2->fVal, tempStr);
                temp.unlock();
                beforePushS1(op2, ot2);
                push(&temp);
                break;
            case vmCtoI: CONVERT(vtInt, iVal, cVal); break;
            case vmCtoF: CONVERT(vtFloat, fVal, cVal); break;
            case vmCtoS:
                tempStr = temp.newString(1);
                tempStr[0] = op2->cVal;
                tempStr[1] = 0;
                temp.unlock();
                beforePushS1(op2, ot2);
                push(&temp);
                break;
            case vmStoI:
                temp.type = vtInt;
                temp.iVal = atoi(op2->lock());
                op2->unlock();
                beforePushS1(op2, ot2);
                push(&temp);
                break;
            case vmStoF:
                temp.type = vtFloat;
                temp.fVal = atof(op2->lock());
                op2->unlock();
                beforePushS1(op2, ot2);
                push(&temp);
                break;
            case vmStoC:
                temp.type = vtChar;
                temp.cVal = *op2->lock();
                op2->unlock();
                beforePushS1(op2, ot2);
                push(&temp);
                break;
            case vmStoB:
                temp.type = vtInt;
                temp.iVal = *op2->lock() ? 1 : 0;
                op2->unlock();
                beforePushS1(op2, ot2);
                push(&temp);
                break;

            case vmArray:
                nArray = getWord();
                break;

            case vmAddI: BINARY(+, vtInt, iVal); break;
            case vmAddC: BINARY(+, vtChar, cVal); break;
            case vmAddF: BINARY(+, vtFloat, fVal); break;
            case vmAddS:
                assert(op1->type == vtString && op2->type == vtString);
                temp.type = vtString;
                op1Str = op1->lock();
                op2Str = op2->lock();
                tempStr = temp.newString(strlen(op1Str) + strlen(op2Str));
                strcpy(tempStr, op1Str);
                strcat(tempStr, op2Str);
                op1->unlock();
                op2->unlock();
                beforePushS2(op1, op2, ot1, ot2);
                push(&temp);
                break;
            
            case vmSubI: BINARY(-, vtInt, iVal); break;
            case vmSubC: BINARY(-, vtChar, cVal); break;
            case vmSubF: BINARY(-, vtFloat, fVal); break;

            case vmMultI: BINARY(*, vtInt, iVal); break;
            case vmMultC: BINARY(*, vtChar, cVal); break;
            case vmMultF: BINARY(*, vtFloat, fVal); break;

            case vmDivI: SAFE_DIV(vtInt, iVal); break;
            case vmDivC: SAFE_DIV(vtChar, cVal); break;
            case vmDivF: BINARY(/, vtFloat, fVal); break;

            case vmModI: BINARY(%, vtInt, iVal); break;
            case vmModC: BINARY(%, vtChar, cVal); break;
            case vmModF: vm_error("NYI ModF");

            case vmNegI: UNARY(-, vtInt, iVal); break;
            case vmNegC: UNARY(-, vtChar, cVal); break;
            case vmNegF: UNARY(-, vtFloat, fVal); break;

            case vmNot: UNARY(!, vtInt, iVal); break;
            case vmAnd:	BINARY(&&, vtInt, iVal); break;
            case vmOr:  BINARY(||, vtInt, iVal); break;
        
            // relational operators
            // TODO: these call a function instead
            case vmEqI: COMP(==, vtInt, iVal); break;
            case vmEqC: COMP(==, vtChar, cVal); break;
            case vmEqF: COMP(==, vtFloat, fVal); break;
            case vmEqS: COMP_STR(==); break;

            case vmNEqI: COMP(!=, vtInt, iVal); break;
            case vmNEqC: COMP(!=, vtChar, cVal); break;
            case vmNEqF: COMP(!=, vtFloat, fVal); break;
            case vmNEqS: COMP_STR(!=); break;

            case vmLTI: COMP(<, vtInt, iVal); break;
            case vmLTC: COMP(<, vtChar, cVal); break;
            case vmLTF: COMP(<, vtFloat, fVal); break;
            case vmLTS: COMP_STR(<); break;

            case vmLTEI: COMP(<=, vtInt, iVal); break;
            case vmLTEC: COMP(<=, vtChar, cVal); break;
            case vmLTEF: COMP(<=, vtFloat, fVal); break;
            case vmLTES: COMP_STR(<=); break;

            case vmGTI: COMP(>, vtInt, iVal); break;
            case vmGTC: COMP(>, vtChar, cVal); break;
            case vmGTF: COMP(>, vtFloat, fVal); break;
            case vmGTS: COMP_STR(>); break;

            case vmGTEI: COMP(>=, vtInt, iVal); break;
            case vmGTEC: COMP(>=, vtChar, cVal); break;
            case vmGTEF: COMP(>=, vtFloat, fVal); break;
            case vmGTES: COMP_STR(>=); break;

            case vmJmp:
                pc = getWord() + fb;
                break;
            case vmJmpZ:
                index = getWord();
                if (op2->iVal == 0)
                    pc = index + fb;
                beforePush1(op2, ot2);
                break;
            case vmJmpNZ:
                index = getWord();
                if (op2->iVal != 0)
                    pc = index + fb;
                beforePush1(op2, ot2);
                break;
            case vmJmpPopZ:
                index = getWord();
                if (op2->iVal == 0) {
                    pc = index + fb;
                } else {
                    beforePush1(op2, ot2);
                }
                break;
            case vmJmpPopNZ:
                index = getWord();
                if (op2->iVal != 0) {
                    pc = index + fb;
                } else {
                    beforePush1(op2, ot2);
                }
                break;

            case vmCallHandler:
                op1 = pop(); // addr
                assert(op1->type == vtInt);
                if (op1->iVal != 0xffffffff) {
                    index = op1->iVal;
                    goto CALL_COMMON;
                }
                pop(); // nothing to do, so pop the obj addr
                break;
            case vmCallI:
                op1 = pop();
                assert(op1->type == vtInt);
                index = op1->iVal;
                if (index == 0)
                    vm_error("attempting to call null funcptr");
                goto CALL_COMMON;
            case vmCall:
                index = getLong();
CALL_COMMON:
                temp.type = vtSavedFB;
                temp.iVal = fb;
                push(&temp);
                temp.type = vtRetAddr;
                temp.iVal = GetFullPC();
                push(&temp);
                SetPC(index);
                fb = pc;
                break;

            case vmCallStubBI:
                index = getWord();
                op1 = pop();
                assert(op1->type == vtRetAddr);
                SetPC(op1->iVal);
                op1 = pop();
                assert(op1->type == vtSavedFB);
                fb = op1->iVal;
                assert(index < nbifuncs);
                retVal.type = vtInt;
                retVal.iVal = 0;
                (*bifuncs[index].func)(this, index);
                if (bifuncs[index].hasRet)
                    push(&retVal);
                break;
            case vmCallBI:
                index = getWord();
                assert(index < nbifuncs);
                retVal.type = vtInt;
                retVal.iVal = 0;
                (*bifuncs[index].func)(this, index);
                if (bifuncs[index].hasRet)
                    push(&retVal);
                VM_OUTPUT(if (killVM) HostFPrintF(hostFile, "killVM = true (%ld)\n", index);)
                break;
            case vmLibStubCall:
                num = code[pc++];
                index = getWord();
                op1 = pop();
                assert(op1->type == vtRetAddr);
                SetPC(op1->iVal);
                op1 = pop();
                assert(op1->type == vtSavedFB);
                fb = op1->iVal;
                assert(num < addIns.size());
                retVal.type = vtVoid;
                retVal.iVal = 0;
                current_addin_data = addIns[num].data;
                (*addIns[num].func)(&ofi, current_addin_data, index);
                if (retVal.type != vtVoid)
                    push(&retVal);
                current_addin_data = NULL;
                break;

            case vmLibCall:
                num = code[pc++];
                index = getWord();
                assert(num < addIns.size());
                retVal.type = vtVoid;
                retVal.iVal = 0;
                current_addin_data = addIns[num].data;
                if (addIns[num].bPocketC) {
                    retVal.type = vtInt; // PocketC always returns something
                    ExecutePocketCLibFunc(addIns[num].bNewPocketC, addIns[num].libRef, index);
                    pc_valOldPCToOrb(retVal);
                } else {
                    (*addIns[num].func)(&ofi, current_addin_data, index);
                }
                if (retVal.type != vtVoid)
                    push(&retVal);
                current_addin_data = NULL;
                break;

            case vmCallVirt: {
                index = code[pc++];
                op1 = &stack[st-index-1];
                funcID = code[pc++];
                assert(op1->type == vtInt);
                objID = deref(op1->iVal)->iVal;
                index = getVirtFunc(objID, funcID);
                goto CALL_COMMON;
            }

            case vmCallIface: {
                #define ifaceID tempLong
                index = code[pc++];
                op1 = &stack[st-index-1];
                ifaceID = getWord();
                funcID = code[pc++];
                assert(op1->type == vtInt);
                objID = deref(op1->iVal)->iVal;
                index = getIfaceFunc(objID, ifaceID, funcID);
                goto CALL_COMMON;
                #undef ifaceID
            }

            case vmRet:
                index = code[pc++];
                op1 = pop();
                assert(op1->type == vtRetAddr);
                SetPC(op1->iVal);
                op1 = pop();
                assert(op1->type == vtSavedFB);
                fb = op1->iVal;
                while (index--)
                    cleanUp(pop());
                if (pc == -1)
                    killVM = true;
                else
                    push(&retVal);
                break;
            case vmRetN:
                index = code[pc++];
                op1 = pop();
                assert(op1->type == vtRetAddr);
                SetPC(op1->iVal);
                op1 = pop();
                assert(op1->type == vtSavedFB);
                fb = op1->iVal;
                while (index--)
                    cleanUp(pop());
                if (pc == -1)
                    killVM = true;
                retVal.type = vtVoid;
                break;

            case vmSetRet:
                copyVal(&retVal, op2);
                beforePushS1(op2, ot2);
                break;
            case vmSetRet0:
                retVal.type = vtInt;
                retVal.iVal = 0;
                vm_error("Function did not return a value");
                break;

            case vmPop:
                cleanUp(pop());
                break;
            case vmPopN:
                index = getWord();
                while (index--)
                    cleanUp(pop());
                break;

            case vmSwap:
                op1 = pop();
                op2 = pop();
                moveVal(&temp, op2);
                push(op1);
                push(&temp);
                break;
            case vmStackDup:
                op1 = pop();
                push(op1);
                copyVal(op2, op1);
                push(op2);
                break;
            case vmAlloc:
                #define oldst tempLong
                oldst = st;
                index = getWord();
                temp.type = vtInt;
                temp.iVal = 0;
                while (index--)
                    push(&temp);
                temp.iVal = 0x80000000 | oldst;
                push(&temp);
                break;
                #undef oldst
            case vmNew:
                op1 = pop(); // string
                op2 = pop(); // count
                assert(op1->type == vtString);
                assert(op2->type == vtInt);
                index = globs.alloc(op2->iVal, op1->lock());
                gsize = globs.size();
                VM_OUTPUT(HostFPrintF(hostFile, "new '%s'[%ld] = 0x%08lx\n", op1->lock(), op2->iVal, index));
                VM_OUTPUT(op1->unlock());
                op1->unlock();
                op1->release();
                op2->iVal = index;
                push(op2);
                break;
            case vmNewObj:
                op1 = pop(); // string
                op2 = pop(); // count
                assert(op1->type == vtString);
                assert(op2->type == vtInt);
                objID = getWord();
                op1Str = op1->lock();
                tempLong = strlen(op1Str);
                index = globs.alloc(op2->iVal, op1Str);
                gsize = globs.size();
                if (index) {
                    for (int i=0;i<op2->iVal;i++)
                        setObjinfo(index + tempLong * i, objID);
                }
                op1->unlockRelease();
                op2->iVal = index;
                push(op2);
                break;

            case vmDelete:
                op1 = STACK_MINUS_1; //pop();
                assert(op1->type == vtInt);
                VM_OUTPUT(HostFPrintF(hostFile, "delete 0x%08lx\n", op1->iVal));
                globs.release(op1->iVal);
                gsize = globs.size();
                break;

            case vmLink:
                temp.type = vtSavedFP;
                temp.iVal = fp;
                push(&temp);
                fp = st - code[pc++] - 3;
                break;
        
            case vmUnLink:
                op1 = pop();
                assert(op1->type == vtSavedFP);
                fp = op1->iVal;
                break;

            case vmHalt:
                killVM = true;
                break;

            case vmLoad:
                op1 = pop();
                copyVal(&temp, deref(op1->iVal));
                push(&temp);
                break;

            case vmStore:
                op1 = pop();
                op2 = pop();
                dst = deref(op2->iVal);
                //if (dst->type != op1->type) {
                //	vm_error("Assigning to a location of different type");
                //}
                cleanUp(dst);
                if (code[pc] == vmPop) {
                    moveVal(dst, op1);
                    pc++;
                } else {
                    copyVal(dst, op1);
                    push(op1);
                }
                break;

            case vmCopy:
                #define srci tempLong
                #define dsti funcID
                assert(stack[st-1].type == vtInt);
                assert(stack[st-2].type == vtInt);
                srci = pop()->iVal;
                dsti = stack[st-1].iVal; // leave on stack
                index = getWord();
                while (index--) {
                    //if (dst->type != src->type) {
                    //	vm_error("Copying to a location of different type");
                    //}
                    dst = deref(dsti++);
                    src = deref(srci++);
                    cleanUp(dst);
                    copyVal(dst, src);
                }
                break;
                #undef dsti
                #undef srci

            case vmGetAt:
                op1 = pop();
                op2 = pop();
                assert(op1->type == vtInt);
                assert(op2->type == vtInt); // string addr
                src = deref(op2->iVal);
                // PocketC-compatibility may cause this instruction to run on non-strings
                if (src->type != vtString)
                    vm_error("@[] operator can only be used on string variables");
                srcStr = src->lock();
                if (op1->iVal > strlen(srcStr))
                    vm_error("string subscript out of range");
                op2->type = vtChar;
                op2->cVal = srcStr[op1->iVal];
                src->unlock();
                push(op2);
                break;

            case vmSetAt:
                op3 = pop();
                op1 = pop();
                op2 = pop();
                assert(op1->type == vtInt);
                assert(op2->type == vtInt); // string addr
                assert(op3->type == vtChar);
                src = deref(op2->iVal);
                // PocketC-compatibility may cause this instruction to run on non-strings
                if (src->type != vtString)
                    vm_error("@[] operator can only be used on string variables");
                src->makeEditable();
                srcStr = src->lock();
                if (op1->iVal >= strlen(srcStr))	// can't modify the '\0'
                    vm_error("string subscript out of range");
                srcStr[op1->iVal] = op3->cVal;
                src->unlock();
                push(op3); // TODO: we could make this operand first, so we don't have to pop it
                break;

            case vmLoadI:
                index = getWord();
                assert(index < gsize);
                copyVal(&temp, &globs[index]);
                push(&temp);
                break;

            case vmLoadFP:
                index = getWord();
                copyVal(&temp, &stack[index + fp]);
                push(&temp);
                break;

            case vmLoadF0:
                op1 = &stack[fp]; // this
            LoadFx:
                index = getWord();
                assert(op1->type == vtInt);
                op2 = deref(op1->iVal + index); // field
                copyVal(&temp, op2);
                push(&temp);
                break;

            case vmLoadF1:
                op1 = &stack[fp+1]; // this
                goto LoadFx;

            case vmStoreI:
                op1 = STACK_MINUS_1;
                index = getWord();
                assert(index < gsize);
                dst = &globs[index];
                //if (dst->type != op1->type) {
                //	vm_error("Assigning to a location of different type");
                //}
                cleanUp(dst);
                copyVal(dst, op1);
                break;
                
            case vmStoreFP:
                op1 = STACK_MINUS_1;
                index = getWord();
                assert(index + fp < st);
                dst = &stack[index + fp];
                //if (dst->type != op1->type) {
                //	vm_error("Assigning to a location of different type");
                //}
                cleanUp(dst);
                copyVal(dst, op1);
                break;

            case vmStoreF0:
                op2 = &stack[fp]; // this
            StoreFx:
                op1 = STACK_MINUS_1;
                index = getWord();
                assert(op2->type == vtInt);
                op3 = deref(op2->iVal + index); // field
                //if (op3->type != op1->type) {
                //	vm_error("Assigning to a location of different type");
                //}
                cleanUp(op3);
                copyVal(op3, op1);
                break;

            case vmStoreF1:
                op2 = &stack[fp+1]; // this
                goto StoreFx;

            case vmIncAI: POST_OP(++, iVal); break;
            case vmIncAC: POST_OP(++, cVal); break;
            case vmIncAF: POST_OP(++, fVal); break;
            case vmDecAI: POST_OP(--, iVal); break;
            case vmDecAC: POST_OP(--, cVal); break;
            case vmDecAF: POST_OP(--, fVal); break;
            case vmIncBI: PRE_OP(++, iVal); break;
            case vmIncBC: PRE_OP(++, cVal); break;
            case vmIncBF: PRE_OP(++, fVal); break;
            case vmDecBI: PRE_OP(--, iVal); break;
            case vmDecBC: PRE_OP(--, cVal); break;
            case vmDecBF: PRE_OP(--, fVal); break;

            case vmIncAII: index = getWord(); POST_OP(+=index, iVal); break;
            case vmDecAII: index = getWord(); POST_OP(-=index, iVal); break;
            case vmIncBII: index = getWord(); PRE_OP(+=index, iVal); break;
            case vmDecBII: index = getWord(); PRE_OP(-=index, iVal); break;

            case vmBAndI: BINARY(&, vtInt, iVal); break;
            case vmBAndC: BINARY(&, vtChar, cVal); break;
            case vmBOrI:  BINARY(|, vtInt, iVal); break;
            case vmBOrC:  BINARY(|, vtChar, cVal); break;
            case vmBNotI:  UNARY(~, vtInt, iVal); break;
            case vmBNotC:  UNARY(~, vtChar, cVal); break;
            case vmSLI:   BINARY(<<, vtInt, iVal); break;
            case vmSLC:   BINARY(<<, vtChar, cVal); break;
            case vmSRI:   BINARY(>>, vtInt, iVal); break; 
            case vmSRC:   BINARY(>>, vtChar, cVal); break;
            case vmXorI:  BINARY(^, vtInt, iVal); break;
            case vmXorC:  BINARY(^, vtChar, cVal); break;

            case vmSwitchI:
            case vmSwitchC:
            case vmSwitchF:
            case vmSwitchS:
                #define nCase tempLong
                #define i funcID
                op1 = pop();
                nCase = code[pc++];
                for (i=0;i<nCase;i++) {
                    // if case equals the top of the stack, jump
                    assert(code[pc] == vmCase);
                    pc++;
                    index = getLong();
                    bool equal = false;
                    if (inst == vmSwitchI) {
                        assert(op1->type == vtInt);
                        equal = (op1->iVal == index);
                    } else if (inst == vmSwitchC) {
                        assert(op1->type == vtChar);
                        equal = (op1->cVal == index);
                    } else if (inst == vmSwitchF) {
                        assert(op1->type == vtFloat);
                        equal = (op1->fVal == *((float*)(&index)));
                    } else if (inst == vmSwitchS) {
                        assert(op1->type == vtString);
                        equal = (0 == strcmp(op1->lock(), &str[index]));
                        op1->unlock();
                    } else {
                        assert(!"switch instruction broken");
                    }

                    index = getWord();
                    if (equal) {
                        pc = index + fb;
                        break;
                    }					
                }
                if (i == nCase) {
                    // if default, go there
                    if (code[pc] == vmDefault) {
                        pc++;
                        pc = getWord() + fb;
                    }
                    // otherwise the next instruction will jump to the end of the switch
                }
                cleanUp(op1);
                break;
                #undef i
                #undef nCase
            case vmNoOp2:
                index = getWord();
            case vmNoOp:
                break;
            
            case vmVtoI:
                if (op2->type != vtInt) {
                    if (op2->type == vtChar)
                        inst = vmCtoI;
                    else if (op2->type == vtFloat)
                        inst = vmFtoI;
                    else if (op2->type == vtString)
                        inst = vmStoI;
                    assert(inst != vmVtoI);
                    goto reeval;
                }
                break;

            case vmVtoC:
                if (op2->type != vtChar) {
                    if (op2->type == vtInt)
                        inst = vmItoC;
                    else if (op2->type == vtFloat)
                        inst = vmFtoC;
                    else if (op2->type == vtString)
                        inst = vmStoC;
                    assert(inst != vmVtoC);
                    goto reeval;
                }
                break;

            case vmVtoF:
                if (op2->type != vtFloat) {
                    if (op2->type == vtChar)
                        inst = vmCtoF;
                    else if (op2->type == vtInt)
                        inst = vmItoF;
                    else if (op2->type == vtString)
                        inst = vmStoF;
                    assert(inst != vmVtoF);
                    goto reeval;
                }
                break;

            case vmVtoS:
                if (op2->type != vtString) {
                    if (op2->type == vtChar)
                        inst = vmCtoS;
                    else if (op2->type == vtFloat)
                        inst = vmFtoS;
                    else if (op2->type == vtInt)
                        inst = vmItoS;
                    assert(inst != vmVtoS);
                    goto reeval;
                }
                break;

            case vmVtoB:
                if (op2->type == vtChar)
                    inst = vmCtoI;
                else if (op2->type == vtFloat)
                    inst = vmFtoI;
                else if (op2->type == vtInt)
                    break;
                else if (op2->type == vtString)
                    inst = vmStoB;
                assert(inst != vmVtoB);
                goto reeval;

            case vmVOp2:
                // type match the top two stack elements
                typeMatch(op1, op2);
                // determine the correct new operator
                inst = code[pc++];
                // validate type
                if ((inst >= vmSubI && inst <= vmDivI && op2->type == vtString) ||
                    ((inst == vmModI || inst >= vmBAndI && inst <= vmXorI) && (op2->type == vtString || op2->type == vtFloat)))
                    vm_error("invalid operand type");

                inst += op2->type - vtInt;
                goto reeval;

            case vmVOp1:
                // determine the correct new operator
                inst = code[pc++];
                // validate type
                if (inst >= vmIncAI && inst <= vmDecBI) {
                    assert(op2->type == vtInt);
                    op3 = deref(op2->iVal);
                    if (op3->type == vtString)
                        vm_error("invalid operand type");
                    
                    inst += op3->type - vtInt;

                } else {
                    if ((inst == vmNegI && op2->type == vtString) ||
                        (inst == vmBNotI && (op2->type == vtString || op2->type == vtFloat)))
                        vm_error("invalid operand type");

                    inst += op2->type - vtInt;
                }
                goto reeval;
            
            case vmStoreV:
                op1 = pop();
                op2 = pop();
                dst = deref(op2->iVal);
                typeCast(op1, dst->type);
                cleanUp(dst);
                if (code[pc] == vmPop) {
                    moveVal(dst, op1);
                    pc++;
                } else {
                    copyVal(dst, op1);
                    push(op1);
                }
                break;
                
            case vmCompAddr:
                index = op2->iVal;
                index &= 0x3fff;
                index |= ((op2->iVal >> 16) & 0xc000);
                op2->iVal = index;
                break;

            default:
                vm_error("unexpected instruction");

        }
        if (inst != vmArray)
            nArray = 1;
    }
    
    if (pushRet) {
        SetPC(prev_pc);
        fb = prev_fb;
    }
    
    callDepth--;
    return true;
}
