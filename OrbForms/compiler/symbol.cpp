#include "compiler.h"

int Compiler::findFunc(string name) {
    for (int i=0;i<funcs.size();i++)
        if (funcs[i].name == name)
            return i;
    return -1;
}

int Compiler::findStructFunc(int structID, string name) {
    for (int i=0;i<structs[structID].funcs.size();i++)
        if (structs[structID].funcs[i].name == name)
            return structs[structID].funcs[i].funcID;
    return -1;
}

int Compiler::findStructVar(int structID, string name) {
    for (int i=0;i<structs[structID].vars.size();i++)
        if (structs[structID].vars[i].name == name)
            return i;
    return -1;
}

int Compiler::findStruct(string name) {
    for (int i=0;i<structs.size();i++)
        if (structs[i].name == name)
            return i;
    return -1;
}

int Compiler::findFuncPtr(string name) {
    for (int i=0;i<funcPtrs.size();i++)
        if (funcPtrs[i].name == name)
            return funcPtrs[i].firstMatch;
    return -1;
}

int Compiler::findFuncPtrMatch(Function& func) {
    int i;
    for (i=0;i<funcPtrs.size();i++) {
        if (func.type == funcPtrs[i].type && func.nArgs == funcPtrs[i].args.size()) {
            // compare arg types
            int j;
            for (j=0;j<func.nArgs;j++) {
                if (!(func.locals[j].type == funcPtrs[i].args[j].type))
                    break;
            }
            if (j == func.nArgs) return i;
        }
    }

    // need to create one
    FunctionPtr fp;
    fp.name = "*"; // no need for a name
    fp.type = func.type;
    for (i=0;i<func.nArgs;i++)
        fp.args.push_back(func.locals[i]);
    int id = funcPtrs.size();
    funcPtrs.push_back(fp);
    return id;
}

int Compiler::findLocal(string name) {
    for (int i=0;i<curFunc->locals.size();i++)
        if (curFunc->locals[i].name == name)
            return i;
    return -1;
}

int Compiler::findGlobal(string name) {
    for (int i=0;i<globals.size();i++)
        if (globals[i].name == name)
            return i;
    return -1;
}

int Compiler::findRes(string name) {
    for (int i=0;i<resSyms.size();i++)
        if (resSyms[i].name == name)
            return i;
    return -1;
}

bool Compiler::hasGadget(int structID) {
    for (int i=0;i<resSyms.size();i++)
        if (resSyms[i].type == rtGadget && resSyms[i].structID == structID)
            return true;
    return false;
}

int Compiler::findConst(string name) {
    for (int i=0;i<consts.size();i++)
        if (consts[i].name == name)
            return i;
    return -1;
}
