#include "pc3.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <io.h>
#include "../../OrbForms/compiler/palmdb.h"

char* biFuncs[] = {
    "alert",
    "puts",
    "gets",
    "clear",

    // String routines
    "strlen",
    "substr",
    "strleft",
    "strright",
    "strupr",
    "strlwr",

    // Math routines
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
    "pow",
    "atan2",
    "rand",
    "random",

    // Sound routines
    "beep",
    "tone",

    // Graphics routines
    "graph_on",
    "graph_off",
    "clearg",
    "text",
    "line",
    "rect",
    "title",
    "textattr",

    // Advanced I/O routines
    "wait",
    "waitp",
    "getc",
    "penx",
    "peny",

    // Late additions
    "hex",
    "frame",

    // Time/Date routines
    "time",
    "date",
    "seconds",
    "ticks",

    // More late additions
    "confirm",
    "mathlib",
    "frame2",

    // Database I/O
    "dbopen",
    "dbcreate",
    "dbclose",
    "dbwrite",
    "dbread",
    "dbpos",
    "dbseek",
    "dbbackup",
    "dbdelete",

    // New event routines
    "event",
    "key",
    "pstate",
    "bstate",

    // MemoDB I/O
    "mmnew",
    "mmfind",
    "mmopen",
    "mmclose",
    "mmputs",
    "mmgetl",
    "mmeof",
    "mmrewind",
    "mmdelete",

    "strstr",
    "bitmap",
    "sleep",
    "resetaot",
    "getsysval",
    "format",

    // Serial I/O
    "seropen",
    "serclose",
    "sersend",
    "serrecv",
    "serdata",

    "textalign",
    "launch",
    "saveg",
    "restoreg",

    "serbuffsize",
    "malloc",
    "free",
    "settype",
    "typeof",
    "clipget",
    "clipset",
    "mmcount",
    "dbenum",
    "dbrec",
    "dbnrecs",
    "dbreadx",
    "dbwritex",
    "dbsize",
    "dbdelrec",
    "dbremrec",
    "dbarcrec",
    "dberase",
    "memcpy",
    "hookhard",
    "hookmenu",
    "exit",
    "strtoc",
    "ctostr",
    "getsd",
    "atexit",
    "textwidth",
    "version",
    "getsi",
    "sersenda",
    "serrecva",
    "unpack",
    "malloct",
    "getsm",
    "deepsleep",
    "dbgetcat",
    "dbsetcat",
    "dbcatname",
    "dbcreatex",
    "dbmoverec",
    "dbinfo",
    "dbtotalsize",
    "hooksilk",
    "hooksync",
    "serwait",
    "dbrename",
    "dbsetcatname",
    "dbwritex2",
    "dbgetappinfo",
    "dbsetappinfo",
};

int nBIFuncs = sizeof(biFuncs)/sizeof(biFuncs[0]);

char BCArgs[vmNumInstr] = {
    4, 1, 2, 2,
    0, 0, 0, 0,
    2, 2, 2, 2, 0,
    2, 2, 2, 2,
    2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    2, 2, 2,
    2, 1, 1, 0, 0, 0, 2, // NOTE: OldCall has 2 bytes
    1, 0, 0, 0, 2,

    // Version 3.0 instructions
    2, 2, 0,
    0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 2, 4, 4,
    5, 0, 0,
    7
};

void dumpcode();
void help();

char* typeName[] = {
    "int", "char", "float", "string"
};

char* inst[] = {
    // constants
    "CInt", "CChar", "CFloat", "CString",
    // typecasts
    "ToInt", "ToChar", "ToFloat", "ToString",
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

    "CWord", "CWordPFP", "Call",
    "Load", "Save",
    "IncA", "DecA", "IncB", "DecB",
    "BAnd", "BOr", "BNot", "SL", "SR", "Xor",
    "NoOp", "NoOp2", "XXX", "XXX",

    "AddExtI", "GetC", "SetC", "Case"
};

unsigned char* code;
float* cFloats;
char* cStrings;
char* name;
unsigned char* data;
int nFloats, nStrChars, codePtr;
int ver = 0;

int main(int argc, char* argv[]) {
    nFloats = nStrChars = 0;
    code = new unsigned char[0x40002];
    cFloats = new float[0x10000];
    cStrings = new char[0x10002];
    memset(code, vmHalt, 0x40002);

    if (argc < 2) help(); // exits

    PalmDB db;
    if (!db.Read(argv[1])) {
        printf("Unable to open '%s'\n", argv[1]);
        return false;
    }
    bool bPRC = db.GetRes();
    name = strdup(db.GetName().c_str());

    if (bPRC) {
        PalmResRecord* prec = db.GetResRec("PCpc", 1001);
        if (prec) {
            codePtr = prec->len - 2;
            memcpy(code, prec->data + 2, codePtr);
        } else {
            puts("unable to get code segment\n");
            return false;
        }
        prec = db.GetResRec("PCpc", 1002);
        if (prec) {
            nFloats = prec->data[0] * 256 + prec->data[1];
            memcpy(cFloats, prec->data + 2, nFloats * sizeof(float));
        } else {
            puts("unable to get float segment\n");
            return false;
        }
        prec = db.GetResRec("PCpc", 1003);
        if (prec) {
            nStrChars = prec->data[0] * 256 + prec->data[1];
            memcpy(cStrings, prec->data + 2, nStrChars);
        } else {
            puts("unable to get string segment\n");
            return false;
        }
        for (int i=1;i<MAX_CODE_SEGS;i++) {
            prec = db.GetResRec("PCpc", 1004 + i);
            if (prec) {
                memcpy(code + 0x10000*i, prec->data, prec->len);
            } else break;
        }
    } else {
        PalmRecord* prec = db.GetRec(0);
        if (prec) {
            codePtr = prec->len - 2;
            memcpy(code, prec->data + 2, codePtr);
        } else {
            puts("unable to get code segment\n");
            return false;
        }
        prec = db.GetRec(1);
        if (prec) {
            nFloats = prec->data[0] * 256 + prec->data[1];
            memcpy(cFloats, prec->data + 2, nFloats * sizeof(float));
        } else {
            puts("unable to get float segment\n");
            return false;
        }
        prec = db.GetRec(2);
        if (prec) {
            nStrChars = prec->data[0] * 256 + prec->data[1];
            memcpy(cStrings, prec->data + 2, nStrChars);
        } else {
            puts("unable to get string segment\n");
            return false;
        }
        for (int i=1;i<MAX_CODE_SEGS;i++) {
            prec = db.GetRec(3 + i);
            if (prec) {
                memcpy(code + 0x10000*i, prec->data, prec->len);
            } else break;
        }
    }
    dumpcode();

    free(data);
    delete code;
    delete cStrings;
    delete cFloats;

    return 0;
}

void help() {
    printf(
        "PocketC bytecoder - by Jeremy Dewey, (c) 1998-2003 OrbWorks\n\n"
        "usage - bytecode file.pdb\n");
    exit(1);
}

char escape_buff[512];
char* escape(char* str) {
    int di=0;
    while (*str) {
        if (*str == '\n') {
            escape_buff[di++] = '\\';
            escape_buff[di++] = 'n';
        } else if (*str == '\t') {
            escape_buff[di++] = '\\';
            escape_buff[di++] = 't';
        } else if (*str == '\r') {
            escape_buff[di++] = '\\';
            escape_buff[di++] = 'r';
        } else
            escape_buff[di++] = *str;
        str++;
    }
    escape_buff[di] = '\0';
    return escape_buff;
}

float x86float(float f) {
    unsigned long fl;
    fl = *((unsigned long*)(&f));
    fl = ((fl & 0xff000000) >> 24) |
          ((fl & 0x00ff0000) >> 8)  |
          ((fl & 0x0000ff00) << 8)  |
          ((fl & 0x000000ff) << 24);
    f = *((float*)&fl);

    return f;
}

int instCount[vmNumInstr] = {0,};
int nFuncs = 0;
char* funcNames[1024] = {0,};
int funcLocs[1024] = {0,};

char* findFunc(int loc) {
    if (nFuncs) {
        for (int i=0;i<nFuncs;i++) {
            if (loc >= funcLocs[i] && loc <= funcLocs[i]+5) {
                return funcNames[i];
            }
        }
    }
    return NULL;
}

void dumpcode() {
    int i=0, ii, iFunc=0;
    int nArgs=0, dbg_loc=0;
    printf("   Name: %s\n", name);

    if (code[0] == vmHalt) {
        // library support
        i = 3;
        if (code[3] == vmHalt) {
            printf("Version: %c.%c%c\n",
                (code[5] & 0x0f) + '0',
                ((code[6] & 0xf0) >> 4) + '0',
                (code[6] & 0x0f) + '0');
            ver = (code[5] << 8) | code[6];
            printf(" Header: %d bytes\n", code[4]);
            i = 5 + code[4];
        } else {
            printf("Version: < 3.50\n");
        }

        int lib_loc;
        if (ver >= 0x0500) {
            lib_loc = (code[11] << 24) + (code[12] << 16) + (code[1] << 8) + code[2];
            dbg_loc = (code[7] << 24) + (code[8] << 16) + (code[9] << 8) + code[10];
        } else if (ver >= 0x0410) {
            lib_loc = (code[1] * 256 + code[2]);
            dbg_loc = (code[7] * 256 + code[8]);
        } else {
            lib_loc = (code[1] * 256 + code[2]);
            dbg_loc = 0;
        }
        if (dbg_loc)
            codePtr = dbg_loc;
        else if (lib_loc)
            codePtr = lib_loc;

        // don't disasm the library section
        if (lib_loc) {
            int lib_cnt = code[lib_loc];
            int lib_ids = lib_loc + 1 + lib_cnt * 2;
            printf("   Libs: %d (@ %d)\n", lib_cnt, lib_loc);

            codePtr = lib_loc;

            lib_loc++;
            while (lib_cnt--) {
                int spos = code[lib_loc] * 256 + code[lib_loc+1];
                int resid = 0xffff;
                if (ver >= 0x0650) {
                    resid = code[lib_ids] * 256 + code[lib_ids+1];
                    lib_ids += 2;
                }

                lib_loc += 2;
                if (spos < nStrChars)
                    printf("         � %s", &cStrings[spos]);
                else
                    printf("Invalid pointer to library name!");

                if (resid != 0xffff) {
                    printf(" [%d]\n", resid);
                } else {
                    printf("\n");
                }
            }
        }

        printf("\n");
    }
    if (dbg_loc) {
        nFuncs = code[dbg_loc] * 256 + code[dbg_loc + 1];
        int addr = dbg_loc + 2;
        for (int i=0;i<nFuncs;i++) {
            int str_loc;
            if (ver >= 0x0500) {
                funcLocs[i] = (code[addr] << 24) + (code[addr+1] << 16) + (code[addr+2] << 8) + code[addr+3];
                str_loc = code[addr+4] * 256 + code[addr+5];
            } else {
                funcLocs[i] = code[addr] * 256 + code[addr+1];
                str_loc = code[addr+2] * 256 + code[addr+3];
            }
            
            if (str_loc < nStrChars)
                funcNames[i] = &cStrings[str_loc];
            else
                funcNames[i] = "<Error in debug data>";
            addr += (ver >= 0x0500) ? 6 : 4;
        }
    }

    int fb = 0;
    VMsym lastInstr[4] = {vmHalt, vmHalt, vmHalt, vmHalt};
    while (i<codePtr) {
        int val, val2, mask;
        ii = i;
        VMsym byte = (VMsym) code[i++];
        if (byte >= vmNumInstr) {
            printf(" %4d\t???? (%d)\n", ii, byte);
            continue;
        }
        instCount[byte]++;
        lastInstr[0] = lastInstr[1];
        lastInstr[1] = lastInstr[2];
        lastInstr[2] = lastInstr[3];
        lastInstr[3] = byte;

        if (byte == vmHalt && i < codePtr && code[i] == vmHalt) {
            printf(" %4d\t%s\n...\n", ii, inst[byte]);
            while (i < codePtr && code[i] == vmHalt) i++;
            continue;
        }

        if (BCArgs[byte]==0) {
            printf(" %4d\t%s\n", ii, inst[byte]);
            continue;
        }
        val = val2 = mask = 0;
        bool b4 = byte == vmOldCall && ver >= 0x0500;
        if (BCArgs[byte]==1) val = code[i++];
        else if (BCArgs[byte]==2 && !b4) {
            val |= code[i++];
            val <<=8;
            val |=code[i++];
        }
        else if (BCArgs[byte]==4 || b4) {
            val |= code[i++];
            val <<=8;
            val |=code[i++];
            val <<=8;
            val |=code[i++];
            val <<=8;
            val |=code[i++];
        }
        else if (byte == vmAddExtI) {
            mask = code[i++];
            val |= code[i++];
            val <<=8;
            val |=code[i++];
            val2 |= code[i++];
            val2 <<=8;
            val2 |=code[i++];
        }

        if (byte == vmNoOp2) {
            if (val < nStrChars)
                printf("%s\n", &cStrings[val]);
            else
                printf("Invalid pointer to source code!\n");
            continue;
        }
        if (byte == vmLink) {
            char* name = findFunc(i);
            printf("\nFunc %d: %s()\n", iFunc++, name);
            if (ver >= 0x0500) fb = i-2;
        }

        printf(" %4d\t", ii);
        if (byte==vmCString) {
            if (val < nStrChars)
                printf("%s\t\t%d\t\"%s\"\n", inst[byte], val, escape(&cStrings[val]));
            else
                printf("%s\t\t%d\tInvalid\n", inst[byte], val);
        }
        else if (byte==vmCFloat) {
            if (val < nFloats)
                printf("%s\t\t%d\t%f\n", inst[byte], val, x86float(cFloats[val]));
            else
                printf("%s\t\t%d\tInvalid\n", inst[byte], val);
        }
        else if (byte==vmCallBI) {
            if (val < nBIFuncs)
                printf("%s\t\t%s\n", inst[byte], biFuncs[val]);
            else
                printf("%s\t\t(id = %d)\n", inst[byte], val);
        }
        else if (byte==vmOldCall) {
            char* name = findFunc(val);
            if (name)
                printf("%s\t\t%s()\n", inst[byte], name);
            else
                printf("%s\t\t%d\n", inst[byte], val);
        }
        else if (byte==vmCWordPFP) {
            char* type;
            val -= 0x6000;
            if (val < nArgs) type = "arg"; 
            else {
                type = "local";
                val -= (nArgs + 2);
            }
            printf("%s\t%s %d\n", inst[byte], type, val);
        }
        else if (byte==vmLink) {
            nArgs = val;
            printf("%s\t\t(%d args)\n", inst[byte], val);
        }
        else if (byte==vmCase) {
            int type = code[i++];
            val |= code[i++];
            val <<=8;
            val |=code[i++];
            val <<=8;
            val |=code[i++];
            val <<=8;
            val |=code[i++];

            val2 |= code[i++];
            val2 <<=8;
            val2 |=code[i++];

            if (type==vtNone)
                printf("Deflt\t\t%d\n", val2);
            else
                printf("%s\t\tt:%d, v:%d, l:%d\n", inst[byte], type, val, val2);
        }
        else if (byte==vmAddExtI) {
            printf("%s\t\t", inst[byte]);
            if ((mask & 0x03) == 1)
                printf("[0x%04x], ", val);
            else if ((mask & 0x03) == 2)
                printf("[0x%04x+FP], ", val);
            else if ((mask & 0x03) == 3)
                printf("0x%04x+FP, ", val);
            else
                printf("%d, ", val);

            if ((mask & 0x0C) == 0x4)
                printf("[0x%04x]\n", val2);
            else if ((mask & 0x0C) == 0x8)
                printf("[0x%04x+FP]\n", val2);
            else if ((mask & 0x0C) == 0x0C)
                printf("0x%04x+FP, ", val);
            else
                printf("%d\n", val2);

        }
        else if (byte == vmJmp || byte == vmJmpNZ || byte == vmJmpZ) {
            printf("%s\t\t%d\n", inst[byte], val + fb);
        }
        else
            printf("%s\t\t%d\n", inst[byte], val);

        if (lastInstr[0] == vmCWord &&
            lastInstr[1] == vmCWord &&
            lastInstr[2] == vmLoad &&
            lastInstr[3] == vmAdd)
        {
            printf("<Could be optimized to AddI A,[B]>\n");
        }

        if (lastInstr[1] == vmCWord &&
            lastInstr[2] == vmCWord &&
            lastInstr[3] == vmAdd)
        {
            printf("<Could be optimized to AddI A,B>\n");
        }
    }

    printf("\n\nInstruction count:\n");
    int k=0;
    for (i=0;i<vmNumInstr;i++) {
        if (k > 3) {
            printf("\n");
            k=0;
        }
        printf("%8s %d\t", inst[i], instCount[i]);
        k++;
    }
}

