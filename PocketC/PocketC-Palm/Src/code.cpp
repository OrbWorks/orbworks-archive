#include "PocketC.h"
void Alert(char*);
#define CODE_PAGE 280

unsigned short newLabel() {
    return nLabels++;
}

unsigned short setLabel(unsigned short lab) {
    
    if (lab >= _Labels) {
        if (MemHandleResize(reloc_h, sizeof(short)*(_Labels+32))) {
            oom();
        }
        _Labels+=32;
    }
    reloc = (unsigned short*)MemHandleLock(reloc_h);
    unsigned short ret = reloc[lab] = codeOff - funcStart;
    MemHandleUnlock(reloc_h);
    return ret;
}

void flushCode() {
    VoidHand dbh = DmResizeRecord(codeDB, 0, codeOff+sizeof(short));
    if (!dbh) oom();
    code_h = dbh;
    DmWrite(MemHandleLock(code_h), sizeof(short) + codeOff - codePtr, code, codePtr);
    MemHandleUnlock(code_h);
}

void codeIncrease() {
    VoidHand dbh = DmResizeRecord(codeDB, 0, _codeSize+sizeof(short)+256);
    if (!dbh) oom();
    code_h = dbh;
    DmWrite(MemHandleLock(code_h), sizeof(short) + _codeSize, code, 256);
    MemHandleUnlock(code_h);
    MemMove(code, &code[256], CODE_PAGE-256);
    codePtr-=256;
    _codeSize+=256;
}

//ULong heapFree, heapMax;
void addByte(unsigned char b) {
    if (CODE_PAGE < codePtr+3) codeIncrease();
    //MemHeapFreeBytes(0, &heapFree, &heapMax);
    code[codePtr++] = b;
    codeOff++;
}

void addInstr(VMsym b, VarType vt) {
    vt;
    addByte((unsigned char)b);
}

void addWord(short w) {
    if (CODE_PAGE < codePtr+4) codeIncrease();
    code[codePtr++] = (w >> 8) & 0xFF;
    code[codePtr++] = w & 0xFF;
    codeOff+=2;
}

void addInt(long i) {
    if (CODE_PAGE < codePtr+6) codeIncrease();
    code[codePtr++] = (i >> 24) & 0xFF;
    code[codePtr++] = (i >> 16) & 0xFF;
    code[codePtr++] = (i >> 8) & 0xFF;
    code[codePtr++] = i & 0xFF;
    codeOff+=4;
}

unsigned char endByte(short off) {
    unsigned char ret;
    ret = code[codePtr-1-off];
    return ret;   
}

void removeBytes(unsigned char* buff, short num) {
    for(short i=0;i<num;i++)
        buff[num-i-1] = code[--codePtr];
    codeOff-=num;
}

unsigned short addString(char* str) {
    // Try to find a copy
    cStrings = (char*)MemHandleLock(strings_h) + sizeof(short);
    unsigned short len = strlen(str);
    unsigned short pos = 0, tlen; 
    while (pos < nStrChars) {
        tlen = (unsigned char)cStrings[pos] * 256 + (unsigned char)cStrings[pos+1];
        //if (tlen < 0) Alert("Negative string length");
        pos+=sizeof(short);
        if (tlen == len && strcmp(str, &cStrings[pos])==0) {
            MemHandleUnlock(strings_h);
            return pos;
        }
        pos += tlen + 1;
    }
    MemHandleUnlock(strings_h);
    
    // unique string, add it to the table
    unsigned short prev = nStrChars;
    while (nStrChars + len + sizeof(short) >= _StrChars) {
        if (!(strings_h = (Handle)DmResizeRecord(codeDB, 2, _StrChars+256+sizeof(short)))) {
            oom();
        }
        _StrChars += 256;
    }
    cStrings = (char*)MemHandleLock(strings_h);
    DmWrite(cStrings, nStrChars+sizeof(short), &len, sizeof(short));
    DmWrite(cStrings, nStrChars+2*sizeof(short), str, len + 1);
    MemHandleUnlock(strings_h);
    nStrChars+= len + 1 + sizeof(short);
    return prev + sizeof(short);
}

unsigned short addFloat(float f) {
    unsigned short ret = 0;
    
    if (nFloats >= _Floats) {
        if (MemHandleResize(floats_h, sizeof(float) *(_Floats+10))) oom(); //c_error("out of memory", tok.line);
        _Floats+=10;
    }
    cFloats = (float*)MemHandleLock(floats_h);
    for (ret=0;ret<nFloats;ret++)
        if (cFloats[ret]==f) goto done;

    cFloats[ret = nFloats++]=f;
done:
    MemHandleUnlock(floats_h);
    return ret;
}

void relocate() {
    unsigned char* code = (unsigned char*)MemHandleLock(code_h);
    funcI = (FuncInfo*)MemHandleLock(func_h);
    reloc = (unsigned short*)MemHandleLock(reloc_h);
    unsigned short i=sizeof(short), lab;
    unsigned char mod[5];
    //if (libNum)
    //	i+=3; // Skip the native libary header
    i+=HEADER_SIZE; // skip all the header
    while (i < dbgLoc + sizeof(short)) {
        if (code[i] == vmJmp || code[i]==vmJmpZ || code[i]==vmJmpNZ) {
            lab = 0;
            lab |= code[i+1];
            lab <<=8;
            lab |= code[i+2];
            lab = reloc[lab];
            mod[0] = (lab >> 8) & 0xFF;
            mod[1] = lab & 0xFF;
            DmWrite(code, i+1, mod, 2);
        }
        if (code[i] == vmCallF || code[i] == vmFuncA) {
            long fID = 0;
            fID |= code[i+1];
            fID <<= 8;
            fID |= code[i+2];
            fID <<= 8;
            fID |= code[i+3];
            fID <<= 8;
            fID |= code[i+4];
            if (funcI[fID].loc == 0) {
                char msg[256];
                StrPrintF(msg, "%s definition not found", funcI[fID].name);
                c_error(msg, tok.line);
            }
            fID = (long)funcI[fID].loc;
            mod[1] = (fID >> 24) & 0xFF;
            mod[2] = (fID >> 16) & 0xFF;
            mod[3] = (fID >> 8) & 0xFF;
            mod[4] = fID & 0xFF;
            if (code[i] == vmCallF)
                mod[0] = vmOldCall;
            else
                mod[0] = vmCInt;
            DmWrite(code, i, mod, 5);
        }
        if (code[i] == vmCase) {
            lab = 0;
            lab |= code[i+6];
            lab <<=8;
            lab |= code[i+7];
            lab = reloc[lab];
            mod[0] = (lab >> 8) & 0xFF;
            mod[1] = lab & 0xFF;
            DmWrite(code, i+6, mod, 2);
        }
        i += (BCArgs[code[i]] + 1);
    }

    globalInit = (GlobalInit*)MemHandleLock(globalInits_h);
    for (i=0;i<nGlobalInits;i++) {
        if (globalInit[i].type == vtMethodPtr) {
            globalInit[i].type = vtInt;
            globalInit[i].val = (long)funcI[globalInit[i].val].loc;
        }
    }
    MemHandleUnlock(globalInits_h);

    MemHandleUnlock(reloc_h);
    MemHandleUnlock(func_h);
    MemHandleUnlock(code_h);

}
