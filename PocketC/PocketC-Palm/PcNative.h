// Native add-in

// the types of values - some of these are only used at compile time
enum VarType {
    vtInt, vtChar, vtFloat, vtString, vtVoid, vtStruct, vtFuncPtr,
    vtRetAddr, vtSavedFP, vtSavedFB, vtHandler
};
// type of a string
enum StringType {
    stConst, stShared
};

struct Value {
    VarType type;
    StringType stype;
    union { 
        long iVal;
        float fVal;
        void* sVal;
        char cVal;
        //struct {
        //char pad[3];
        //char _cVal;
        //} _;
    };
};
//#define cVal _._cVal

struct PocketCInterface;

#define NATIVE_STARTUP 0xffff
#define NATIVE_SHUTDOWN 0xfffe
typedef void* (*NATIVE_ENTRY_FUNC)(PocketCInterface*, void* pData, UInt16 iFunc);
typedef bool (*NATIVE_TS_ITER)(PocketCInterface*, Value*, VarType, int, void*); // return true to continue iterating

struct PocketCInterface {
    UInt16 version;
    // VM interface
    Value* (*vm_pop)();
    void   (*vm_push)(Value*); // moves the value - ownership is stack
    Value* vm_retVal;
    void   (*vm_error)(char*); // throws
    Value* (*vm_deref)(long ptr); // may throw
    void   (*vm_call)(long ptr); // modifies stack
    bool   (*vm_callBI)(char* name); // modifies stack
    
    // Value interface
    void  (*val_release)(Value*); // cleanup a string
    void  (*val_acquire)(Value*, Value*);
    char* (*val_lock)(Value*);
    void  (*val_unlock)(Value*);
    void  (*val_unlockRelease)(Value*);
    char* (*val_newString)(Value*, int len);
    char* (*val_newConstString)(Value*, const char*);
    bool  (*val_copyString)(Value*, const char*);

    // Type string processing
    long  (*ts_iterate)(long addr, char* strFormat, int count, NATIVE_TS_ITER func, void* pContext);
    long  (*ts_data_size)(long addr, char* strFormat, int count);
};
