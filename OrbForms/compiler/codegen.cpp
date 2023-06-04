#include "compiler.h"

//#define SIMPLE_CODE

bool Compiler::GenCode() {
    l_break = l_cont = -1;
    labels.clear();
    l_funcend = getLabel();
    genStmts(funcInitTree.tree);
    funcInitTree.Delete();
    genStmts(funcTree.tree);
    funcTree.Delete();
    setLabel(l_funcend);
    genStmts(funcDestTree.tree);
    funcDestTree.Delete();
#if VERBOSE
    printf("    fixing labels\n");
#endif
    fixLabels();
    return true;
}

bool Compiler::MergeFuncs() {
#if VERBOSE
    printf("Merging funcs\n");
#endif

    vector<byte> globCode;

    int id = findFunc("main");
    if (id >= 0) {
        globCode.push_back(vmCall);
        globCode.push_back( id & 0x000000ff);
        globCode.push_back((id & 0x0000ff00) >> 8);
        globCode.push_back((id & 0x00ff0000) >> 16);
        globCode.push_back((id & 0xff000000) >> 24);
        globCode.push_back(vmHalt);
        funcs[id].marked = true;
    }

    int i;
    // generate the global init/destroy funclets
    if (globInitTree.tree) {
        globCode.push_back(vmNoOp2);
        int addr = addString("__global_init");
        globCode.push_back(addr & 0x00ff);
        globCode.push_back((addr & 0xff00) >> 8);
        iGlobInitLoc = globCode.size();
        Function func;
        curFunc = &func;
        genStmts(globInitTree.tree);
#ifdef WIN32
        globCode.reserve(globCode.size() + func.code.size());
#else
        globCode.reserve(func.code.size());
#endif
        for (int j=0;j<func.code.size();j++)
            globCode.push_back(func.code[j]);
        globCode.push_back(vmRetN);
        globCode.push_back(0);
    }

    if (globDestTree.tree) {
        globCode.push_back(vmNoOp2);
        int addr = addString("__global_destroy");
        globCode.push_back(addr & 0x00ff);
        globCode.push_back((addr & 0xff00) >> 8);
        iGlobDestLoc = globCode.size();
        Function func;
        curFunc = &func;
        genStmts(globDestTree.tree);
#ifdef WIN32
        globCode.reserve(globCode.size() + func.code.size());
#else
        globCode.reserve(func.code.size());
#endif
        for (int j=0;j<func.code.size();j++)
            globCode.push_back(func.code[j]);
        globCode.push_back(vmRetN);
        globCode.push_back(0);
    }

    // mark the gadget methods that are handlers
    for(i=0;i<funcs.size();i++) {
        if (funcs[i].structID != -1 && hasGadget(funcs[i].structID)) {
            for (int h=0;h<nResHandlers[rtGadget];h++) {
                if (funcs[i].methodName == handlerNames[rtGadget][h]) {
                    funcs[i].marked = true;
                    if (funcs[i].lib != -1)
                        // if this is in a native lib, mark it too
                        libraries[funcs[i].lib].marked = true;
                    break;
                }
            }
        }
    }

    // mark global function pointers/objects
    for (i=0;i<globInits.size();i++) {
        if (globInits[i].val.type == vtFuncPtr) {
            int h = globInits[i].val.iVal;
            if (h != -1)
                funcs[h].marked = true;
        } else if (globInits[i].val.type == vtObjInfo) {
            int h = globInits[i].val.iVal;
            if (!structs[h].marked) {
                structs[h].marked = true;
#if VERBOSE
                printf("adding %s [global]\n", structs[h].name.c_str());
#endif
            }
        }
    }

    // mark methods that are in the global init/dest
    processMarkedFunc(globCode, "global init");

    // find functions and objects that are actually used
    bool changed;
    do {
        changed = false;
        for (i=0;i<funcs.size();i++) {
            if (!funcs[i].builtin && funcs[i].marked && funcs[i].code.size() != 0) {
                if (processMarkedFunc(funcs[i].code, funcs[i].name))
                    changed = true;
            }
        }
        for (i=0;i<structs.size();i++) {
            if ((structs[i].isObject || structs[i].isIface) && structs[i].marked) {
                if (processMarkedObject(i))
                    changed = true;
            }
        }
    } while (changed);

    // allocate space for code
    int totalCodeSize = globCode.size();
    for (i=0;i<funcs.size();i++) {
        if (!funcs[i].builtin) {
            if (funcs[i].marked && funcs[i].code.size() != 0) {
                if (funcs[i].code.size() + 3 > CODE_SEGMENT_SIZE)
                    c_error("function size too large", -1);
                int currentSeg = totalCodeSize / CODE_SEGMENT_SIZE;
                if ((totalCodeSize + funcs[i].code.size() + 3) / CODE_SEGMENT_SIZE > currentSeg) {
                    // function would cross segment boundary
                    totalCodeSize = ((totalCodeSize / CODE_SEGMENT_SIZE) + 1) * CODE_SEGMENT_SIZE;
                }
                funcs[i].loc = totalCodeSize + 3;
                totalCodeSize += funcs[i].code.size() + 3;
            }
        }
    }

    GenObjInfo();

    // copy global code
    writeCode(globCode, "");

    // copy function code
    for (i=0;i<funcs.size();i++) {
        if (!funcs[i].builtin) {
            if (funcs[i].marked && funcs[i].code.size() != 0) {
                writeCode(funcs[i].code, funcs[i].name);
            }
        }
    }

#if VERBOSE
    printf("Fixing call addresses\n");
#endif
    FixupAddresses();
    return true;
}

void Compiler::writeCode(vector<byte>& fcode, string name) {
    int codeSeg = codeSize / CODE_SEGMENT_SIZE;
    if (codeSize + fcode.size() + 3 > (codeSeg + 1) * CODE_SEGMENT_SIZE) {
        // this segment is finished, move to the next
        codeSeg++;
        codeSize = codeSeg * CODE_SEGMENT_SIZE;// round up the code size
    }
    if (codeSeg >= MAX_CODE_SEGMENTS)
        c_error("too much code", -1);
    if (!codeSegments[codeSeg]) {
        codeSegments[codeSeg] = new dbvector<byte>();
#ifndef WIN32
        if (!codeSegments[codeSeg])
            oom();
        extern DmOpenRef PcDB;
        codeSegments[codeSeg]->init(PcDB);
#endif
    }

    dbvector<byte>& code = *codeSegments[codeSeg];

    if (!name.empty()) {
        int sloc = addString(name.c_str());
        code.push_back(vmNoOp2);
        code.push_back(sloc & 0x00ff);
        code.push_back((sloc & 0xff00) >> 8);
        codeSize += 3;
    }

    // write out code, fixing all references to objinfo and function locations
    int start = code.size();
    int i=0;
    while (i < fcode.size()) {
        byte b = fcode[i];
        if ((b & 0xC0) == 0xC0) {
            code.push_back(b);
            i++;
            byte cb = fcode[i];
            code.push_back(cb);
            assert(bcsize[cb] == 0);
            if ((b & 0x02)) {
                code.push_back(fcode[++i]);
                code.push_back(fcode[++i]);
            }
            if ((b & 0x08)) {
                code.push_back(fcode[++i]);
                code.push_back(fcode[++i]);
            }
            if ((b & 0x20)) {
                code.push_back(fcode[++i]);
                code.push_back(fcode[++i]);
            }
            i++;
        } else {
            if (b == vmCall || b == vmFuncA) {
                assert(bcsize[b] == 4);
                unsigned id = fcode[i+1] + ((unsigned)fcode[i+2] << 8) + ((unsigned)fcode[i+3] << 16) + ((unsigned)fcode[i+4] << 24);
                assert(id < funcs.size());
                if (funcs[id].loc == -1) {
                    string err = "function '";
                    err += funcs[id].name;
                    err += "' referenced but not defined";
                    c_error(err.c_str(), -1);
                }
                if (b == vmFuncA)
                    code.push_back(vmCInt);
                else
                    code.push_back(b);

                unsigned off = funcs[id].loc;
                code.push_back((byte)(off & 0x000000ff));
                code.push_back((byte)((off & 0x0000ff00) >> 8));
                code.push_back((byte)((off & 0x00ff0000) >> 16));
                code.push_back((byte)((off & 0xff000000) >> 24));

            } else if (b == vmSObjInfo) {
                code.push_back(vmSCWord);
                code.push_back(fcode[i+1]);
                code.push_back(fcode[i+2]);
                word id = fcode[i+3] + ((unsigned)fcode[i+4] << 8);
                id = structs[id].objinfo;
                assert(id != -1);
                code.push_back((byte)(id & 0x00ff));
                code.push_back((byte)((id & 0xff00) >> 8));

            } else if (b == vmCallIface) {
                code.push_back(b);
                code.push_back(fcode[i+1]);
                word id = fcode[i+2] + ((unsigned)fcode[i+3] << 8);
                id = structs[id].objinfo;
                assert(id != -1);
                code.push_back((byte)(id & 0x00ff));
                code.push_back((byte)((id & 0xff00) >> 8));
                code.push_back(fcode[i+4]);

            } else if (b == vmNewObj) {
                code.push_back(b);
                word id = fcode[i+1] + ((unsigned)fcode[i+2] << 8);
                id = structs[id].objinfo;
                assert(id != -1);
                code.push_back((byte)(id & 0x00ff));
                code.push_back((byte)((id & 0xff00) >> 8));

            } else {
                code.push_back(b);
                for (int j=0;j<bcsize[b];j++)
                    code.push_back(fcode[i+j+1]);
            }
            i += bcsize[b] + 1;
        }
    }

    codeSize += i;
    assert(code.size() - start == fcode.size());
}

bool Compiler::processMarkedFunc(vector<byte>& funcCode, string name) {
    bool changed = false;
    // find functions and objects referenced
    int j=0;
    while (j<funcCode.size()) {
        byte b = funcCode[j];
        if ((b & 0xC0) == 0xC0) {
            j++;
            byte cb = funcCode[j];
            if (b & 0x02)
                j += 2;
            if (b & 0x08)
                j += 2;
            if (b & 0x20)
                j += 2;
            b = cb;
        }
        if (b == vmCall || b == vmFuncA) {
            assert(bcsize[b] == 4);
            unsigned id = funcCode[j+1] + ((unsigned)funcCode[j+2] << 8) +
                ((unsigned)funcCode[j+3] << 16) + ((unsigned)funcCode[j+4] << 24);
            assert(id < funcs.size());
            if (!funcs[id].marked) {
                funcs[id].marked = true;
                changed = true;
            }
        } else if (b == vmLibCall || b == vmLibStubCall) {
            // mark the library as being used
            assert(bcsize[b] == 3);
            unsigned id = funcCode[j+1];
            assert(id < libraries.size());
            libraries[id].marked = true;
        } else if (b == vmSObjInfo) {
            // mark the object as being used
            word id = funcCode[j+3] + ((unsigned)funcCode[j+4] << 8);
            if (!structs[id].marked) {
                structs[id].marked = true;
                changed = true;
#if VERBOSE
                printf("adding %s [local: %s]\n", structs[id].name.c_str(), name.c_str());
#endif
            }
        } else if (b == vmNewObj) {
            // mark the object as being used
            word id = funcCode[j+1] + ((unsigned)funcCode[j+2] << 8);
            if (!structs[id].marked) {
                structs[id].marked = true;
                changed = true;
#if VERBOSE
                printf("adding %s [new: %s]\n", structs[id].name.c_str(), name.c_str());
#endif
            }
        }
        
        j += bcsize[b] + 1;
    }
    return changed;
}

// an object is used if:
// 1. it is a global (vtObjInfo)
// 2. it is a parameter of a used function (covered by other cases)
// 3. it is a local variable (vmSObjInfo)
// 4. it is allocated at runtime (vmNewObj)
// 5. it is the base (or interface) of a used object
// 6. it is a member of a used object

bool Compiler::processMarkedObject(int objID) {
    bool changed = false;
    int j;
    // mark all virtual methods as being used
    for (j=0;j<structs[objID].funcs.size();j++) {
        Function& func = funcs[structs[objID].funcs[j].funcID];
        if (func.isVirtual) {
            if (!func.marked) {
                func.marked = true;
                changed = true;
                if (func.builtin) {
                    int wid = buildFuncWrapper(structs[objID].funcs[j].funcID);
                    funcs[wid].marked = true;
                }
            }
        }
    }

    // mark all base objects
    for (j=0;j<structs[objID].bases.size();j++) {
        Struct& st = structs[structs[objID].bases[j]];
        if ((st.isObject || st.isIface) && !st.marked) {
            st.marked = true;
            changed = true;
#if VERBOSE
            printf("adding %s [base]\n", st.name.c_str());
#endif
        }
    }

    // mark all member objects
    for (j=0;j<structs[objID].vars.size();j++) {
        Variable& var = structs[objID].vars[j];
        if (var.type.indir == 0 && var.type.vtype == vtStruct) {
            Struct& st = structs[var.type.structID];
            if ((st.isObject || st.isIface) && !st.marked) {
                st.marked = true;
                changed = true;
#if VERBOSE
                printf("adding %s [member]\n", st.name.c_str());
#endif
            }
        }
    }

    return changed;
}

void Compiler::FixupAddresses() {
    // fix global inits
    int i;
    for (i=0;i<globInits.size();i++) {
        if (globInits[i].val.type == vtFuncPtr) {
            long h = globInits[i].val.iVal;
            if (h != -1)
                globInits[i].val.iVal = funcs[h].loc;
            globInits[i].val.type = vtInt;
        } else if (globInits[i].val.type == vtObjInfo) {
            long h = globInits[i].val.iVal;
            globInits[i].val.iVal = structs[h].objinfo;
            assert(structs[h].objinfo);
            globInits[i].val.type = vtInt;
        }
    }

    for (i=0;i<globTypes.size();i++) {
        if (globTypes[i] == vtHandler)
            globTypes[i] = vtInt;
    }
}

void Compiler::GenObjInfo() {
    // create obj info table
    int i;
    for (i=0;i<structs.size();i++) {
        Struct& st = structs[i];
        if (!st.isObject || !st.marked) continue;
        int j;
        // name
        objinfo.push_back(0); // null-starter
        for (j=0;j<st.name.length();j++)
            objinfo.push_back(st.name[j]);
        objinfo.push_back(0);

        st.objinfo = objinfo.size();
        // base
        if (st.baseID == -1) {
            objinfo.push_back(0xff);
            objinfo.push_back(0xff);
        } else {
            assert(structs[st.baseID].objinfo != -1);
            objinfo.push_back(structs[st.baseID].objinfo & 0x00ff);
            objinfo.push_back((structs[st.baseID].objinfo & 0xff00) >> 8);
        }

        objinfo.push_back(st.size & 0x00ff);
        objinfo.push_back((st.size  & 0xff00) >> 8);

        // interfaces only have their names in the table
        if (st.isIface) {
            // nVirtualMethods
            objinfo.push_back(0);
            // nIfaces
            objinfo.push_back(0);
            // nIfaceMethods
            objinfo.push_back(0);
            objinfo.push_back(0);
            // nSubObjects
            objinfo.push_back(0);
            objinfo.push_back(0);
            continue;
        }
        
        // write virtual method table
        int nVirtual = 0;
        for (j=0;j<st.funcs.size();j++) {
            if (st.funcs[j].isVirtual) nVirtual++;
        }

        if (nVirtual > 255) {
            string err = "object '";
            err += st.name;
            err += "' has too many virtual methods";
            c_error(err.c_str(), -1);
        }
        objinfo.push_back(nVirtual);
        for (j=0;j<st.funcs.size();j++) {
            if (st.funcs[j].isVirtual) {
                Function& func = funcs[st.funcs[j].funcID];
                assert(func.isVirtual);
                assert(func.marked);
                if (func.loc == -1) {
                    string err = "virtual function '";
                    err += func.name;
                    err += "' not defined";
                    c_error(err.c_str(), -1);
                }
                unsigned off = func.loc;
                if (func.builtin) {
                    // retrieve the already generated wrapper
                    int wid = buildFuncWrapper(st.funcs[j].funcID);
                    assert(funcs[wid].loc != -1);
                    off = funcs[wid].loc;
                }
                objinfo.push_back(off & 0x000000ff);
                objinfo.push_back((off & 0x0000ff00) >> 8);
                objinfo.push_back((off & 0x00ff0000) >> 16);
                objinfo.push_back((off & 0xff000000) >> 24);
            }
        }

        // list of interfaces must include all interfaces inherited recursively
        // for each directly inherited interface, there must be a consecutive
        // list of ordered function addresses (recursively inherited interfaces
        // point into these directly inherited lists)
        vector<int> ifaces;
        getInterfaces(i, ifaces);

        // create a list of top level ifaces
        vector<int> topIfaces;
        for (j=0;j<ifaces.size();j++) {
            bool found = false; 
            for (int k=0;k<ifaces.size();k++) {
                if (j == k) continue;
                assert(ifaces[j] != ifaces[k]);
                if (isBaseType(ifaces[k], ifaces[j])) {
                    found = true;
                    break;
                }
            }
            if (!found)
                topIfaces.push_back(ifaces[j]);
        }

        // calculate number of method entries
        int nIfaceMethods = 0;
        for (j=0;j<topIfaces.size();j++) {
            nIfaceMethods += structs[topIfaces[j]].funcs.size();
        }
        objinfo.push_back(ifaces.size());
        objinfo.push_back(nIfaceMethods & 0x00ff);
        objinfo.push_back((nIfaceMethods & 0xff00) >> 8);

        // write ifaces
        int ifaceMethodBase = objinfo.size() + ifaces.size() * 6;
        for (j=0;j<ifaces.size();j++) {
            int off = structs[ifaces[j]].objinfo;
            assert(off != -1);
            objinfo.push_back(off & 0x00ff);
            objinfo.push_back((off & 0xff00) >> 8);
            off = 0;
            int res_offset = 0;
            bool found = false;
            for (int k=0;k<topIfaces.size();k++) {
                if ((found = findInterfaceOffset(topIfaces[k], ifaces[j], off, res_offset)) == true)
                    break;
                off += structs[topIfaces[k]].funcs.size();
            }
            assert(found);
            off = ifaceMethodBase + res_offset * 4;
            objinfo.push_back(off & 0x000000ff);
            objinfo.push_back((off & 0x0000ff00) >> 8);
            objinfo.push_back((off & 0x00ff0000) >> 16);
            objinfo.push_back((off & 0xff000000) >> 24);
        }

        // write the method tables
        for (j=0;j<topIfaces.size();j++) {
            Struct& sti = structs[topIfaces[j]];
            if (!sti.isIface) continue;
            for (int k=0;k<sti.funcs.size();k++) {
                int fid = findStructFunc(i, sti.funcs[k].name);
                int off = funcs[fid].loc;
                if (funcs[fid].builtin) {
                    // retrieve the already generated wrapper
                    int wid = buildFuncWrapper(fid);
                    assert(funcs[wid].loc != -1);
                    off = funcs[wid].loc;
                }
                objinfo.push_back(off & 0x000000ff);
                objinfo.push_back((off & 0x0000ff00) >> 8);
                objinfo.push_back((off & 0x00ff0000) >> 16);
                objinfo.push_back((off & 0xff000000) >> 24);
            }
        }

        // write subobject table
        objinfo.push_back(0);
        objinfo.push_back(0);
        int oisize = objinfo.size();
        int nSubObjects = 0;
        for (j=0;j<st.vars.size();j++) {
            Variable& var = st.vars[j];
            if (var.type.vtype == vtStruct && var.type.indir == 0 && structs[var.type.structID].isObject) {
                if (var.alen) {
                    int offset = var.offset;
                    for (int k=0;k<var.alen;k++) {
                        objinfo.push_back(offset & 0x00ff);
                        objinfo.push_back((offset & 0xff00) >> 8);
                        assert(structs[var.type.structID].objinfo != -1);
                        objinfo.push_back(structs[var.type.structID].objinfo & 0x00ff);
                        objinfo.push_back((structs[var.type.structID].objinfo & 0xff00) >> 8);
                        offset += structs[var.type.structID].size;
                        nSubObjects++;
                    }
                } else {
                    objinfo.push_back(var.offset & 0x00ff);
                    objinfo.push_back((var.offset & 0xff00) >> 8);
                    assert(structs[var.type.structID].objinfo != -1);
                    objinfo.push_back(structs[var.type.structID].objinfo & 0x00ff);
                    objinfo.push_back((structs[var.type.structID].objinfo & 0xff00) >> 8);
                    nSubObjects++;
                }
            }
        }

        objinfo[oisize - 2] = (nSubObjects & 0x00ff);
        objinfo[oisize - 1] = (nSubObjects & 0xff00) >> 8;
    }
}

void Compiler::getInterfaces(int structID, vector<int>& ifaces) {
    Struct& st = structs[structID];
    for (int i=0;i<st.bases.size();i++) {
        bool found = false;
        if (structs[st.bases[i]].isIface) {
            for (int j=0;j<ifaces.size();j++) {
                if (ifaces[j] == st.bases[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ifaces.push_back(st.bases[i]);
                getInterfaces(st.bases[i], ifaces);
            }
        } else {
            getInterfaces(st.bases[i], ifaces);
        }
    }
}

bool Compiler::findInterfaceOffset(int structID, int ifaceID, int init_offset, int& res_offset) {
    if (structID == ifaceID) {
        res_offset = init_offset;
        return true;
    }

    Struct& st = structs[structID];
    int offset = init_offset;
    for (int i=0;i<st.bases.size();i++) {
        assert(structs[st.bases[i]].isIface);
        if (st.bases[i] == ifaceID) {
            res_offset = offset;
            return true;
        }
        if (findInterfaceOffset(st.bases[i], ifaceID, offset, res_offset)) {
            return true;
        }
        offset += structs[st.bases[i]].funcs.size();
    }
    return false;
}

int Compiler::addString(const char* str) {
    // does string already exist?
    // @TODO: optimize this?
    int i = 0;
    for (i=0;i<strings.size();i++) {
        if (strcmp(str, &strings[i]) == 0) return i;
    }
    int len = strlen(str);
    int pos = strings.size();
    if (strings.size() + len >= 0x10000)
        c_error("string segment too large", -1);

#ifdef WIN32
    strings.reserve(strings.size() + len + 1);
#else
    strings.reserve(len + 1);
#endif

    for (i=0;i<len+1;i++)
        strings.push_back(str[i]);

    return pos;
}

void Compiler::addByte(byte b) {
    curFunc->code.push_back(b);
}

void Compiler::addWord(int i) {
    curFunc->code.push_back(i & 0x00ff);
    curFunc->code.push_back((i & 0xff00) >> 8);
}

void Compiler::addLong(int i) {
    unsigned int u = i;
    curFunc->code.push_back(u & 0x000000ff);
    curFunc->code.push_back((u & 0x0000ff00) >> 8);
    curFunc->code.push_back((u & 0x00ff0000) >> 16);
    curFunc->code.push_back((u & 0xff000000) >> 24);
}

int Compiler::getLabel() {
    int ret = labels.size();
    labels.push_back(-1);
    return ret;
}

void Compiler::setLabel(int l) {
    labels[l] = curFunc->code.size();
}

bool Compiler::fixLabels() {
    int i=0;
    while (i < curFunc->code.size()) {
        byte b = curFunc->code[i];
        if ((b & 0xC0) == 0xC0) {
            i++;
            byte cb = curFunc->code[i];
            assert(bcsize[cb] == 0);
            if ((b & 0x02))
                i += 2;
            if ((b & 0x08))
                i += 2;
            if ((b & 0x20))
                i += 2;
            b = cb;
        }
        if ((b >= vmJmp && b <= vmJmpPopNZ) || b == vmDefault) {
            assert(bcsize[b] == 2);
            int id = (int)curFunc->code[i+1] + (int)curFunc->code[i+2] * 256;
            assert(id < labels.size());
            int off = labels[id];
            curFunc->code[i+1] = off & 0x00ff;
            curFunc->code[i+2] = (off & 0xff00) >> 8;
        } else if (b == vmCase) {
            assert(bcsize[b] == 6);
            int id = (int)curFunc->code[i+5] + (int)curFunc->code[i+6] * 256;
            assert(id < labels.size());
            int off = labels[id];
            curFunc->code[i+5] = off & 0x00ff;
            curFunc->code[i+6] = (off & 0xff00) >> 8;
        }
        i += bcsize[b] + 1;
    }
    return false;
}

bool Compiler::genStmts(StmtNode* st) {
    while (st) {
        genStmt(st);
        st = st->next;
    }
    return true;
}

bool Compiler::genExprs(ExprNode* e) {
    while (e) {
        genExpr(e);
        e = e->next;
    }
    return true;
}

bool Compiler::genStmt(StmtNode* st) {
    //stNone, stExpr, stIf, stWhile, stDo, stFor, stBreak, stContinue, stReturn, stArray,
    //stProlog, stEpilog
    
    switch (st->stmt) {
        case stNone:
            break;
        case stExpr:
            genExprs(st->expr[0]);
            break;
        case stIf:
        {
            int l_else = getLabel(), end = getLabel();
            genExprs(st->expr[0]);
            addByte(vmJmpZ);
            addWord(l_else);
            genStmts(st->kids[0]);
            if (st->kids[1]) {
                addByte(vmJmp);
                addWord(end);
            }
            setLabel(l_else);
            if (st->kids[1]) {
                genStmts(st->kids[1]);
            }
            setLabel(end);
            break;
        }
        case stWhile:
        {
            int l_pbreak = l_break, l_pcont = l_cont;
            int cond = l_cont = getLabel(), end = l_break = getLabel();
            setLabel(cond);
            genExprs(st->expr[0]);
            addByte(vmJmpZ);
            addWord(end);
            genStmts(st->kids[0]);
            addByte(vmJmp);
            addWord(cond);
            setLabel(end);
            l_break = l_pbreak, l_cont = l_pcont;
            break;
        }
        case stDo:
        {
            int l_pbreak = l_break, l_pcont = l_cont;
            int begin = l_cont = getLabel(), end = l_break = getLabel();
            setLabel(begin);
            genStmts(st->kids[0]);
            genExprs(st->expr[0]);
            addByte(vmJmpNZ);
            addWord(begin);
            setLabel(end);
            l_break = l_pbreak, l_cont = l_pcont;
            break;
        }
        case stFor:
        {
            int l_pbreak = l_break, l_pcont = l_cont;
            int begin = getLabel(), iter = l_cont = getLabel(), end = l_break = getLabel();

            if (!st->expr[2])
                l_cont = begin;

            if (st->expr[0]) {
                genExprs(st->expr[0]);
                if (!(st->expr[0]->type.vtype == vtVoid && st->expr[0]->type.indir == 0))
                    addByte(vmPop);
            }
            setLabel(begin);
            if (st->expr[1]) {
                genExprs(st->expr[1]);
                addByte(vmJmpZ);
                addWord(end);
            }
            genStmts(st->kids[0]);
            setLabel(iter);
            if (st->expr[2]) {
                genExprs(st->expr[2]);
                if (!(st->expr[2]->type.vtype == vtVoid && st->expr[2]->type.indir == 0))
                    addByte(vmPop);
            }
            addByte(vmJmp);
            addWord(begin);
            setLabel(end);
            l_break = l_pbreak, l_cont = l_pcont;
            break;
        }
        case stBreak:
        {
            if (l_break == -1)
                c_error("break without loop", -1);
            addByte(vmJmp);
            addWord(l_break);
            break;
        }
        case stContinue:
        {
            if (l_cont == -1)
                c_error("continue without loop", -1);
            addByte(vmJmp);
            addWord(l_cont);
            break;
        }
        case stReturn:
        {
            if (st->expr[0]) {
                if (curFunc->type.vtype == vtStruct && curFunc->type.indir == 0) {
                    // push the address of the return thing
                    addByte(vmLoadFP);
                    addWord(0); // first "arg"
                }
                genExprs(st->expr[0]);
                if (curFunc->type.vtype == vtStruct && curFunc->type.indir == 0) {
                    if (structs[curFunc->type.structID].hasCopy) {
                        int fID = findStructFunc(curFunc->type.structID, "_copy");
                        assert(fID != -1);
                        genFuncCall(fID);
                    } else {
                        addByte(vmCopy);
                        addWord(structs[curFunc->type.structID].size);
                    }
                }
            }

            addByte(vmJmp);
            addWord(l_funcend);
            break;
        }
        case stSwitch:
        {
            int l_pbreak = l_break;
            l_break = getLabel();
            SwitchStmtNode* sst = reinterpret_cast<SwitchStmtNode*>(st);
            int nCase = sst->casep.size();
            genExprs(st->expr[0]);
            addByte(vmSwitchI + (st->expr[0]->type.vtype - vtInt));
            addByte(nCase);

            for (int i=0;i<nCase;i++) {
                VarType type = sst->vals[i].type;
                OpType ot = sst->casep[i]->expr[0]->op;
                assert(ot >= opCInt && ot <= opCWord);
                addByte(vmCase);
                if (type == vtFloat)
                    addLong(*((int*)(&(sst->vals[i].fVal))));
                else if (type == vtChar)
                    addLong(sst->vals[i].cVal);
                else
                    addLong(sst->vals[i].iVal);
                int l_case = getLabel();
                addWord(l_case);
                sst->casep[i]->data = l_case;
            }
            if (sst->def) {
                addByte(vmDefault);
                int l_default = getLabel();
                sst->def->data = l_default;
                addWord(l_default);
            }
            addByte(vmJmp);
            addWord(l_break);
            genStmts(sst->kids[0]);
            setLabel(l_break);
            l_break = l_pbreak;
            break;
        }
        case stCase:
        {
            setLabel(st->data);
            break;
        }
        case stDefault:
        {
            setLabel(st->data);
            break;
        }
        case stArray:
        {
            addByte(vmArray);
            addWord(st->data);
            break;
        }
        case stProlog:
        {
            addByte(vmLink);
            addByte(curFunc->nArgs);
            break;
        }
        case stDefReturn:
        {
            bool hasRet = !(curFunc->type.vtype == vtVoid && curFunc->type.indir == 0);
            if (hasRet) {
                // generate default return values
                if (curFunc->type.indir || curFunc->type.vtype == vtInt || curFunc->type.vtype == vtVariant) {
                    addByte(vmCWord);
                    addWord(0);
                } else if (curFunc->type.vtype == vtFloat) {
                    addByte(vmCFloat);
                    float f = 0;
                    addLong(*((int*)(&f)));
                } else if (curFunc->type.vtype == vtChar) {
                    addByte(vmCChar);
                    addByte(0);
                } else if (curFunc->type.vtype == vtString) {
                    addByte(vmCString);
                    addWord(addString(""));
                } else if (curFunc->type.vtype == vtStruct) {
                    addByte(vmLoadFP);
                    addWord(0);
                }
            }
            break;
        }
        case stEpilog:
        {
            bool hasRet = !(curFunc->type.vtype == vtVoid && curFunc->type.indir == 0);
            if (curFunc->structID != -1 && curFunc->methodName == "_copy") {
                // return this
                addByte(vmLoadFP);
                addWord(0);
                hasRet = true;
            }
            if (hasRet)
                addByte(vmSetRet);
            int locSize = curFunc->locSize - curFunc->argSize - PROLOG_SIZE;
            if (locSize > 1) {
                addByte(vmPopN);
                addWord(locSize);
            } else if (locSize == 1) {
                addByte(vmPop);
            }
            addByte(vmUnLink);
            if (hasRet)
                addByte(vmRet);
            else
                addByte(vmRetN);
            addByte(curFunc->nArgs);
            break;
        }

        default:
            c_error("unexpected StmtNode type", -1);
    }

    return true;
}

/*
    opAdd, opSub, opMult, opDiv, opMod,
    opAssign, opLoad, opGetProp, opSetProp,
    opBOr, opBAnd, opXOr, opSL, opSR,
    opOr, opAnd,
    opLT, opGT, opLTE, opGTE, opEq, opNEq,
    opNot, opNeg, opIncB, opIncA, opDecB, opDecA,
    opCall, opBICall, opSwap, opStackDup, opNew, opDelete, opDiscard,
    opCInt, opCChar, opCString, opCFloat, opCWord, opCWordPFP,
    opItoC, opItoF, opItoS, opFtoI, opFtoC, opFtoS,
    opCtoI, opCtoF, opCtoS, opStoI, opStoF, opStoC,
    opStoB,
*/

// assertion helpers
static bool isPtr(ExprNode* e) {
    return (e->type.indir > 0);
}

static bool isFuncPtr(ExprNode* e) {
    return (e->type.vtype == vtFuncPtr && e->type.indir == 0);
}

static bool isSimple(ExprNode* e) {
    if (e->type.indir) return false;
    return (e->type.vtype >= vtInt && e->type.vtype <= vtString);
}

static bool isVariant(ExprNode* e) {
    if (e->type.indir) return false;
    return (e->type.vtype == vtVariant);
}

static bool isIntegral(ExprNode* e) {
    if (e->type.indir) return false;
    return (e->type.vtype == vtInt || e->type.vtype == vtChar);
}

static bool isNumeric(ExprNode* e) {
    if (e->type.indir) return false;
    return (e->type.vtype >= vtInt && e->type.vtype <= vtFloat);
}

#define MAKE_LOC(addr, loc) (loc ? (addr & 0xC000) : addr)
#define MAKE_LOCVAL(e) (e->op == opCWordPFP ? (e->val.iVal | 0xC000) : e->val.iVal)
#define MAKE_OP(op, type) (VMsym)(op + (int)type)
#define IS_WORD(e) (e->op == opCWord || e->op == opCWordPFP)
#define IS_LOC(e) (e->op == opCWordPFP)

static VarType simpleType(Type type) {
    if (type.indir || type.vtype == vtFuncPtr)
        return vtInt;
    return type.vtype;
}

void Compiler::emitExt(VMsym inst, OperandType a, OperandType b, OperandType r, int av, int bv, int rv) {
    if ((a == otStack || a == otNone) && (b == otStack || b == otNone) && (r == otStack || r == otNone))
        addByte(inst);
    else {
        byte opinfo = 0;
        opinfo = 0xC0 | ((byte)a << 4) | ((byte)b << 2) | ((byte)r);
        addByte(opinfo);
        addByte(inst);
        if (a == otImmed || a == otAddr)
            addWord(av);
        if (b == otImmed || b == otAddr)
            addWord(bv);
        if (r == otImmed || r == otAddr)
            addWord(rv);
    }
}

bool Compiler::genBinary(VMsym inst, ExprNode* l, ExprNode* r) {
    assert(l && r);
    OperandType a = otStack, b = otStack;
    int av = 0, bv = 0;
    if (IS_WORD(l)) {
        a = otImmed;
        av = MAKE_LOCVAL(l);
    } else if (l->op == opLoadI) {
        a = otAddr;
        av = l->val.iVal;
    } else {
        genExprs(l);
    }

    if (IS_WORD(r)) {
        b = otImmed;
        bv = MAKE_LOCVAL(r);
    } else if (r->op == opLoadI) {
        b = otAddr;
        bv = r->val.iVal;
    } else {
        genExprs(r);
    }
    emitExt(inst, a, b, otStack, av, bv);
    return true;
}

bool Compiler::genVariantBinary(VMsym inst, ExprNode* l, ExprNode* r) {
    assert(l && r);
    genExprs(l);
    genExprs(r);
    addByte(vmVOp2);
    addByte(inst); // int version
    return true;
}

bool Compiler::genUnary(VMsym inst, ExprNode* l) {
    assert(l);
    OperandType a = otStack;
    int av = 0;
    if (IS_WORD(l)) {
        a = otImmed;
        av = MAKE_LOCVAL(l);
    } else if (l->op == opLoadI) {
        a = otAddr;
        av = l->val.iVal;
    } else {
        genExprs(l);
    }

    emitExt(inst, a, otNone, otStack, av);
    return true;
}

bool Compiler::genVariantUnary(VMsym inst, ExprNode* l) {
    assert(l);
    genExprs(l);
    addByte(vmVOp1);
    addByte(inst); // int version
    return true;
}

void Compiler::genAssign(ExprNode* e) {
    ExprNode *l = e->kids[0], *r = e->kids[1];
    assert(l && r);
    // @VERIFY: direct store in op

    if (e->type.vtype == vtStruct && e->type.indir == 0) {
        if (r->op == opCall || r->op == opCallVirt || r->op == opCallIface) {
            // generate the struct address
            genExprs(l);
            genExprs(r);
        } else {
            genExprs(l);
            genExprs(r);
            if (structs[e->type.structID].hasCopy) {
                int fID = findStructFunc(e->type.structID, "_copy");
                assert(fID != -1);
                genFuncCall(fID);
            } else {
                addByte(vmCopy);
                addWord(structs[e->type.structID].size);
            }
        }		
    } else {
        genExprs(l);
        genExprs(r);
        if (l->type.vtype == vtVariant && l->type.indir == 1)
            addByte(vmStoreV);
        else
            addByte(vmStore);
    }
}

void Compiler::genDiscard(ExprNode* e) {
    ExprNode* l = e->kids[0];
    assert(l);
    genExprs(l);
    addByte(vmPop);
}

void Compiler::genAlloc(ExprNode* e) {
    ExprNode* l = e->kids[0];
    addByte(vmAlloc);
    addWord(structs[l->type.structID].size);
    genExprs(l);
    addByte(vmPopN);
    addWord(structs[l->type.structID].size);
}

void Compiler::genFuncCall(int fID) {
    if (funcs[fID].builtin) {
        if (funcs[fID].lib == -1) {
            addByte(vmCallBI);
            addWord(funcs[fID].loc);
        } else {
            addByte(vmLibCall);
            addByte(funcs[fID].lib);
            addWord(funcs[fID].loc);
            //addByte(funcs[fID].nArgs);
        }
    } else {
        addByte(vmCall);
        addLong(fID);
    }
}

void Compiler::genCall(ExprNode* call) {
    ExprNode *l = call->kids[0], *r = call->kids[1];
    genExprs(r);
    if (l) {
        genExprs(l);
        addByte(vmCallI);
    } else {
        genFuncCall(call->val.iVal);
    }
}

bool Compiler::genExpr(ExprNode* e) {
    assert(e);
    ExprNode *l = e->kids[0], *r = e->kids[1];
    bool bPtrInc = false;
    switch (e->op) {
        case opAdd:
            if (e->type.vtype == vtStruct) {
                // this is s = s[i]
                e->type.vtype = vtInt;
            }
            assert(isSimple(e) || isPtr(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmAddI, l, r);
            else
                genBinary(MAKE_OP(vmAddI, simpleType(e->type)), l, r);
            break;
        case opSub:
            assert(isNumeric(e) || isPtr(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmSubI, l, r);
            else
                genBinary(MAKE_OP(vmSubI, simpleType(e->type)), l, r);
            break;
        case opMult:
            assert(isNumeric(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmMultI, l, r);
            else
                genBinary(MAKE_OP(vmMultI, simpleType(e->type)), l, r);
            break;
        case opDiv:
            assert(isNumeric(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmDivI, l, r);
            else
                genBinary(MAKE_OP(vmDivI, simpleType(e->type)), l, r);
            break;
        case opMod:
            assert(isNumeric(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmModI, l, r);
            else
                genBinary(MAKE_OP(vmModI, simpleType(e->type)), l, r);
            break;
        case opAssign:
            genAssign(e);
            break;
        case opSStore:
            genExprs(l);
            if (isVariant(e))
                addByte(vmStoreV);
            else
                addByte(vmStore);
            break;
        case opLoadI:
            if (e->val.iVal < 0xc000) {
                addByte(vmLoadI);
                addWord(e->val.iVal);
            } else {
                addByte(vmLoadFP);
                addWord(e->val.iVal - 0xc000);
            }
            break;
        case opLoadF0:
            addByte(vmLoadF0);
            addWord(e->val.iVal);
            break;
        case opLoadF1:
            addByte(vmLoadF1);
            addWord(e->val.iVal);
            break;

        case opStoreI:
            genExprs(l);
            assert(!isVariant(e));
            if (e->val.iVal < 0xc000) {
                addByte(vmStoreI);
                addWord(e->val.iVal);
            } else {
                addByte(vmStoreFP);
                addWord(e->val.iVal - 0xc000);
            }
            break;
        case opStoreF0:
            genExprs(l);
            assert(!isVariant(e));
            addByte(vmStoreF0);
            addWord(e->val.iVal);
            break;
        case opStoreF1:
            genExprs(l);
            assert(!isVariant(e));
            addByte(vmStoreF1);
            addWord(e->val.iVal);
            break;

        case opAddF0:
            // Add [0+FP],x
            emitExt(vmAddI, otAddr, otImmed, otStack, 0xc000, e->val.iVal, 0);
            break;
        case opAddF1:
            // Add [0+FP],x
            emitExt(vmAddI, otAddr, otImmed, otStack, 0xc001, e->val.iVal, 0);
            break;
            
        case opLoad:
            assert(l);
            genExprs(l);
            addByte(vmLoad);
            break;
        case opGetProp: {
            Variable& var = structs[e->val.iVal].vars[e->data];
            if (var.loc_get == -1)
                c_error("write-only property", -1); // @TODO: find line
            genExprs(l);
            if (var.lib == -1) {
                addByte(vmCallBI);
                addWord(var.loc_get);
            } else {
                addByte(vmLibCall);
                addByte(var.lib);
                addWord(var.loc_get);
            }
            break;
        }
        case opSetProp: {
            Variable& var = structs[e->val.iVal].vars[e->data];
            genExprs(l);
            genExprs(r);
            if (var.lib == -1) {
                addByte(vmCallBI);
                addWord(var.loc_set);
            } else {
                addByte(vmLibCall);
                addByte(var.lib);
                addWord(var.loc_set);
            }
            break;
        }
        case opGetAt:
            genExprs(l);
            genExprs(r);
            addByte(vmGetAt);
            break;
        case opSetAt:
            genExprs(l);
            genExprs(e->kids[2]);
            genExprs(r);
            addByte(vmSetAt);
            break;
        case opCopy:
            genExprs(l);
            genExprs(r);
            addByte(vmCopy);
            addWord(e->val.iVal);
            break;
        case opBOr:
            assert(isIntegral(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmBOrI, l, r);
            else
                genBinary(MAKE_OP(vmBOrI, simpleType(e->type)), l, r);
            break;
        case opBAnd:
            assert(isIntegral(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmBAndI, l, r);
            else
                genBinary(MAKE_OP(vmBAndI, simpleType(e->type)), l, r);
            break;
        case opXOr:
            assert(isIntegral(e) || isVariant(e));
            if (isVariant(e))
                genVariantBinary(vmXorI, l, r);
            else
                genBinary(MAKE_OP(vmXorI, simpleType(e->type)), l, r);
            break;
        case opBNot:
            assert(isIntegral(e) || isVariant(e));
            if (isVariant(e))
                genVariantUnary(vmBNotI, l);
            else
                genUnary(MAKE_OP(vmBNotI, simpleType(e->type)), l);
            break;
        case opSL:
            assert(isIntegral(e));
            genBinary(MAKE_OP(vmSLI, simpleType(e->type)), l, r);
            break;
        case opSR:
            assert(isIntegral(e));
            genBinary(MAKE_OP(vmSRI, simpleType(e->type)), l, r);
            break;
        case opOr: {
            assert(e->type.indir==0 && e->type.vtype==vtInt);
            //genBinary(vmOr, l, r);
            int after = getLabel();
            genExprs(l);
            addByte(vmJmpPopNZ);
            addWord(after);
            genExprs(r);
            setLabel(after);
            }
            break;
        case opAnd: {
            assert(e->type.indir==0 && e->type.vtype==vtInt);
            //genBinary(vmAnd, l, r);
            int after = getLabel();
            genExprs(l);
            addByte(vmJmpPopZ);
            addWord(after);
            genExprs(r);
            setLabel(after);
            }
            break;
        case opEq:
            assert(e->type.vtype == vtInt && e->type.indir == 0);
            assert(isSimple(l) || isPtr(l) || isFuncPtr(l) || isVariant(l));
            if (isVariant(l) || isVariant(r))
                genVariantBinary(vmEqI, l, r);
            else
                genBinary(MAKE_OP(vmEqI, simpleType(l->type)), l, r);
            break;
        case opNEq:
            assert(e->type.vtype == vtInt && e->type.indir == 0);
            assert(isSimple(l) || isPtr(l) || isFuncPtr(l) || isVariant(l));
            if (isVariant(l) || isVariant(r))
                genVariantBinary(vmNEqI, l, r);
            else
                genBinary(MAKE_OP(vmNEqI, simpleType(l->type)), l, r);
            break;
        case opLT:
            assert(e->type.vtype == vtInt && e->type.indir == 0);
            assert(isSimple(l) || isPtr(l) || isVariant(l));
            if (isVariant(l) || isVariant(r))
                genVariantBinary(vmLTI, l, r);
            else
                genBinary(MAKE_OP(vmLTI, simpleType(l->type)), l, r);
            break;
        case opLTE:
            assert(e->type.vtype == vtInt && e->type.indir == 0);
            assert(isSimple(l) || isPtr(l) || isVariant(l));
            if (isVariant(l) || isVariant(r))
                genVariantBinary(vmLTEI, l, r);
            else
                genBinary(MAKE_OP(vmLTEI, simpleType(l->type)), l, r);
            break;
        case opGT:
            assert(e->type.vtype == vtInt && e->type.indir == 0);
            assert(isSimple(l) || isPtr(l) || isVariant(l));
            if (isVariant(l) || isVariant(r))
                genVariantBinary(vmGTI, l, r);
            else
                genBinary(MAKE_OP(vmGTI, simpleType(l->type)), l, r);
            break;
        case opGTE:
            assert(e->type.vtype == vtInt && e->type.indir == 0);
            assert(isSimple(l) || isPtr(l) || isVariant(l));
            if (isVariant(l) || isVariant(r))
                genVariantBinary(vmGTEI, l, r);
            else
                genBinary(MAKE_OP(vmGTEI, simpleType(l->type)), l, r);
            break;
        case opNot:
            assert(e->type.vtype == vtInt && e->type.indir == 0);
            genUnary(vmNot, l);
            break;
        case opNeg:
            assert(isNumeric(e) || isVariant(e));
            if (isVariant(e))
                genVariantUnary(vmNegI, l);
            else
                genUnary(MAKE_OP(vmNegI, simpleType(e->type)), l);
            break;
        case opIncA:
            assert(isNumeric(e) || isPtr(e) || isVariant(e));
            assert(e->data);
            if (e->data > 1) {
                genExprs(l);
                addByte(vmIncAII);
                addWord(e->data);
            } else {
                if (isVariant(e))
                    genVariantUnary(vmIncAI, l);
                else
                    genUnary(MAKE_OP(vmIncAI, simpleType(e->type)), l);
            }
            break;
        case opIncB:
            assert(isNumeric(e) || isPtr(e) || isVariant(e));
            assert(e->data);
            if (e->data > 1) {
                genExprs(l);
                addByte(vmIncBII);
                addWord(e->data);
            } else {
                if (isVariant(e))
                    genVariantUnary(vmIncBI, l);
                else
                    genUnary(MAKE_OP(vmIncBI, simpleType(e->type)), l);
            }
            break;
        case opDecA:
            assert(isNumeric(e) || isPtr(e) || isVariant(e));
            assert(e->data);
            if (e->data > 1) {
                genExprs(l);
                addByte(vmDecAII);
                addWord(e->data);
            } else {
                if (isVariant(e))
                    genVariantUnary(vmDecAI, l);
                else
                    genUnary(MAKE_OP(vmDecAI, simpleType(e->type)), l);
            }
            break;
        case opDecB:
            assert(isNumeric(e) || isPtr(e) || isVariant(e));
            assert(e->data);
            if (e->data > 1) {
                genExprs(l);
                addByte(vmDecBII);
                addWord(e->data);
            } else {
                if (isVariant(e))
                    genVariantUnary(vmDecBI, l);
                else
                    genUnary(MAKE_OP(vmDecBI, simpleType(e->type)), l);
            }
            break;
        case opCall:
            genCall(e);
            break;
        case opCallHandler:
            genExprs(l);
            addByte(vmCallHandler);
            break;
        case opCallVirt:
            genExprs(r);
            addByte(vmCallVirt);
            addByte(e->data); // stack offset
            addByte(e->val.iVal); // function index
            break;
        case opCallIface:
            genExprs(r);
            addByte(vmCallIface);
            addByte(e->data & 0xffff); // stack offset
            addWord((e->data >> 16) & 0xffff); // interface id
            addByte(e->val.iVal); // function index
            break;
        case opSwap:
            if (l) genExprs(l);
            addByte(vmSwap);
            break;
        case opStackDup:
            if (l) genExprs(l);
            addByte(vmStackDup);
            break;
        case opNew:
        case opNewObj:
            assert(l);
            assert(r);
            genExprs(l);
            genExprs(r);
            if (e->op == opNew) {
                addByte(vmNew);
            } else {
                addByte(vmNewObj);
                addWord(e->data);
            }
            if (e->kids[2]) {
                addByte(vmStackDup);
                int fail_label = getLabel();
                addByte(vmJmpZ);
                addWord(fail_label);
                genExprs(e->kids[2]); // call _init(), the object is opStackDup
                setLabel(fail_label);
            }
            break;
        case opDelete: {
            assert(l);
            genExprs(l);
            addByte(vmStackDup);
            int fail_label = getLabel();
            addByte(vmJmpZ);
            addWord(fail_label);
            if (r) genExprs(r); // call _destroy(), the object is opStackDup
            addByte(vmDelete);
            setLabel(fail_label);
            addByte(vmPop);
            break;
        }
        case opAlloc:
            genAlloc(e);
            break;
        case opDiscard:
            genDiscard(e);
            break;
        case opFuncA:
            addByte(vmFuncA);
            addLong(e->val.iVal);
            break;
        case opCInt:
            addByte(vmCInt);
            addLong(e->val.iVal);
            break;
        case opSCInt:
            addByte(vmSCInt);
            addWord(e->data);
            addLong(e->val.iVal);
            break;
        case opCChar:
            addByte(vmCChar);
            addByte(e->val.cVal);
            break;
        case opSCChar:
            addByte(vmSCChar);
            addWord(e->data);
            addByte(e->val.cVal);
            break;
        case opCString:
            addByte(vmCString);
            addWord(e->val.iVal);
            break;
        case opSCString:
            addByte(vmSCString);
            addWord(e->data);
            addWord(e->val.iVal);
            break;
        case opCFloat:
            addByte(vmCFloat);
            addLong(*((int*)(&e->val.fVal)));
            break;
        case opSCFloat:
            addByte(vmSCFloat);
            addWord(e->data);
            addLong(*((int*)(&e->val.fVal)));
            break;
        case opCWord:
            addByte(vmCWord);
            addWord(e->val.iVal);
            break;
        case opSCWord:
            addByte(vmSCWord);
            addWord(e->data);
            addWord(e->val.iVal);
            break;
        case opCWordPFP:
            addByte(vmCWordPFP);
            addWord(e->val.iVal);
            break;
        case opSObjInfo:
            addByte(vmSObjInfo);
            addWord(e->data);
            addWord(e->val.iVal);
            break;
        case opStackInit:
            addByte(vmStackInit);
            addWord(e->val.iVal);
            break;
        case opMove:
            addByte(vmMove);
            addWord(e->val.iVal);
            addWord(e->data);
            break;
        case opItoC:
        case opItoF:
        case opItoS:
        case opFtoI:
        case opFtoC:
        case opFtoS:
        case opCtoI:
        case opCtoF:
        case opCtoS:
        case opStoI:
        case opStoF:
        case opStoC:
            assert(l);
            genExprs(l);
            addByte((VMsym)(vmItoC + (byte)(e->op) - (byte)opItoC));
            break;
        case opStoB:
            assert(l);
            genExprs(l);
            addByte(vmStoB);
            break;
        case opVtoI:
        case opVtoC:
        case opVtoF:
        case opVtoS:
        case opVtoB:
            assert(l);
            genExprs(l);
            addByte((VMsym)(vmVtoI + (byte)(e->op) - (byte)opVtoI));
            break;
        case opCompAddr:
            assert(l);
            genExprs(l);
            addByte(vmCompAddr);
            break;
        case opSetReg:
            genExprs(l);
            addByte(vmSetReg);
            addByte(e->data);
            break;
        case opGetReg:
            addByte(vmGetReg);
            addByte(e->data);
            break;
        case opNoOp:
            addByte(vmNoOp);
            break;
        case opNoOp2:
            addByte(vmNoOp2);
            addWord(e->val.iVal);
            break;

        default:
            c_error("unknown op\n", -1);
    }

    return true;
}

/*
enum VMsym {
    // constants
    vmCInt=0, vmCChar, vmCFloat, vmCString,
    // typecasts
    vmItoC, vmItoF, vmItoS, vmFtoI, vmFtoC, vmFtoS,
    vmCtoI, vmCtoF, vmCtoS, vmStoI, vmStoF, vmStoC,
    vmStoB,
    // variables
    vmArray,
    // operators
    vmAddI, vmAddC, vmAddF, vmAddS, vmSubI, vmSubC, vmSubF,
    vmMultI, vmMultC, vmMultF, vmDivI, vmDivC, vmDivF,
    vmModI, vmModC, vmModF, vmNegI, vmNegC, vmNegF,
    vmNot, vmAnd, vmOr,
    // relational operators
    vmEqI, vmEqC, vmEqF, vmEqS, vmNEqI, vmNEqC, vmNEqF, vmNEqS, 
    vmLTI, vmLTC, vmLTF, vmLTS, vmLTEI, vmLTEC, vmLTEF, vmLTES,
    vmGTI, vmGTC, vmGTF, vmGTS, vmGTEI, vmGTEC, vmGTEF, vmGTES,
    // flow control
    vmJmp, vmJmpZ, vmJmpNZ,
    vmCall, vmCallI, vmCallBI, vmRet, vmSetRet, vmSetRet0,
    vmPop, vmPopN, vmSwap, vmStackDup, vmAlloc, vmNew, vmDelete,
    vmLink, vmUnLink, vmHalt,

    // Version 3.0 stuff
    vmCWord, vmCWordPFP,
    vmLoad, vmStore, vmCopy, vmSet, vmGet,
    vmLoadI, vmLoadFP, vmStoreI, vmStoreFP,
    vmIncAI, vmIncAC, vmIncAF, vmIncAS, vmDecAI, vmDecAC, vmDecAF, vmDecAS, 
    vmIncBI, vmIncBC, vmIncBF, vmIncBS, vmDecBI, vmDecBC, vmDecBF, vmDecBS,
    // bitwise
    vmBAndI, vmBAndC, vmBOrI, vmBOrC, vmBNotI, vmBNotC, vmSLI, vmSLC, vmSRI, vmSRC, vmXorI, vmXorC,
    vmNoOp, vmNoOp2,
    // vmCallF,
    vmFuncA,

    vmNumInstr
};
*/

#include "inst.h"
