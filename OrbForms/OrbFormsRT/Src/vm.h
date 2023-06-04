class VM {
public:
    VM();
    ~VM();

    bool Load();
    void Unload();
    bool Call(long addr, bool pushRet, bool yield);
    bool killVM;
    HostFILE* hostFile;

    long GetFullPC();
    void SetPC(long pc);
    byte GetCodeByte(long addr);
    
    void setObjinfo(long addr, long objID);
    Value* deref(long addr);
    long decodeWord(int addr);
    void cleanUp(Value* pv) {
        if (pv->type == vtString && pv->sVal) {
            //free(pv->sVal);
            //pv->sVal = NULL;
            pv->release();
        }
    }
    void vm_error(const char* err, bool bAssert = false);
    char* stackDump(int depth);
    void copyVal(Value* dst, Value* src);
#ifdef DEBUG
    void moveVal(Value* dst, Value* src);
#else
    void moveVal(Value* dst, Value* src) {
        long* pd = (long*)dst;
        long* ps = (long*)src;
        *pd++ = *ps++;
        *(short*)pd = *(short*)ps;
    }
#endif
    void push(Value*);
    Value* pop();
    int getWord();
    long getLong();
#ifdef DEBUG
    void beforePush1(Value*, OperandType);
    void beforePushS1(Value*, OperandType);
    void beforePush2(Value*, Value*, OperandType, OperandType);
    void beforePushS2(Value*, Value*, OperandType, OperandType);
#else
    void beforePush1(Value* a, OperandType ota) {
        if (ota == otStack) pop();
    }
    void beforePushS1(Value* a, OperandType ota) {
        if (ota == otStack) cleanUp(pop());
    }
    void beforePush2(Value* a, Value* b, OperandType ota, OperandType otb) {
        if (otb == otStack) pop();
        if (ota == otStack) pop();
    }
    void beforePushS2(Value* a, Value* b, OperandType ota, OperandType otb) {
        if (otb == otStack) cleanUp(pop());
        if (ota == otStack) cleanUp(pop());
    }
#endif
    void typeMatch(Value*, Value*);
    void typeCast(Value*, VarType);

    Value retVal;

    long getVirtFunc(long objID, long funcID);
    long getIfaceFunc(long objID, long ifaceID, long funcID);
    long getObjectSize(long objID);

//protected: // this is merely a suggestion
    byte* code;
    byte* codeSegments[MAX_CODE_SEGMENTS];
    byte* objinfo;
    char* str;
    //vector<Value> globs;
    Heap globs;
    long gsize;
    long csize;
    long ssize;
    long oisize;
    vector<Value> stack;
    Value* stack_top;
    long pc;
    long segmentStart;
    long st;
    long fp;
    long fb;
    long reg[10];
    long callDepth;
    
    // database handles
    MemHandle hString, hObjInfo;
    MemHandle hCodeSegments[MAX_CODE_SEGMENTS];
};

typedef void (*builtin_func)(VM*, int);

struct BuiltinFunc {
    builtin_func func;
    bool hasRet;
};

extern BuiltinFunc bifuncs[];
extern int nbifuncs;

class InfoStream {
public:
    InfoStream() : data(0), iter(0) { }
    InfoStream(const byte* _data) : data(_data), iter(_data) { }
    void init(const byte* _data) {
        data = _data;
        iter = _data;
    }
    byte getByte() {
        return *iter++;
    }
    short getWord() {
        short s = ((long)*iter++ << 8);
        s |= *iter++;
        return s;
    }
    long getLong() {
        long l = ((long)*iter++ << 24);
        l |= ((long)*iter++ << 16);
        l |= ((long)*iter++ << 8);
        l |= *iter++;
        return l;
    }
    long getOffset() {
        return (iter - data);
    }
    void skip(long len) {
        iter += len;
    }
private:
    const byte* data, * iter;
};
        
