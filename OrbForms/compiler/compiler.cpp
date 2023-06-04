#include "compiler.h"

#ifdef WIN32
#include <iostream>

HINSTANCE g_hDll = NULL;

BOOL WINAPI DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hDll = hDll;
        return TRUE;
    }
    return FALSE;
}
#endif

Compiler::Compiler(ICompileStatus* pStatus /*=NULL*/) : tok(lex.tok)  {
    globOffset = 0;
    curStruct = -1;
    curLibrary = -1;
    curLibraryFunc = -1;
    curSwitch = NULL;
    bProjectRead = false;
    iGlobDestLoc = -1;
    iGlobInitLoc = -1;
    lex.init(this);
    bDebug = false;
    bInSysFile = false;
    bPocketCCompat = true;
    this->pStatus = pStatus;
    codeSize = 0;
    for (int i=0;i<MAX_CODE_SEGMENTS;i++)
        codeSegments[i] = NULL;
}

Compiler::~Compiler() {
    for (int i=0;i<MAX_CODE_SEGMENTS;i++)
        delete codeSegments[i];
}

#ifdef WIN32
void SliceFile(char* pStr) {
    assert(pStr);
    int i = strlen(pStr);
    while (i && pStr[i] != '\\')
        i--;
    pStr[i+1] = '\0';
}

static byte* getRes(const char* type, const char* name, int* pSize) {
    byte* res;
    HGLOBAL hGlob;
    HRSRC hRes = FindResource(g_hDll, name, type);
    assert(hRes);
    hGlob = LoadResource(g_hDll, hRes);
    assert(hGlob);
    res = (byte*)LockResource(hGlob);
    assert(res);
    *pSize = SizeofResource(g_hDll, hRes);
    return res;
}
#endif

#ifndef WIN32
#define MAKEINTRESOURCE(x) x
#endif

static void addFromResource(PalmDB& db, const char* type, int id, int winResID) {
    PalmResRecord rec;
    rec.type = type;
    rec.id = id;
#ifdef WIN32
    rec.data = getRes("RAW", MAKEINTRESOURCE(winResID), &rec.len);
#else
    MemHandle hMem = DmGetResource('wres', winResID);
    rec.data = (byte*)MemHandleLock(hMem);
    rec.len = MemHandleSize(hMem);
#endif
    db.AddResRec(rec);
#ifndef WIN32
    MemHandleUnlock(hMem);
#endif
}

#ifdef WIN32
void Compiler::DumpFuncs() {
    for (int i=0;i<funcs.size();i++) {
        puts(funcs[i].name.c_str());
    }
}

string Compiler::GenDocType(Type type) {
    string ret;
    switch (type.vtype) {
        case vtInt:
            ret = "int";
            break;
        case vtFloat:
            ret = "float";
            break;
        case vtString:
            ret = "string";
            break;
        case vtChar:
            ret = "char";
            break;
        case vtVoid:
            ret = "void";
            break;
        case vtStruct:
            ret = structs[type.structID].name.c_str();
            break;
    }
    for (int i=0;i<type.indir;i++)
        ret += "*";

    return ret;

}

void Compiler::GenDocFunc(Function& func) {
    bool isMethod = func.structID != -1;
    if (isMethod) {
        if (func.methodName == "_init" || func.methodName == "_destroy" || func.methodName == "_copy")
            return;
    }
    char* tabs = isMethod ? "\t\t\t" : "\t\t\t\t";
    int i;
    printf("%s<method name=\"%s\">\n", tabs, isMethod ? func.methodName.c_str() : func.name.c_str());
    // params
    i = 0;
    if (isMethod) i++;
    for (;i<func.nArgs;i++) {
        printf("%s\t<param type=\"%s\" name=\"%s\">TODO: param</param>\n", tabs, GenDocType(func.locals[i].type).c_str(), func.locals[i].name.c_str());
    }
    // return
    if (func.type.vtype == vtVoid && func.type.indir == 0) {
        printf("%s\t<return type=\"void\"/>\n", tabs);
    } else {
        printf("%s\t<return type=\"%s\">TODO: return</return>\n", tabs, GenDocType(func.type).c_str());
    }
    printf("%s\t<desc>TODO: description of %s</desc>\n", tabs, func.name.c_str());
    printf("%s</method>\n", tabs);
}

void Compiler::GenDocObject(Struct& st) {
    int i;
    printf("\t\t\t<object name=\"%s\">\n", st.name.c_str());
    printf("\t\t\t\t<desc>TODO: description of %s</desc>\n", st.name.c_str());
    printf("\t\t\t\t<sdesc>TODO: short description of %s</sdesc>\n", st.name.c_str());
    // properties
    for (i=0;i<st.vars.size();i++) {
        if (st.vars[i].stype == syProperty) {
            printf("\t\t\t\t<property type=\"%s\" name=\"%s\">\n", GenDocType(st.vars[i].type).c_str(), st.vars[i].name.c_str());
            printf("\t\t\t\t\t<desc>TODO: property %s</desc>\n", st.vars[i].name.c_str());
            printf("\t\t\t\t</property>\n");
        }
    }
    // methods
    for (i=0;i<st.funcs.size();i++) {
        GenDocFunc(funcs[st.funcs[i].funcID]);
    }
    printf("\t\t\t</object>\n");
    /*
            <property type="int" name="x">
                <desc>The last x event</desc>
            </property>
    */
}

void Compiler::GenDocXML(char* sysfile) {
    int i;
    try {
        // load in the system file
        Parse(sysfile);
        printf("<API>\n");
        // dump global funcs
        for (i=0;i<funcs.size();i++) {
            if (funcs[i].structID == -1) {
                GenDocFunc(funcs[i]);
            }
        }
        // dump objects
        for (i=0;i<structs.size();i++) {
            GenDocObject(structs[i]);
        }
        printf("</API>\n");
    }
    catch (...) {
    }
}
#endif

void Compiler::CreateDemoCode() {
    ExprNode* expr = new ExprNode(opCall);
    expr->val.type = vtInt;
    expr->val.iVal = findFunc("_plat1");
    globInitTree.AddExprStmt(expr);

    expr = new ExprNode(opCall);
    expr->val.type = vtInt;
    expr->val.iVal = findFunc("_plat2");
    globInitTree.AddExprStmt(expr);

    expr = new ExprNode(opCall);
    expr->val.type = vtInt;
    expr->val.iVal = findFunc("_plat3");
    globInitTree.AddExprStmt(expr);
}

#ifndef WIN32
string g_cid;
#endif

bool Compiler::Compile(char* srcfile, char* sysfile, char* destfile, bool bStandalone,
    bool bPocketC, char* dbgsyms, char* language, ErrorInfo& info)
{
#ifndef WIN32
    bStandalone = false;
#endif
    bool res = true;

    //printf("vmNumInstr = %d\n", (int)vmNumInstr);
    assert(vmNumInstr <= 0xC0);

    errorInfo.desc = "Internal compiler error";
    errorInfo.file = "";
    errorInfo.line = 0;
    errorInfo.ch = 0;

    if (dbgsyms) {
        bDebug = true;
    }

    bPocketCCompat = bPocketC;
    lex.bPocketCCompat = bPocketC;

    try {
#ifndef WIN32
        extern DmOpenRef PcDB;
        // allocate a reasonable amount of memory to cover the API + a small app
        strings.init(PcDB, 1024);
        funcs.reserve(350);
        structs.reserve(80);
        consts.reserve(120);
#endif
        addString(""); // this first string must always be the empty string

        // load in the system file
        bInSysFile = true;
        if (pStatus)
            pStatus->setStatus("Parsing system APIs", false);
        Parse(sysfile);
        bInSysFile = false;
#ifdef DEMO
        CreateDemoCode();
#endif
#ifdef WIN32
        if (language && *language) {
            string file = lex.FindFile(language, false, true);
            loadTranslation(file.c_str());
        }
#endif
        if (bDebug) {
            lex.addMacro("DEBUG", "");
        }
        lex.addMacro("__ORBC__", "");
        if (bPocketCCompat)
            lex.addMacro("__POCKETC__", "");

#ifndef WIN32
        lex.addMacro("__ONBOARD__", "");
#endif

        // parse the user file
        Parse(srcfile);
        if (pStatus)
            pStatus->setStatus("Hooking up handlers", false);
        HookUpHandlers();
        if (pStatus)
            pStatus->setStatus("Generating final code", false);
        MergeFuncs();

#ifdef WIN32
        // write out translation file
        writeTranslation();

        // build the code segment
        if (strstr(destfile, ".vm")) {
            FILE* f = fopen(destfile, "wb");
            if (!f) {
                c_error("unable to open output file", -1);
            }
            fwrite(&iGlobInitLoc, sizeof(int), 1, f);
            fwrite(&iGlobDestLoc, sizeof(int), 1, f);
            fwrite(&codeSize, sizeof(codeSize), 1, f);
            for (int i=0;i<MAX_CODE_SEGMENTS;i++) {
                if (!codeSegments[i])
                    break;
                dbvector<byte>& code = *codeSegments[i];
                fwrite(&code[0], sizeof(byte), code.size(), f);
                if (i+1 < MAX_CODE_SEGMENTS && codeSegments[i+1]) {
                    // pad past end of segment
                    byte zero = vmNoOp;
                    for (int j=0;j<CODE_SEGMENT_SIZE-code.size();j++)
                        fwrite(&zero, sizeof(byte), 1, f);
                }
            }

            int size = globTypes.size();
            fwrite(&size, sizeof(size), 1, f);
            if (size)
                fwrite(&globTypes[0], sizeof(VarType), size, f);

            size = globInits.size();
            fwrite(&size, sizeof(size), 1, f);
            if (size)
                fwrite(&globInits[0], sizeof(GlobalInit), size, f);

            size = strings.size();
            fwrite(&size, sizeof(size), 1, f);
            if (size)
                fwrite(&strings[0], sizeof(char), size, f);

            size = objinfo.size();
            fwrite(&size, sizeof(size), 1, f);
            if (size)
                fwrite(&objinfo[0], sizeof(byte), size, f);

            fclose(f);
        } else if (strstr(destfile, ".prc")) {
#endif
            if (pStatus)
                pStatus->setStatus("Writing application", false);

            PalmDB db;
            string dbname = destfile;
            if (!app.dbname.empty())
                dbname = app.dbname;
            else if (!app.dname.empty())
                dbname = app.dname;
            db.SetRes(true);
            if (app.locked)
                db.SetLocked(true);
            if (app.hidden)
                db.SetHidden(true);
            if (app.creatorID.empty()) app.creatorID = "oBVT";
            if (!app.resOnly)
                db.SetCreatorType(app.creatorID.c_str(), "appl");
            else
                db.SetCreatorType(app.creatorID.c_str(), "data");
            db.SetName(dbname.c_str());
            
#ifndef WIN32
            g_cid = app.creatorID;
#endif
            
            PalmResRecord rec;

#ifdef DEMO
            if (false) {
#else
#ifdef WIN32
            if (bStandalone) {
#else
            if (false) {
#endif
#endif
                // code 0, 1, 2
                addFromResource(db, "code", 0, 2000);
                addFromResource(db, "code", 1, 2001);
                addFromResource(db, "code", 2, 2002);

                // data 0
                addFromResource(db, "data", 0, 2003);

                // alerts
                addFromResource(db, "Talt", 9000, 2004);
                addFromResource(db, "Talt", 9001, 2005);
                addFromResource(db, "Talt", 9002, 2006);
                addFromResource(db, "Talt", 9003, 2007);
                addFromResource(db, "Talt", 9004, 2008);
                addFromResource(db, "Talt", 9005, 2009);
                
                // launcher prefs, to increase stack size
                addFromResource(db, "pref", 0, 2010);

                if (bPocketCCompat) {
                    // output form
                    addFromResource(db, "MBAR", 9000, 2011);
                    // graphics form
                    addFromResource(db, "tFRM", 9000, 2012);
                    addFromResource(db, "tFRM", 9100, 2012);
                }
            } else {
                if (!app.resOnly) {
                    // code 0
                    addFromResource(db, "code", 0, 1000);

                    // code 1
                    rec.type = "code";
                    rec.id = 1;
#ifdef WIN32
                    byte* codeRes = getRes("RAW", MAKEINTRESOURCE(1001), &rec.len);
                    rec.data = new byte[rec.len]; // need to modify it
                    memcpy(rec.data, codeRes, rec.len);
#else
                    MemHandle hMem = DmGetResource('wres', 1001);
                    rec.len = MemHandleSize(hMem);
                    rec.data = new byte[rec.len];
                    memcpy(rec.data, (byte*)MemHandleLock(hMem), rec.len);
                    MemHandleUnlock(hMem);
#endif
                    for (int ai=0;ai<rec.len; ai++) {
                        if (strcmp((char*)&rec.data[ai], "AppletName") == 0) {
                            strncpy((char*)&rec.data[ai], dbname.c_str(), 32);
                            break;
                        }
                    }
                    db.AddResRec(rec);
                    delete rec.data;

                    // data 0
                    addFromResource(db, "data", 0, 1002);

                    // alert 1001 - NoRuntime
                    addFromResource(db, "Talt", 1001, 1003);
                }

#ifndef WIN32
                if (app.icon.res.empty() && app.icon.sres.empty()) {
                    addFromResource(db, "tAIB", 1000, 1004);
                    addFromResource(db, "tAIB", 1001, 1005);
                }
#endif
            }

            if (!app.resOnly) {
                // compiler header
                {
                    PalmDBStream dbstr(15);
                    dbstr.addWord(0x0410); // version
                    if (app.forms.size() == 0)
                        dbstr.addByte(0); // no UI
                    else
                        dbstr.addByte(1); // has UI
                    dbstr.addLong(iGlobInitLoc);
                    dbstr.addLong(iGlobDestLoc);
                    dbstr.addLong(codeSize);
                    rec.type = "ofHD";
                    rec.id = 0;
                    rec.len = dbstr.size;
                    rec.data = dbstr.data;
                    if (!db.AddResRec(rec))
                        c_error("unable to write header", -1);
                }

                // native add-in header
                {
                    int nLibs = 0;
                    int addInId = 3000;
                    for (int i=0;i<libraries.size();i++) {
                        if (libraries[i].marked) nLibs++;
                    }

                    if (nLibs) {
                        PalmDBStream dbstr(1 + libraries.size() * 4);
                        dbstr.addByte(libraries.size());
                        for (int i=0;i<libraries.size();i++) {
                            if (libraries[i].marked) {
                                dbstr.addWord(addString(libraries[i].name.c_str()));
                                if (libraries[i].bPocketC) {
                                    dbstr.addWord(0xffff);
                                } else {
                                    if (bStandalone) {
#ifdef WIN32
                                        dbstr.addWord(addInId);
                                        addLibraryContents(libraries[i].name, db, addInId);
                                        addInId++;
#endif
                                    } else {
                                        dbstr.addWord(0);
                                    }
                                }
                            } else {
                                // this library is not used, so put in an empty entry
                                dbstr.addWord(0);
                                dbstr.addWord(0);
                            }
                        }
                        rec.type = "ofNA";
                        rec.id = 0;
                        rec.len = dbstr.size;
                        rec.data = dbstr.data;
                        if (!db.AddResRec(rec))
                            c_error("unable to write native add-in header", -1);
                    }
                }

                for (int i=0;i<MAX_CODE_SEGMENTS;i++) {
                    if (!codeSegments[i] || !codeSegments[i]->size())
                        break;

                    rec.type = "ofCD";
                    rec.id = i;
                    dbvector<byte>& code = *codeSegments[i];
                    rec.len = code.size();
                    rec.data = (byte*)&code[0];
                    if (!db.AddResRec(rec))
                        c_error("unable to write code segment", -1);
                }
                if (globTypes.size()) {
                    assert(sizeof(VarType)==1);
                    rec.type = "ofGT";
                    rec.id = 0;
                    rec.len = globTypes.size() * sizeof(VarType);
                    if (rec.len > 0x10000)
                        c_error("too much global data", -1);
                    rec.data = (byte*)&globTypes[0];
                    if (!db.AddResRec(rec))
                        c_error("unable to write global type segment", -1);
                }
                
                if (globInits.size()) {
                    PalmDBStream dbstr(globInits.size() * 10);
                    for (int i=0;i<globInits.size();i++) {
                        dbstr.addLong(globInits[i].offset);
                        dbstr.addByte(globInits[i].val.type);
                        dbstr.addByte(globInits[i].val.stype);
                        dbstr.addLong(globInits[i].val.iVal);
                    }
                    rec.type = "ofGI";
                    rec.id = 0;
                    rec.len = dbstr.size;
                    if (rec.len > 0x10000)
                        c_error("too much global initialization data", -1);
                    rec.data = dbstr.data;
                    if (!db.AddResRec(rec))
                        c_error("unable to write global initializer segment", -1);
                }

                rec.type = "ofST";
                rec.id = 0;
                rec.len = strings.size();
                if (rec.len > 0x10000) c_error("too much string data", -1);
                rec.data = (byte*)&strings[0];
                if (!db.AddResRec(rec))
                    c_error("unable to write string segment", -1);

                if (objinfo.size() > 0) {
                    rec.type = "ofOI";
                    rec.id = 0;
                    rec.len = objinfo.size();
                    if (rec.len > 0x10000)
                        c_error("too many object types", -1);
                    rec.data = (byte*)&objinfo[0];
                    if (!db.AddResRec(rec))
                        c_error("unable to write object info segment", -1);
                }
            }

            int osver = 0x300;
            genApp(db, osver);

#ifdef WIN32
            char szosver[16];
            sprintf(szosver, "%x.%x", (osver & 0xff00) >> 8, (osver & 0xff));
            cout << "This application requires OS " << szosver << " or higher, based on resources used." << endl;
#endif

            if (!db.Write(destfile)) {
                c_error("unable to write output file", -1);
            }
#ifdef WIN32
        for (int i=0;i<post_build.size();i++) {
            system(post_build[i].c_str());
        }
#endif

#ifdef WIN32
        }
#endif
    }
    catch (CompError&) {
        res = false;
        info = errorInfo;
    }
    catch (...) {
        res = false;
        info = errorInfo;
    }
    return res;
}

#ifndef WIN32
#include <PalmOS.h>
short FindMemo(char* name);
short FindDoc(char* name);
#endif

Source* Compiler::GetFile(const char* file, bool bThrowError /*=true*/, int* plen /*=NULL*/) {
#ifdef WIN32
    FILE* f = fopen(file, "rb");
    if (!f) {
        if (bThrowError) {
            char msg[512];
            sprintf(msg, "unable to open file '%s'", file);
            c_error(msg);
        }
        return NULL;
    }

    int len = _filelength(f->_file);
    char* data = new char[len+1];
    if (data) {
        fread(data, 1, len, f);
        data[len] = 0;
    }
    fclose(f);
    if (plen)
        *plen = len;
    return new BufferSource(data, true);
#else
    char* fileTable[] = {
        "orbc.sys",
        "pc_all.oc",
        "pc_most.oc",
        "pc_database.oc",
        "pc_date.oc",
        "pc_events.oc",
        "pc_format.oc",
        "pc_graphics.oc",
        "pc_io.oc",
        "pc_math.oc",
        "pc_memo.oc",
        "pc_memory.oc",
        "pc_network.oc",
        "pc_serial.oc",
        "pc_sound.oc",
        "pc_string.oc",
        "pc_system.oc",
        "pc_vfs.oc"
    };
    int cFileTable = sizeof(fileTable) / sizeof(fileTable[0]);
    for (int i = 0; i < cFileTable; i++) {
        if (string(fileTable[i]) == file)
            return new HandleSource(DmGetResource('wres', i));
    }

    short id = FindMemo((char*)file);
    if (id != -1) {
        return new MemoSource(id);
    }
    id = FindDoc((char*)file);
    if (id != -1) {
        return new DocSource(id);
    }
    if (bThrowError) {
        char msg[512];
        sprintf(msg, "unable to open file '%s'", file);
        c_error(msg);
    }
    return NULL;
#endif
}

void Compiler::c_error(const char* err, int line) {
    int ch = 0;
    if (!bProjectRead) {
        if (line == -2) {
            line = tok.line;
            ch = tok.ch;
        }

        if (line != -1) {
            int topFileLevel = lex.incLevel;
            while (lex.incStack[topFileLevel].osource->GetType() == srcMacro)
                topFileLevel--;
            assert(topFileLevel >= 0);

            errorInfo.file = lex.incStack[topFileLevel].name;
            errorInfo.sourceType = lex.incStack[topFileLevel].osource->GetType();
            if (lex.incLevel != topFileLevel) {
                errorInfo.line = lex.incStack[topFileLevel].line;
                errorInfo.offset = lex.incStack[topFileLevel].src - lex.incStack[topFileLevel].isrc;
                errorInfo.ch = errorInfo.offset - lex.incStack[topFileLevel].startOfLine;
            } else {
                errorInfo.line = line;
                errorInfo.ch = ch;
                errorInfo.offset = lex.source - lex.incStack[lex.incLevel].isrc;
            }
        } else {
            errorInfo.line = 0;
            errorInfo.ch = 0;
            errorInfo.offset = 0;
            errorInfo.sourceType = srcNone;
        }
        errorInfo.desc = err;
    }
    throw CompError();
}

void Compiler::c_error(const char* err, const char* filename, int line) {
    int ch = 0;
    assert(!bProjectRead);
    errorInfo.desc = err;
    errorInfo.file = filename;
    errorInfo.line = line;
    errorInfo.ch = ch;
    throw CompError();
}

#ifdef WIN32
// DLL interface
OrbCompiler::OrbCompiler() { }
OrbCompiler::~OrbCompiler() { };

bool OrbCompiler::Compile(char* file, char* sysfile, char* dest,
    bool bStandalone, bool bPocketCCompat, char* dbgsyms, char* language, ErrorInfo& error)
{
    bool ret;
    size_t cch = 0;
    getenv_s(&cch, NULL, 0, "POCKETC_COMPAT");
    if (cch != 0)
        bPocketCCompat = true;
    Compiler* comp = new Compiler();
    ret = comp->Compile(file, sysfile, dest, bStandalone, bPocketCCompat, dbgsyms, language, error);
    delete comp;
    return ret;
}

bool OrbCompiler::QuickParse(char* buff, bool bPocketCCompat, QuickParseInfo& info) {
    bool ret;
    size_t cch = 0;
    getenv_s(&cch, NULL, 0, "POCKETC_COMPAT");
    if (cch != 0)
        bPocketCCompat = true;
    Compiler* comp = new Compiler();
    ret = comp->QuickParse(buff, bPocketCCompat, info);
    delete comp;
    return ret;
}

bool OrbCompiler::GenDocXML(char* file) {
    Compiler* comp = new Compiler();
    comp->GenDocXML(file);
    delete comp;
    return true;
}

bool OrbCompiler::ReadProject(char* file, PalmProject& proj, ErrorInfo& error) {
    bool ret;
    Compiler* comp = new Compiler();
    ret = comp->ReadProject(file, proj, error);
    delete comp;
    return ret;
}

bool OrbCompiler::WriteProject(char* file, PalmProject& proj) {
    bool ret;
    Compiler* comp = new Compiler();
    ret = comp->WriteProject(file, proj);
    delete comp;
    return ret;
}

string OrbCompiler::FindFile(const char* file, const char* relativeFile) {
    Lex lex;
    char fullPath[256]={0};
    strncpy(fullPath, relativeFile, 255);
    SliceFile(fullPath);
    lex.strSourceDir = fullPath;
    lex.bPocketCCompat = true;
    return lex.FindFile(file, true, true);
}
#endif
