#include "PocketC.h"

short addMacro(char* name, char nTokens, short data) {
    if (nMacros >= _macro) {
        if (MemHandleResize(macro_h, sizeof(Macro)*(_macro+10)))
            oom();
        _macro+=10;
    }
    macro = (Macro*)MemHandleLock(macro_h);
    strcpy(macro[nMacros].name, name);
    macro[nMacros].len = strlen(name);
    macro[nMacros].nTokens = nTokens;
    macro[nMacros].data = data;
    MemHandleUnlock(macro_h);
    return nMacros++;
}

bool delMacro(char* name) {
    short res, nt;
    unsigned short d;

    res = findMacro(name, nt, d);
    if (res != -1) {
        macro = (Macro*)MemHandleLock(macro_h);
        macro[res].len = 0;
        macro[res].name[0] = '\0';
        MemHandleUnlock(macro_h);
        return true;
    }
    return false;
}

short findMacro(char* name, short& nTokens, unsigned short& data) {
    short len = strlen(name), i = 0;
    macro = (Macro*)MemHandleLock(macro_h);
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
    MemHandleUnlock(macro_h);
    return i;	
}

void macroDataInc(short delta) {
    if (MemHandleResize(macrod_h, _macrod+delta))
        oom();
    _macrod += delta;
}

short addMacroByte(char b) {
    short ret = nMacrod;
    if (nMacrod + 1 > _macrod) macroDataInc(10);
    macrod = (char*)MemHandleLock(macrod_h);
    macrod[nMacrod++] = b;
    MemHandleUnlock(macrod_h);
    return ret;
}

short addMacroInt(long l) {
    short ret = nMacrod;
    if (nMacrod + 4 > _macrod) macroDataInc(10);
    macrod = (char*)MemHandleLock(macrod_h);
    memcpy(&macrod[nMacrod], &l, 4);
    MemHandleUnlock(macrod_h);
    nMacrod+=4;
    return ret;
}

short addMacroFloat(float f) {
    short ret = nMacrod;
    if (nMacrod + 4 > _macrod) macroDataInc(10);
    macrod = (char*)MemHandleLock(macrod_h);
    memcpy(&macrod[nMacrod], &f, 4);
    MemHandleUnlock(macrod_h);
    nMacrod+=4;
    return ret;
}

short addMacroString(char* s) {
    short ret = nMacrod;
    short len = strlen(s);
    if (nMacrod + len + 1 > _macrod) macroDataInc(len + 1);
    macrod = (char*)MemHandleLock(macrod_h);
    memcpy(&macrod[nMacrod], s, len + 1);
    MemHandleUnlock(macrod_h);
    nMacrod+=len + 1;
    return ret;
}

short addGlobal(VarType type, char* name, short size) {
    if (nGlobals >= _global) {
        if (MemHandleResize(global_h, sizeof(GlobalInfo) *(nGlobals+10)))
            oom(); //c_error("out of memory.", tok.line);
        _global+=10;
    }
    globalI = (GlobalInfo*)MemHandleLock(global_h);
    globalI[nGlobals].type = type;
    strcpy(globalI[nGlobals].name, name);
    globalI[nGlobals].arraySize = size;
    globalI[nGlobals].globalOffset = globalOff;
    globalOff+=(size ? size : 1);
    MemHandleUnlock(global_h);
    return nGlobals++;
}

void addGlobalInit(VarType type, short offset, long val) {
    if (nGlobalInits >= _globalInits) {
        if (MemHandleResize(globalInits_h, sizeof(GlobalInit) *(nGlobalInits+10)))
            oom(); //c_error("out of memory.", tok.line);
        _globalInits+=10;
    }
    globalInit = (GlobalInit*)MemHandleLock(globalInits_h);
    globalInit[nGlobalInits].type = type;
    globalInit[nGlobalInits].offset = offset;
    globalInit[nGlobalInits].val = val;
    nGlobalInits++;
    MemHandleUnlock(globalInits_h);
}

short findGlobal(char* name) {
    wasArray = false;
    short i = 0, ret = -1;
    globalI = (GlobalInfo*)MemHandleLock(global_h);
    while (i < nGlobals) {
        if (!strcmp(name, globalI[i].name)) {
            ret = globalI[i].globalOffset;
            wasArray = globalI[i].arraySize;
            break;
        }
        i++;
    }
    MemHandleUnlock(global_h);
    return ret;
}

short addLocal(VarType type, char* name, short size) {
    if (nLocals >= _local) {
        if (MemHandleResize(local_h, sizeof(LocalInfo) *(nLocals+10)))
            oom();
        _local+=10;
    }
    localI = (LocalInfo*)MemHandleLock(local_h);
    localI[nLocals].type = type;
    strcpy(localI[nLocals].name, name);
    localI[nLocals].arraySize = size;
    localI[nLocals].stackOffset=localOff;
    localOff+=(size ? size : 1);
    MemHandleUnlock(local_h);
    return nLocals++;
}

short findLocal(char* name) {
    wasArray = false;
    short i = 0, ret = -1;
    localI = (LocalInfo*)MemHandleLock(local_h);
    while (i < nLocals) {
        if (!strcmp(name, localI[i].name)) {
            ret = localI[i].stackOffset;
            wasArray = localI[i].arraySize;
            break;
        }
        i++;
    }
    MemHandleUnlock(local_h);
    return ret;
}

short addFunc(char* name, short nArgs, long loc) {
    if (nFuncs >= _func) {
        if (MemHandleResize(func_h, sizeof(FuncInfo) *(nFuncs+10)))
            oom(); //c_error("out of memory.", tok.line);
        _func+=10;
    }
    short fID = findFunc(name, false);
    char msg[256];
    funcI = (FuncInfo*)MemHandleLock(func_h);
    if (fID >= 0) {
        if (funcI[fID].loc) {
            StrPrintF(msg, "%s already defined", name);
            c_error(msg, tok.line);
        }
        if (funcI[fID].nArgs != nArgs) {
            StrPrintF(msg, "%s declared with different # of parameters", name);
            c_error(msg, tok.line);
        }
        localI = (LocalInfo*)MemHandleLock(local_h);
        for (short i=0;i<nArgs;i++) {
            if (localI[i].type != funcI[fID].argType[i]) {
                StrPrintF(msg, "%s parameter %d differs from declaration", name, i + 1);
                c_error(msg, tok.line);
            }
        }
        funcI[fID].loc = (void(*)(short))loc;
        MemHandleUnlock(local_h);
        MemHandleUnlock(func_h);
        return fID;

    } else {
        strcpy(funcI[nFuncs].name, name);
        funcI[nFuncs].loc = (void(*)(short))loc;
        funcI[nFuncs].lib = 0;
        funcI[nFuncs].nArgs = nArgs;
        localI = (LocalInfo*)MemHandleLock(local_h);
        for (short i=0;i<nArgs;i++)
            funcI[nFuncs].argType[i] = localI[i].type;
        MemHandleUnlock(local_h);
        MemHandleUnlock(func_h);
        return nFuncs++;
    }
}

short addLibFunc(char* name, short nArgs, VarType arg1, VarType arg2, VarType arg3, VarType arg4, VarType arg5, VarType arg6, VarType arg7, VarType arg8, VarType arg9, VarType arg10) {
    if (nFuncs >= _func) {
        if (MemHandleResize(func_h, sizeof(FuncInfo) *(nFuncs+10)))
            oom();
        _func+=10;
    }
    funcI = (FuncInfo*)MemHandleLock(func_h);
    strcpy(funcI[nFuncs].name, name);
    funcI[nFuncs].loc = (void(*)(short))libFuncNum;
    funcI[nFuncs].lib = libNum;
    funcI[nFuncs].nArgs = nArgs;

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
    
    MemHandleUnlock(func_h);
    libFuncNum++;
    return nFuncs++;
}

short findFunc(char* name, bool isBI) {
    short i = 0, ret = -1;
    if (!isBI) funcI = (FuncInfo*)MemHandleLock(func_h);
    while (i < (isBI ? nBIFuncs : nFuncs)) {
        if (!strcmp(name, (isBI ? funcTable[i].name : funcI[i].name))) { ret = i; break; }
        i++;
    }
    if (!isBI) MemHandleUnlock(func_h);
    return ret;
}
