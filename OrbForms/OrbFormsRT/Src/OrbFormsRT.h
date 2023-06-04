#include <PalmOS.h>
#include <PalmChars.h>
#include <SonyCLIE.h>

void Alert(const char* msg);
//#define DEBUG
#ifdef DEBUG
#define assert(x) { if (!(x)) { ErrDisplayFileLineMsg(__FILE__, __LINE__, #x); } }
#else
#define assert(x)
#endif

#define FAST_STACK

#define byref

#include "stdcover.h"
#include "..\..\compiler\orbforms.h"
#include "vector.h"
#include "heap.h"
#include "vm.h"
#include "palmlib.h"
#include "MathLib.h"

#define ver30 sysMakeROMVersion(3,0,0,sysROMStageRelease,0)
#define ver31 sysMakeROMVersion(3,1,0,sysROMStageRelease,0)
#define ver35 sysMakeROMVersion(3,5,0,sysROMStageRelease,0)
#define ver40 sysMakeROMVersion(4,0,0,sysROMStageRelease,0)
#define ver50 sysMakeROMVersion(5,0,0,sysROMStageRelease,0)

#define appVersionNum 0x0413

extern const char* g_appArgs;
extern int nCards;
extern UInt32 romVersion;
extern bool bHighDensity;
extern UInt32 density;
extern bool hideSecrets;
extern bool hasUI;
extern bool killApp;
#ifdef DEBUG
extern bool bInOpen;
#endif
extern bool bHookHard;
extern char g_buff[];
extern word parentForm;
void UIYield();
void * GetObjectPtr(UInt16 objectID);

enum ResType {
    rtForm=0, rtLabel, rtField, rtButton, rtPushButton, rtRepeatButton, rtCheckbox, rtPopup,
    rtList, rtSelector, rtBitmap, rtGraffiti, rtScroll, rtGadget, rtMenu, rtApp,
    rtString, rtMenuItem, rtIcon, rtMenuBar, rtSlider,
    rtLastResType
};

struct OApp {
    dword handlers[4];
    dword addr;
};

struct OControl {
    word id;
    word fid;
    dword addr;
    dword offset;
    ResType type;
    bool visible;
    bool enabled;
    word x, y, w, h;
    word ox, oy, ow, oh;
    dword handlers[6];
    void* pdata;
    word ldata; // length for UILabel (incl. '\0')
    word flags;
};

#define OFORM_RESIZABLE 0x0010
#define RESIZE_X_MASK 0x0003
#define RESIZE_X_ATTACH_LEFT 0x0000
#define RESIZE_X_ATTACH_RIGHT 0x0001
#define RESIZE_X_STRETCH 0x0002
#define RESIZE_Y_MASK 0x000C
#define RESIZE_Y_ATTACH_TOP 0x0000
#define RESIZE_Y_ATTACH_BOTTOM 0x0004
#define RESIZE_Y_STRETCH 0x0008
#define RESIZE_STRETCH (RESIZE_X_STRETCH | RESIZE_Y_STRETCH)

struct OForm {
    word id;
    dword addr;
    dword offset;
    word firstCtrl;
    word nCtrls;
    word x, y, w, h;
    word ox, oy, ow, oh;
    dword handlers[10];
    void* pdata;
    word ldata; // title length (incl. '\0')
    word flags;
};

class Timer {
public:
    Timer();
    ~Timer();
    void SetTimer(word id, dword dtime);
    void KillTimer(word id);
    dword NextTimer(word id);
private:
    int FindID(word id);
    struct FormTimer {
        word id;
        dword time;
    };
    vector<FormTimer> timers;
};
    
struct OMenuItem {
    word id;
    dword addr;
    dword handler;
};

struct OEvent {
    word x, y;
    word nx, ny;
    bool inside;
    word key;
    word value, prev;
    dword code;
};

extern VM* vm;
extern word appVer;
extern OApp app;
extern vector<OForm> forms;
extern vector<OControl> ctrls;
extern vector<OMenuItem> menuItems;
extern OEvent curEvent;
extern Timer timer;
extern NativePile* ppile;

int FindForm(word);
int FindCtrl(word);
int FindActiveCtrl(word);
int FindMenuItem(word);

//FormPtr CreateForm(int index);
//void AddControl(FormPtr&, dword& offset, int iCtrl);
bool CallHandler(dword addr, dword handler);
void CloseOrbForm(word id, bool bDelete, bool bCleanup);
void CloseAllOrbForms();
void LibStart();
void LibStop();
LocalID MyFindDatabase(char* name, word& card);

// Library helper funcions
word popID(VM* vm);

// Native plug-in
struct OrbFormsInterface;

#define NATIVE_STARTUP 0xffff
#define NATIVE_SHUTDOWN 0xfffe
typedef void* (*NATIVE_ENTRY_FUNC)(OrbFormsInterface*, void* pData, UInt16 iFunc);
typedef void (*NOBJ_DELETE_FUNC)(OrbFormsInterface*, void* pData, void* pObj);
typedef bool (*NATIVE_EVENT_FUNC)(OrbFormsInterface*, void* pData, EventType* pEvent);
typedef bool (*NATIVE_TS_ITER)(OrbFormsInterface*, Value*, VarType, int, void*); // return true to continue iterating

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

    // Native Object interface
    void* (*nobj_pop)();
    void* (*nobj_get_data)(long id);
    long  (*nobj_create)(void* obj, NOBJ_DELETE_FUNC delfunc);
    long  (*nobj_addref)(long id);
    long  (*nobj_delref)(long id); // may call delete
    
    // Gadget interface
    void* (*gadget_get_data)(long id);
    void  (*gadget_set_data)(long id, void*);
    void  (*gadget_get_bounds)(long id, int* pX, int* pY, int* pW, int* pH);
    
    // System event functions
    bool  (*event_reg_func)(NATIVE_EVENT_FUNC eventfunc);
    bool  (*event_unreg_func)(NATIVE_EVENT_FUNC eventfunc);
    
    // Type string processing
    long  (*ts_iterate)(long addr, char* strFormat, int count, NATIVE_TS_ITER func, void* pContext);
    long  (*ts_data_size)(long addr, char* strFormat, int count);
};

struct OAddIn {
    NATIVE_ENTRY_FUNC func;
    void* data;
    MemHandle hAddIn;
    DmOpenRef dbRef;
    bool bInit;
    bool bPocketC;
    bool bNewPocketC;
    word libRef;
};

struct OEventFunc {
    NATIVE_EVENT_FUNC func;
    void* data;
};

extern vector<OAddIn> addIns;
extern vector<OEventFunc> eventFuncs;
extern OrbFormsInterface ofi;
bool LoadAddIn(char* name, int id);
void InitOFI();

class AddInObject : public NativeObject {
public:
    ~AddInObject();
    NOBJ_DELETE_FUNC delFunc;
    void* data;
    void* aidata;
};
extern void* current_addin_data;

//
// resize support
//

typedef enum {
    DIA_TYPE_NONE = 0,
    DIA_TYPE_SONY2,
    DIA_TYPE_PALM10,
    DIA_TYPE_PALM11,
    DIA_TYPE_UNKNOWN
} DIAtype;

typedef enum {
    DIA_STATE_MAX = 0,
    DIA_STATE_MIN,
    DIA_STATE_NO_STATUS_BAR,
    DIA_STATE_USER,
    DIA_STATE_UNDEFINED
} DIAState;

DIAtype InitDIA(void);
void CloseDIA(void);
void SetFormDIAPolicy(UInt16 formID);
bool HandleResizeNotification(UInt32 notificationType);
bool LayoutForm(word formID);
void InitFormDIA(word formID);
bool LayoutStandardForm(word formID);
void InitStandardFormDIA(word formID);
void SetDIAAllowResize(bool);

void SetDIAState(DIAState state);
DIAState GetDIAState();
bool GetDIARotation();
void SetDIARotation(bool portrait);


// PocketC eventing
struct EventItem {
    short type;
    short v1, v2;
};

class EventQueue {
public:
    enum { eqNone, eqKey, eqPenDown, eqPenUp, eqPenMove, eqUp, eqDown,
        eqHK1, eqHK2, eqHK3, eqHK4, eqMenu, eqHome, eqFind, eqCalc,
        eqHotSync, eqLeft, eqRight, eqSelect, eqUndefined,
        eqScan, eqLowBatt, eqTrigger, eqUpdate, eqResize };
    void AddEvent(short type, short v1 = 0, short v2 = 0);
    bool PopEvent();
    void Clear();
    void RemoveInput();
    short type, v1, v2;
private:
    enum { maxEvents = 10 };
    EventItem events[maxEvents];
    short nEvents;
};

extern bool nativeEnableEvents, enableResize;
bool PocketCYield(long);

extern MemHandle hOutput;
word LoadPocketCLib(char* name, bool& bNewPocketC);
void ClosePocketCLib(word libRef);
void ExecutePocketCLibFunc(bool bNewPocketC, word libRef, word iFunc);
void pc_valOldPCToOrb(Value& val);
