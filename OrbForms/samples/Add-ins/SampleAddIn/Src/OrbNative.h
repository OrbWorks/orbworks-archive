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
        struct {
        char pad[3];
        char _cVal;
        } _;
    };
};
#define cVal _._cVal

struct OrbFormsInterface;

#define NATIVE_STARTUP 0xffff
#define NATIVE_SHUTDOWN 0xfffe
typedef void* (*NATIVE_ENTRY_FUNC)(OrbFormsInterface*, void* data, UInt16 iFunc);
typedef void (*NOBJ_DELETE_FUNC)(OrbFormsInterface*, void* data, void* obj);
typedef bool (*NATIVE_EVENT_FUNC)(OrbFormsInterface*, void* data, EventType* event);
typedef bool (*NATIVE_TS_ITER)(OrbFormsInterface*, Value* val, VarType type, int size, void* context);

struct OrbFormsInterface {
    UInt16 version;
    // VM interface
    Value* (*vm_pop)();
    void   (*vm_push)(Value*); // moves the value - ownership is stack
    Value* vm_retVal;
    void   (*vm_error)(char*); // throws
    Value* (*vm_deref)(long ptr); // may throw
    void   (*vm_call)(long ptr); // modifies stack
    void   (*vm_callBI)(long index); // modifies stack
    
    // Value interface
    void  (*val_release)(Value*); // cleanup a string
    void  (*val_acquire)(Value*, Value*);
    char* (*val_lock)(Value*);
    void  (*val_unlock)(Value*);
    void  (*val_unlockRelease)(Value*);
    char* (*val_newString)(Value*, int len);
    char* (*val_newConstString)(Value*, const char*);
    bool  (*val_copyString)(Value*, const char*);

    // Native struct interface
    void* (*nobj_pop)();
    void* (*nobj_get_data)(long id);
    long  (*nobj_create)(void* obj, NOBJ_DELETE_FUNC delfunc);
    long  (*nobj_addref)(long id);
    long  (*nobj_delref)(long id); // may call delete

    // Gadget interface
    void* (*gadget_get_data)(long id);
    void  (*gadget_set_data)(long id, void*);
    void  (*gadget_get_bounds)(long id, int* pX, int* pY, int* pW, int* pH);

    // System event interface
    bool  (*event_reg_func)(NATIVE_EVENT_FUNC eventfunc);
    bool  (*event_unreg_func)(NATIVE_EVENT_FUNC eventfunc);
    
    // Type string processing
    long  (*ts_iterate)(long addr, char* strFormat, int count, NATIVE_TS_ITER func, void* pContext);
    long  (*ts_data_size)(long addr, char* strFormat, int count);
};

