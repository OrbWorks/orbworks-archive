#include "stdafx.h"
#include "pde.h"

void peepInsert(VMsym instr, long loc, VarType vt) {
    peepInstr[0] = peepInstr[1];
    peepInstr[1] = peepInstr[2];
    peepInstr[2] = peepInstr[3];
    peepInstr[3] = instr;

    peepLoc[0] = peepLoc[1];
    peepLoc[1] = peepLoc[2];
    peepLoc[2] = peepLoc[3];
    peepLoc[3] = loc;

    peepType[0] = peepType[1];
    peepType[1] = peepType[2];
    peepType[2] = peepType[3];
    peepType[3] = vt;
}

void peepRemove() {
    peepInstr[3] = peepInstr[2];
    peepInstr[2] = peepInstr[1];
    peepInstr[1] = peepInstr[0];
    peepInstr[0] = vmHalt;

    peepLoc[3] = peepLoc[2];
    peepLoc[2] = peepLoc[1];
    peepLoc[1] = peepLoc[0];
    peepLoc[0] = 0;

    peepType[3] = peepType[2];
    peepType[2] = peepType[1];
    peepType[1] = peepType[0];
    peepType[0] = vtNone;
}

static unsigned char extBits(VMsym sym) {
    if (sym == vmCWord) return 0;
    if (sym == vmCWordPFP) return 3;
    if (sym == vmGetG) return 1;
    if (sym == vmGetL) return 2;
    ASSERT(!"That sucks!");
    return 0;
}

void doPeep() {
    /*
    if (peepInstr[0] == vmGetL && peepInstr[1] == vmCWord && peepInstr[2] == vmAdd && 
        (peepInstr[3] == vmLoad || peepInstr[3] == vmSave)
    {
        int pos = peepLoc[0];
        int addr = ((unsigned short)code[pos+1] << 8) | (code[pos+2]);
        pos = peepLoc[2];
        int cnst = ((unsigned short)code[pos+1] << 8) | (code[pos+2]);

    */
    if (peepInstr[3] == vmAdd) {
        // get rid of add 0
        if (peepInstr[2] == vmCWord && ((code[peepLoc[2]+1] << 8) | code[peepLoc[2]+2]) == 0) {
            peepRemove();
            peepRemove();
            codePtr -= 4;
        } else if ((peepInstr[1] == vmCWord || peepInstr[1] == vmCWordPFP) && peepInstr[2] == vmCWord) {
            // two constants
            int pos = peepLoc[1];
            int val1 = ((unsigned short)code[pos+1] << 8) | (code[pos+2]);
            pos = peepLoc[2];
            int val2 = ((unsigned short)code[pos+1] << 8) | (code[pos+2]);
            val1 += val2;
            if (val1 == (val1 & 0xffff)) {
                pos = peepLoc[1];
                code[pos+1] = (val1 >> 8) & 0xFF;
                code[pos+2] = val1 & 0xFF;
                peepRemove();
                peepRemove();
                codePtr -= 4;
            }
        } else if ((peepInstr[1] == vmCWord || peepInstr[1] == vmCWordPFP || peepInstr[1] == vmGetL || peepInstr[1] == vmGetG) && 
                (peepInstr[2] == vmCWord || peepInstr[2] == vmCWordPFP || peepInstr[2] == vmGetL || peepInstr[2] == vmGetG))
        {
            if ((peepInstr[1] == vmGetL || peepInstr[1] == vmGetG) && peepType[1] != vtInt)
                return;
            if ((peepInstr[2] == vmGetL || peepInstr[2] == vmGetG) && peepType[2] != vtInt)
                return;
            
            int pos = peepLoc[1];
            int val1 = ((unsigned short)code[pos+1] << 8) | (code[pos+2]);
            pos = peepLoc[2];
            int val2 = ((unsigned short)code[pos+1] << 8) | (code[pos+2]);

            pos = peepLoc[1];
            code[pos] = vmAddExtI;
            code[pos+1] = (extBits(peepInstr[2]) << 2) | extBits(peepInstr[1]);
            
            if (peepInstr[1] == vmGetL)
                val1 += 0x6000;
            if (peepInstr[2] == vmGetL)
                val2 += 0x6000;

            code[pos+2] = (val1 >> 8) & 0xFF;
            code[pos+3] = val1 & 0xFF;
            code[pos+4] = (val2 >> 8) & 0xFF;
            code[pos+5] = val2 & 0xFF;
            peepInstr[1] = vmAddExtI;
            peepRemove();
            peepRemove();
            codePtr --;
        }
    
    } else if (peepInstr[2] == vmCWord && peepInstr[3] == vmLoad) {
        // convert to vmGetG
        code[peepLoc[2]] = vmGetG;
        peepInstr[2] = vmGetG;
        peepType[2] = peepType[3];
        peepRemove();
        codePtr--;

    } else if (peepInstr[2] == vmCWordPFP && peepInstr[3] == vmLoad) {
        // convert to vmGetL
        int pos = peepLoc[2];
        int addr = ((unsigned short)code[pos+1] << 8) | (code[pos+2]);
        code[pos] = vmGetL;
        peepInstr[2] = vmGetL;
        peepType[2] = peepType[3];
        addr -= 0x6000;
        code[pos+1] = (addr >> 8) & 0xFF;
        code[pos+2] = addr & 0xFF;
        peepRemove();
        codePtr--;
    }
}

unsigned short newLabel() {
    if (nLabels + 1 >= MAX_RELOC) c_error("too much relocation data", tok.line);
    return nLabels++;
}

unsigned short setLabel(unsigned short lab) {
    peepInstr[0] = peepInstr[1] = peepInstr[2] = peepInstr[3] = vmHalt;
    
    unsigned short ret = reloc[lab] = codePtr - funcStart;
    return ret;
}

void addByte(unsigned char b) {
    if (codePtr > MAX_CODE_SEGS * CODE_SEG_SIZE - 2) c_error("program too large", tok.line);
    code[codePtr++] = b;
}

void addInstr(VMsym b, VarType vt) {
    doPeep();
    peepInsert(b, codePtr, vt);
    addByte((unsigned char)b);
}

void addWord(short w) {
    if (codePtr > MAX_CODE_SEGS * CODE_SEG_SIZE - 4) c_error("program too large", tok.line);
    code[codePtr++] = (w >> 8) & 0xFF;
    code[codePtr++] = w & 0xFF;
}

void addInt(long i) {
    if (codePtr > MAX_CODE_SEGS * CODE_SEG_SIZE - 6) c_error("program too large", tok.line);
    code[codePtr++] = (i >> 24) & 0xFF;
    code[codePtr++] = (i >> 16) & 0xFF;
    code[codePtr++] = (i >> 8) & 0xFF;
    code[codePtr++] = i & 0xFF;
}

unsigned char endByte(short off) {
    unsigned char ret;
    ret = code[codePtr-1-off];
    return ret;
}

void removeBytes(unsigned char* buff, short num) {
    peepRemove();
    
    for(int i=0;i<num;i++)
        buff[num-i-1] = code[--codePtr];
}

unsigned short addString(char* str) {
    // Try to find a copy
    unsigned short pos = 0;
    while (pos < nStrChars) {
        if (strcmp(str, &cStrings[pos])==0) {
            return pos;
        }
        pos += strlen(&cStrings[pos]) + 1;
    }

    // unique string, add it to the table
    unsigned short prev = nStrChars, len = strlen(str);
    if ((unsigned long)nStrChars + len > (unsigned short)0xffff)
        c_error("too much string data", tok.line);
    memcpy(cStrings+nStrChars, str, len+1);
    nStrChars+= len + 1;
    return prev;
}

unsigned short addFloat(float f) {
    int ret;

    unsigned long fl;
    fl = *((unsigned long*)(&f));
    fl = ((fl & 0xff000000) >> 24) |
          ((fl & 0x00ff0000) >> 8)  |
          ((fl & 0x0000ff00) << 8)  |
          ((fl & 0x000000ff) << 24);
    //f = *((float*)&fl);

    for (ret=0;ret<nFloats;ret++)
        if (cFloats[ret]==fl) goto done;
    if (nFloats +1 >= MAX_FLOAT) c_error("too much float data", tok.line);
    cFloats[ret = nFloats++]=fl;
done:
    return ret;
}

long choppedBytes;

void padFunc(int fID) {
    long start = funcI[fID].loc;
    long dest = (start & 0xffff0000) + 0x10000;
    long len = dest - start;

    // is the function actually implemented?
    if (!start)
        return;

    memmove(&code[dest], &code[start], codePtr - start);
    memset(&code[start], vmHalt, len);
    codePtr += len;
    codeEnd += len;

    int i;
    for (i=0;i<nFuncs;i++) {
        if (!funcI[i].lib && funcI[i].loc >= start) {
            if (funcI[i].loc)
                funcI[i].loc += len;
            if (funcI[i].end)
                funcI[i].end += len;
        }
    }
    choppedBytes -= len;
}

void chopFunc(int fID) {
    long start = funcI[fID].loc;
    long end = funcI[fID].end;
    long len = end - start;
    
    // is the function actually implemented?
    if (!start)
        return;
//	if (!end)
//		MessageBox(NULL, "bad", "bad", 0);

    memcpy(&code[start], &code[end], codePtr - start - len);
    codePtr -= len;
    codeEnd -= len;

    int i;
    for (i=0;i<nFuncs;i++) {
        if (!funcI[i].lib && funcI[i].loc > start) {
            if (funcI[i].loc)
                funcI[i].loc -= len;
            if (funcI[i].end)
                funcI[i].end -= len;
        }
    }
    choppedBytes += len;
}

bool markFuncs() {
    bool changed = false;
    for (int fID=0;fID<nFuncs;fID++) {
        int i;
        if (!funcI[fID].marked)
            continue;
        i = funcI[fID].loc;
        if (i == 0) {
            char msg[256];
            wsprintf(msg, "%s definition not found", funcI[fID].name);
            c_error(msg, tok.line);
        }

        while (i < codeEnd) {
            if (code[i] == vmCallF || code[i] == vmFuncA) {
                long fID = 0;
                fID |= code[i+1];
                fID <<= 8;
                fID |= code[i+2];
                fID <<= 8;
                fID |= code[i+3];
                fID <<= 8;
                fID |= code[i+4];
                if (!funcI[fID].marked) {
                    changed = true;
                    funcI[fID].marked = true;
                }
            }
            if (code[i] == vmLibCall) {
                int lib = code[i+1];
                (*pLibs)[lib].marked = true;
            }
            if (code[i] == vmUnLink) break;
            i += (BCArgs[code[i]] + 1);
        }
    }

    return changed;
}

#ifdef FUNC_CHOP
void relocate() {
    int i=HEADER_SIZE, lab;
    // do the first label fixup
    while (i < codeEnd) {
        if (code[i] == vmJmp || code[i]==vmJmpZ || code[i]==vmJmpNZ) {
            lab = 0;
            lab |= code[i+1];
            lab <<=8;
            lab |= code[i+2];
            lab = reloc[lab];
            code[i+1] = (lab >> 8) & 0xFF;
            code[i+2] = lab & 0xFF;
        }
        if (code[i] == vmCase) {
            lab = 0;
            lab |= code[i+6];
            lab <<=8;
            lab |= code[i+7];
            lab = reloc[lab];
            code[i+6] = (lab >> 8) & 0xFF;
            code[i+7] = lab & 0xFF;
        }
        ASSERT(code[i] < vmNumInstr);
        i += (BCArgs[code[i]] + 1);
    }

    for (i=0;i<nFuncs;i++) {
        funcI[i].marked = false;
    }

    choppedBytes = 0;
    int fID = findFunc("main", false);
    funcI[fID].marked = true;
    for (i=0;i<nGlobalInits;i++) {
        if (globalInit[i].type == vtMethodPtr)
            funcI[globalInit[i].val].marked = true;
    }

    while (markFuncs()) ;
    int nGenFuncs = 0;

    for (i=0;i<nFuncs;i++) {
        if (!funcI[i].lib) {
            if (funcI[i].marked) {
                nGenFuncs++;
            } else {
                chopFunc(i);
            }
        }
    }

    // ensure functions don't cross segments
    for (i=0;i<nFuncs;i++) {
        if (!funcI[i].lib) {
            if ((funcI[i].end & 0xffff) > CODE_SEG_SIZE ||
                (funcI[i].loc & 0xffff0000) != (funcI[i].end & 0xffff0000)) {
                padFunc(i);
            }
        }
    }

    if (libNum) {
        long loc_libNames = 0;
        loc_libNames = code[11];
        loc_libNames <<= 8;
        loc_libNames |= code[12];
        loc_libNames <<= 8;
        loc_libNames |= code[1];
        loc_libNames <<= 8;
        loc_libNames |= code[2];
        loc_libNames -= choppedBytes;

        code[11] = (libNum ? ((loc_libNames >> 24) & 0xFF) : 0);
        code[12] = (libNum ? ((loc_libNames >> 16) & 0xFF) : 0);
        code[1]  = (libNum ? ((loc_libNames >> 8) & 0xFF) : 0);
        code[2]  = (libNum ? (loc_libNames & 0xFF) : 0);

        // remove unused libraries
        long loc_libIds = loc_libNames + 1 + 2 * libNum;
        long actualLibNum = libNum;
        for (short i=0;i < libNum; i++) {
            if (!(*pLibs)[i].marked) {
                code[loc_libIds] = 0xff;
                code[loc_libIds+1] = 0xfe;
                actualLibNum--;
            }
            loc_libIds += 2;
        }
        if (actualLibNum == 0) {
            code[loc_libNames] = 0;
        }
    }

    code[18] = (funcI[fID].loc >> 24) & 0xFF;
    code[19] = (funcI[fID].loc >> 16) & 0xFF;
    code[20] = (funcI[fID].loc >> 8) & 0xFF;
    code[21] = funcI[fID].loc & 0xFF;

    // function info
    code[7] = (codePtr >> 24) & 0xff;
    code[8] = (codePtr >> 16) & 0xff;
    code[9] = (codePtr >> 8) & 0xff;
    code[10] = codePtr & 0xff;

    addWord(nGenFuncs);
    for (i=0;i<nFuncs;i++) {
        if (!funcI[i].lib && funcI[i].marked) {
            addInt(funcI[i].loc);
            addWord(addString(funcI[i].name));
        }
    }

    // total code segment size
    code[13] = (codePtr >> 24) & 0xff;
    code[14] = (codePtr >> 16) & 0xff;
    code[15] = (codePtr >> 8) & 0xff;
    code[16] = codePtr & 0xff;

    // set all the function addresses
    i = HEADER_SIZE;
    while (i < codeEnd) {
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
                wsprintf(msg, "%s definition not found", funcI[fID].name);
                c_error(msg, tok.line);
            }
            fID = funcI[fID].loc;
            code[i+1] = (fID >> 24) & 0xFF;
            code[i+2] = (fID >> 16) & 0xFF;
            code[i+3] = (fID >> 8) & 0xFF;
            code[i+4] = fID & 0xFF;
            if (code[i] == vmCallF)
                code[i] = vmOldCall;
            else
                code[i] = vmCInt;
        }
        i += (BCArgs[code[i]] + 1);
    }

    for (i=0;i<nGlobalInits;i++) {
        if (globalInit[i].type == vtMethodPtr) {
            globalInit[i].type = vtInt;
            globalInit[i].val = funcI[globalInit[i].val].loc;
        }
    }
}
#else
void relocate() {
    int i=13, lab;
    while (i < codePtr) {
        if (code[i] == vmJmp || code[i]==vmJmpZ || code[i]==vmJmpNZ) {
            lab = 0;
            lab |= code[i+1];
            lab <<=8;
            lab |= code[i+2];
            lab = reloc[lab];
            code[i+1] = (lab >> 8) & 0xFF;
            code[i+2] = lab & 0xFF;
        }
        if (code[i] == vmCallF || code[i] == vmFuncA) {
            unsigned short fID = 0;
            fID |= code[i+1];
            fID <<= 8;
            fID |= code[i+2];
            if (funcI[fID].loc == 0) {
                char msg[256];
                wsprintf(msg, "%s definition not found", funcI[fID].name);
                c_error(msg, tok.line);
            }
            fID = funcI[fID].loc;
            code[i+1] = (fID >> 8) & 0xFF;
            code[i+2] = fID & 0xFF;
            if (code[i] == vmCallF)
                code[i] = vmOldCall;
            else
                code[i] = vmCWord;
        }
        if (code[i] == vmCase) {
            lab = 0;
            lab |= code[i+6];
            lab <<=8;
            lab |= code[i+7];
            lab = reloc[lab];
            code[i+6] = (lab >> 8) & 0xFF;
            code[i+7] = lab & 0xFF;
        }
        i += (BCArgs[code[i]] + 1);
    }
}
#endif
