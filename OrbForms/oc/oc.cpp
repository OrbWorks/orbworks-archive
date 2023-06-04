// oc.cpp : Defines the entry point for the console application.
//
#pragma warning(disable:4786)
#include <stdlib.h>
#include <io.h>
#include <assert.h>
#include <string>
#include <vector>
using namespace std;

#ifdef DEBUG
#define INTERNAL_BUILD
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <crtdbg.h>

#include "..\compiler\comp_pub.h"
#include "..\emuctrl\emuctrl.h"

char* GetFile(char* file) {
    FILE* f = fopen(file, "rb");
    if (!f) {
        printf("Unable to open file '%s'", file);
        return NULL;
    }

    int len = _filelength(f->_file);
    char* data = (char*)malloc(len+1);
    fread(data, 1, len, f);
    data[len] = 0;
    fclose(f);
    return data;
}

void usage() {
#ifdef INTERNAL_BUILD
    printf("oc [-pocketc] [-standalone] [-debug] [-language language.txt] source.orb dest.prc\noc -qp source.orb\noc -proj source.orb dest.orb\noc -install file.prc\noc -sym file.oc\noc -xml file.oc\n");
#else
    printf("OrbC compiler - (c) 2002-2006 OrbWorks\nUsage:\n  oc.exe [-pocketc] [-standalone] [-debug] [-language language.txt] project.orb out.prc\n");
#endif
}

#pragma warning(disable:4018)

int main(int argc, char* argv[])
{
#ifdef INTERNAL_BUILD
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    bool bDumpFuncs = false, bQuickParse = false, bProject = false;
    bool bDoc = false, bInstall = false, bSymParse = false, bDebug = false, bPocketC = false;
    int ret = 0;
    int ifArg = 1;
    char* language = NULL;

    if (argc < 3) {
        usage();
        return -1;
    }

#ifdef INTERNAL_BUILD
    if (!strcmp(argv[1], "-qp"))
        bQuickParse = true;
    else if (!strcmp(argv[1], "-proj")) {
        bProject = true;
        if (argc < 4) {
            usage();
            return -1;
        }
    } else if (!strcmp(argv[1], "-xml")) {
        bDoc = true;
    } else if (!strcmp(argv[1], "-install")) {
        bInstall = true;
        if (argc < 3) {
            usage();
            return -1;
        }
    } else if (!strcmp(argv[1], "-sym")) {
        bSymParse = true;
        if (argc < 3) {
            usage();
            return -1;
        }
    }
#endif

    OrbCompiler* comp = new OrbCompiler;
    
#ifdef INTERNAL_BUILD
    if (bQuickParse) {
        QuickParseInfo info;
        char* type[] = { "Func", "Meth", "Hand", " Obj", "Gadg", "Stru", "Ifac" };
        char* buff = GetFile(argv[2]);
        if (buff) {
            bool success = comp->QuickParse(buff, true, info);
            int i;
            for (i=0;i<info.symbols.size();i++) {
                switch (info.symbols[i].type) {
                    case stFunction:
                        printf("%4d %s: %s\n", info.symbols[i].line, type[info.symbols[i].type], info.symbols[i].name.c_str());
                        break;
                    case stHandler:
                    case stMethod:
                        printf("%4d %s: %s.%s\n", info.symbols[i].line, type[info.symbols[i].type], info.symbols[i].object.c_str(), info.symbols[i].name.c_str());
                        break;
                    case stObject:
                    case stStruct:
                    case stInterface:
                    case stGadget:
                        printf("%4d %s: %s\n", info.symbols[i].line, type[info.symbols[i].type], info.symbols[i].name.c_str());
                        break;
                }
            }
            for (i=0;i<info.gadgets.size();i++) {
                printf("Gadget '%s':\n", info.gadgets[i].name.c_str());
                for (int j=0;j<info.gadgets[i].handlers.size();j++)
                    printf("   %s\n", info.gadgets[i].handlers[j].c_str());
            }
            printf("QP: %s\n", success ? "succeeded" : "failed");
            free(buff);
        }

    } else if (bProject) {
        PalmProject proj;
        ErrorInfo error;
        if (comp->ReadProject(argv[2], proj, error)) {
            if (!comp->WriteProject(argv[3], proj)) {
                printf("Writing '%s' failed\n", argv[2]);
            }
        } else {
            printf("Reading '%s' failed - %s @ %d:%d\n", argv[2], error.desc.c_str(), error.line, error.ch);
        }
        
    } else if (bDoc) {
        comp->GenDocXML(argv[2]);
    } else if (bInstall) {
        char buff[1024];
        GetFullPathName(argv[2], 1024, buff, NULL);
        if (!InstallFile(buff)) {
            printf("Failed to install '%s'\n", argv[2]);
        }
    } else if (bSymParse) {
        OrbSymParser parser;
        vector<SPSymbol> syms;
        char* buff = GetFile(argv[2]);
        char* types[] = { "global", "local", "param", "member", "function", "method", "handler", "object", "const", "funcptr", "resource", "struct", "iface" };
        char* scopes[] = { "global", "local", "object" };
        char* stars[] = { "", "*", "**", "***" };
        parser.AddBuffer(string(argv[2]), string(buff), true);
        parser.GetAllSyms(syms);
        printf("%d symbols:\n", syms.size());
        for (int i=0;i<syms.size();i++) {
            printf("%3d Name: %s, Type: %s%s%s Bases: ", i, syms[i].name.c_str(), syms[i].type.c_str(), stars[syms[i].indir], syms[i].array ? "[]" : "");
            for (int j=0;j<syms[i].bases.size();j++)
                printf("%s ", syms[i].bases[j].c_str());
            printf("\n");
            printf("    SymType: %s, Scope: %s, ScopeName: %s\n", types[syms[i].stype], scopes[syms[i].scope], syms[i].scopeName.c_str());
            printf("    Start: %d, End: %d, Decl: %s, Help: %s\n", syms[i].start, syms[i].end, syms[i].decl.c_str(), syms[i].help.c_str());
            printf("    Line: %d, File: %s\n\n", syms[i].line, syms[i].file.c_str());
        }
        free(buff);
    } else
#endif
    {
        ErrorInfo error;
        int i = 1;
        bool bStandalone = false;
        if (strcmp(argv[i], "-pocketc") == 0) {
            bPocketC = true;
            i++;
        }
        if (strcmp(argv[i], "-standalone") == 0) {
            bStandalone = true;
            i++;
        }
        if (strcmp(argv[i], "-debug") == 0) {
            bDebug = true;
            i++;
        }
        if (strcmp(argv[i], "-language") == 0) {
            language = argv[i+1];
            i+=2;
        }
#ifdef DEMO
        if (bStandalone) {
            bStandalone = false;
            printf("-standalone flag ignored when using OrbForms Designer DEMO");
        }
#endif

#ifdef INTERNAL_BUILD
        if (!comp->Compile(argv[i], "orbc.sys", argv[i+1], bStandalone, bPocketC, bDebug ? "debug" : NULL, language, error)) {
#else
        if (argc <= i+1) {
            usage();
            return -1;
        }
        if (!comp->Compile(argv[i], "orbc.sys", argc > i+1 ? argv[i+1] : "out.prc", bStandalone, bPocketC, bDebug ? "debug" : NULL, language, error)) {
#endif
            if (error.line)
                printf("%s - %s @ %d:%d\n", error.desc.c_str(), error.file.c_str(), error.line, error.ch);
            else
                puts(error.desc.c_str());
            ret = -1;
        }
    }

    delete comp;
    return ret;
}
