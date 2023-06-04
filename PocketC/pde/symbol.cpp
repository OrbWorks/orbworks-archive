#include "stdafx.h"
#include "pde.h"

short addMacro(char* name, char nTokens, unsigned short data) {
    if (nMacros + 1 >= MAX_MACRO) c_error("too many macros", tok.line);
    strcpy(macro[nMacros].name, name);
    macro[nMacros].len = strlen(name);
    macro[nMacros].nTokens = nTokens;
    macro[nMacros].data = data;
    return nMacros++;
}

bool delMacro(char* name) {
    short res, nt;
    unsigned short d;

    res = findMacro(name, nt, d);
    if (res != -1) {
        macro[res].len = 0;
        macro[res].name[0] = '\0';
        return true;
    }
    return false;
}

short findMacro(char* name, short& nTokens, unsigned short& data) {
    short len = strlen(name), i = 0;
    while (i < nMacros) {
        if (macro[i].len == len && !strcmp(name, macro[i].name)) {
            nTokens = macro[i].nTokens;
            data = macro[i].data;
            goto done;
        }
        i++;
    }
    i = -1;
done:
    return i;
}

unsigned short addMacroByte(char b) {
    if (nMacrod + 1 >= MAX_MACROD) c_error("too much macro data", tok.line);
    short ret = nMacrod;
    macrod[nMacrod++] = b;
    return ret;
}

unsigned short addMacroInt(long l) {
    if (nMacrod + 4 >= MAX_MACROD) c_error("too much macro data", tok.line);
    short ret = nMacrod;
    memcpy(&macrod[nMacrod], &l, 4);
    nMacrod+=4;
    return ret;
}

unsigned short addMacroFloat(float f) {
    if (nMacrod + 4 >= MAX_MACROD) c_error("too much macro data", tok.line);
    short ret = nMacrod;
    memcpy(&macrod[nMacrod], &f, 4);
    nMacrod+=4;
    return ret;
}

unsigned short addMacroString(char* s) {
    unsigned short ret = nMacrod;
    short len = strlen(s);
    if (nMacrod + len + 1 >= MAX_MACROD) c_error("too much macro data", tok.line);
    memcpy(&macrod[nMacrod], s, len + 1);
    nMacrod+=len + 1;
    return ret;
}

short addGlobal(VarType type, char* name, short size) {
    if (nGlobals + 1 >= MAX_GLOBAL) c_error("too many globals", tok.line);
    globalI[nGlobals].type = type;
    strcpy(globalI[nGlobals].name, name);
    globalI[nGlobals].arraySize = size;
    globalI[nGlobals].globalOffset = globalOff;
    globalOff+=(size ? size : 1);
    return nGlobals++;
}

void addGlobalInit(VarType type, unsigned short offset, int val) {
    if (nGlobalInits + 1 >= MAX_GLOBINIT) c_error("too many global initializers", tok.line);
    globalInit[nGlobalInits].type = type;
    globalInit[nGlobalInits].offset = offset;
    globalInit[nGlobalInits].val = val;
    nGlobalInits++;
}

#pragma warning (disable:4800)
short findGlobal(char* name, VarType* vt) {
    wasArray = false;
    short i = 0, ret = -1;
    while (i < nGlobals) {
        if (!strcmp(name, globalI[i].name)) {
            ret = globalI[i].globalOffset;
            wasArray = (bool)globalI[i].arraySize;
            if (vt) *vt = globalI[i].type;
            break;
        }
        i++;
    }
    return ret;
}

short addLocal(VarType type, char* name, short size) {
    if (nLocals + 1 >= MAX_LOCAL) c_error("too many locals", tok.line);
    localI[nLocals].type = type;
    strcpy(localI[nLocals].name, name);
    localI[nLocals].arraySize = size;
    localI[nLocals].stackOffset=localOff;
    localOff+=(size ? size : 1);
    return nLocals++;
}

short findLocal(char* name, VarType* vt) {
    wasArray = false;
    short i = 0, ret = -1;
    while (i < nLocals) {
        if (!strcmp(name, localI[i].name)) {
            ret = localI[i].stackOffset;
            wasArray = (bool)localI[i].arraySize;
            if (vt) *vt = localI[i].type;
            break;
        }
        i++;
    }
    return ret;
}

short addFunc(char* name, short nArgs, long loc) {
    short fID = findFunc(name, false);
    char msg[256];
    if (fID >= 0) {
        if (funcI[fID].loc) {
            wsprintf(msg, "%s already defined", name);
            c_error(msg, tok.line);
        }
        if (funcI[fID].nArgs != nArgs) {
            wsprintf(msg, "%s declared with different # of parameters", name);
            c_error(msg, tok.line);
        }
        for (short i=0;i<nArgs;i++) {
            if (localI[i].type != funcI[fID].argType[i]) {
                wsprintf(msg, "%s parameter %d differs from declaration", name, i);
                c_error(msg, tok.line);
            }
        }
        funcI[fID].loc = loc;
        funcI[fID].end = 0;
        return fID;

    } else {
        if (nFuncs + 1 >= MAX_FUNC) c_error("too many functions", tok.line);
        strcpy(funcI[nFuncs].name, name);
        funcI[nFuncs].loc = loc;
        funcI[nFuncs].end = 0;
        funcI[nFuncs].lib = 0;
        funcI[nFuncs].nArgs = (char)nArgs;
        for (char i=0;i<nArgs;i++)
            funcI[nFuncs].argType[i] = localI[i].type;
        return nFuncs++;
    }
}

short addLibFunc(char* name, short nArgs, VarType arg1, VarType arg2, VarType arg3, VarType arg4, VarType arg5, VarType arg6, VarType arg7, VarType arg8, VarType arg9, VarType arg10) {
    if (nFuncs + 1 >= MAX_FUNC) c_error("too many functions", tok.line);
    strcpy(funcI[nFuncs].name, name);
    funcI[nFuncs].loc = libFuncNum;
    funcI[nFuncs].lib = (char)libNum;
    funcI[nFuncs].nArgs = (char)nArgs;

    funcI[nFuncs].argType[0] = arg1;
    funcI[nFuncs].argType[1] = arg2;
    funcI[nFuncs].argType[2] = arg3;
    funcI[nFuncs].argType[3] = arg4;
    funcI[nFuncs].argType[4] = arg5;
    funcI[nFuncs].argType[5] = arg6;
    funcI[nFuncs].argType[6] = arg7;
    funcI[nFuncs].argType[7] = arg8;
    funcI[nFuncs].argType[8] = arg9;
    funcI[nFuncs].argType[9] = arg10;
    
    libFuncNum++;
    return nFuncs++;
}

short findFunc(char* name, bool isBI) {
    short i = 0, ret = -1;
    while (i < (isBI ? nBIFuncs : nFuncs)) {
        if (!strcmp(name, (isBI ? funcTable[i].name : funcI[i].name))) { ret = i; break; }
        i++;
    }
    return ret;
}
