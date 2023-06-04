#define ASSERT(x)

//
// Virtual machine
//
#include "PocketC.h"
//#include <HostControl.h>
void Alert(char*);

extern char* inst[];
extern bool bForceExit;
extern short appVer;

// this depends on the fact that a Value is 6-bytes. May vary on other platforms
inline void moveVal(short* dest, short* src) {
    long* pd = (long*)dest;
    long* ps = (long*)src;
    *pd++ = *ps++;
    *(short*)pd = *(short*)ps;
    
    //*dest++ = *src++;
    //*dest++ = *src++;
    //*dest++ = *src++;
}

#ifdef POCKETC_FAT
#include "pilotlib.cpp"
#endif

// code segment helpers
long fullAddr(long addr) {
    for (short i=0;i<MAX_CODE_SEGS;i++) {
        if (code == codesegs[i])
            return 0x10000 * (long)i + addr;
    }
    return addr;
}

unsigned char getCode(long addr) {
    short i = (addr >> 16);
    return codesegs[i][addr & 0xffff];
}

void setPC(long addr) {
    pc = addr & 0xffff;
    code = codesegs[addr >> 16];
}

unsigned short getWord() {
    unsigned short ret = 0;
    ret |= code[pc++];
    ret <<=8;
    ret |= code[pc++];
    return ret;
}

long getInt() {
    long ret = 0;
    unsigned char* codepc = &code[pc];
    ret |= *codepc++;
    ret <<= 8;
    ret |= *codepc++;
    ret <<= 8;
    ret |= *codepc++;
    ret <<= 8;
    ret |= *codepc++;
    pc += 4;
    return ret;
}

void rt_oom() {
    // Trash the stack so that we can display error dialog
    for (short i=0; i<st; i++)
        if (stack[i].type==vtString && stack[i].sVal) ReleaseString(&stack[i]);
    //h_free(stack_h); This is freed later
    st = 0;
    vm_error("Out of memory", pc);
}	

//
//
// stack functions
//
void push(Value& val) {
    moveVal((short*)&stack[st++], (short*)&val);
    if (st >= stackSize) { // Grow stack
        MemHandleUnlock(stack_h);
        Err e = MemHandleResize(stack_h, (stackSize+40)*sizeof(Value));
        if (e) {
            // Cleanup stack to free enough memory to run
            stack = (Value*)MemHandleLock(stack_h);
            for (short i=0; i<st; i++)
                if (stack[i].type==vtString) ReleaseString(&stack[i]);
            //h_free(stack_h); This is freed later
            st = 0;
            vm_error("Stack overflow -- out of memory", pc);
        }
        stackSize+=40;
        stack = (Value*)MemHandleLock(stack_h);
    }
}

void pop(Value& var) {
    moveVal((short*)&var, (short*)&stack[--st]);
}

void shrinkStack() {
    if (stackSize > 81 + st) { // Shrink stack
        MemHandleUnlock(stack_h);
        MemHandleResize(stack_h, (stackSize-40)*sizeof(Value));
        stackSize-=40;
        stack = (Value*)MemHandleLock(stack_h);
    
    }
}

void ValueToNew(Value& val) {
    if (val.type == vtString) {
        Handle h = val.sVal;
        char* str = (char*)MemHandleLock(h);
        if (!NewStringFromConst(&val, str)) rt_oom();
        MemHandleUnlock(h);
        MemHandleFree(h);
    }
}

void ValueToOld(Value& val) {
    if (val.type == vtString) {
        Handle hStr = h_strdup3(LockString(&val));
        if (!hStr) rt_oom();
        UnlockReleaseString(&val);
        val.sVal = hStr;
    }
}

void oldpush(Value& val) {
    ValueToNew(val);
    push(val);
}

void oldpop(Value& val) {
    pop(val);
    ValueToOld(val);
}

void oldTypeCast(Value& val, VarType type) {
    ValueToNew(val);
    typeCast(val, type);
    ValueToOld(val);
}

void oldTypeMatch(Value& l, Value& r) {
    ValueToNew(l); ValueToNew(r);
    typeMatch(l, r);
    ValueToOld(l); ValueToOld(r);
}

void oldCleanup(Value& val) {
    if (val.type == vtString)
        MemHandleFree(val.sVal);
    val.sVal = 0;
}

void copyVal(Value* dest, Value* src) {
    //memcpy(dest, src, sizeof(Value));
    moveVal((short*)dest, (short*)src);
    if (dest->type == vtString) {
        AcquireString(dest, dest);
    }
    /*
    else if (dest->type == vtDouble) {
        dest->iVal = CopyDouble(dest->iVal);
    }
    */
}

//
//
// VM Value helper routines
//
#define _typeMatch(a,b) { if ((a).type != (b).type) typeMatch((a), (b)); }
void typeMatch(Value &a, Value &b) {
    if (a.type == b.type) return;
    if (b.type == vtString) {
        typeCast(a, vtString);
    } else if (a.type == vtString) {
        typeCast(b, vtString);
    } else if (b.type == vtFloat) {
        typeCast(a, vtFloat);
    } else if (a.type == vtFloat) {
        typeCast(b, vtFloat);
    } else if (b.type == vtInt) {
        typeCast(a, vtInt);
    } else {
        typeCast(b, vtInt);
    }
}

VoidHand cast_h_malloc(short size) {
    VoidHand res = h_malloc(size);
    if (!res) rt_oom();
    return res;
}

#define _typeCast(v,t) { if ((v).type != (t)) typeCast((v), (t)); }
void typeCast(Value& v, VarType type) {
    if (v.type == type) return;
    if (type == vtString) {
        char* temp;
        
        if (v.type == vtChar) {
            char c = v.cVal;
            temp = NewString(&v, 1);
            if (!temp) rt_oom();
            temp[0] = c;
            temp[1] = '\0';
            MemHandleUnlock(v.sVal);
        } else if (v.type == vtInt) {
            long l = v.iVal;
            temp = NewString(&v, 12);
            if (!temp) rt_oom();
            itoa(l, temp, 10);
            MemHandleUnlock(v.sVal);
        } else {
            float f = v.fVal;
            temp = NewString(&v, 16);
            if (!temp) rt_oom();
            ftoa(f, temp);
            MemHandleUnlock(v.sVal);
        }
        v.type = vtString;
        return;
    }
    if (type == vtFloat) {
        if (v.type == vtString) {
            float f;
            char* data = LockString(&v);
            f = (float)atof(data);
            UnlockReleaseString(&v);
            v.fVal = f;
        }
        else if (v.type == vtInt) v.fVal = (float)v.iVal;
        else v.fVal = (float)(short)v.cVal;
        v.type = vtFloat;
        return;
    }
    if (type == vtInt) {
        if (v.type == vtString) {
            long l;
            char* data = LockString(&v);
            l = atoi(data);
            UnlockReleaseString(&v);
            v.iVal = l;
        }
        else if (v.type == vtFloat) v.iVal = (long)v.fVal;
        else v.iVal = (long)v.cVal;
        v.type = vtInt;
        return;
    }
    if (v.type == vtString) {
        char c;
        char* s = LockString(&v);
        c = s[0];
        UnlockReleaseString(&v);
        v.cVal = c;
    }
    else if (v.type == vtInt) v.cVal = (char)v.iVal;
    else v.cVal = (char)(long)v.fVal;
    v.type = vtChar;
}

void cleanup(Value& v) {
    if (v.type==vtString)
        if (v.sVal) {
            ReleaseString(&v);
            v.sVal = NULL;
        }
}

void truthVal(Value& v) {
    short t=0;
    switch (v.type) {
        case vtInt:
            if (v.iVal != 0) v.iVal = 1;
            break;
        case vtChar:
            if (v.cVal != 0) v.iVal = 1;
            else v.iVal = 0;
        case vtFloat:
            if (v.fVal != 0.0) v.iVal = 1;
            else v.iVal = 0;
            break;
        case vtString:
            if (v.sVal) {
                char* data = LockString(&v);
                if (*data)
                    t=1;
                UnlockReleaseString(&v);
            }
            v.iVal = t;
            break;
    }
    v.type = vtInt;
}

short compare(Value& l, Value& r) {
    switch(l.type) {
        case vtInt:
            return (l.iVal==r.iVal ? 0 : (l.iVal > r.iVal)*2-1);
        case vtChar:
            return (l.cVal==r.cVal ? 0 : (l.cVal > r.cVal)*2-1);
        case vtFloat:
            return (l.fVal==r.fVal ? 0 : (l.fVal > r.fVal)*2-1);
        case vtString:
            return h_strcmp(&l, &r);
    }
    return 0;
}


//
//
// VM - the real deal!!!!
//
//extern ULong heapFree, heapMax;



/*
ULong instCount[vmNumInstr];
char* instName[] = {
    // constants
    "CInt", "CChar", "CFloat", "CString",
    // typecasts
    "ToInt", "ToChar", "ToFloat", "ToStr",
    // variables
    "GetG", "SetG", "GetL", "SetL", "Array",
    "IncGA", "DecGA", "IncLA", "DecLA",
    "IncGB", "DecGB", "IncLB", "DecLB",
    // operators
    "Add", "Sub", "Mult", "Div", "Mod", "Neg", "Not", "And", "Or",
    // relational operators
    "Eq", "NEq", "LT", "LTE", "GT", "GTE",
    // flow control
    "Jmp", "JmpZ", "JmpNZ",
    "OldCall", "CallBI", "Ret", "SetRet", "SetRet0", "Pop", "PopN",
    "Link", "UnLink", "Halt", "Swap", "LibCall",

    "CWord", "CWdPFP", "Call",
    "Load", "Save",
    "IncA", "DecA", "IncB", "DecB",
    "BAnd", "BOr", "BNot", "SL", "SR", "Xor",
    "NoOp", "NoOp2", "XXX", "XXX",
    "AddExtI", "GetC", "SetC", "Case"
};
*/

Value* deref(short ptr) {
    if (ptr < 0x6000) {
        if (ptr < globalOff) return &globals[ptr];
        else vm_error("Global ref out of range", pc);
    } else {
        ptr -= 0x6000;
        if (ptr < st) return &stack[ptr];
        else vm_error("Stack ref out of range", pc);
    }
    return NULL; // Never occurs
}

extern long atexit_func;

bool exec() {
    //MemSet(instCount, sizeof(instCount), 0);
    
    short arrayIndex = -1, count = 50;
    unsigned short index, loc, mask;
    long val;
    //long lIndex;
    Value op1, op2, op3, *v, *v2;
    char* ca, *cb, *cc;
    //Handle h;
    //String* pStr;
    unsigned char byte;
    bool execRes = true;
    
    //HostFILE* pFile = HostFOpen("d:\\Development\\PocketC\\Debugger.txt", "w");
    
    while (true) {
    //HostFPrintF(pFile, "%5d\t%s\tst: %d\n", pc, instName[code[pc]], st);
    //HostFFlush(pFile);
    //if (debug) fprintf(stderr, "%4d\t%s\n", pc, inst[code[pc]]);
    //if (pc >= codePtr || pc >= CODE_SEG_SIZE) vm_error("PC out of range", pc);

    if (killVM) {
        execRes = false;
        goto _WeOuttaHere;
    }

    //if (stopApp && !atexit_func) {
    //	goto _WeOuttaHere;
    //}
        
    if (nativeEnableEvents && !killEvents && ++count > 100) {
         count=0;
         if (UIYield(false)) {
         	execRes = false;
         	goto _WeOuttaHere;
         }
    }
    //MemHeapFreeBytes(0, &heapFree, &heapMax);
    byte = code[pc++];
    
    //instCount[byte]++;
    
    switch (byte) {
        case vmCInt: { // add vmCInt2, vmCInt1 ?
            op1.type = vtInt;
            op1.iVal = getInt();
            if (arrayIndex < 0) push(op1);
            else for (index=0;index<arrayIndex;index++) push(op1);
            break;
        }

        case vmCFloat: {
            index = getWord();
            op1.type = vtFloat;
            op1.fVal = cFloats[index];
            if (arrayIndex < 0) push(op1);
            else for (index=0;index<arrayIndex;index++) push(op1);
            break;
        }

        case vmCString: {
            index = getWord();
            op1.type = vtString;
            if (arrayIndex < 0) {
                NewConstString(&op1, &cStrings[index]);
                push(op1);
            } else {
                NewConstString(&op1, &cStrings[index]);
                for (index=0;index<arrayIndex;index++) {
                    push(op1);
                }
            }
            break;
        }

        case vmCChar: {
            op1.type = vtChar;
            op1.cVal = code[pc++];
            if (arrayIndex < 0) push(op1);
            else for(index=0;index<arrayIndex;index++) push(op1);
            break;
        }

        case vmCWord: { // add vmCInt2, vmCInt1 ?
            op1.type = vtInt;
            op1.iVal = getWord();
            if (arrayIndex < 0) push(op1);
            else for (index=0;index<arrayIndex;index++) push(op1);
            break;
        }

        case vmCWordPFP:
            op1.type = vtInt;
            op1.iVal = getWord() + fp;
            push(op1);
            break;
            
        /*
        case vmCDouble: {
            index = getWord();
            double* pd;
            op1.type = vtDouble;
            op1.iVal = NewDouble(&pd);
            *pd = (double*)(&cFloats[index]);
        }
        */

        case vmToInt:
        case vmToChar:
        case vmToFloat:
        case vmToString: {
            _typeCast(stack[st-1], (VarType)(byte - vmToInt + vtInt));
            break;
        }

        case vmLoad: {
            v = deref(stack[st-1].iVal);
            copyVal(&stack[st-1], v);
            break;
        }
        
        case vmSave: {
            v = deref(stack[st-2].iVal);
            _typeCast(stack[st-1], v->type);
            cleanup(*v);
            if (code[pc] == vmPop) {
                moveVal((short*)v, (short*)&stack[st-1]);
                st-=2;
                pc++;
            } else {
                moveVal((short*)&stack[st-2], (short*)&stack[st-1]);
                copyVal(v, &stack[st-2]);
                st--;
            }

            /*
            v = deref(stack[st-2].iVal);
            moveVal((short*)&stack[st-2], (short*)&stack[st-1]);
            st--;
            _typeCast(stack[st-1], v->type);
            cleanup(*v);
            if (code[pc] == vmPop) {
                moveVal((short*)v, (short*)&stack[--st]);
                pc++;
            } else {
                copyVal(v, &stack[st-1]);
            }
            */
                
            /*
            pop(op1); // Value is on top of stack
            pop(op2); // Address comes next
            ASSERT(op2.type == vtInt);
            v = deref(op2.iVal);
            _typeCast(op1, v->type);
            copyVal(v, &op1);
            push(op1); // Put the value back on the stack
            */
            break;
        }

        case vmArray: {
            // This is now used only for pushing locals
            pop(op1);
            _typeCast(op1, vtInt);
            arrayIndex = op1.iVal;
            if (arrayIndex < 0) vm_error("Negative array index.", pc);
            break;
        }

        case vmIncA:
        case vmIncB: {
            //pop(op1);
            //v = deref(op1.iVal);
            v = deref(stack[--st].iVal);
            if (v->type == vtString)
                vm_error("Attempt to inc string", pc);
            
            if (byte == vmIncB)
                push(*v); // This doesn't copy a string
                
            switch (v->type) {
                case vtInt: v->iVal++; break;
                case vtChar: v->cVal++; break;
                case vtFloat: v->fVal++; break;
            }
            if (byte == vmIncA)
                push(*v);
            break;
        }				
        
        case vmDecA:
        case vmDecB: {
            //pop(op1);
            //v = deref(op1.iVal);
            v = deref(stack[--st].iVal);
            if (v->type == vtString)
                vm_error("Attempt to dec string", pc);
            
            if (byte == vmDecB)
                push(*v); // This doesn't copy a string
                
            switch (v->type) {
                case vtInt: v->iVal--; break;
                case vtChar: v->cVal--; break;
                case vtFloat: v->fVal--; break;
            }
            if (byte == vmDecA)
                push(*v);
            break;
        }

        case vmAdd: {
            pop(op2);
            v = &stack[st-1];
            _typeMatch(*v, op2);
            switch (v->type) {
                case vtChar: v->cVal += op2.cVal; break;
                case vtInt: v->iVal += op2.iVal; break;
                case vtFloat: v->fVal += op2.fVal; break;
                case vtString: {
                    ca = LockString(v);
                    cb = LockString(&op2);
                    cc = NewString(&op3, strlen(ca) + strlen(cb));
                    if (!cc) {
                        UnlockString(v);
                        UnlockReleaseString(&op2);
                        vm_error("Out of memory concatenating strings", pc);
                    }
                    strcpy(cc, ca);
                    strcat(cc, cb);
                    UnlockReleaseString(v);
                    UnlockReleaseString(&op2);
                    MemHandleUnlock(op3.sVal);
                    *v = op3;
                    break;
                }
            }
            break;
        }

        case vmSub: {
            pop(op2);
            v = &stack[st-1];
            _typeMatch(*v, op2);
            switch (v->type) {
                case vtChar: v->cVal -= op2.cVal; break;
                case vtInt: v->iVal -= op2.iVal; break;
                case vtFloat: v->fVal -= op2.fVal; break;
                case vtString:
                    ReleaseString(&op2);
                    vm_error("Attempt to subtract strings.", pc);
                    break;
            }
            break;
        }

        case vmMult: {
            pop(op2);
            v = &stack[st-1];
            _typeMatch(*v, op2);
            switch (v->type) {
                case vtChar: v->cVal = v->cVal * op2.cVal; break;
                case vtInt: v->iVal = v->iVal * op2.iVal; break;
                case vtFloat: v->fVal = v->fVal * op2.fVal; break;
                case vtString:
                    ReleaseString(&op2);
                    vm_error("Attempt to multiply strings.", pc);
                    break;
            }
            break;
        }

        /*case vmMult: {
            pop(op2);
            pop(op1);
            _typeMatch(op1, op2);
            op3.type = op1.type;
            switch (op1.type) {
                case vtChar: op3.cVal = op1.cVal * op2.cVal; break;
                case vtInt: op3.iVal = op1.iVal * op2.iVal; break;
                case vtFloat: op3.fVal = op1.fVal * op2.fVal; break;
                case vtString:
                    op3.sVal = op1.sVal;
                    h_free(op2.sVal);
                    vm_warn("Attempt to multiply strings.", pc);
                    break;
            }
            push(op3);
            break;
        }*/

        case vmDiv: {
            pop(op2);
            v = &stack[st-1];
            _typeMatch(*v, op2);
            switch (v->type) {
                case vtChar:
                    if (op2.cVal == 0) goto div_zero;
                    v->cVal = v->cVal / op2.cVal;
                    break;
                case vtInt:
                    if (op2.iVal == 0) goto div_zero;
                    v->iVal = v->iVal / op2.iVal;
                    break;
                case vtFloat:
                    if (op2.fVal == 0.0) goto div_zero;
                    v->fVal = v->fVal / op2.fVal;
                    break;
                case vtString:
                    ReleaseString(&op2);
                    vm_error("Attempt to divide strings.", pc);
                    break;
            }
            break;
div_zero:
            vm_error("Divide by zero", pc);
            break;
        }

        case vmMod: {
            pop(op2);
            v = &stack[st-1];
            _typeMatch(*v, op2);
            switch (v->type) {
                case vtChar:
                    if (op2.cVal == 0) goto mod_zero;
                    v->cVal = v->cVal % op2.cVal;
                    break;
                case vtInt:
                    if (op2.iVal == 0) goto mod_zero;
                    v->iVal = v->iVal % op2.iVal;
                    break;
                case vtFloat:
                    vm_error("Attempt to mod floats", pc);
                    break;
                case vtString:
                    ReleaseString(&op2);
                    vm_error("Attempt to mod strings", pc);
                    break;
            }
            break;
mod_zero:
            vm_error("Mod by zero", pc);
            break;
        }

        case vmNeg: {
            v = &stack[st-1];
            switch (v->type) {
                case vtChar: v->cVal = -v->cVal; break;
                case vtInt: v->iVal = -v->iVal; break;
                case vtFloat: v->fVal = -v->fVal; break;
                /*
                case vtDouble: {
                    double* pd = GetDouble(v->iVal);
                    *pd = -*pd;
                }
                */
                case vtString:
                    vm_error("Attempt to negate string.", pc);
                    break;
            }
            break;
        }

        case vmBAnd: {
            pop(op2);
            pop(op1);
            _typeMatch(op1, op2);
            op3.type = op1.type;
            switch (op1.type) {
                case vtChar: op3.cVal = op1.cVal & op2.cVal; break;
                case vtInt: op3.iVal = op1.iVal & op2.iVal; break;
                case vtFloat:
                    vm_error("Attempt to & floats.", pc);
                    break;
                case vtString:
                    op3 = op1;
                    ReleaseString(&op2);
                    vm_error("Attempt to & strings.", pc);
                    break;
            }
            push(op3);
            break;
        }

        case vmBOr: {
            pop(op2);
            pop(op1);
            _typeMatch(op1, op2);
            op3.type = op1.type;
            switch (op1.type) {
                case vtChar: op3.cVal = op1.cVal | op2.cVal; break;
                case vtInt: op3.iVal = op1.iVal | op2.iVal; break;
                case vtFloat:
                    vm_error("Attempt to | floats.", pc);
                    break;
                case vtString:
                    op3 = op1;
                    ReleaseString(&op2);
                    vm_error("Attempt to | strings.", pc);
                    break;
            }
            push(op3);
            break;
        }

        case vmXor: {
            pop(op2);
            pop(op1);
            _typeMatch(op1, op2);
            op3.type = op1.type;
            switch (op1.type) {
                case vtChar: op3.cVal = op1.cVal ^ op2.cVal; break;
                case vtInt: op3.iVal = op1.iVal ^ op2.iVal; break;
                case vtFloat:
                    vm_error("Attempt to ^ floats.", pc);
                    break;
                case vtString:
                    op3 = op1;
                    ReleaseString(&op2);
                    vm_error("Attempt to ^ strings.", pc);
                    break;
            }
            push(op3);
            break;
        }

        case vmSR: {
            pop(op2);
            pop(op1);
            _typeCast(op2, vtInt);
            op3.type = op1.type;
            switch (op1.type) {
                case vtChar: op3.cVal = op1.cVal >> op2.iVal; break;
                case vtInt: op3.iVal = op1.iVal >> op2.iVal; break;
                case vtFloat:
                    vm_error("Attempt to shift float.", pc);
                    break;
                case vtString:
                    op3 = op1;
                    ReleaseString(&op2);
                    vm_error("Attempt to shift string.", pc);
                    break;
            }
            push(op3);
            break;
        }

        case vmSL: {
            pop(op2);
            pop(op1);
            _typeCast(op2, vtInt);
            op3.type = op1.type;
            switch (op1.type) {
                case vtChar: op3.cVal = op1.cVal << op2.iVal; break;
                case vtInt: op3.iVal = op1.iVal << op2.iVal; break;
                case vtFloat:
                    vm_error("Attempt to shift float.", pc);
                    break;
                case vtString:
                    op3 = op1;
                    ReleaseString(&op2);
                    vm_error("Attempt to shift string.", pc);
                    break;
            }
            push(op3);
            break;
        }

        case vmBNot: {
            switch (stack[st-1].type) {
                case vtChar: stack[st-1].cVal = ~stack[st-1].cVal; break;
                case vtInt: stack[st-1].iVal = ~stack[st-1].iVal; break;
                case vtFloat:
                    vm_error("Attempt to ~ float.", pc);
                    break;
                case vtString:
                    vm_error("Attempt to ~ string.", pc);
                    break;
            }
            break;
        }

        case vmNot: {
            truthVal(stack[st-1]);
            stack[st-1].iVal = !stack[st-1].iVal;
            break;
        }

        case vmAnd: {
            v = &stack[st-2];
            v2 = &stack[st-1];
            truthVal(*v);
            truthVal(*v2);
            v->iVal = v->iVal && v2->iVal;
            st--;
            break;

            /*
            truthVal(stack[st-1]);
            truthVal(stack[st-2]);
            stack[st-2].iVal = stack[st-1].iVal && stack[st-2].iVal;
            st--;
            break;
            */
        }

        case vmOr: {
            v = &stack[st-2];
            v2 = &stack[st-1];
            truthVal(*v);
            truthVal(*v2);
            v->iVal = v->iVal || v2->iVal;
            st--;
            break;
            
            /*
            pop(op2);
            pop(op1);
            truthVal(op1);
            truthVal(op2);
            op1.iVal = op1.iVal || op2.iVal;
            push(op1);
            break;
            */
        }

        case vmEq:
        case vmNEq:
        case vmLT:
        case vmLTE:
        case vmGT:
        case vmGTE: {
            v2 = &stack[--st];
            v = &stack[--st];
            _typeMatch(*v, *v2);
            short res = compare(*v, *v2);
            cleanup(*v);
            cleanup(*v2);
            op1.type = vtInt;
            if (res == 0) {
                if (byte==vmEq || byte==vmLTE || byte==vmGTE) op1.iVal = 1;
                else op1.iVal = 0;
            } else if (res > 0) {
                if (byte==vmGT || byte==vmGTE || byte==vmNEq) op1.iVal = 1;
                else op1.iVal = 0;
            } else {
                if (byte==vmLT || byte==vmLTE || byte==vmNEq) op1.iVal = 1;
                else op1.iVal = 0;
            }
            push(op1);
            break;
            /*
            pop(op2);
            pop(op1);
            _typeMatch(op1, op2);
            short res = compare(op1, op2);
            cleanup(op1);
            cleanup(op2);
            op1.type = vtInt;
            if (res == 0) {
                if (byte==vmEq || byte==vmLTE || byte==vmGTE) op1.iVal = 1;
                else op1.iVal = 0;
            } else if (res > 0) {
                if (byte==vmGT || byte==vmGTE || byte==vmNEq) op1.iVal = 1;
                else op1.iVal = 0;
            } else {
                if (byte==vmLT || byte==vmLTE || byte==vmNEq) op1.iVal = 1;
                else op1.iVal = 0;
            }
            push(op1);
            break;
            */
        }

        case vmGetC: {
            pop(op1); // index
            //pop(op2); // string addr
            //v = deref(op2.iVal);
            v = deref(stack[--st].iVal);
            if (v->type	!= vtString)
                vm_error("@[] operator only	allowed	on a string	variable", pc);
            ca = LockString(v);
            short len =	strlen(ca);
            if (op1.iVal > len)
                vm_error("String index out of range", pc);
            op1.type = vtChar;
            op1.cVal = ca[op1.iVal];
            UnlockString(v);
            push(op1);
            break;
        }

        case vmSetC: {
            Value c;
            pop(c);
            pop(op1); // index
            _typeCast(c, vtChar);
            //pop(op2); // string addr
            //v = deref(op2.iVal);
            v = deref(stack[--st].iVal);
            if (v->type	!= vtString)
                vm_error("@[] operator only	allowed	on a string	variable", pc);
            EnsureMalleable(v);
            ca = LockString(v);
            short len =	strlen(ca);
            if (op1.iVal >=	len)
                vm_error("String index out of range", pc);
            ca[op1.iVal] = c.cVal;
            UnlockString(v);
            op1.type = vtChar;
            op1.cVal = c.cVal;
            push(op1);
            break;
        }

        case vmCall: {
            //pop(op1);
            //ASSERT(op1.type == vtInt);
            //val = op1.iVal;
            ASSERT(stack[st-1].type == vtInt);
            val = stack[--st].iVal;
            op1.type = vtAddr;
            op1.iVal = fullAddr(pc);
            push(op1);
            if (appVer >= 0x0500) {
                op1.type = vtSavedFB;
                op1.iVal = fb;
                push(op1);
            }
            setPC(val);
            if (appVer >= 0x0500) {
                fb = pc;
            }
            break;
        }
            
        case vmJmp: {
            loc = getWord();
            pc = loc + fb;
            break;
        }

        case vmJmpZ: {
            loc = getWord();
            v = &stack[--st];
            truthVal(*v);
            if (!v->iVal) pc = loc + fb;
            break;
        }
            
        case vmJmpNZ: {
            loc = getWord();
            v = &stack[--st];
            truthVal(*v);
            if (v->iVal) pc = loc + fb;
            break;
        }

        case vmSetRet: {
            pop(retVal);
            break;
        }
        
        /*
        case vmSetRetJmp: {
            pop(retVal);
            loc = getWord();
            pc = loc;
        }
        */

        case vmSetRet0: {
            retVal.type = vtInt;
            retVal.iVal = 0;
            break;
        }

        case vmCallBI: {
            index = code[pc++];
            retVal.type = vtInt;
            retVal.iVal = 0;
            if (index < nBIFuncs && index >= 0)
                ((void(*)(short))funcTable[index].loc)(index);
            else vm_error("Attempt to call non-present builtin function", pc);
            if (code[pc] == vmPop) {
                cleanup(retVal);
                pc++;
            } else {
                push(retVal);
            }
            break;
        }

        case vmPopN: {
            short num = getWord();
            for (index=0;index<num;index++) {
                cleanup(stack[--st]);
            }
            if (code[pc] == vmUnLink) {
                // fallthrough
                pc++;
            } else {
                break;
            }
        }

        case vmUnLink: {
            pop(op1);
            if (op1.type != vtStackIndex) vm_error("Stack corruption detected", pc);
            fp = op1.iVal;
            if (code[pc] == vmRet) {
                // fallthrough
                pc++;
            } else {
                break;
            }
        }

        case vmRet: {
            short numArgs = code[pc++];
            if (appVer >= 0x0500) {
                pop(op1);
                if (op1.type != vtSavedFB) vm_error("Stack corruption detected", pc);
                fb = op1.iVal;
            }
            pop(op1);
            if (op1.type != vtAddr) vm_error("Stack corruption detected", pc);
            for (index=0;index<numArgs;index++) {
                cleanup(stack[--st]);
            }
            setPC(op1.iVal);
            if (code[pc] == vmPop) {
                cleanup(retVal);
                pc++;
            } else {
                push(retVal);
            }
            // pop() no longer shrinks the stack, so do it here
            shrinkStack();
            if (pc==0) goto _WeOuttaHere;
            break;
        }

        case vmPop: {
            cleanup(stack[--st]);
            break;
        }

        case vmLink: {
            index = code[pc++];
            op1.type = vtStackIndex;
            op1.iVal = fp;
            push(op1);
            fp = st - index - (appVer >= 0x0500 ? 3 : 2);
            break;
        }

        case vmHalt:
            bForceExit = true;
            goto _WeOuttaHere;

        case vmSwap: {
            pop(op1);
            pop(op2);
            push(op1);
            push(op2);
            break;
        }
        
        case vmLibCall: {
            loc = code[pc++];
            index = code[pc++];
            retVal.type = vtInt;
            retVal.iVal = 0;
            if (addIns[loc].libRef == 0xffff) {
                (*addIns[loc].func)(&pci, addIns[loc].data, index);
                push(retVal); // native add-ins use new strings
            } else {
                PocketCLibExecute(addIns[loc].libRef, index);
                oldpush(retVal); // since it may be old school
            }
            break;
        }
        
        case vmNoOp:
            break;
            
        case vmNoOp2:
            index = getWord();
            break;

        case vmAddExtI: {
            mask = code[pc++];
            unsigned long val1, val2;
            val1 = getWord();
            val2 = getWord();
            
            if (mask & 0x03) {
                if ((mask & 0x03) == 2 || (mask & 0x03) == 3)
                    val1 += fp;
                
                if ((mask & 0x03) != 3) {
                    v = deref(val1);
                    ASSERT(v->type == vtInt);
                    val1 = v->iVal;
                }
            }
            if (mask & 0x0C) {
                if ((mask & 0x0C) == 8 || (mask & 0x0C) == 0x0C)
                    val2 += fp;
                if ((mask & 0x0C) != 0x0C) {
                    v = deref(val2);
                    ASSERT(v->type == vtInt);
                    val2 = v->iVal;
                }
            }
                        
            op1.type = vtInt;
            op1.iVal = val1 + val2;
            push(op1);
            break;
        }
        
        case vmCase:
            mask = code[pc++]; // type
            val = getInt();
            loc = getWord();
            copyVal(&op1, &stack[st-1]);
            switch (mask) {
                case vtInt:
                    op2.type = vtInt;
                    op2.iVal = val;
                    break;
                case vtChar:
                    op2.type = vtChar;
                    op2.cVal = val;
                    break;
                case vtString:
                    NewConstString(&op2, &cStrings[val]);
                    break;
                default:
                    vm_error("unexpected case type", pc);
            }
            _typeMatch(op1, op2);
            if (op1.type == vtFloat)
                vm_error("cannot use floats in a switch", pc);
            if (compare(op1, op2) == 0) {
                pc = loc + fb;
                cleanup(op1);
                pop(op1);
            }
            
            cleanup(op1);
            cleanup(op2);
            break;

        case vmGetG:
        case vmGetL: {
            index = getWord();
            if (byte == vmGetL) index += fp + 0x6000;
            if (arrayIndex >= 0) index+=arrayIndex;
            v = deref(index);
            copyVal(&op1, v);
            push(op1);
            break;
        }

        case vmSetG:
        case vmSetL: {
            index = getWord();
            if (arrayIndex >= 0) index+=arrayIndex;
            if (byte == vmSetL) index += fp + 0x6000;
            v = deref(index);
            
            _typeCast(stack[st-1], v->type);
            copyVal(v, &stack[st-1]);
            break;
        }

        case vmIncGA:
        case vmDecGA:
        case vmIncGB:
        case vmDecGB: {
            index = getWord();
            if (arrayIndex >= 0) index+=arrayIndex;
            v = deref(index);

            if (byte >= vmIncGB) push(*v);

            if (v->type==vtInt)
                v->iVal += (byte==vmIncGA || byte==vmIncGB ? 1 : -1);
            else if (v->type==vtChar)
                v->cVal += (byte==vmIncGA || byte==vmIncGB ? 1 : -1);
            else if (v->type==vtFloat)
                v->fVal += (byte==vmIncGA || byte==vmIncGB ? 1.0 : -1.0);

            if (byte < vmIncGB) push(*v);

            break;
        }

        case vmIncLA:
        case vmDecLA:
        case vmIncLB:
        case vmDecLB: {
            index = getWord() + fp;
            if (arrayIndex >= 0) index+=arrayIndex;
            v = deref(index + 0x6000);

            if (byte >= vmIncLB) push(*v);

            if (v->type==vtInt)
                v->iVal += (byte==vmIncLA || byte==vmIncLB ? 1 : -1);
            else if (v->type==vtChar)
                v->cVal += (byte==vmIncLA || byte==vmIncLB ? 1 : -1);
            else if (v->type==vtFloat)
                v->fVal += (byte==vmIncLA || byte==vmIncLB ? 1.0 : -1.0);

            if (byte < vmIncLB) push(*v);

            break;
        }

        case vmOldCall: {
            if (appVer >= 0x0500) {
                val = getInt();
            } else {
                val = getWord();
            }
            op1.type = vtAddr;
            op1.iVal = fullAddr(pc);
            push(op1);
            if (appVer >= 0x0500) {
                op1.type = vtSavedFB;
                op1.iVal = fb;
                push(op1);
            }
            setPC(val);
            if (appVer >= 0x0500) {
                fb = pc;
            }
            break;
        }
        

        default:
            vm_error("Undefined opcode", pc);
    } // end switch

    if (byte != vmArray) arrayIndex = -1;
    } // end while(true)
_WeOuttaHere:
    // cleanup
    // Instrumentation

    /*{
        MemHandleResize(output, vmNumInstr * 30);
        char* str = (char*)MemHandleLock(output);
        short off=0;
        for (short _=0;_<vmNumInstr;_++) {
            char temp[25];
            StrPrintF(temp, "%s %li\n", instName[_], instCount[_]);
            DmStrCopy(str, off, temp);
            off += strlen(temp);
        }
        MemHandleUnlock(output);
    }*/

    return execRes; // if exited normally
}
