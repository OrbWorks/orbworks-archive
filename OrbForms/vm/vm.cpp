#include "vm.h"

void lib_vector_virtcall(VM* vm, int);
void lib_memcpy(VM* vm, int);
void lib_malloct(VM* vm, int);

VM::VM() {
    code = NULL;
    str = NULL;
    //globs = NULL;
    stack = NULL;
    pc = 0;
    st = 0;
    fb = 0;
    fp = 0;
}

VM::~VM() {
    // @TODO: delete stuff
}

void VM::vm_error(char* err) {
    unsigned int strLoc = code[fb-1] * 256 + code[fb-2];
    char* name = &str[strLoc];
    printf("%s @ %d: %s\n", name, pc, err);
    // TODO: throw exception
    killVM = true;

}

#define BINARY(op, _type, _val) \
    temp.type = _type; \
    temp._val = (op1->_val op op2->_val); \
    beforePush(op1, op2, ot1, ot2); \
    push(&temp)
    
#define COMP(op, _type, _val) \
    temp.type = vtInt; \
    temp.iVal = (op1->_val op op2->_val); \
    beforePush(op1, op2, ot1, ot2); \
    push(&temp)
    
#define SAFE_DIV(_type, _val) \
    if (op2->_val == 0) { vm_error("divide by zero error"); } \
    else { \
    temp.type = _type; \
    temp._val = (op1->_val / op2->_val); \
    beforePush(op1, op2, ot1, ot2); \
    push(&temp); }
    
#define UNARY(op, _type, _val) \
    temp.type = _type; \
    temp._val = op op2->_val; \
    beforePush(op2, NULL, ot2, otNone); \
    push(&temp)
    
#define CONVERT(_type, _vald, _vals) \
    temp.type = _type; \
    temp._vald = op2->_vals; \
    beforePush(op2, NULL, ot2, otNone); \
    push(&temp)

#define COMP_STR(op) \
    temp.type = vtInt; \
    temp.iVal = (strcmp(op1->sVal, op2->sVal) op 0); \
    beforePush(op1, op2, ot1, ot2); \
    push(&temp)

#define POST_OP(op, _val) \
    op1 = deref(op2->iVal); \
    beforePush(op2, NULL, ot2, otNone); \
    push(op1); \
    op1->_val op

#define PRE_OP(op, _val) \
    op1 = deref(op2->iVal); \
    op1->_val op; \
    beforePush(op2, NULL, ot2, otNone); \
    push(op1)


Value* VM::deref(int addr) {
    if ((addr & 0xC0000000) == 0xC0000000) {
        int off = fp + (addr & 0x3FFFFFFF);
        if (off >= st)	{
            printf("Invalid stack ref (st=%d): 0x%x (%d)\n", st, addr, off);
            vm_error("invalid stack reference");
        }
        return &stack[off];
    } else if (addr & 0x80000000) {
        int off = addr & 0x3FFFFFFF;
        if (off >= st) {
            printf("Invalid stack ref (st=%d): 0x%x (%d)\n", st, addr, off);
            vm_error("invalid stack reference");
        }
        return &stack[off];
    } else {
        if (addr >= globs.size()) {
            printf("Invalid global ref (gsize=%d): %d\n", globs.size(), addr);
            vm_error("invalid global reference");
        }
        return &globs[addr];
    }
}

int VM::decodeWord(int addr) {
    int off;
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
    moveVal(&stack[st++], v);
}

Value* VM::pop() {
    return &stack[--st];
}

int VM::getWord() {
    int ret = code[pc] + code[pc+1] * 256;
    pc += 2;
    return ret;
}

int VM::getLong() {
    int ret = code[pc] + (int)(code[pc+1] << 8) + (int)(code[pc+2] << 16) + (int)(code[pc+3] << 24);
    pc += 4;
    return ret;
}

void VM::moveVal(Value* dst, Value* src) {
    memcpy(dst, src, sizeof(Value));
    if (src->type == vtString)
        src->sVal = NULL;
}

void VM::copyVal(Value* dst, Value* src) {
    memcpy(dst, src, sizeof(Value));
    if (dst->type == vtString)
        dst->sVal = strdup(src->sVal);
}

#include "..\compiler\PalmDB.h"

bool VM::Load(char* name) {
    VarType* gtypes;
    GlobalInit* globis;
    int gisize;
    objinfo = NULL;

    if (strstr(name, ".vm")) {
        FILE* f = fopen(name, "rb");
        if (!f) {
            printf("Unable to open '%s'\n", name);
            return false;
        }

        fread(&iGlobInit, sizeof(int), 1, f);
        fread(&iGlobDest, sizeof(int), 1, f);

        fread(&csize, sizeof(int), 1, f);
        code = new byte[csize];
        fread(code, sizeof(byte), csize, f);
        
        fread(&gsize, sizeof(int), 1, f);
        gtypes = new VarType[gsize];
        fread(gtypes, sizeof(VarType), gsize, f);

        fread(&gisize, sizeof(int), 1, f);
        globis = new GlobalInit[gisize];
        fread(globis, sizeof(GlobalInit), gisize, f);

        fread(&ssize, sizeof(int), 1, f);
        str = new char[ssize];
        fread(str, sizeof(char), ssize, f);

        if (!feof(f)) {
            int oisize;
            fread(&oisize, sizeof(int), 1, f);
            objinfo = new byte[oisize];
            fread(objinfo, sizeof(byte), oisize, f);
        }

        fclose(f);
    } else {
        PalmDB db;
        db.Read(name);

        PalmResRecord* prec = db.GetResRec("ofCD", 0);
        if (prec) {
            csize = prec->len;
            code = new byte[csize];
            memcpy(code, prec->data, csize);
        } else {
            puts("unable to get code segment\n");
            return false;
        }
        prec = db.GetResRec("ofGT", 0);
        if (prec) {
            gsize = prec->len / sizeof(VarType);
            gtypes = new VarType[gsize];
            memcpy(gtypes, prec->data, prec->len);
        } else {
            gsize = 0;
            //puts("unable to get global type segment\n");
            //return false;
        }
        prec = db.GetResRec("ofGI", 0);
        if (prec) {
            gisize = prec->len / 10;
            globis = new GlobalInit[gisize];
            PalmDBStream dbstr(prec->data);
            for (int i=0;i<gisize;i++) {
                globis[i].offset = dbstr.getLong();
                globis[i].val.type = dbstr.getByte();
                globis[i].val.stype = dbstr.getByte();
                globis[i].val.iVal = dbstr.getLong();
            }
        } else {
            gisize = 0;
            //puts("unable to get global init segment\n");
            //return false;
        }
        prec = db.GetResRec("ofST", 0);
        if (prec) {
            ssize = prec->len;
            str = new char[ssize];
            memcpy(str, prec->data, ssize);
        } else {
            puts("unable to get string segment\n");
            return false;
        }
        prec = db.GetResRec("ofHD", 0);
        if (prec) {
            PalmDBStream dbstr(prec->data);
            dbstr.getWord();
            dbstr.getByte();
            iGlobInit = dbstr.getLong();
            iGlobDest = dbstr.getLong();
        } else {
            puts("unable to get header\n");
            return false;
        }
        prec = db.GetResRec("ofOI", 0);
        if (prec) {
            int oisize = prec->len;
            objinfo = new byte[oisize];
            memcpy(objinfo, prec->data, oisize);
        }
    }
        
    //globs = new Value[gsize];
    globs.init(gsize);
    int i;
    for (i=0;i<gsize;i++) {
        globs[i].type = gtypes[i];
        if (gtypes[i] == vtString)
            globs[i].sVal = strdup("");
    }

    for (i=0;i<gisize;i++) {
        assert(globis[i].val.type == globs[globis[i].offset].type);
        globs[globis[i].offset].iVal = globis[i].val.iVal;
        if (globis[i].val.type == vtString)
            globs[globis[i].offset].sVal = strdup(&str[globs[globis[i].offset].iVal]);
    }
    
    stack = new Value[1000];
    return true;
}

void VM::beforePush(Value* a, Value* b, OperandType ota, OperandType otb) {
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

void VM::pushTypes(char* type) {
    Value temp;
    int count = 1;
    while (*type) {
stackLoop:
        temp.iVal = 0;
        switch (*type) {
            case 'i': temp.type = vtInt; break;
            case 'c': temp.type = vtChar; break;
            case 'f': temp.type = vtFloat; break;
            case 's': temp.type = vtString; break;
            default:
                count = 0;
                while (*type >= '0' && *type <= '9') {
                    count *=10;
                    count += *type - '0';
                    type++;
                }
                goto stackLoop;
        }
        if (temp.type == vtString) {
            while (count--) {
                temp.sVal = strdup("");
                push(&temp);
            }
        } else {
            while (count--) {
                push(&temp);
            }

        }
        count = 1;
        type++;
    }
}

#define DEBUG
#include "..\compiler\inst.h"

//#define VM_OUTPUT(x) x
#define VM_OUTPUT(x)

bool VM::Run() {
    if (iGlobInit != -1)
        Call(iGlobInit, true);
    Call(0, true);
    if (iGlobDest != -1)
        Call(iGlobDest, true);
    return true;
}

long VM::getVirtFunc(int objID, int funcID) {
    assert(funcID < objinfo[objID + 4]);
    funcID = objID + 5 + 4 * funcID;
    return objinfo[funcID] + (int)(objinfo[funcID + 1] << 8) + (int)(objinfo[funcID + 2] << 16) + (int)(objinfo[funcID + 3] << 24);
}

long VM::getIfaceFunc(int objID, int ifaceID, int funcID) {
    int ifaceBase = objID + 5 + objinfo[objID + 4] * 4;
    int nIfaces = objinfo[ifaceBase];
    ifaceBase += 3;
    bool found = false;
    for (int i=0;i<nIfaces;i++) {
        int index = objinfo[ifaceBase] + (int)(objinfo[ifaceBase + 1] << 8);
        if (index == ifaceID) {
            found = true;
            break;
        }
        ifaceBase += 6;
    }
    if (!found) vm_error("object does not implement required interface");
    ifaceBase += 2;
    long index = objinfo[ifaceBase] + (int)(objinfo[ifaceBase + 1] << 8) + (int)(objinfo[ifaceBase + 2] << 16) + (int)(objinfo[ifaceBase + 3] << 24);
    funcID = index + funcID * 4;
    return objinfo[funcID] + (int)(objinfo[funcID + 1] << 8) + (int)(objinfo[funcID + 2] << 16) + (int)(objinfo[funcID + 3] << 24);
}

long VM::getObjectSize(int objID) {
    return objinfo[objID + 2] + ((int)objinfo[objID + 3] << 8);
}

void VM::setObjinfo(int addr, int objID) {
    Value* v = deref(addr);
    assert(v->type == vtInt);
    v->iVal = objID;
    
    // get subobject count
    int ifaceBase = objID + 5 + objinfo[objID + 4] * 4;
    int nIfaces = objinfo[ifaceBase];
    int nIfaceMethods = objinfo[ifaceBase+1] + (objinfo[ifaceBase+2] << 8);

    int iSubObjectBase = ifaceBase + 3 + nIfaces * 6 + nIfaceMethods * 4 + 2;
    int nSubObjects = objinfo[iSubObjectBase-2] + (objinfo[iSubObjectBase-1] << 8);

    for (int i=0;i<nSubObjects;i++) {
        int offset = objinfo[iSubObjectBase++];
        offset += (objinfo[iSubObjectBase++] << 8);
        int subobjID = objinfo[iSubObjectBase++];
        subobjID += (objinfo[iSubObjectBase++] << 8);
        setObjinfo(addr + offset, subobjID);
    }
}

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
        if (v->type == vtChar) {
            char c = v->cVal;
            v->sVal = strdup("a");
            v->sVal[0] = c;
        } else if (v->type == vtInt) {
            long l = v->iVal;
            v->sVal = (char*)malloc(16);
            itoa(l, v->sVal, 10);
        } else {
            float f = v->fVal;
            v->sVal = (char*)malloc(16);
            gcvt(f, 10, v->sVal);
        }
        v->type = vtString;
        return;
    }
    if (type == vtFloat) {
        if (v->type == vtString) {
            float f;
            f = (float)atof(v->sVal);
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
            l = atoi(v->sVal);
            v->iVal = l;
        }
        else if (v->type == vtFloat) v->iVal = (long)v->fVal;
        else v->iVal = (long)v->cVal;
        v->type = vtInt;
        return;
    }
    if (v->type == vtString) {
        char c;
        c = v->sVal[0];
        v->cVal = c;
    }
    else if (v->type == vtInt) v->cVal = (char)v->iVal;
    else v->cVal = (char)(long)v->fVal;
    v->type = vtChar;
}

bool VM::Call(int addr, bool bPushRet) {
    killVM = false;
    int nArray = 1, index;
    OperandType ot1, ot2;
    Value* op1, *op2, *op3, *src, *dst;
    Value immed1, immed2;
    Value temp;
    byte inst;

    pc = fb = addr;
    if (bPushRet) {
        Value v;
        v.type = vtSavedFB;
        v.iVal = -1;
        push(&v);
        v.type = vtRetAddr;
        push(&v);
    }

    while (!killVM) {
        inst = code[pc++];
        ot1 = ot2 = otNone;
        if ((inst & 0xC0) == 0xC0) {
            // handle extended instructions
            byte ext = inst;
            int stoff = 1, val1, val2;
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
            op2 = &stack[st-1];
            ot2 = otStack;
            op1 = &stack[st-2];
            ot1 = otStack;
        }

reeval:
        VM_OUTPUT(fprintf(stderr, "* %5d %6.6s sp=%d\n", pc, instNames[inst], st);)

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
                    temp.sVal = strdup(&str[index]);
                    push(&temp);
                }
                break;
            case vmSCString:
                index = decodeWord(getWord());
                dst = deref(index);
                assert(dst->type == vtString);
                cleanUp(dst);
                dst->sVal = strdup(&str[getWord()]);
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
                cleanUp(dst);
                copyVal(dst, src);
                break;

            case vmStackInit: {
                char* type = &str[getWord()];
                pushTypes(type);
                break;
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
                temp.type = vtString;
                temp.sVal = (char*)malloc(16);
                itoa(op2->iVal, temp.sVal, 10);
                beforePush(op2, NULL, ot2, otNone);
                push(&temp);
                break;
            case vmFtoI: CONVERT(vtInt, iVal, fVal); break;
            case vmFtoC: CONVERT(vtChar, cVal, fVal); break;
            case vmFtoS:
                temp.type = vtString;
                temp.sVal = (char*)malloc(16);
                gcvt(op2->fVal, 10, temp.sVal);
                beforePush(op2, NULL, ot2, otNone);
                push(&temp);
                break;
            case vmCtoI: CONVERT(vtInt, iVal, cVal); break;
            case vmCtoF: CONVERT(vtFloat, fVal, cVal); break;
            case vmCtoS:
                temp.type = vtString;
                temp.sVal = strdup("a");
                temp.sVal[0] = op2->cVal;
                beforePush(op2, NULL, ot2, otNone);
                push(&temp);
                break;
            case vmStoI:
                temp.type = vtInt;
                temp.iVal = atoi(op2->sVal);
                beforePush(op2, NULL, ot2, otNone);
                push(&temp);
                break;
            case vmStoF:
                temp.type = vtFloat;
                temp.fVal = atof(op2->sVal);
                beforePush(op2, NULL, ot2, otNone);
                push(&temp);
                break;
            case vmStoC:
                temp.type = vtChar;
                temp.cVal = *op2->sVal;
                beforePush(op2, NULL, ot2, otNone);
                push(&temp);
                break;
            case vmStoB:
                temp.type = vtInt;
                temp.iVal = *op2->sVal ? 1 : 0;
                beforePush(op2, NULL, ot2, otNone);
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
                temp.sVal = (char*)malloc(strlen(op1->sVal) + strlen(op2->sVal) + 1);
                strcpy(temp.sVal, op1->sVal);
                strcat(temp.sVal, op2->sVal);
                beforePush(op1, op2, ot1, ot2);
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
            // @TODO: these call a function instead
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
                beforePush(op2, NULL, ot2, otNone);
                break;
            case vmJmpNZ:
                index = getWord();
                if (op2->iVal != 0)
                    pc = index + fb;
                beforePush(op2, NULL, ot2, otNone);
                break;
            case vmJmpPopZ:
                index = getWord();
                if (op2->iVal == 0) {
                    pc = index + fb; // no pop
                } else {
                    beforePush(op2, NULL, ot2, otNone);
                }
                break;
            case vmJmpPopNZ:
                index = getWord();
                if (op2->iVal != 0) {
                    pc = index + fb; // no pop
                } else {
                    beforePush(op2, NULL, ot2, otNone);
                }
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
                temp.iVal = pc;
                push(&temp);
                pc = index;
                fb = pc;
                break;

            case vmCallBI:
                index = getWord();
                if (index == 318) {
                    lib_vector_virtcall(this, index);
                } else if (index == 203) {
                    lib_memcpy(this, index);
                } else if (index == 204) {
                    lib_malloct(this, index);
                    push(&retVal);
                } else {
                    assert(index < nbifuncs);
                    (*bifuncs[index].func)(this, index);
                    if (bifuncs[index].hasRet)
                        push(&retVal);
                }
                break;

            case vmCallStubBI:
                index = getWord();
                op1 = pop();
                assert(op1->type == vtRetAddr);
                pc = op1->iVal;
                op1 = pop();
                assert(op1->type == vtSavedFB);
                fb = op1->iVal;
                assert(index < nbifuncs);
                (*bifuncs[index].func)(this, index);
                if (bifuncs[index].hasRet)
                    push(&retVal);
                break;

            case vmCallVirt: {
                index = code[pc++];
                op1 = &stack[st-index-1];
                int funcID = code[pc++];
                assert(op1->type == vtInt);
                int objID = deref(op1->iVal)->iVal;
                index = getVirtFunc(objID, funcID);
                goto CALL_COMMON;
            }

            case vmCallIface: {
                index = code[pc++];
                op1 = &stack[st-index-1];
                int ifaceID = getWord();
                int funcID = code[pc++];
                assert(op1->type == vtInt);
                int objID = deref(op1->iVal)->iVal;
                index = getIfaceFunc(objID, ifaceID, funcID);
                goto CALL_COMMON;
            }

            case vmRet:
                index = code[pc++];
                op1 = pop();
                assert(op1->type == vtRetAddr);
                pc = op1->iVal;
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
                pc = op1->iVal;
                op1 = pop();
                assert(op1->type == vtSavedFB);
                fb = op1->iVal;
                while (index--)
                    cleanUp(pop());
                if (pc == -1)
                    killVM = true;
                break;

            case vmSetRet:
                moveVal(&retVal, op2);
                beforePush(op2, NULL, ot2, otNone);
                break;
            case vmSetRet0:
                retVal.type = vtInt;
                retVal.iVal = 0;
                vm_error("no return value");
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
            {
                int oldst = st;
                index = getWord();
                temp.type = vtInt;
                temp.iVal = 0;
                while (index--)
                    push(&temp);
                temp.iVal = 0x80000000 | oldst;
                push(&temp);
                break;
            }
            case vmAllocT:
            {
                char* type = &str[getWord()];
                int oldst = st;
                pushTypes(type);
                temp.iVal = 0x80000000 | oldst;
                push(&temp);
                break;
            }
            case vmNew:
                op1 = pop(); // string
                op2 = pop(); // count
                assert(op1->type == vtString);
                assert(op2->type == vtInt);
                index = globs.alloc(op2->iVal, op1->sVal);
                free(op1->sVal);
                op2->iVal = index;
                push(op2);
                break;
            case vmNewObj: {
                op1 = pop(); // string
                op2 = pop(); // count
                int objID = getWord();
                int len = strlen(op1->sVal);
                assert(op1->type == vtString);
                assert(op2->type == vtInt);
                index = globs.alloc(op2->iVal, op1->sVal);
                if (index) {
                    for (int i=0;i<op2->iVal;i++)
                        setObjinfo(index + len * i, objID);
                }
                free(op1->sVal);
                op2->iVal = index;
                push(op2);
                break;
            }

            case vmDelete:
                op1 = &stack[st-1]; //pop();
                assert(op1->type == vtInt);
                globs.release(op1->iVal);
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
                cleanUp(deref(op2->iVal));
                copyVal(deref(op2->iVal), op1);
                push(op1);
                break;

            case vmCopy:
                assert(stack[st-1].type == vtInt);
                assert(stack[st-2].type == vtInt);
                src = deref(pop()->iVal);
                dst = deref(stack[st-1].iVal); // leave on stack
                index = getWord();
                while (index--) {
                    cleanUp(dst);
                    copyVal(dst++, src++);
                }
                break;

            case vmGetAt:
            {
                Value* src;
                op1 = pop();
                op2 = pop();
                assert(op1->type == vtInt);
                assert(op2->type == vtInt); // string addr
                src = deref(op2->iVal);
                assert(src->type == vtString);
                if (op1->iVal > (int)strlen(src->sVal))
                    vm_error("string subscript out of range");
                op2->type = vtChar;
                op2->cVal = src->sVal[op1->iVal];
                push(op2);
                break;
            }

            case vmSetAt:
            {
                Value* src;
                op3 = pop();
                op1 = pop();
                op2 = pop();
                assert(op1->type == vtInt);
                assert(op2->type == vtInt); // string addr
                assert(op3->type == vtChar);
                src = deref(op2->iVal);
                assert(src->type == vtString);
                if (op1->iVal >= (int)strlen(src->sVal))	// can't modify the '\0'
                    vm_error("string subscript out of range");
                src->sVal[op1->iVal] = op3->cVal;
                push(op3); // TODO: we could make this operand first, so we don't have to pop it
                break;
            }

            case vmLoadI:
                index = getWord();
                assert(index < globs.size());
                copyVal(&temp, &globs[index]);
                push(&temp);
                break;

            case vmLoadFP:
                index = getWord();
                copyVal(&temp, &stack[index + fp]);
                push(&temp);
                break;

            case vmLoadF0:
                index = getWord();
                op1 = &stack[fp]; // this
                assert(op1->type == vtInt);
                op2 = deref(op1->iVal + index); // field
                copyVal(&temp, op2);
                push(&temp);
                break;

            case vmLoadF1:
                index = getWord();
                op1 = &stack[fp+1]; // this
                assert(op1->type == vtInt);
                op2 = deref(op1->iVal + index); // field
                copyVal(&temp, op2);
                push(&temp);
                break;

            case vmStoreI:
                op1 = &stack[st-1];
                index = getWord();
                assert(index < globs.size());
                cleanUp(&globs[index]);
                assert(globs[index].type == op1->type);
                copyVal(&globs[index], op1);
                break;

            case vmStoreFP:
                op1 = &stack[st-1];
                index = getWord();
                assert(index + fp < st);
                cleanUp(&stack[index + fp]);
                assert(stack[index + fp].type == op1->type);
                copyVal(&stack[index + fp], op1);
                break;

            case vmStoreF0:
                op1 = &stack[st-1];
                index = getWord();
                op2 = &stack[fp]; // this
                assert(op2->type == vtInt);
                op3 = deref(op2->iVal + index); // field
                assert(op3->type == op1->type);
                cleanUp(op3);
                copyVal(op3, op1);
                break;

            case vmStoreF1:
                op1 = &stack[st-1];
                index = getWord();
                op2 = &stack[fp+1]; // this
                assert(op2->type == vtInt);
                op3 = deref(op2->iVal + index); // field
                assert(op3->type == op1->type);
                cleanUp(op3);
                copyVal(op3, op1);
                break;

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
            {
                op1 = pop();
                int nCase = code[pc++];
                for (int i=0;i<nCase;i++) {
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
                        equal = (0 == strcmp(op1->sVal, &str[index]));
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
            }
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
                typeCast(op1, deref(op2->iVal)->type);
                cleanUp(deref(op2->iVal));
                copyVal(deref(op2->iVal), op1);
                push(op1);
                break;

            case vmCompAddr:
                op1 = pop();
                index = op1->iVal;
                index &= 0x3fff;
                index |= ((op1->iVal >> 16) & 0xc000);
                op1->iVal = index;
                push(op1);
                break;

            default:
                vm_error("unexpected instruction");

        }
        if (inst != vmArray)
            nArray = 1;
    }
    return true;
}

void usage() {
    printf("vm file.vm\n");
}

void main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        return;
    }

    VM* vm = new VM;
    if (vm->Load(argv[1])) {
        vm->Run();
    }
    delete vm;
}
