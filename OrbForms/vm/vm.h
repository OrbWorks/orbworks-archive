#ifdef __MWERKS__
#include <Palm.h>
#else

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <assert.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "..\compiler\orbforms.h"
#include <vector>
using namespace std;
#include "heap.h"

class VM {
public:
    VM();
    ~VM();

    bool Load(char* file);
    bool Run();
    bool Call(int addr, bool bPushRet);
    bool killVM;

    void setObjinfo(int addr, int obj_id);
    Value* deref(int addr);
    int decodeWord(int addr);
    void cleanUp(Value* pv) {
        if (pv->type == vtString && pv->sVal) {
            free(pv->sVal);
            pv->sVal = NULL;
        }
    }
    void vm_error(char* err);
    void copyVal(Value* dst, Value* src);
    void moveVal(Value* dst, Value* src);
    void push(Value*);
    Value* pop();
    void pushTypes(char*);
    int getWord();
    int getLong();
    void beforePush(Value*, Value*, OperandType, OperandType);
    void typeMatch(Value*, Value*);
    void typeCast(Value*, VarType);

    Value retVal;

    long getVirtFunc(int objID, int funcID);
    long getIfaceFunc(int objID, int ifaceID, int funcID);
    long getObjectSize(int objID);

//protected:
    byte* code;
    byte* objinfo;
    char* str;
    Heap globs;
    int gsize;
    int csize;
    int ssize;
    Value* stack;
    int pc;
    int st;
    int fp;
    int fb;
    int iGlobInit, iGlobDest;
    int reg[10];
};

typedef void (*builtin_func)(VM*, int);

struct BuiltinFunc {
    builtin_func func;
    bool hasRet;
};

extern BuiltinFunc bifuncs[];
extern int nbifuncs;
