#include "..\compiler\compiler.h"

char* funcNames[] = {
    "NYI",
    "__vector_call",
    "__vector_bicall",
    "__vector_libcall",
    "get Double.double",
    "set Double.double",
    "puts",
    "alert",
    "ticks",
    "get UILabel.text",
    "set UILabel.text",
    "get control.text",
    "set control.text",
    "get control.x",
    "get control.y",
    "get control.w",
    "get control.h",
    "get UIForm.x",
    "get UIForm.y",
    "get UIForm.w",
    "get UIForm.h",
    "get UIForm.title",
    "set UIForm.title",
    "get control.value",
    "set control.value",
    "set UIFBitmap.bmpid",
    "set control.visible",
    "get UIField.text",
    "set UIField.text",
    "get UIField.inspt",
    "set UIField.inspt",
    "get UIField.seltext",
    "set UIField.seltext",
    "get UIField.fontid",
    "set UIField.fontid",
    "get UIField.scrollpos",
    "set UIField.scrollpos",
    "get UIField.dirty",
    "set UIField.dirty",
    "UIField.insert",
    "UIField.select",
    "UIField.cut",
    "UIField.copy",
    "UIField.paste",
    "UIField.undo",
    "UIField.del",
    "get Event.x",
    "get Event.y",
    /*
    "UIDraw.pixel",
    "UIDraw.frame",
    "UIDraw.rect",
    "UIDraw.line",
    */
    "Draw.bitmap",
    "Draw.text",
    "Draw.draw",
    "Draw.indexFromColor",

    "get Event.inside",
    "get UISlider.value",
    "get UISlider.min",
    "get UISlider.max",
    "get UISlider.page",
    "set UISlider.value",
    "set UISlider.min",
    "set UISlider.max",
    "set UISlider.page",
    "get Event.value",
    "get Event.prev",
    "get Event.key",
    "get UIList.selitem",
    "set UIList.selitem",
    "UIList.gettext",
    "set UIList.topitem",
    "get UIList.count",
    "UIList.makevisible",
    "UIList.scroll",
    "UIList.popup",
    "UIList.setitems",
    "UIForm.select",
    "UIList.redraw",
    "UIField.grabfocus",
    "UIField.relfocus",
    "UIForm.timer",
    "get Dict.count",
    "Dict.init",
    "nobject.destroy",
    "Dict.clear",
    "Dict.add",
    "Dict.del",
    "Dict.find",
    "Dict.has",
    "UIForm.load",
    "UIForm.domodal",
    "UIForm.close",
    "nobject.copy",
    "Draw.init",
    "Draw.begin",
    "Draw.end",
    "Draw.line",
    "Draw.attachGadget",
    "Draw.attachForm",
    "Draw.pixel",
    "Draw.rect",
    "Draw.frame",
    "strlen",
    "substr",
    "strleft",
    "strright",
    "strupper",
    "strlower",
    "strstr",
    "hex",
    "format",
    "strctos",
    "strstoc",
    "cos",
    "sin",
    "tan",
    "acos",
    "asin",
    "atan",
    "cosh",
    "sinh",
    "tanh",
    "acosh",
    "asinh",
    "atanh",
    "exp",
    "log",
    "log10",
    "sqrt",
    "atan2",
    "pow",
    "random",
    "rand",
    "Memo._init",
    "get Memo.text",
    "set Memo.text",
    "get Memo.nrecs",
    "Memo.find",
    "Memo.create",
    "Memo.open",
    "Memo.puts",
    "Memo.line",
    "Memo.end",
    "Memo.rewind",
    "Memo.close",
    "Memo.remove",
    "Memo.erase",
    "Draw.fg",
    "Draw.bg",
    "Draw.textColor",
    "Draw.font",
    "Draw.underline",
    "Draw.textCenter",
    "Draw.read",
    "Draw.write",
    "Draw.create",
    "Draw.copyForm",
    "Draw.copyGadget",
    "Draw.release",
    "UIApp.getdepth",
    "UIApp.setdepth",
    "Database._init",
    "Database.open",
    "Database.opentc",
    "Database.create",
    "Database.close",
    "Database.delete",
    "Database.setcatname",
    "Database.getcatname",
    "Database.moverec",
    "Database.removerec",
    "Database.delrec",
    "Database.archiverec",
    "Database.getrec",
    "Database.newrec",
    "get Database.nrecs",
    "get Database.backup",
    "set Database.backup",
    "get Database.size",
    "get Database.type",
    "get Database.cid",
    "get Database.name",
    "DBRecord.read",
    "DBRecord.write",
    "DBRecord.erase",
    "DBRecord.close",
    "get DBRecord.offset",
    "set DBRecord.offset",
    "get DBRecord.id",
    "get DBRecord.cat",
    "set DBRecord.cat",
    "get DBRecord.size",
    "DBRecord._init",
    "tone",
    "tonea",
    "tonema",
    "beep",
    "resetaot",
    "launch",
    "username",
    "serialnum",
    "osver",
    "osvers",
    "orbver",
    "getclip",
    "setclip",
    "memcpy",
    "malloct",
    "DatabaseMgr.enum",
    "StrinList._init",
    "StrinList.count",
    "StrinList.clear",
    "StrinList.insert",
    "StrinList.del",
    "StrinList.find",
    "StrinList.tokens",
    "StrinList.item",
    "Prefs.set",
    "Prefs.get",
    "Prefs.del",
    "UIList.setitemslist",
    "UIApp.hookhard",
    "free",
    "get Date.year",
    "get Date.month",
    "get Date.day",
    "get Date.hour",
    "get Date.minute",
    "get Date.second",
    "get Date.ymd",
    "set Date.year",
    "set Date.month",
    "set Date.day",
    "set Date.hour",
    "set Date.minute",
    "set Date.second",
    "set Date.ymd",
    "Date.now",
    "Date.selectdate",
    "Date.time",
    "Date.date",
    "Date.ldate",
    "Draw.textWidth",
    "Draw.textHeight",
    "is_emulator",
    "display_no_run",
    "throw_no_run",
    "UIField.scroll",
    "UIScroll.update",
    "get UIScroll.value",
    "get UIScroll.min",
    "get UIScroll.max",
    "get UIScroll.page",
    "set UIScroll.value",
    "set UIScroll.min",
    "set UIScroll.max",
    "set UIScroll.page",
    "set UIForm.menu",
    "Database.getres",
    "Database.newres",
    "Database.removeres",
    "get Database.res",
    "Database.getappinfo",
    "Database.setappinfo",
    "Dict.key",
    "Dict.value",
    "UIForm.dodialog",
    "confirm",
    "UIApp.abort",
    "sleep",
    "prompt",
    "Date.selecttime",
    "Draw.selectIndex",
    "UIForm.help",
    "set control.x",
    "set control.y",
    "set control.w",
    "set control.h",
    "get Event.code",
    "UIForm.redraw",
    "Date.diffdays",
    "Database.createappinfo",
    "Database.hasappinfo",
    "get Database.dbref",
    "get DBRecord.handle",
    "lformat",
    "lparse",
    "Database.getresinfo",
    "Memo.delete",
    "Memo.archive",
    "get Date.weekDay",
    "get Draw.handle",
    "UIForm.activefield",
    "UIField.phonelookup",
    "UIForm.keyboard",
    "keystate",
    "penstate",
    "set control.bmp",
    "set control.sbmp",
    "get UIString.text",
    "get Event.nx",
    "get Event.ny",
    "UIApp.getscreenattrib",
    "get Draw.nw",
    "get Draw.nh",
    "Draw.nbegin",
    "Draw.fgRGB",
    "Draw.bgRGB",
    "Draw.textRGB",
    "set UIField.editable",
    "UIApp.getSysPref",
    "Draw.uiColorIndex",
    "set Database.name",
    "set UIField.numeric",
    "Draw.textTrunc",
    "get Database.version", // 312
    "set database.version",
    "customAlert",
    "customPrompt",
    "events",
    "UIList.setdrawfunc",
    "lib_vector_virtcall",
    "lib_expected_count",
    "get Database.nobeam", // 320
    "set Database.nobeam",
    "get DBRecord.uniqueid",
    "Database.findrec",
    "tickspersec",
    "_assert", // 325
    "_debuglog",
    "debugstack",
    "fatal",
    "StringList.sort", // 329
    "get UIForm.obscured", // 330
    "Database.getdate", // 331
    "get Database.inrom",
    "DBRecord.fromfile",
    "DBRecord.resfromfile",
    // PocketC support APIs
    "get Memo.index", // 335
    "customPromptDefault",
    "clear",
    "getsm",
    "graph_on", // 339
    "graph_off", // 340
    "saveg",
    "restoreg",
    "title",
    "pushdraw",
    "popdraw", // 345
    "wait",
    "waitp",
    "getc",
    "pstate",
    "pcevent", // 350
    "hookmenu",
    "hooksilk",
    "hooksync",
    "key",
    "penx", // 355
    "peny",
    "npenx",
    "npeny",
    "pctext",
    "clearg", // 360
    "bitmap",
    "atexit",
    "drawnative",
    "bucreate",
    "budelete", // 365
    "buset",
    "bucopy",
    "bucopyrect",
    "bitmaprm",
    "enableresize", // 370
    // v4
    "strinsert",
    "strreplace",
    "freemem",
    "battery",
    "srand", // 375
    "Draw.bitmapm",
    "Draw.drawm",
};

#define DEBUG
#include "..\compiler\inst.h"

char* GetFile(char* file) {
    FILE* f = fopen(file, "rb");
    if (!f) {
        char msg[512];
        printf(msg, "Unable to open file '%s'", file);
        return NULL;
    }

    int len = filelength(f->_file);
    char* data = (char*)malloc(len+1);
    fread(data, 1, len, f);
    data[len] = 0;
    fclose(f);
    return data;
}

void usage() {
    printf("bc.exe [-gcsltom] file.[vm|prc] - dump bytecode\n");
}

int csize, ssize, gisize, gtsize, iGlobInit, iGlobDest, oisize;
byte* code = NULL;
byte* objinfo = NULL;
char* str = NULL;
GlobalInit* globs = NULL;
VarType* gtypes;
bool bNoAddr = false;
bool bHasUI, bPRC;
int version;
string name;

struct BCLibrary {
    string name;
    int id;
};
vector<BCLibrary> libs;
char* getStr(int off);

bool readFile(char* fileName) {
    if (strstr(fileName, ".vm")) {
        FILE* f = fopen(fileName, "rb");
        if (!f) {
            printf("Unable to open '%s'\n", fileName);
            return false;
        }

        fread(&iGlobInit, sizeof(int), 1, f);
        fread(&iGlobDest, sizeof(int), 1, f);

        fread(&csize, sizeof(int), 1, f);
        code = new byte[csize];
        fread(code, sizeof(byte), csize, f);

        fread(&gtsize, sizeof(int), 1, f);
        gtypes = new VarType[gtsize];
        fread(gtypes, sizeof(VarType), gtsize, f);

        fread(&gisize, sizeof(int), 1, f);
        globs = new GlobalInit[gisize];
        fread(globs, sizeof(GlobalInit), gisize, f);

        fread(&ssize, sizeof(int), 1, f);
        str = new char[ssize];
        fread(str, sizeof(char), ssize, f);

        if (!feof(f)) {
            fread(&oisize, sizeof(int), 1, f);
            objinfo = new byte[oisize];
            fread(objinfo, sizeof(byte), oisize, f);
        }
        fclose(f);
    } else {
        bPRC = true;
        PalmDB db;
        if (!db.Read(fileName)) {
            printf("Unable to open '%s'\n", fileName);
            return false;
        }
        name = db.GetName();

        csize = 0;
        PalmResRecord* prec = db.GetResRec("ofHD", 0);
        if (prec) {
            PalmDBStream dbstr(prec->data);
            version = dbstr.getWord();
            bHasUI = dbstr.getByte() == 1;
            iGlobInit = dbstr.getLong();
            iGlobDest = dbstr.getLong();
            if (version >= 0x0400) {
                csize = dbstr.getLong();
            }
        } else {
            puts("unable to get header\n");
            return false;
        }

        if (csize) {
            code = new byte[csize];
            memset(code, vmNoOp, csize);
        }

        for (int i=0;i<MAX_CODE_SEGMENTS;i++) {
            prec = db.GetResRec("ofCD", i);
            if (prec) {
                if (!csize) {
                    csize = prec->len;
                    code = new byte[csize];
                }
                int segSize = prec->len;
                memcpy(code + i * CODE_SEGMENT_SIZE, prec->data, segSize);
            } else {
                if (i == 0) {
                    puts("unable to get code segment\n");
                    return false;
                } else {
                    break;
                }
            }
        }
        prec = db.GetResRec("ofGT", 0);
        if (prec) {
            gtsize = prec->len / sizeof(VarType);
            gtypes = new VarType[gtsize];
            memcpy(gtypes, prec->data, prec->len);
        } else {
            gtsize = 0;
            gtypes = NULL;
            //puts("unable to get global type segment\n");
            //return false;
        }
        prec = db.GetResRec("ofGI", 0);
        if (prec) {
            gisize = prec->len / 10;
            globs = new GlobalInit[gisize];
            PalmDBStream dbstr(prec->data);
            for (int i=0;i<gisize;i++) {
                globs[i].offset = dbstr.getLong();
                globs[i].val.type = dbstr.getByte();
                globs[i].val.stype = dbstr.getByte();
                globs[i].val.iVal = dbstr.getLong();
            }
        } else {
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
        prec = db.GetResRec("ofOI", 0);
        if (prec) {
            oisize = prec->len;
            objinfo = new byte[oisize];
            memcpy(objinfo, prec->data, oisize);
        }
        prec = db.GetResRec("ofNA", 0);
        if (prec) {
            PalmDBStream dbstr(prec->data);
            int nLibs = dbstr.getByte();
            for (int i=0;i<nLibs;i++) {
                BCLibrary lib;
                lib.name = getStr(dbstr.getWord());
                lib.id = dbstr.getWord();
                libs.push_back(lib);
            }
        }
    }
    return true;
}

void printOp(int type, int data) {
    if (type == otStack)
        printf("SP");
    else if (type == otImmed)
        printf("0x%04x", data);
    else if (type == otAddr)
        printf("[0x%04x]", data);
}

char buff[6000];
char* getStr(int off) {
    if (off < ssize) {
        int i=0,j=0;
        char c;
        buff[j++] = '"';
        while ((c=str[off+i])) {
            if (c == '\t') {
                buff[j++] = '\\';
                buff[j++] = 't';
            } else if (c == '\n') {
                buff[j++] = '\\';
                buff[j++] = 'n';
            } else if (c == '\r') {
                buff[j++] = '\\';
                buff[j++] = 'r';
            } else
                buff[j++] = c;
            i++;
        }
        buff[j++] = '"';
        buff[j] = 0;
        return buff;

    } else {
        return "<Invalid>";
    }
}

char* getObjectName(int objID) {
    if (objID >= oisize || objID < 3) return "<Invalid>";
    objID -= 2; // skip terminator
    while (objinfo[objID]) objID--;
    objID++;
    return (char*)&objinfo[objID];
}

char* getChar(char c) {
    int j=0;
    buff[j++] = '\'';
    if (c == '\t') {
        buff[j++] = '\\';
        buff[j++] = 't';
    } else if (c == '\n') {
        buff[j++] = '\\';
        buff[j++] = 'n';
    } else if (c == '\r') {
        buff[j++] = '\\';
        buff[j++] = 'r';
    } else
        buff[j++] = c;

    buff[j++] = '\'';
    buff[j] = 0;
    return buff;
}

void dumpCode() {
    printf("======  Code ======\n");
    int i = 0, fb = 0;
    int data, data_addr, cs_data, cs_addr;
    int if_stack, if_iface, if_func;
    VarType swType = vtVoid;
    byte ext;
    while (i < csize) {
        bool bExt = false;
        if ((code[i] & 0xC0) == 0xC0) {
            ext = code[i++];
            bExt = true;
        }

        int _bcsize = bcsize[code[i]];
        if (_bcsize == 1) {
            data = code[i+1];
        } else if (_bcsize == 2) {
            data = code[i+1] + ((int)code[i+2] << 8);
        } else if (_bcsize == 3) {
            cs_addr = code[i+1] + ((int)code[i+2] << 8);
            cs_data = code[i+3];
        } else if (_bcsize == 4) {
            cs_addr = code[i+1] + ((int)code[i+2] << 8);
            cs_data = code[i+3] + ((int)code[i+4] << 8);
            data = code[i+1] + ((int)code[i+2] << 8) + ((int)code[i+3] << 16) + ((int)code[i+4] << 24);
            if_stack = code[i+1];
            if_iface = code[i+2] + ((int)code[i+3] << 8);
            if_func = code[i+4];
        } else if (_bcsize == 6) {
            // case
            data = code[i+1] + ((int)code[i+2] << 8) + ((int)code[i+3] << 16) + ((int)code[i+4] << 24);
            data_addr = code[i+5] + ((int)code[i+6] << 8);
            cs_addr = code[i+1] + ((int)code[i+2] << 8);
            cs_data = code[i+3] + ((int)code[i+4] << 8) + ((int)code[i+5] << 16) + ((int)code[i+6] << 24);
        } else
            data = 0;

        if (code[i] == vmNoOp2) {
            printf(": %s", getStr(data));
        } else {
            if (bNoAddr)
                printf("      %6.6s ", instNames[code[i]]);
            else
                printf("%5d %6.6s ", (bExt ? i-1 : i), instNames[code[i]]);
            if (code[i] == vmLink)
                fb = i;
            switch (code[i]) {
                case vmNoOp2:
                    break;
                case vmCall: {
                    int strLoc = code[data-2] + ((int)code[data-1] << 8);
                    printf("(%d) %s", data, getStr(strLoc));
                    break;
                }
                case vmSCInt:
                    printf("[0x%04x] <- %d", cs_addr, cs_data);
                    break;
                case vmSCWord:
                    printf("[0x%04x] <- %d", cs_addr, cs_data);
                    break;
                case vmCallIface:
                    printf("%s.%d (st:%d)", getObjectName(if_iface), if_func, if_stack);
                    break;
                case vmCallVirt:
                    printf("%d (st:%d)", code[i+2], code[i+1]);
                    break;
                case vmCString:
                case vmStackInit:
                    printf("(%d) %s", data, getStr(data));
                    break;
                case vmSCString:
                    printf("[0x%04x] <- (%d) %s", cs_addr, cs_data, getStr(cs_data));
                    break;
                case vmCChar:
                    printf("%s", getChar(data));
                    break;
                case vmSCChar:
                    printf("[0x%04x] <- %s", cs_addr, getChar(cs_data));
                    break;
                case vmCFloat:
                    printf("%f", *((float*)(&data)));
                    break;
                case vmSCFloat:
                    printf("[0x%04x] <- %f", cs_addr, *((float*)(&cs_data)));
                    break;
                case vmCallBI:
                    if (data < (sizeof(funcNames) / sizeof(funcNames[0])))
                        printf("%s", funcNames[data]);
                    else
                        printf("<Invalid #>");
                    break;
                //case vmCallLib:
                    //if (data
                    //printf("%d, %d
                    //break;
                case vmLoadI:
                case vmStoreI:
                    printf("0x%04x", data);
                    break;
                case vmMove:
                    printf("[0x%04x] <- [0x%04x]", cs_data, cs_addr);
                    break;
                case vmJmp:
                case vmJmpZ:
                case vmJmpNZ:
                case vmJmpPopZ:
                case vmJmpPopNZ:
                case vmDefault:
                    printf("%d", data + fb);
                    break;
                case vmCase:
                    if (swType == vtInt)
                        printf("%d", data);
                    else if (swType == vtChar)
                        printf("'%c'", (char)data);
                    else if (swType == vtFloat)
                        printf("%f", *((float*)(&data)));
                    else if (swType == vtString)
                        printf("(%d) %s", data, getStr(data));

                    printf(", %d", data_addr + fb);
                    break;

                default:
                    if (bcsize[code[i]])
                        printf("%d ", data);
                    break;
            }
            if (code[i] >= vmSwitchI && code[i] <= vmSwitchS)
                swType = (VarType)(vtInt + (code[i] - vmSwitchI));
        }
        i += bcsize[code[i]] + 1;

        if (bExt) {
            int op = -1;
            if (ext & 0x03) {
                if (ext & 0x02) {
                    op = code[i] + ((int)code[i+1] << 8);
                    i += 2;
                }
                printOp(ext & 0x03, op);
                printf(" <= ");
            }

            if (ext & 0x30) {
                if (ext & 0x20) {
                    op = code[i] + ((int)code[i+1] << 8);
                    i += 2;
                }
                printOp((ext & 0x30) >> 4, op);
                if (ext & 0x0C)
                    printf(",");
            }

            if (ext & 0x0C) {
                if (ext & 0x08) {
                    op = code[i] + ((int)code[i+1] << 8);
                    i += 2;
                }
                printOp((ext & 0x0C) >> 2, op);
            }
        }

        printf("\n");
    }
    printf("\n");
}

void dumpGlobals() {
    printf("======  Global Inits ======\n");
    for (int i=0;i<gisize;i++) {
        printf("%05d: ", globs[i].offset);
        switch (globs[i].val.type) {
            case vtInt:
                printf("Int %d\n", globs[i].val.iVal);
                break;
            case vtChar:
                printf("Chr '%c'\n", globs[i].val.cVal);
                break;
            case vtFloat:
                printf("Flo %f\n", globs[i].val.fVal);
                break;
            case vtString:
                if (globs[i].val.iVal < ssize)
                    printf("Str %s\n", &str[globs[i].val.iVal]);
                else
                    printf("(invalid)\n");
                break;
            default:
                printf("Unknown type!!!\n");
        }
    }
    printf("\n");
}

void dumpStrings() {
    printf("======  String Table ======\n");
    int off = 0;
    while (off < ssize) {
        printf("%05d: %s\n", off, &str[off]);
        off += strlen(&str[off]) + 1;
    }
    printf("\n");
}

void dumpGlobalTypes() {
    char* typeName[] = { "Int", "Chr", "Flo", "Str", "Voi", "<S>", "<F>", "<R>", "<P>", "<B>", "<H>" };
    printf("======   Global Types  ======");
    for (int i=0;i<gtsize;i++) {
        if (i % 10 == 0)
            printf("\n%05d: ", i);
        char * name = gtypes[i] <= vtHandler ? typeName[gtypes[i]] : "Err";
        printf("%s ", name);
    }
    printf("\n");
}

void dumpLibs() {
    printf("======  Native Add-ins ======\n");
    if (libs.size()) {
        for (int i=0;i<libs.size();i++) {
            printf("%05d: %s\n", libs[i].id, libs[i].name.c_str());
        }
    } else {
        printf("No addins\n");
    }
}

void dumpHeader() {
    if (bPRC) {
        printf("Name:    %s\nVersion: 0x%x\nHas UI:  %s\n", name.c_str(), version, bHasUI ? "true" : "false");
    }
}

void dumpMethods() {
    for (int i = 0; i < (sizeof(funcNames) / sizeof(funcNames[0])); i++) {
        printf("%3d %s\n", i, funcNames[i]);
    }
}

void dumpObjects() {
    printf("======  Object info  ======\n");
    int i = 0;
    while (i < oisize) {
        i++; // skip null-starter
        printf("Name: %s ", &objinfo[i]);
        i += strlen((char*)&objinfo[i]) + 1;
        int base = objinfo[i] + ((int)objinfo[i+1] << 8);
        int size = objinfo[i+2] + ((int)objinfo[i+3] << 8);
        if (base != 0xffff) {
            if (base < oisize)
                printf("(base: %s, size: %d)\n", getObjectName(base), size);
            else
                printf("(%d, size: %d)\n", base, size);
        } else {
            printf("(no base, size: %D)\n", size);
        }
        i+=4;
        int nVirtuals = objinfo[i++];
        int j;
        for (j=0;j<nVirtuals;j++) {
            int addr = objinfo[i] + ((int)objinfo[i+1] << 8) + ((int)objinfo[i+2] << 16) + ((int)objinfo[i+3] << 24);
            int strLoc = code[addr-2] + ((int)code[addr-1] << 8);
            printf("  Func: %d %s\n", addr, getStr(strLoc));
            i+=4;
        }
        int nIfaces = objinfo[i++];
        int nIfaceMethods = objinfo[i] + (objinfo[i+1] << 8);
        i+=2;
        int ifaceMethodBase = i + 6 * nIfaces;
        if (nIfaces) {
            printf("  Ifaces: ");
            for (j=0;j<nIfaces;j++) {
                int iface_id = objinfo[i] + ((int)objinfo[i+1] << 8);
                i+=2;
                int addr = objinfo[i] + ((int)objinfo[i+1] << 8) + ((int)objinfo[i+2] << 16) + ((int)objinfo[i+3] << 24);
                i+=4;
                printf("[%s: %d] ", getObjectName(iface_id), (addr - ifaceMethodBase)/4);
            }
            printf("\n");
            for (j=0;j<nIfaceMethods;j++) {
                int addr = objinfo[i] + ((int)objinfo[i+1] << 8) + ((int)objinfo[i+2] << 16) + ((int)objinfo[i+3] << 24);
                int strLoc = code[addr-2] + ((int)code[addr-1] << 8);
                printf("     Func: %d %s\n", addr, getStr(strLoc));
                i+=4;
            }
        }
        int nSubObjects = objinfo[i] + (objinfo[i+1] << 8);
        i+=2;
        if (nSubObjects) {
            printf("  Objects: ");
            for (j=0;j<nSubObjects;j++) {
                int addr = objinfo[i] + (objinfo[i+1] << 8);
                i+=2;
                int obj_id = objinfo[i] + (objinfo[i+1] << 8);
                i+=2;
                printf("[%d: %d] ", addr, obj_id);
            }
            printf("\n");
        }
    }
}

void main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        return;
    }

    bool bCode = false, bGlobals = false, bStrings = false, bLibs = false, bGlobTypes = false, bObjinfo = false, bMethods = false;
    int iFile = 1;
    if (argv[1][0] == '-') {
        iFile = 2;
        char* p = &argv[1][1];
        while (*p) {
            if (*p == 'g')
                bGlobals = true;
            else if (*p == 't')
                bGlobTypes = true;
            else if (*p == 'c')
                bCode = true;
            else if (*p == 's')
                bStrings = true;
            else if (*p == 'a')
                bNoAddr = true;
            else if (*p == 'l')
                bLibs = true;
            else if (*p == 'o')
                bObjinfo = true;
            else if (*p == 'm')
                bMethods = true;
            else
                printf("Unknown options '%c'\n", *p);
            p++;
        }
    } else {
        bCode = true;
        bGlobals = bStrings = bLibs = false;
    }

    if (bMethods) {
        dumpMethods();
    } else if (readFile(argv[iFile])) {
        dumpHeader();
        if (bLibs)
            dumpLibs();
        if (bObjinfo)
            dumpObjects();
        if (bGlobTypes)
            dumpGlobalTypes();
        if (bGlobals)
            dumpGlobals();
        if (bCode)
            dumpCode();
        if (bStrings)
            dumpStrings();
    }
}
