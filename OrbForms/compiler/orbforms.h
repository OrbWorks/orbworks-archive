typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;
#define byref

#define PROLOG_SIZE 3

#define MAX_CODE_SEGMENTS 16
#define CODE_SEGMENT_SIZE 0x8000L
#define CODE_SEGMENT_SHIFT 15

enum OperandType {
    otNone, otStack, otImmed, otAddr
    //otStack, otImmed, otAddr, otAddrOff, otNone // otDAddr EXT OP ADDR OFF Load
};

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
    vmJmpPopZ, vmJmpPopNZ,
    vmCall, vmCallI, vmCallBI, vmCallHandler,
    vmRet, vmRetN, vmSetRet, vmSetRet0,
    vmPop, vmPopN, vmSwap, vmStackDup, vmAlloc, vmAllocT, vmNew, vmDelete,
    vmLink, vmUnLink, vmHalt, vmLibCall,

    vmCWord, vmCWordPFP,
    vmLoad, vmStore, vmCopy, vmSet, vmGet,
    vmLoadI, vmLoadFP, vmLoadF0, vmLoadF1,
    vmStoreI, vmStoreFP, vmStoreF0, vmStoreF1,
    vmIncAI, vmIncAC, vmIncAF, vmIncAS, vmDecAI, vmDecAC, vmDecAF, vmDecAS, 
    vmIncBI, vmIncBC, vmIncBF, vmIncBS, vmDecBI, vmDecBC, vmDecBF, vmDecBS,
    vmIncAII, vmDecAII, vmIncBII, vmDecBII,
    // bitwise
    vmBAndI, vmBAndC, vmBOrI, vmBOrC, vmBNotI, vmBNotC, vmSLI, vmSLC, vmSRI, vmSRC, vmXorI, vmXorC,
    vmNoOp, vmNoOp2, /*vmCallF,*/ vmFuncA,
    vmSwitchI, vmSwitchC, vmSwitchF, vmSwitchS, vmCase, vmDefault,
    vmGetAt, vmSetAt,
    // stack init
    vmStackInit, vmSCInt, vmSCChar, vmSCFloat, vmSCString, vmSCWord,
    vmSetReg, vmGetReg, vmMove, // vmMoveNF, vmMoveFN, vmMoveFF
    vmCallStubBI, vmLibStubCall,

    // inheritance
    vmCallVirt, vmCallIface, vmNewObj, vmSObjInfo,
    // variant types
    vmVtoI, vmVtoC, vmVtoF, vmVtoS, vmVtoB, vmVOp1, vmVOp2,
    vmStoreV,

    vmCompAddr,

    vmNumInstr
};

extern int bcsize[vmNumInstr];

#ifdef _WINDOWS_
typedef byte VarType;
enum _VarType {
#else
enum VarType {
#endif
    vtInt, vtChar, vtFloat, vtString, vtVariant, vtVoid, vtStruct, vtFuncPtr,
    vtRetAddr, vtSavedFP, vtSavedFB, vtHandler, vtObjInfo
};

#ifdef _WINDOWS_
typedef byte StringType;
enum _StringType {
#else
enum StringType {
#endif
    stConst, stShared
};

#ifdef _WINDOWS_
struct Value {
    VarType type;
    StringType stype;
    Value() : type(vtInt), iVal(0) {}
    union { 
        long iVal;
        float fVal;
        char* sVal;
        char cVal;
    };
};
#else
struct OrbString {
    OrbString() : ref(0) {}
    short ref;
    char str[1];
};

struct Value {
    Value() : type(vtInt), iVal(0), stype(stConst) {}
    VarType type;
    StringType stype;
    union { 
        long iVal;
        float fVal;
        void* sVal;
        struct {
        char pad[3];
        char _cVal;
        } _;
    };
    char* lock();
    void unlock();
    void release();
    void acquire(Value*);
    void unlockRelease();
    char* newString(int len);
    char* newConstString(const char*);
    bool copyString(const char*);
    bool makeEditable();
};
#define cVal _._cVal
#endif

struct GlobalInit {
    long offset;
    Value val;
};
