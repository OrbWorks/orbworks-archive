#ifndef ORBCOMPILER_PUB_H
#define ORBCOMPILER_PUB_H

#ifdef WIN32
#ifndef ovector
#define ovector vector
#endif
using namespace std;

#ifndef OC_EXPORT
#define OC_EXPORT __declspec(dllimport)
#endif
#endif

enum SourceType { srcNone, srcMemo, srcDoc, srcVFS, srcRes, srcMacro };

struct ErrorInfo {
    string file;
    int line;
    int ch;
    int offset;
    string desc;
    SourceType sourceType;
};

#ifdef WIN32
// quick parse
enum QPSymbolType { stFunction, stMethod, stHandler, stObject, stGadget,
    stStruct, stInterface };

struct QPSymbol {
    string object;
    string name;
    QPSymbolType type;
    int line;
};

struct QPGadget {
    string name;
    ovector<string> handlers;
};

struct QuickParseInfo {
    ovector<QPSymbol> symbols;
    ovector<QPGadget> gadgets;
};

// symbol parse

enum SPType { sptGlobal, sptLocal, sptParam, sptMember,	sptFunction,
    sptMethod, sptHandler, sptObject, sptConst, sptFuncPtr, sptResource,
    sptStruct, sptInterface
};

enum SPScope { spsGlobal, spsLocal, spsObject };

struct SPSymbol {
    SPType stype;
    SPScope scope;
    string scopeName;

    // inheritance info
    ovector<string> bases;
    int depth;

    string name;
    string type;
    string decl;
    string help;
    int indir;
    bool array;

    // file location
    string file;
    int line;
    int start, end;
};

class OC_EXPORT OrbSymParser {
public:
    OrbSymParser(bool bPocketCCompat = false);
    ~OrbSymParser();

    void AddFile(string& file, bool bIncludes);
    void AddBuffer(string& file, string& text, bool bIncludes);
    void AddRes(string& file, ovector<string>& names, ovector<string>& types);
    void RemoveFile(string& file);
    bool IsProjectFile(string& file);
    bool FindSyms(string& file, string& text, int pos, bool bRequireContext, ovector<SPSymbol>& syms);
    bool FindMethodDecl(string& file, string& text, int pos, string& decl);
    bool FindSymbolDecl(string& file, string& text, int pos, string& decl);
    bool FindDocSymbol(string& file, string& text, int pos, string& docSym, string& decl, string& help);
    bool FindDef(string& file, string& text, int pos, string& ofile, int& oline);
    void Clear();
    void GetAllSyms(ovector<SPSymbol>& syms);
};

#include "resource.h"

class OC_EXPORT OrbCompiler {
public:
    OrbCompiler();
    ~OrbCompiler();

    bool Compile(char* file, char* sysfile, char* dest, bool bStandalone,
        bool bPocketCCompat, char* dbgsyms, char* language, ErrorInfo& error);
    bool ReadProject(char* file, PalmProject& proj, ErrorInfo& error);
    bool WriteProject(char* file, PalmProject& proj);
    bool QuickParse(char* buff, bool bPocketCCompat, QuickParseInfo& info);
    bool GenDocXML(char* file);
    string FindFile(const char* file, const char* relativeFile);
};
#endif
#endif
