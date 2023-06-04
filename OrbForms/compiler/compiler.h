#ifdef WIN32
#pragma warning(disable:4786)
#pragma warning(disable:4018)
#include <stdlib.h>
#include <io.h>
#include <assert.h>
#include <string>
#include <vector>
#define ovector vector
#define dbvector vector
#include <map>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
using namespace std;
#else
#include "stdcover.h"
#include "string.h"
#include "vector.h"
#endif

#include "orbforms.h"

#ifdef WIN32
#define OC_EXPORT __declspec(dllexport)
#endif
#include "comp_pub.h"

enum TokenID {
    // syntactic elements
    tiLibrary, tiEndLib, tiInclude,
    tiIdent, tiConstString, tiConstInt, tiConstFloat, tiConstChar,
    // variable types and declarators
    tiStatic, tiInt, tiChar, tiFloat, tiString, tiFuncPtr, tiVoid,
    tiStruct, tiHandler, tiObject, tiIface, tiVirtual, tiPointer,
    // keywords
    tiIf, tiElse, tiFor, tiWhile, tiDo, tiReturn, tiBreak, tiContinue, tiSizeof, tiTypeof, 
    tiNew, tiDelete, tiSwitch, tiCase, tiDefault,
    //debug
    tiAssert, tiTAssert, tiDebug, tiDebugLog,
    // punctuation
    tiLParen, tiRParen, tiLBrace, tiRBrace, tiLBrack, tiRBrack,
    tiSemiColon, tiComma, tiDot, tiArrow, tiAt, tiColon,
    tiMinus, tiPlus, tiMult, tiDiv, tiMod, tiAddr, tiAnd, tiOr, tiNot,
    // Comparison
    tiEq, tiNEq, tiLT, tiLTE, tiGT, tiGTE,
    // Assignment
    tiAssign, tiMinusAssign, tiPlusAssign, tiMultAssign, tiDivAssign, tiModAssign,
    tiXorAssign, tiBOrAssign, tiBAndAssign, tiSLAssign, tiSRAssign,
    tiInc, tiDec,
    // Bitwise
    tiBNot, tiXor, tiBOr, tiSL, tiSR,
    tiConst, tiEnum,
    // directives
    tiDefine, tiUndef, tiIfDef, tiPElse, tiEndIf, tiIfNDef,
    tiEnd
};

#define NUM_KEYWORDS 46
extern char* keyword[NUM_KEYWORDS];
extern TokenID keytoken[NUM_KEYWORDS];
#define NO_LINE -1

struct Token {
    TokenID id;
    char* value;
    long intVal;
    float floatVal;
    int line;
    int ch;
    int offset;
};

enum SymbolType {
    syGlobal, syLocal, syMember, syProperty, syConst, syEnum, syResource
};

struct Type {
    Type(): vtype(vtVoid), structID(0), indir(0) { }
    bool operator==(Type& o) {
        if (vtype == o.vtype && indir == o.indir) {
            if (vtype == vtStruct && structID != o.structID)
                return false;
            if (vtype == vtFuncPtr && structID != o.structID)
                return false;
            return true;
        }
        return false;
    }
    VarType vtype;
    int structID; // only valid if vtype == vtStruct/vtFuncPtr
    int indir;
    // @TODO: arrays will likely need to be first class types
    // for multidim. arrays to work
};

struct Variable {
    Variable() : offset(0), size(0), alen(0), stype(syGlobal),
        loc_get(-1), loc_set(-1), lib(-1), ndim(0), isInherited(false) {}
    string name;
    int offset; // from FP, GP, or beginning of struct
    Type type;
    int size; // 1 unless array or struct
    int alen; // for array, 0 otherwise
    int ndim; // 1-5 if array
    int dims[5];
    int loc_get, loc_set; // property locations
    int lib;	// library if property
    bool isInherited;
    SymbolType stype;
};

struct StructFunction {
    int funcID;
    bool isVirtual;
    string name;
};

// declaration of a structure
struct Struct {
    Struct() : size(0), hasInit(false), hasDestroy(false), hasCopy(false), objinfo(-1), marked(false) { }
    string name;
    int size;
    int objinfo;
    bool hasInit;
    bool hasDestroy;
    bool hasCopy;
    bool isObject;
    bool isIface;
    bool marked;
    int baseID;
    vector<int> bases;
    ovector<Variable> vars;
    ovector<StructFunction> funcs;
};

struct Function {
    Function() : nArgs(0), argSize(0), loc(-1), locSize(0),	lib(-1),
        builtin(false), marked(false), handler(false), structID(-1) { }
    string name;
    string methodName;
    int nArgs;
    int argSize;

    Type type; // return type info
    int loc; // location in app
    int lib;
    bool builtin;
    int structID;
    bool isVirtual;

    bool marked; // for call graph
    bool handler;
    int locSize;
    vector<byte> code;
    ovector<Variable> locals; // locals / args
};

struct Const {
    string name;
    Value val;
};

struct Macro {
    string name;
    string value;
};

struct Library {
    string name;
    bool marked;
    bool bPocketC;
};

struct TranslationEntry {
    string base;
    string translation;
    string compilerComment;
    string userComment;
    int order;
};

struct FunctionPtr {
    bool operator==(FunctionPtr& o) {
        if (!(type == o.type) || args.size() != o.args.size()) {
                return false;
        }
        for (int i=0;i<args.size();i++) {
            if (!(args[i].type == o.args[i].type))
                return false;
        }
        return true;
    }
    string name;
    Type type; // return type
    ovector<Variable> args;
    int firstMatch;
};

enum OpType {
    opAdd, opSub, opMult, opDiv, opMod,
    opLoad, opAssign, opGetProp, opSetProp,
    opBOr, opBAnd, opXOr, opBNot, opSL, opSR,
    opOr, opAnd,
    opLT, opGT, opLTE, opGTE, opEq, opNEq,
    opNot, opNeg, opIncB, opIncA, opDecB, opDecA,
    opCall, opFuncA, opSwap, opStackDup, opDiscard, opAlloc,
    opCInt, opCChar, opCString, opCFloat, opCWord, opCWordPFP,
    opSCInt, opSCChar, opSCString, opSCFloat, opSCWord,
    opStackInit,
    opItoC, opItoF, opItoS, opFtoI, opFtoC, opFtoS,
    opCtoI, opCtoF, opCtoS, opStoI, opStoF, opStoC,
    opVtoI, opVtoC, opVtoF, opVtoS, opVtoB,
    opStoB,
    opGetAt, opSetAt, opNew, opDelete,
    opNoOp, opNoOp2,
    opLoadI, opStoreI, opLoadF0, opStoreF0, opLoadF1, opStoreF1,
    opAddF0, opAddF1,
    opSStore, opMove, opSetReg, opGetReg, opCopy, opCallHandler,
    opCallVirt, opCallIface, opSObjInfo, opNewObj, opCompAddr
};

enum StmtType {
    stNone, stExpr, stIf, stWhile, stDo, stFor, stBreak, stContinue, stReturn, stArray,
    stProlog, stEpilog, stSwitch, stCase, stDefault, stDefReturn
};

class ExprNode {
public:
    ExprNode();
    ExprNode(OpType);
    ~ExprNode();

    OpType op;
    ExprNode* kids[3];
    ExprNode* next;

    // result
    Type type;
    Value val; // for constants
    int data;
    bool isKnownType; // this expression will not require a virtcall
};

class StmtNode {
public:
    StmtNode();
    StmtNode(StmtType);
    ~StmtNode();

    StmtType stmt;
    ExprNode* expr[3];
    StmtNode* kids[2];
    StmtNode* next;
    int data;

    void DeleteList();
};

class SwitchStmtNode : public StmtNode {
public:
    SwitchStmtNode() : def(NULL) { stmt = stSwitch; }
    vector<StmtNode*> casep;
    ovector<Value> vals;
    StmtNode* def;
};

struct StmtTree {
    StmtTree() : tree(NULL), lastNode(NULL) { }
    ~StmtTree() { Delete(); }
    void AddStmt(StmtNode*);
    void AddExprStmt(ExprNode*);
    void PrependStmt(StmtNode*);
    void PrependExprStmt(ExprNode*);
    void RemoveStmt(StmtNode*);
    void RemoveExprStmt(ExprNode*);
    void Delete();

    StmtNode* tree, *lastNode;
};

#include "source.h"

struct IncludeStack {
    int line;
    int startOfLine;
    Source* osource;
    char* src;
    char* isrc;
    char* name;
};

#include "resource.h"
#include "palmdb.h"

class CompError {
};

class ICompiler {
public:
    virtual void c_error(const char* err, int line = -2) = 0;
};

class ICompileStatus {
public:
    virtual void setStatus(const char* text, bool bFunc) = 0;
};

class Lex {
public:
    Lex();
    ~Lex();

    void push(const char* name, Source* osource);
    void pop();
    void init(ICompiler* pComp);
    void c_error(const char* err, int line = -2);
    
    // lexer
    string getAssertText();
    void nextToken(); // processes #defines
    void _nextToken(); // internal version
    VarType varType(TokenID id);
    VarType constVarType(TokenID id);
    char hval(char c);
    char ch() { return *source; }
    char nch() { return *source ? *(source+1) : '\0';}
    char gch() { return *source++; }
    char ugch() { return *--source; }
    void addLine();
    void blanks();
    void getString(char, char*);
    void addMacro(char* name, char* value);
    int findMacro(char* name);
    void delMacro(char* name);

    string FindFile(const char* name, bool bAddin = false, bool bProjectRelative = false);

    ovector<Macro> macros;
    Token tok; // current token
    char buff[4096]; // buffer for tok.value
    IncludeStack incStack[10];
    int incLevel;
    char* source;
    int currLine;
    int startOfLine;
    int iflevel, skiplevel;
    string strCompilerDir, strSourceDir;
    bool bPocketCCompat;
    ICompiler* comp;
};

class Compiler : ICompiler {
public:
    Compiler(ICompileStatus* pStatus = NULL);
    ~Compiler();

    bool Compile(char* srcfile, char* sysfile, char* destfile, bool bStandalone,
        bool bPocketC, char* dbgsyms, char* language, ErrorInfo& info);
#ifdef WIN32
    bool WriteProject(char* file, PalmProject& proj);
    bool ReadProject(char* file, PalmProject& proj, ErrorInfo& error);
    bool QuickParse(char* buff, bool bPocketCCompat, QuickParseInfo& info);
    void DumpFuncs();
    void GenDocXML(char* sysfile);
#endif

protected:
    // compiler phases
    bool Parse(const char* srcfile);
    bool Optimize();
    bool GenCode();
    bool HookUpHandlers();
    bool MergeFuncs();
    void GenObjInfo();
    void FixupAddresses();
    void CreateDemoCode();

    void GenDocFunc(Function&);
    void GenDocObject(Struct&);
    string GenDocType(Type);

    void c_error(const char* err, int line = -2);
    void c_error(const char* err, const char* filename, int line);
    ErrorInfo errorInfo;

    // lexer
    void nextToken();
    Token& tok;
    
    // parser
    bool doTop();
    bool doLibrary();
    bool doFunc(string name, Type type);
    bool doStructFunc(string name, Type type, int structID);
    bool doHandler();
    void doFuncDecl(string name, Type type, int structID, Function& func);
    bool doFuncBody(Function& func);
    bool compareFunc(Function&, Function&);
    Variable doDecl(string name, Type type, SymbolType symType, bool bArg, int& offset);
    Variable doDecl(Type type, SymbolType symType, bool bArg, int& offset);
    int doStruct(bool isObject, bool isIface);
    int doFuncPtr();
    bool doRes();
    void doArrayInit(bool expl, bool local, Type type, int alen);
    void doStructInit(bool expl, bool local, Type type);
    void doObjinfoInit(int structID, int alen, int addr);
    void doOneInit(bool expl, bool local, Type type);
    void doEnum();
    void doConst();
    void buildAutoFuncs(int structID);
    void buildCopyBody(int structID, StmtTree& tree, bool isAutoFunc);
    void buildInitBody(int structID, StmtTree& tree, bool isAutoFunc);
    void buildDestroyBody(int structID, StmtTree& tree, bool isAutoFunc);

    StmtNode* doStmts();
    StmtNode* doStmt();

    ExprNode* reqExpr();
    ExprNode* doExpr(); // returns null if no expression
    ExprNode* structToAddr(ExprNode*, bool makeRetRef = true); // "Load struct" is usually invalid
    // comma, condition, assign
    ExprNode* exprOr();
    ExprNode* exprAnd();
    ExprNode* exprBOr();
    ExprNode* doExprBOr(ExprNode*, bool bCompound);
    ExprNode* exprBXOr();
    ExprNode* doExprBXOr(ExprNode*, bool bCompound);
    ExprNode* exprBAnd();
    ExprNode* doExprBAnd(ExprNode*, bool bCompound);
    ExprNode* exprEq();
    ExprNode* exprRel();
    ExprNode* exprShift();
    ExprNode* doExprShift(TokenID, ExprNode*, bool bCompound);
    ExprNode* exprAdd();
    ExprNode* doExprAdd(TokenID, ExprNode*, bool bCompound);
    ExprNode* exprMult();
    ExprNode* doExprMult(TokenID, ExprNode*, bool bCompound);
    ExprNode* exprPrimary();
    ExprNode* exprPostPrimary(ExprNode*);
    ExprNode* exprElement();
    ExprNode* exprPostOp(ExprNode*);
    ExprNode* exprFunc(int funcID, bool isMethod);
    ExprNode* exprFuncPtr(int funcPtrID);
    ExprNode* exprVariantFuncPtr();
    ExprNode* buildSpecial(int structID, int funcID, int alen, ExprNode* addr, ExprNode* array = NULL, bool allowVirt = false);
    int makeTempLocal(int structID);

    void typeMatch(ExprNode**, ExprNode**);
    void cast(ExprNode**, VarType type);
    void fullCast(ExprNode**, Type type, bool expl);
    Type getType();
    string getTypeString(Type type, int alen);
    string getTypeString_struct(int sid);
    bool isType();
    void toBool(ExprNode**);
    void toInt(ExprNode**);
    void toPointerOp(ExprNode**, Type pointerType, bool bMult=true);
    void onlyNumeric(ExprNode*, bool allowPtr);
    void onlyIntegral(ExprNode*, bool allowPtr);
    void onlySimple(ExprNode*, bool allowPtr, bool allowFunc = false);
    int typeSize(Type);
    int curStruct;
    int curLibrary;
    int curLibraryFunc;
    vector<int> prevStruct; // TODO: get rid of this
    ExprNode* objContext;
    SwitchStmtNode* curSwitch;

    int addLibrary(string name, bool bPocketC);
#ifdef WIN32
    void addLibraryContents(string name, PalmDB& db, int id);
#endif
    ovector<Library> libraries;

    void match(TokenID, char* err = NULL);
    void check(TokenID, char* err = NULL);
    void checkInt(char* err = NULL);
    int matchInt();

    // symbols
    int findFunc(string name);
    int findStruct(string name);
    int findLocal(string name);
    int findGlobal(string name);
    int findRes(string name);
    bool hasGadget(int structID);
    int findConst(string name);
    int findStructFunc(int structID, string name);
    int findStructVar(int structID, string name);
    int findFuncPtr(string name);
    int findFuncPtrMatch(Function& func);
    int buildFuncWrapper(int fid);
    bool isBaseType(int derivedID, int baseID);
    void getInterfaces(int structID, vector<int>& ifaces);
    bool findInterfaceOffset(int structID, int ifaceID, int init_offset, int& res_offset);

    // resource parser
    bool doResApp();
    bool doResForm();
    bool doResControl(ResType, PalmForm&);
    bool doResMenuBar(PalmForm&);
    bool doResMenu(PalmMenuBar&);
    bool doResMenuItem(PalmMenu&);
    bool doResBitmap(bool external);
    bool doResIcon();
    bool doResString();
    bool doResResource();
    int addResSymbol(ResType, int structID, string, int);
    ResType getResType();
    PropType getPropType();
    void getResNameType(ResType rt, string& name, int& sid);
    void getResNameType(ResType rt, string& name, string& type, int& sid);
    bool bProjectRead;

#ifdef WIN32
    // project writer
    bool WriteApp(FILE*, PalmApp& app);
    bool WriteForm(FILE*, PalmForm& form);
    bool WriteMenuBar(FILE*, PalmMenuBar& bar);
    bool WriteMenu(FILE*, PalmMenu& menu);
    bool WriteMenuItem(FILE*, PalmMenuItem& item);
    bool WriteControl(FILE*, PalmControl& ctrl);
#endif

    // optimizer
    void optConsts(ExprNode*&);
    void optConst(ExprNode*&);
    void optStmts(StmtNode*&);
    void optStmt(StmtNode*&);
    void optExprs(ExprNode*&);
    void optExpr(ExprNode*&);
    void optMinimizeConsts(ExprNode*&); // CInt -> CWord
    void optMinimizeConst(ExprNode*&); // CInt -> CWord
    void optAddrModes(ExprNode*&); // Load CWord -> LoadGlobal
    void optAddrMode(ExprNode*&); // Load CWord -> LoadGlobal

    // code gen
    int addString(const char*);
    int addString(string& str) { return addString(str.c_str()); }
    void addByte(byte);
    void addWord(int);
    void addLong(int);
    bool genStmts(StmtNode*); // gen a statement and siblings
    bool genExprs(ExprNode*); // gen an expression and siblings
    bool genStmt(StmtNode*);
    bool genExpr(ExprNode*);
    bool genBinary(VMsym, ExprNode*, ExprNode*);
    bool genVariantBinary(VMsym, ExprNode*, ExprNode*);
    bool genUnary(VMsym, ExprNode*);
    bool genVariantUnary(VMsym, ExprNode*);
    void genAssign(ExprNode*);
    void genCall(ExprNode*);
    void genFuncCall(int funcID);
    void genDiscard(ExprNode*);
    void genAlloc(ExprNode*);
    void emitExt(VMsym, OperandType a, OperandType b, OperandType res, int av=0, int bv=0, int rv=0);
    bool processMarkedFunc(vector<byte>& funcCode, string name);
    bool processMarkedObject(int objID);
    void writeCode(vector<byte>& code, string name);
    int addrLastAssign;
    bool bLastAssignLocal;
    int l_break, l_cont, l_funcend;
    int getLabel();
    void setLabel(int);
    bool fixLabels();
    vector<int> labels;

    bool copyResFromPRC(PalmDB& db, string type, int id, string res, string& error);
    bool copyAllResFromPRC(PalmDB& db, string res, string& error);
    bool genApp(PalmDB& db, int& osver);
    bool genForms(PalmDB& db, int& osver);
    bool genMenus(PalmDB& db);
    bool genUI(PalmDB& db);
    void addHandler(PalmDBStream& db, long h);
    void setupHandler(const char* objname, const char* eventname,
                      int sID, int gsID, bool isGadget, long* pHandler);

    // symbol tables
    bool isGlobSym(string name);
    ovector<Variable> globals;
    ovector<Function> funcs;
    ovector<Struct> structs;
    ovector<Const> consts;
    ovector<FunctionPtr> funcPtrs;
    Function* curFunc;
    string stackString;
    int stackOffset;
    StmtTree funcTree;
    StmtTree funcInitTree;
    StmtTree funcDestTree;
    StmtTree globInitTree;
    StmtTree globDestTree;
    int globOffset;
    bool bDebug;

    // output tables
    dbvector<char> strings;
    int iGlobInitLoc, iGlobDestLoc;
    int codeSize;
    dbvector<byte>* codeSegments[MAX_CODE_SEGMENTS];
    vector<byte> objinfo;
    ovector<GlobalInit> globInits;
    ovector<VarType> globTypes;

    // resources
    ovector<ResSymbol> resSyms;
    PalmApp app;

#ifdef WIN32
    // translation
    void loadTranslation(const char* filename);
    void writeTranslation();
    string translateString(const char*); // uses file @ line
    string translateString(const char*, const char* comment);
    string translateString(const char* str, const char* name, const char* comment);
    
    string translationFile;
    map<string, TranslationEntry> translationTable;
    int nUsedTranslationEntries;
#else
    string translateString(const char* s) { return string(s); }
    string translateString(const char* s, const char*) { return string(s); }
    string translateString(const char* s, const char*, const char*) { return string(s); }
#endif

#ifdef WIN32
    ovector<string> post_build;
#endif

    // utility
    Source* GetFile(const char* name, bool bThrowError = true, int* plen = NULL);
    ICompileStatus* pStatus;
    bool bInSysFile;
    bool bPocketCCompat;
    Lex lex;
};
