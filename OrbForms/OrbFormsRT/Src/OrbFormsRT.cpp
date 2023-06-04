#include "OrbFormsRT.h"
#include "OrbFormsRTRsc.h"


/***********************************************************************
 *
 *   Entry Points
 *
 ***********************************************************************/



/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
//static Boolean HideSecretRecords;
VM* vm = NULL;
//char assertBuff[512];
int nCards;
bool hasUI = false;
bool bForceExit = true;
bool killApp = false;
bool bHookHard = false;
bool nativeEnableEvents = true, enableResize = false;
byte* uiStream;
char g_buff[256];

word parentForm = 0;
word appVer = 0;

OApp app;
vector<OForm> forms;
vector<OControl> ctrls;
vector<OMenuItem> menuItems;
Timer timer;

/***********************************************************************
 *
 *   Internal Constants
 *
 ***********************************************************************/
#define appFileCreator             'OrbF'

// Define the minimum OS version we support (3.0 for now).
#define ourMinVersion	sysMakeROMVersion(3,0,0,sysROMStageRelease,0)

/***********************************************************************
 *
 *   Internal Functions
 *
 ***********************************************************************/
bool AppEventLoop(bool bRuntime, bool bBlocking);

void Alert(const char* msg) {
    FrmCustomAlert(MessageAlert, msg, NULL, NULL);
}

void* operator new (unsigned long size) {
    void* v = MemPtrNew(size);
    assert(v);
    return v;
}

void* operator new[] (unsigned long size) {
    void* v = MemPtrNew(size);
    assert(v);
    return v;
}

void operator delete (void * ptr) {
    MemChunkFree(ptr);
}

/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
    UInt32 romVersion;

    // See if we're on in minimum required version of the ROM or later.
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    if (romVersion < requiredVersion)
        {
        if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
            {
            FrmAlert (RomIncompatibleAlert);
        
            // Palm OS 1.0 will continuously relaunch this app unless we switch to 
            // another safe one.
            if (romVersion < ourMinVersion)
                {
                AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
                }
            }
        
        return sysErrRomIncompatible;
        }

    return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    GetObjectPtr
 *
 * DESCRIPTION: This routine returns a pointer to an object in the current
 *              form.
 *
 * PARAMETERS:  formId - id of the form to display
 *
 * RETURNED:    void *
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
void * GetObjectPtr(UInt16 objectID)
{
    FormPtr pForm;

    pForm = FrmGetActiveForm();
    return FrmGetObjectPtr(pForm, FrmGetObjectIndex(pForm, objectID));
}

void oom() {
    if (vm)
        vm->vm_error("Out of memory");
    ErrThrow(1);
}


Timer::Timer() { timers.init(5); }
Timer::~Timer() { }

void Timer::SetTimer(word id, dword dtime) {
    int index = FindID(id);
    dword time = TimGetTicks() + dtime;
    if (index != -1)
        timers[index].time = time;
    else {
        FormTimer ft = { id, time };
        timers.add(ft);
    }
}

void Timer::KillTimer(word id) {
    int index = FindID(id);
    if (index != -1)
        timers[index].time = -1;
}

dword Timer::NextTimer(word id) {
    int index = FindID(id);
    dword dtime = -1;
    if (index != -1 && timers[index].time != -1) {
        dtime = timers[index].time;
        dword time = TimGetTicks();
        if (time > dtime)
            dtime = 0;
        else
            dtime-= time;
    }
    return dtime;
}

int Timer::FindID(word id) {
    for (int i=0;i<timers.size();i++)
        if (timers[i].id == id) return i;
    return -1;
}

const int nResHandlers[rtLastResType] = {
    10, // rtForm
    0, // rtLabel
    2, // rtField
    1, // rtButton
    1, // rtPushButton
    1, // rtRepeatButton
    1, // rtCheckbox
    2, // rtPopup
    1, // rtList
    1, // rtSelector
    0, // rtBitmap
    0, // rtGraffiti
    2, // rtScroll
    6, // rtGadget
    0, // rtMenu
    4, // rtApp
    0, // rtString
    1, // rtMenuItem
    0, // rtIcon
    0, // rtMenuBar
    2, // slider
};

/*
void LoadUI() {
    int i,j,m;
    InfoStream info(uiStream);
    app.addr = info.getWord();
    for (i=0;i<nResHandlers[rtApp];i++)
        app.handlers[i] = info.getLong();
    int nForms = info.getWord();
    int nCtrls = info.getWord();
    int nMenuItems = info.getWord();
    
    // load menu items
    menuItems.init(nMenuItems);
    menuItems.lock();
    for (i=0;i<nMenuItems;i++) {
        OMenuItem item;
        item.id = info.getWord();
        item.addr = info.getWord();
        item.handler = info.getLong();
        menuItems.add(item);
    }
    
    // load forms
    forms.init(nForms); forms.lock();
    ctrls.init(nCtrls); ctrls.lock();
    for (i=0;i<nForms;i++) {
        OForm form;
        form.offset = info.getOffset();
        form.id = info.getWord();
        form.addr = info.getWord();
        form.pdata = NULL;
        form.ldata = 0;
        info.skip(19);
        int nFormCtrls = info.getByte();
        form.nCtrls = nFormCtrls;
        form.firstCtrl = ctrls.size();
        
        for (m=0;m<nResHandlers[rtForm];m++)
            form.handlers[m] = info.getLong();
        forms.add(form);
        
        for (j=0;j<nFormCtrls;j++) {
            ResType rt = (ResType)info.getByte();
            OControl ctrl;
            ctrl.id = info.getWord();
            ctrl.fid = form.id;
            ctrl.addr = info.getWord();
            ctrl.type = rt;
            ctrl.pdata = NULL;
            ctrl.ldata = 0;
            info.skip(4); // x, y
            switch (rt) {
                case rtLabel: info.skip(6); break;
                case rtBitmap: info.skip(2); break;
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                    info.skip(17);
                    break;
                case rtField: info.skip(10); break;
                case rtGraffiti: break;
                case rtGadget: info.skip(4); break;
                case rtPopup: info.skip(13); break;
                case rtSelector: info.skip(11); break;
                case rtScroll: info.skip(10); break;
                case rtSlider: info.skip(15); break;
                case rtList: info.skip(9); break;
                default:
                    assert(!"Unexpected resource type - LoadUI");
            } 
            for (m=0;m<nResHandlers[rt];m++)
                ctrl.handlers[m] = info.getLong();
            ctrls.add(ctrl);
        }
    }
}
*/

void LoadUI() {
    int i,j,m;
    InfoStream info(uiStream);
    app.addr = info.getWord();
    for (i=0;i<nResHandlers[rtApp];i++)
        app.handlers[i] = info.getLong();
    int nForms = info.getWord();
    int nCtrls = info.getWord();
    int nMenuItems = info.getWord();
    
    // load menu items
    menuItems.init(nMenuItems);
    menuItems.lock();
    for (i=0;i<nMenuItems;i++) {
        OMenuItem item;
        item.id = info.getWord();
        item.addr = info.getWord();
        item.handler = info.getLong();
        menuItems.add(item);
    }
    
    // load forms
    forms.init(nForms); forms.lock();
    ctrls.init(nCtrls); ctrls.lock();
    for (i=0;i<nForms;i++) {
        OForm form = {0};
        form.offset = info.getOffset();
        form.id = info.getWord();
        form.addr = info.getWord();
        form.x = form.ox = info.getWord();
        form.y = form.oy = info.getWord();
        form.w = form.ow = info.getWord();
        form.h = form.oh = info.getWord();

        int nFormCtrls = info.getWord();
        form.nCtrls = nFormCtrls;
        form.firstCtrl = ctrls.size();
        form.pdata = NULL;
        form.ldata = info.getWord();
        if (appVer >= 0x300)
            form.flags = info.getWord();
        
        int nFormHandlers = nResHandlers[rtForm];
        if (appVer < 0x300)
            nFormHandlers--;
        for (m=0;m<nFormHandlers;m++)
            form.handlers[m] = info.getLong();
        forms.add(form);
        
        for (j=0;j<nFormCtrls;j++) {
            ResType rt = (ResType)info.getByte();
            OControl ctrl = {0};
            ctrl.type = rt;
            if (appVer >= 0x300)
                ctrl.flags = info.getWord();

            switch (rt) {
                case rtLabel:
                    ctrl.id = info.getWord();
                    ctrl.fid = form.id;
                    ctrl.addr = info.getWord();
                    ctrl.offset = info.getOffset();
                    ctrl.ldata = info.getWord();
                    ctrl.pdata = NULL;
                    if (appVer >= 0x300) {
                        ctrl.x = ctrl.ox = info.getWord();
                        ctrl.y = ctrl.oy = info.getWord();
                    }
                    break;
                case rtBitmap:
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                case rtField:
                case rtPopup:
                case rtSelector:
                case rtScroll:
                case rtSlider:
                case rtList:
                    ctrl.id = info.getWord();
                    ctrl.fid = form.id;
                    ctrl.addr = info.getWord();
                    ctrl.offset = info.getOffset();
                    if (appVer >= 0x300) {
                        ctrl.x = ctrl.ox = info.getWord();
                        ctrl.y = ctrl.oy = info.getWord();
                        if (rt != rtBitmap) {
                            ctrl.w = ctrl.ow = info.getWord();
                            ctrl.h = ctrl.oh = info.getWord();
                        }
                    }
                    break;
                case rtGadget:
                    ctrl.id = info.getWord();
                    ctrl.fid = form.id;
                    ctrl.addr = info.getWord();
                    ctrl.offset = info.getOffset();
                    ctrl.x = ctrl.ox = info.getWord();
                    ctrl.y = ctrl.oy = info.getWord();
                    ctrl.w = ctrl.ow = info.getWord();
                    ctrl.h = ctrl.oh = info.getWord();
                    ctrl.pdata = NULL;
                    break;
                case rtGraffiti:
                    if (appVer >= 0x300) {
                        ctrl.x = ctrl.ox = info.getWord();
                        ctrl.y = ctrl.oy = info.getWord();
                    }
                    break;
                default:
                    assert(!"Unexpected resource type - LoadUI");
            }
            
            for (m=0;m<nResHandlers[rt];m++)
                ctrl.handlers[m] = info.getLong();
            ctrls.add(ctrl);
        }
    }
}

/*
FormPtr CreateForm(int index) {
    word id;
    dword soff;
    char* title;
    Coord x, y, w, h;
    Boolean modal;
    word defButton, helpID, menuID;

    InfoStream info(uiStream + forms[index].offset);
    id = info.getWord();
    info.getWord(); // addr
    x = forms[index].x = info.getWord();
    y = forms[index].y = info.getWord();
    w = forms[index].w = info.getWord();
    h = forms[index].h = info.getWord();
    soff = info.getLong();
    if (soff)
        title = (char*)(uiStream + soff);
    else
        title = "OrbForms";
    forms[index].ldata = strlen(title) + 1;
    modal = info.getByte();
    defButton = info.getWord();
    helpID = info.getWord();
    menuID = info.getWord();
    
    FormPtr pForm = FrmNewForm(id, title, x, y, w, h,
        modal, defButton, helpID, menuID);
    
    int nCtrls = info.getByte();
    info.skip(4 * nResHandlers[rtForm]);
    dword offset = info.getOffset() + forms[index].offset;
    for (int i=0;i<nCtrls;i++) {
        AddControl(pForm, offset, forms[index].firstCtrl + i);
    }
        
    return pForm;
}

void AddControl(FormPtr& pForm, dword& offset, int index) {
    InfoStream info(uiStream + offset);
    word id, bmpID, sbmpID, relID, nVisible, iMin, iMax, iPage;
    Coord x, y, w, h;
    dword soff, maxChars;
    const char* text;
    FontID fontID;
    byte group, flags;
    Boolean leftAnchor, graph, editable, underline, singleLine;
    Boolean dynSize, autoShift, hasScroll, numeric;
    ControlStyleType type;
    //ListPtr pList;

    ResType rt = (ResType)info.getByte();
    id = info.getWord();
    info.getWord(); // addr
    x = ctrls[index].x = info.getWord();
    y = ctrls[index].y = info.getWord();
    switch (rt) {
        case rtLabel:
            fontID = (FontID)info.getWord();
            soff = info.getLong();
            text = soff ? (char*)(uiStream + soff) : "<Label>";
            ctrls[index].ldata = strlen(text) + 1;
            FrmNewLabel(&pForm, id, text, x, y, fontID);
            break;
        case rtBitmap:
            bmpID = info.getWord();
            FrmNewBitmap(&pForm, id, bmpID, x, y);
            break;
        case rtButton:
            type = buttonCtl;
            goto buttonCommon;
        case rtPushButton:
            type = pushButtonCtl;
            goto buttonCommon;
        case rtRepeatButton:
            type = repeatingButtonCtl;
            goto buttonCommon;
        case rtCheckbox:
            type = checkboxCtl;
buttonCommon:
            w = info.getWord();
            h = info.getWord();
            group = info.getByte();
            leftAnchor = info.getByte();
            graph = info.getByte();
            bmpID = info.getWord();
            sbmpID = info.getWord();
            fontID = (FontID)info.getWord();
            soff = info.getLong();
            text = soff ? (char*)(uiStream + soff) : "<Control>";
            if (graph && romVersion >= ver35)
                CtlNewGraphicControl((void**)&pForm, id, type, bmpID, sbmpID,
                    x, y, w, h, group, leftAnchor);
            else
                CtlNewControl((void**)&pForm, id, type, text, x, y, w, h,
                    fontID, group, leftAnchor);
            break;
        case rtField:
            w = info.getWord();
            h = info.getWord();
            leftAnchor = info.getByte();
            fontID = (FontID)info.getWord();
            maxChars = info.getWord();
            flags = info.getByte();
            autoShift = (flags >> 6) & 1;
            dynSize = (flags >> 5) & 1;
            editable = (flags >> 4) & 1;
            hasScroll = (flags >> 3) & 1;
            numeric = (flags >> 2) & 1;
            singleLine = (flags >> 1) & 1;
            underline = flags & 1;
            FldNewField((void**)&pForm, id, x, y, w, h, fontID, maxChars,
                editable, underline?grayUnderline:noUnderline, singleLine,
                dynSize, leftAnchor?leftAlign:rightAlign, autoShift,
                hasScroll, numeric);
            break;
        case rtGadget:
            w = info.getWord();
            h = info.getWord();
            FrmNewGadget(&pForm, id, x, y, w, h);
            break;
        case rtGraffiti:
            if (romVersion >= ver35)
                FrmNewGsi(&pForm, x, y);
            break;
        case rtPopup:
            w = info.getWord();
            h = info.getWord();
            relID = info.getWord();
            leftAnchor = info.getByte();
            fontID = (FontID)info.getWord();
            soff = info.getLong();
            text = soff ? (char*)(uiStream + soff) : "<Control>";
            group = -1;
            type = popupTriggerCtl;
            CtlNewControl((void**)&pForm, id, type, text, x, y, w, h,
                fontID, group, leftAnchor);
            break;
        case rtSelector:
            w = info.getWord();
            h = info.getWord();
            leftAnchor = info.getByte();
            fontID = (FontID)info.getWord();
            soff = info.getLong();
            text = soff ? (char*)(uiStream + soff) : "<Control>";
            group = -1;
            type = selectorTriggerCtl;
            CtlNewControl((void**)&pForm, id, type, text, x, y, w, h,
                fontID, group, leftAnchor);
            break;
        case rtScroll:
            w = info.getWord();
            h = info.getWord();
            iMin = info.getWord();
            iMax = info.getWord();
            iPage = info.getWord();
            assert(!"NYI - scroll");
        case rtSlider:
            w = info.getWord();
            h = info.getWord();
            iMin = info.getWord();
            iMax = info.getWord();
            iPage = info.getWord();
            graph = info.getByte();
            bmpID = info.getWord();
            sbmpID = info.getWord();
            
            group = -1;
            leftAnchor = 1;
            type = feedbackSliderCtl;
            
            if (graph && romVersion >= ver35)
                CtlNewSliderControl((void**)&pForm, id, type, bmpID, sbmpID,
                    x, y, w, h, iMin, iMax, iPage, iMin);
            else if (romVersion >= ver35)
                CtlNewSliderControl((void**)&pForm, id, type, NULL, NULL,
                    x, y, w, h, iMin, iMax, iPage, iMin);
            break;
        case rtList:
            w = info.getWord();
            h = info.getWord();
            fontID = (FontID)info.getWord();
            nVisible = info.getByte();
            relID = info.getWord();
            LstNewList((void**)&pForm, id, x, y, w, h, fontID, nVisible, relID);
            break;
            
        default:
            assert(!"Unexpected control type!");
    }
    info.skip(nResHandlers[rt] * 4);
    ctrls[index].w = w;
    ctrls[index].h = h;
    
    offset += info.getOffset();
}
*/

int FindForm(word id) {
    for (int i=0;i<forms.size();i++)
        if (forms[i].id == id)
            return i;
    return -1;
}

int FindCtrl(word id) {
    for (int i=0;i<ctrls.size();i++)
        if (ctrls[i].id == id)
            return i;
    return -1;
}

int FindActiveCtrl(word id) {
    word fid = FrmGetActiveFormID();
    for (int i=0;i<ctrls.size();i++)
        if (ctrls[i].id == id && ctrls[i].fid == fid)
            return i;
    return -1;
}


int FindMenuItem(word id) {
    for (int i=0;i<menuItems.size();i++)
        if (menuItems[i].id == id)
            return i;
    return -1;
}

int FindGadgetFromPt(word iForm, word x, word y) {
    int f = forms[iForm].firstCtrl;
    int n = forms[iForm].nCtrls;
    for (int i=f;i<f+n;i++) {
        OControl& c = ctrls[i];
        if (c.type == rtGadget && c.visible && c.enabled &&
            x >= c.x && x <= c.x + c.w &&
            y >= c.y && y <= c.y + c.h)
        {
            return i;
        }
    }
    return -1;
}

bool CallHandler(dword addr, dword handler) {
    if (handler == 0xffffffff) {
        return false;
    } else {
        Value obj;
        obj.type = vtInt;
        obj.iVal = addr;
        vm->push(&obj);
        if (handler & 0x80000000) {
            long lib = (handler >> 16) & 0xff;
            long func = handler & 0x0000ffff;
            if (lib == -1) {
                assert(func < nbifuncs);
                (*bifuncs[func].func)(vm, func);
            } else {
                (*addIns[lib].func)(&ofi, addIns[lib].data, handler & 0x0000ffff);
            }
        } else {
            vm->Call(handler, true, false);			
        }
        return true;
    }
}

void ResetFormControls(word iForm) {
    int f = forms[iForm].firstCtrl;
    int n = forms[iForm].nCtrls;
    for (int i=f;i<f+n;i++) {
        ctrls[i].visible = true;
        ctrls[i].enabled = true;
    }
}

bool SendGadgetEvent(word iForm, int iHandler) {
    bool res = false;
    int f = forms[iForm].firstCtrl;
    int n = forms[iForm].nCtrls;
    for (int i=f;i<f+n;i++) {
        OControl& c = ctrls[i];
        if (c.type == rtGadget) {
            if (iHandler == 0) c.visible = c.enabled = true;
            if ((iHandler == 2 && !c.visible) || (iHandler > 2 && !c.enabled)) {
                // don't pass this event
            } else {
                res = CallHandler(c.addr, c.handlers[iHandler]) || res;
            }
        }
    }
    return res;
}

static Boolean GenericFormHandleEvent(EventPtr);
bool bDeleteOnClose = true;
bool bCleanupOnClose = true;

void CloseOrbForm(word id, bool bDeleteForm, bool bCleanup) {
    FormPtr pForm = FrmGetFormPtr(id);
    if (pForm) {
        EventType evt;
        MemSet(&evt, 0, sizeof(evt));
        evt.eType = frmCloseEvent;
        evt.data.frmClose.formID = id;
        bDeleteOnClose = bDeleteForm;
        bCleanupOnClose = bCleanup;
        GenericFormHandleEvent(&evt);
    }
}

/*
void CloseAllOrbForms() {
    int id = FrmGetActiveFormID();
    // close all the hidden forms first, sending no events
    // (closing the active form before a hidden form would cause the
    // hidden form to be redrawn)
    for (int i=0;i<forms.size();i++) {
        if (forms[i].id != id)
            CloseOrbForm(forms[i].id, false, true); // PARAMS CHANGED!
    }
    // close the active form, and send an event
    CloseOrbForm(id, true, true);
}
*/

/*
static void FreeUIData() {
    // TODO: this should really be done when the form closes
    // erase control data
    for (int i=0;i<ctrls.size();i++) {
        if (ctrls[i].pdata) {
            MemPtrFree(ctrls[i].pdata);
            ctrls[i].pdata = NULL;
            ctrls[i].ldata = 0;
        }
    }
    for (int i=0;i<forms.size();i++) {
        if (forms[i].pdata) {
            MemPtrFree(forms[i].pdata);
            forms[i].pdata = NULL;
            forms[i].ldata = 0;
        }
    }
}
*/

extern long atexit_func;
const char* g_appArgs;

static void Execute(const char* dbname, const char* args) {
    MemHandle hMem = 0, hUI = 0;
    long iGlobInit, iGlobDest;
    
    g_appArgs = args;
    
#ifndef ORBFORMS_FAT
    LocalID lid = 0;
    DmOpenRef dbref;
    int i;

    for (i=0;i<nCards;i++) {
        lid = DmFindDatabase(i, dbname);
        if (lid) break;
    }
    if (!lid) {
        Alert("Unable to find application");
        return;
    }
    dbref = DmOpenDatabase(i, lid, dmModeReadOnly);
    if (!dbref) {
        Alert("Unable to open application");
        return;
    }
#endif
    
    // read the database header
    hMem = DmGet1Resource('ofHD', 0);
    if (!hMem) {
        Alert("Unable to open application header");
        goto CleanupDB;
    }
    
    {
        InfoStream info;
        info.init((const byte*)MemHandleLock(hMem));
        appVer = info.getWord();
        hasUI = info.getByte();
        iGlobInit = info.getLong();
        iGlobDest = info.getLong();
    }
    
    MemHandleUnlock(hMem);
    DmReleaseResource(hMem);
    if (appVer > appVersionNum) {
        FrmAlert(VersionAlert);
        goto CleanupDB;
    }
    
    if (hasUI) {
        hUI = DmGet1Resource('ofUI', 0);
        if (!hUI) {
            Alert("unable to load UI segment");
            goto CleanupDB;
        }
        uiStream = (byte*)MemHandleLock(hUI);
    }
        
    vm = new VM;
    ErrTry {
        if (vm->Load()) {
            LibStart();
            if (hasUI) {
                ErrTry {
                    LoadUI();
                    bForceExit = false;
                    if (iGlobInit != -1)
                        vm->Call(iGlobInit, true, false);
                    CallHandler(app.addr, app.handlers[0]);
                    AppEventLoop(false, false);
                    FrmCloseAllForms();
                    CallHandler(app.addr, app.handlers[1]);
                    if (iGlobDest != -1)
                        vm->Call(iGlobDest, true, false);
                } ErrCatch (err) {
                    bForceExit = true;
                    FrmCloseAllForms();
                } ErrEndCatch
                forms.unlock();
                ctrls.unlock();
                menuItems.unlock();
            } else {
                // not event driven
                nativeEnableEvents = true;
                ErrTry {
                    hOutput = (MemHandle)MemHandleNew(OutputFieldLen);
                    *((char*)MemHandleLock(hOutput)) = '\0';
                    MemHandleUnlock(hOutput);
                    bForceExit = false;
                    FrmGotoForm(OutputForm);
                    UIYield();
                    if (iGlobInit != -1)
                        vm->Call(iGlobInit, true, true);
                    vm->Call(0, true, true);
                    if (atexit_func)
                        vm->Call(atexit_func, true, true);
                    if (iGlobDest != -1)
                        vm->Call(iGlobDest, true, true);
                    AppEventLoop(false, false);
                    FrmCloseAllForms();
                    MemHandleFree(hOutput);
                } ErrCatch (err) {
                    if (err == 5 && atexit_func) {
                        ErrTry {
                            vm->Call(atexit_func, true, true);
                        } ErrCatch(err) {
                        } ErrEndCatch
                    }
                    bForceExit = true;
                    FrmCloseAllForms();
                    MemHandleFree(hOutput);
                } ErrEndCatch
            }
            vm->Unload();
            LibStop();
        }
    } ErrCatch (err) {
        ;
    } ErrEndCatch

    delete vm;
    vm = NULL;
    
    if (hUI) {
        MemHandleUnlock(hUI);
        DmReleaseResource(hUI);
    }	

CleanupDB:
#ifndef ORBFORMS_FAT
    DmCloseDatabase(dbref);
#else
    ;
#endif
}


static Boolean GenericFormDoCommand(UInt16 command)
{
    Boolean handled = false;
    int index = FindMenuItem(command);
    if (index == -1)
        handled = false;
    else
        handled = CallHandler(menuItems[index].addr, menuItems[index].handler);    	
    return handled;
}

int debugTag;
OEvent curEvent;
word ctrlActiveID;
bool bPenLive; // TODO: is this sufficient?

// PocketC event queue
EventQueue eventQueue;

#ifdef DEBUG
bool bInOpen = false;
#endif


static Boolean GenericFormHandleEvent(EventPtr pEvent)
{
    Boolean handled = false;
    FormPtr pForm;
    word formID;
    word ctrlID;
    int index, cindex, i;
    int iHandler;
    bool hard, formEvent;
    static bool lastEventWasUpdate = false;
    #ifdef DEBUG
    bool bInOpenOld = bInOpen;
    bInOpen = false;
    #endif
    if (killApp) return false;
   
    switch (pEvent->eType) 
    {
        case menuEvent:
            return GenericFormDoCommand(pEvent->data.menu.itemID);

        case frmOpenEvent:
            ctrlActiveID = -1;
            bPenLive = false;
            formID = pEvent->data.frmOpen.formID;
            index = FindForm(formID);
            if (index != -1) {
                #ifdef DEBUG
                bInOpen = true;
                #endif
                InitFormDIA(formID);
                LayoutForm(formID);
                ResetFormControls(index);
                CallHandler(forms[index].addr, forms[index].handlers[2]);
                SendGadgetEvent(index, 0); // onopen
                #ifdef DEBUG
                bInOpen = false;
                #endif
                FrmDrawForm(FrmGetFormPtr(formID));
                curEvent.code = 0x8000;
                CallHandler(forms[index].addr, forms[index].handlers[4]);
                SendGadgetEvent(index, 2); // ondraw
                handled = true;
            }
            break;
            
        case frmCloseEvent:
            formID = pEvent->data.frmClose.formID;
            index = FindForm(formID);
            if (index == -1)
                handled = false;
            else {
                if (!bForceExit) {
                    // as long as we aren't bailing on an exception, call the handlers
                    SendGadgetEvent(index, 1); // onclose
                    handled = CallHandler(forms[index].addr, forms[index].handlers[3]);
                }
            }
            
            if (bCleanupOnClose) {
                for (i=0;i<ctrls.size();i++) {
                    if (ctrls[i].fid == formID && ctrls[i].type != rtGadget && ctrls[i].pdata) {
                        MemPtrFree(ctrls[i].pdata);
                        ctrls[i].pdata = NULL;
                        ctrls[i].ldata = 0;
                    }
                }
                if (index != -1 && forms[index].pdata) {
                    MemPtrFree(forms[index].pdata);
                    forms[index].pdata = NULL;
                    forms[index].ldata = 0;
                }
            }
            
            if (bDeleteOnClose) {
                pForm = FrmGetFormPtr(formID);
                FrmEraseForm(pForm);
                FrmDeleteForm(pForm);
                handled = true; // since we are deleting the form, the event is handled
            }
            
            // restore defaults
            bDeleteOnClose = true;
            bCleanupOnClose = true;
            break;
        
        case winDisplayChangedEvent:
            formID = FrmGetActiveFormID();
            index = FindForm(formID);
            if (index != -1 && (forms[index].flags & OFORM_RESIZABLE)) {
                FormPtr pForm = FrmGetFormPtr(formID);
                if (LayoutForm(formID)) { // || lastEventWasUpdate) {
                    // layout/draw parent if needed
                    if (parentForm) {
                        int parentIndex = FindForm(parentForm);
                        if (parentIndex != -1) {
                            // layout the parent form
                            LayoutForm(parentForm);
                            FormPtr pParent = FrmGetFormPtr(parentForm);
                            
                            // redraw
                            FrmSetActiveForm(pParent);
                            CallHandler(forms[parentIndex].addr, forms[parentIndex].handlers[9]);
                            curEvent.code = 0x8000;
                            FrmDrawForm(pParent);
                            CallHandler(forms[parentIndex].addr, forms[parentIndex].handlers[4]);
                            SendGadgetEvent(parentIndex, 2); // ondraw

                            // reset active form/window
                            FrmSetActiveForm(pForm);
                            WinHandle winH = FrmGetWindowHandle(pForm); 
                            WinSetActiveWindow(winH);
                        }

                    }
                    CallHandler(forms[index].addr, forms[index].handlers[9]);
                    curEvent.code = 0x8000;
                    FrmDrawForm(pForm);
                    CallHandler(forms[index].addr, forms[index].handlers[4]);
                    SendGadgetEvent(index, 2); // ondraw
                }
                handled = true;
            }
        	break;
        	
       	case winEnterEvent: {
       		EventType e;
            MemSet(&e, sizeof(EventType), 0);
            e.eType = (eventsEnum)winDisplayChangedEvent;
            EvtAddUniqueEventToQueue(&e, 0, true);
        	}
       		break;
        	
        case frmSaveEvent:
            break;
            
        case frmUpdateEvent:
            formID = pEvent->data.frmUpdate.formID;
            index = FindForm(formID);
            if (index == -1)
                handled = false;
            else {
                curEvent.code = pEvent->data.frmUpdate.updateCode;
                //FrmEraseForm(FrmGetFormPtr(formID));
                FrmDrawForm(FrmGetFormPtr(formID));
                CallHandler(forms[index].addr, forms[index].handlers[4]);
                SendGadgetEvent(index, 2);
                handled = true;
                lastEventWasUpdate = true;
            }
            break;

        case ctlSelectEvent:
        case ctlRepeatEvent:
        case lstSelectEvent:
            ctrlID = pEvent->data.ctlSelect.controlID;
            index = FindCtrl(ctrlID);
            if (index == -1) {
                handled = false;
            } else {
                iHandler = 0;
                if (pEvent->eType == ctlRepeatEvent) {
                    curEvent.value = pEvent->data.ctlRepeat.value;
                } else if (pEvent->eType == ctlSelectEvent) {
                    curEvent.value = pEvent->data.ctlSelect.value;
                    if (ctrls[index].type == rtSlider)
                        iHandler = 1;
                } else {
                    curEvent.value = pEvent->data.lstSelect.selection;
                }
                handled = CallHandler(ctrls[index].addr, ctrls[index].handlers[iHandler]);
                if (pEvent->eType == ctlRepeatEvent)
                    handled = false;
            }
            break;
        
        case fldChangedEvent:
            ctrlID = pEvent->data.fldChanged.fieldID;
            index = FindCtrl(ctrlID);
            if (index == -1)
                handled = false;
            else
                handled = CallHandler(ctrls[index].addr, ctrls[index].handlers[0]);
            break;
            
        case popSelectEvent:
            ctrlID = pEvent->data.popSelect.controlID;
            index = FindCtrl(ctrlID);
            if (index == -1) {
                handled = false;
            } else {
                curEvent.value = pEvent->data.popSelect.selection;
                curEvent.prev = pEvent->data.popSelect.priorSelection;
                handled = CallHandler(ctrls[index].addr, ctrls[index].handlers[1]);
            }
            break;
            
        case sclExitEvent:
            ctrlID = pEvent->data.ctlSelect.controlID;
            index = FindCtrl(ctrlID);
            if (index == -1) {
                handled = false;
            } else {
                curEvent.value = pEvent->data.sclExit.newValue;
                curEvent.prev = pEvent->data.sclExit.value;
                handled = CallHandler(ctrls[index].addr, ctrls[index].handlers[1]);
            }
            break;
            
        case sclRepeatEvent:
            ctrlID = pEvent->data.ctlSelect.controlID;
            index = FindCtrl(ctrlID);
            if (index == -1) {
                handled = false;
            } else {
                curEvent.value = pEvent->data.sclRepeat.newValue;
                curEvent.prev = pEvent->data.sclRepeat.value;
                handled = CallHandler(ctrls[index].addr, ctrls[index].handlers[0]);
            }
            handled = false; // since it is a repeat event
            break;

        case keyDownEvent:
            pForm = FrmGetActiveForm();
            formID = FrmGetFormId(pForm);
            index = FindForm(formID);
            if (index == -1)
                handled = false;
            else {
                hard = false;
                formEvent = true;
                if (pEvent->data.keyDown.modifiers & virtualKeyMask) {
                    hard = true;
                    if (pEvent->data.keyDown.chr==pageUpChr) {
                        curEvent.key = 4;
                    } else if (pEvent->data.keyDown.chr==pageDownChr) {
                        curEvent.key = 5;
                    } else if (pEvent->data.keyDown.chr >= hard1Chr && pEvent->data.keyDown.chr <= hard4Chr) {
                        curEvent.key = pEvent->data.keyDown.chr - hard1Chr;
                    } else if (pEvent->data.keyDown.chr >= vchrJogUp && pEvent->data.keyDown.chr <= vchrJogBack) {
                        curEvent.key = 9 + pEvent->data.keyDown.chr - vchrJogUp;
                    } else if (pEvent->data.keyDown.chr == vchrNavChange) {
                        if ((pEvent->data.keyDown.modifiers & autoRepeatKeyMask) == 0 &&
                            (pEvent->data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == navChangeSelect)
                        {
                            // select
                            curEvent.key = 8;
                        } else if (pEvent->data.keyDown.modifiers & autoRepeatKeyMask) {
                            if ((pEvent->data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == navBitLeft) {
                                curEvent.key = 6;
                            } else if ((pEvent->data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == navBitRight) {
                                curEvent.key = 7;
                            } else {
                                break;
                            }
                        } else {
                            if ((pEvent->data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == (navBitLeft | navChangeLeft)) {
                                curEvent.key = 6;
                            } else if ((pEvent->data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == (navBitRight | navChangeRight)) {
                                curEvent.key = 7;
                            } else {
                                break;
                            }
                        }
                    } else if (pEvent->data.keyDown.chr==vchrRockerCenter) {
                        curEvent.key = 8;
                    } else if (pEvent->data.keyDown.chr==vchrRockerLeft) {
                        curEvent.key = 6;
                    } else if (pEvent->data.keyDown.chr==vchrRockerRight) {
                        curEvent.key = 7;
                    } else if (pEvent->data.keyDown.chr==vchrNextField) {
                        curEvent.key = 17;
                    } else if (pEvent->data.keyDown.chr==vchrPrevField) {
                        curEvent.key = 18;
                    } else {
                        break; // not something recognized, ignore
                    }
                } else {
                    curEvent.key = pEvent->data.keyDown.chr;
                    formEvent = false;
                }
                word objIndex = FrmGetFocus(pForm);
                if (objIndex == noFocus || formEvent)
                    handled = CallHandler(forms[index].addr, forms[index].handlers[hard?1:0]);
                else if (objIndex != noFocus && FrmGetObjectType(pForm, objIndex) == frmFieldObj) {
                    word objId = FrmGetObjectId(pForm, objIndex);
                    index = FindCtrl(objId);
                    handled = false; // never handle the onchar event
                    if (index != -1) {
                        FieldPtr pFld = (FieldPtr)FrmGetObjectPtr(pForm, objIndex);
                        FldHandleEvent(pFld, pEvent);
                        CallHandler(ctrls[index].addr, ctrls[index].handlers[1]);
                        handled = true; // since we called fld handle event
                        
                    }
                }
            }
            break;
        
        case penDownEvent:
            ctrlActiveID = -1;
            bPenLive = true;
        case penUpEvent:
        case penMoveEvent:
            // TODO: form is eating events from controls!!!
            pForm = FrmGetActiveForm();
            formID = FrmGetFormId(pForm);
            index = FindForm(formID);
            if (index != -1) {
                if (pEvent->eType == penDownEvent) iHandler = 3;
                else if (pEvent->eType == penUpEvent) iHandler = 4;
                else iHandler = 5;
                
                cindex = FindGadgetFromPt(index, pEvent->screenX, pEvent->screenY);
                if (pEvent->eType == penDownEvent)
                    ctrlActiveID = cindex;
                
                if (cindex == -1 || ctrls[cindex].handlers[iHandler] == 0xffffffff) {
                    // form
                    curEvent.x = pEvent->screenX;
                    curEvent.y = pEvent->screenY;
                    curEvent.nx = curEvent.x * density / 72;
                    curEvent.ny = curEvent.y * density / 72;
                    handled = CallHandler(forms[index].addr, forms[index].handlers[iHandler+2]);
                    handled = false; // don't eat events!
                } else // {
                
                
                if (ctrlActiveID != -1 && bPenLive) {
                    curEvent.x = pEvent->screenX - ctrls[ctrlActiveID].x;
                    curEvent.y = pEvent->screenY - ctrls[ctrlActiveID].y;
                    curEvent.nx = curEvent.x * density / 72;
                    curEvent.ny = curEvent.y * density / 72;
                    curEvent.inside = cindex == ctrlActiveID;
                    handled = CallHandler(ctrls[ctrlActiveID].addr, ctrls[ctrlActiveID].handlers[iHandler]);
                }
            }
            if (pEvent->eType == penUpEvent)
                bPenLive = false;
            break;

        case nilEvent:
            formID = FrmGetActiveFormID();
            if (timer.NextTimer(formID) == 0) {
                timer.KillTimer(formID);
                index = FindForm(formID);
                if (index != -1) {
                    CallHandler(forms[index].addr, forms[index].handlers[8]);
                }
            }
            break;
        }
    
    lastEventWasUpdate = pEvent->eType == frmUpdateEvent;
    #ifdef DEBUG
    bInOpen = bInOpenOld;
    #endif
    
    return handled;
}

/***********************************************************************
 *
 * FUNCTION:    OutputFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MainForm" of this application.
 *
 * PARAMETERS:  pEvent  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
MemHandle hOutput;

void OutputFormUpdateScrollBar() {
    word scrollPos, textHeight, fieldHeight;
    short maxValue;
    FieldPtr fld;
    ScrollBarPtr bar;

    fld = (FieldPtr)GetObjectPtr(OutputOutputField);
    bar = (ScrollBarPtr)GetObjectPtr(OutputScroll);
    
    FldGetScrollValues (fld, &scrollPos, &textHeight,  &fieldHeight);

    if (textHeight > fieldHeight) maxValue = textHeight - fieldHeight;
    else if (scrollPos) maxValue = scrollPos;
    else maxValue = 0;

    SclSetScrollBar (bar, scrollPos, 0, maxValue, fieldHeight-1);
}

static void OutputFormScroll (short linesToScroll) {
    word blankLines;
    short min, max, value, pageSize;
    FieldPtr fld;
    ScrollBarPtr bar;
    
    fld = (FieldPtr)GetObjectPtr(OutputOutputField);

    if (linesToScroll < 0) {
        blankLines = FldGetNumberOfBlankLines (fld);
        FldScrollField (fld, -linesToScroll, winUp);
        
        // If there were blank lines visible at the end of the field
        // then we need to update the scroll bar.
        if (blankLines) {
            // Update the scroll bar.
            bar = (ScrollBarPtr)GetObjectPtr(OutputScroll);
            SclGetScrollBar (bar, &value, &min, &max, &pageSize);
            if (blankLines > -linesToScroll) max += linesToScroll;
            else max -= blankLines;
            SclSetScrollBar (bar, value, min, max, pageSize);
        }
    } else if (linesToScroll > 0) FldScrollField (fld, linesToScroll, winDown);
}

static void OutputFormScrollPage(WinDirectionType direction) {
    short value, min, max, pageSize;
    word linesToScroll;
    FieldPtr fld;
    ScrollBarPtr bar;

    fld = (FieldPtr)GetObjectPtr(OutputOutputField);
    
    if (FldScrollable(fld, direction)) {
        linesToScroll = FldGetVisibleLines (fld) - 1;
        FldScrollField(fld, linesToScroll, direction);

        // Update the scroll bar.
        bar = (ScrollBarPtr)GetObjectPtr(OutputScroll);
        SclGetScrollBar(bar, &value, &min, &max, &pageSize);

        if (direction == winUp) value -= linesToScroll;
        else value += linesToScroll;
        
        SclSetScrollBar(bar, value, min, max, pageSize);
    }
}

static Boolean OutputFormDoCommand(word command) {
    FieldPtr pField;
    Boolean handled = true;

    switch (command) {
        case EditSelectAll:
            pField = (FieldPtr)GetObjectPtr(OutputOutputField);
            FldSetSelection(pField, 0, FldGetTextLength(pField));
            break;
            
        case EditCopy:
            pField = (FieldPtr)GetObjectPtr(OutputOutputField);
            FldCopy(pField);
            break;
            
        default:
            handled = false;
            break;
    }
    return handled;
}

static Boolean OutputFormHandleEvent(EventPtr eventP) {
    Boolean	handled	= false;
    FormPtr	frmP;
    FieldPtr pField;

    switch (eventP->eType) {
        case menuEvent:
            return OutputFormDoCommand(eventP->data.menu.itemID);

        case keyDownEvent:
            if (eventP->data.keyDown.chr==pageUpChr) {
                OutputFormScrollPage(winUp);
                handled	= true;
            } else if (eventP->data.keyDown.chr==pageDownChr) {
                OutputFormScrollPage(winDown);
                handled	= true;
            }
            break;

        case fldChangedEvent:
            OutputFormUpdateScrollBar();
            handled	= true;
            break;
            
        case winDisplayChangedEvent:
            if (LayoutStandardForm(FrmGetActiveFormID())) {
                FrmDrawForm(FrmGetActiveForm());
                handled = true;
            }
            break;

        case frmOpenEvent:
            frmP = FrmGetActiveForm();
            pField = (FieldPtr)GetObjectPtr(OutputOutputField);
            FldSetTextHandle(pField, hOutput);
            InitStandardFormDIA(FrmGetActiveFormID());
            LayoutStandardForm(FrmGetActiveFormID());
            FldSetScrollPosition(pField, -1);
            OutputFormUpdateScrollBar();
            
            FrmDrawForm(frmP);
            handled	= true;
            break;

        case sclRepeatEvent:
            OutputFormScroll(eventP->data.sclRepeat.newValue - eventP->data.sclRepeat.value);
            // not handled, so that the button continues repeating;
            break;

        case frmCloseEvent:	{
            pField = (FieldPtr)GetObjectPtr(OutputOutputField);
            FldSetTextHandle(pField, 0);
            break;
        }
    }

    return handled;
}

static Boolean GraphFormHandleEvent(EventPtr eventP) {
    Boolean	handled	= false;
    FormPtr	frmP;

    switch (eventP->eType) {
        case winDisplayChangedEvent:
            if (LayoutStandardForm(FrmGetActiveFormID())) {
                FrmDrawForm(FrmGetActiveForm());
                handled = true;
            }
            break;

        case frmOpenEvent:
            frmP = FrmGetActiveForm();
            InitStandardFormDIA(FrmGetActiveFormID());
            LayoutStandardForm(FrmGetActiveFormID());
            FrmDrawForm(frmP);
            handled	= true;
            break;

        case frmUpdateEvent:
            eventQueue.AddEvent(EventQueue::eqUpdate, eventP->data.frmUpdate.updateCode, 0);
            break;
    }

    return handled;
}

/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean AppHandleEvent(EventPtr pEvent)
{
    UInt16 formId;
    FormPtr pForm;

    if (pEvent->eType == frmLoadEvent)
    {
        // Load the form resource.
        formId = pEvent->data.frmLoad.formID;
        pForm = FrmInitForm(formId);
        FrmSetActiveForm(pForm);

        // Set the event handler for the form.  The handler of the currently
        // active form is called by FrmHandleEvent each time is receives an
        // event.
        switch (formId)
        {
            case OutputForm:
                SetFormDIAPolicy(formId);
                FrmSetEventHandler(pForm, OutputFormHandleEvent);
                break;

            case GraphForm:
                if (enableResize)
                    SetFormDIAPolicy(formId);
                FrmSetEventHandler(pForm, GraphFormHandleEvent);
                break;

            default: {
                int index = FindForm(formId);
                if (index != -1) {
                    if (forms[index].flags & OFORM_RESIZABLE)
                        SetFormDIAPolicy(formId);
                }
                FrmSetEventHandler(pForm, GenericFormHandleEvent);
                break;
            }

        }
        return true;
    }
    
    return false;
}


/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

vector<OEventFunc> eventFuncs;

bool AppEventLoop(bool bRuntime, bool bBlocking)
{
    UInt16 error;
    EventType event;
    do {
        // EvtGetEvent will return nilEvent if bImmediate, so a timer
        // will always be given the chance to run
        dword timeout = (bRuntime && !bBlocking) ? 0 : timer.NextTimer(FrmGetActiveFormID());
        if (timeout == -1)
            timeout = evtWaitForever;

        EvtGetEvent(&event, timeout);
        
        bool handled = false;
        for (int i=0;i<eventFuncs.size();i++) {
            if (eventFuncs[i].func) {
                handled = (*eventFuncs[i].func)(&ofi, eventFuncs[i].data, &event);
                if (handled) break;
            }
        }
        
        if (!handled) {
            if (bHookHard && event.eType==keyDownEvent &&
                    event.data.keyDown.chr >= hard1Chr &&
                    event.data.keyDown.chr <= hard4Chr)
            {
                FrmDispatchEvent(&event);
            }
            else
            {
                if (! SysHandleEvent(&event))
                    if (! MenuHandleEvent(0, &event, &error))
                        if (! AppHandleEvent(&event))
                            FrmDispatchEvent(&event);
            }
        }


    } while (event.eType != appStopEvent &&
        !(bRuntime && bBlocking) &&
        !(bRuntime && event.eType == nilEvent));
    
    if (event.eType == appStopEvent) {
        if (bRuntime)
            EvtAddEventToQueue(&event); // Put the message back in the Q
        return true;
    }
    return false;
}

void UIYield() {
    UInt16 error;
    EventType event;

    do {
        EvtGetEvent(&event, 0);

        if (! SysHandleEvent(&event))
            if (! MenuHandleEvent(0, &event, &error))
                if (! AppHandleEvent(&event))
                    FrmDispatchEvent(&event);

    } while (event.eType != nilEvent);
}

//
// PocketC eventing
//

EventType lastEvent;
bool bHookMenu = false;
bool bHookSilk = false;
bool bHookSync = false;

bool PocketCYield(long time) {
    short eventQ, eventQv1, eventQv2;
    Err error;
    bool justOne = (time != 0);
    if (time == 1)
        time = -1;
    dword start = TimGetTicks(), currTime;
    
    //if (killEvents)
    //	return false;
    do {
dumb_event:
        if (time == -1 || time == 0)
            EvtGetEvent(&lastEvent, time);
        else {
            currTime = TimGetTicks();
            if (currTime - start >= time)
                return false; // time has expired
            EvtGetEvent(&lastEvent, time - (currTime - start));
        }
    
        if (lastEvent.eType==nilEvent) {
            if (!justOne)
                return false;
            else
                goto dumb_event;
        }

        // check for hard button press
        if (bHookHard && lastEvent.eType==keyDownEvent &&
            lastEvent.data.keyDown.chr >= hard1Chr &&
            lastEvent.data.keyDown.chr <= hard4Chr) goto hook;
            
        if ((bHookMenu || bHookSilk) && lastEvent.eType==keyDownEvent &&
            lastEvent.data.keyDown.chr==menuChr) goto hook;
            
        if (bHookSync && lastEvent.eType==keyDownEvent &&
            lastEvent.data.keyDown.chr==hardCradleChr) goto hook;
            
        if (bHookSilk && lastEvent.eType==keyDownEvent &&
            (lastEvent.data.keyDown.chr == launchChr ||
            lastEvent.data.keyDown.chr == calcChr ||
            lastEvent.data.keyDown.chr == findChr)) goto hook;
            
        if (! SysHandleEvent(&lastEvent))
            if (! MenuHandleEvent(0, &lastEvent, &error))
                if (! AppHandleEvent(&lastEvent))
                    FrmDispatchEvent(&lastEvent);

        if (lastEvent.eType==appStopEvent) {
            EvtAddEventToQueue(&lastEvent); // Put the message back in the Q
            if (atexit_func) {
                //killEvents = true;
                //CallFunc(atexit_func);
                //killVM = true;
            }
            ErrThrow(3); // Throw an exception, get out of app
        }
hook:
        // this event system is so broken... only holding the last event is dumb
        // if we hit a scan decode event, don't overwrite it
        if (lastEvent.eType==penDownEvent) {
            eventQ = 2;
        } else if (lastEvent.eType==penUpEvent) {
            eventQ = 3;
        } else if (lastEvent.eType==penMoveEvent) {
            eventQ = 4;
        } else if (lastEvent.eType==keyDownEvent) {
            if (lastEvent.data.keyDown.chr==pageUpChr) eventQ = 5;
            else if (lastEvent.data.keyDown.chr==pageDownChr) eventQ = 6;
            else if (lastEvent.data.keyDown.chr >= hard1Chr && lastEvent.data.keyDown.chr <= hard4Chr) eventQ = 7 + lastEvent.data.keyDown.chr - hard1Chr;
            else if (lastEvent.data.keyDown.chr==menuChr) eventQ = 11;
            else if (lastEvent.data.keyDown.chr==launchChr) eventQ = 12;
            else if (lastEvent.data.keyDown.chr==findChr) eventQ = 13;
            else if (lastEvent.data.keyDown.chr==calcChr) eventQ = 14;
            else if (lastEvent.data.keyDown.chr==hardCradleChr) eventQ = 15;
            else if (lastEvent.data.keyDown.chr==lowBatteryChr) goto dumb_event;
            else if (lastEvent.data.keyDown.chr==vchrResetAutoOff) goto dumb_event;
            else if (lastEvent.data.keyDown.chr==vchrNavChange) {
                if ((lastEvent.data.keyDown.modifiers & autoRepeatKeyMask) == 0 &&
                    (lastEvent.data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == navChangeSelect)
                {
                    // select
                    eventQ = 18;
                } else if (lastEvent.data.keyDown.modifiers & autoRepeatKeyMask) {
                    if ((lastEvent.data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == navBitLeft) {
                        eventQ = 16;
                    } else if ((lastEvent.data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == navBitRight) {
                        eventQ = 17;
                    }
                } else {
                    if ((lastEvent.data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == (navBitLeft | navChangeLeft)) {
                        eventQ = 16;
                    } else if ((lastEvent.data.keyDown.keyCode & (navBitsAll | navChangeBitsAll)) == (navBitRight | navChangeRight)) {
                        eventQ = 17;
                    }
                }
            } else if (lastEvent.data.keyDown.chr==vchrRockerCenter) {
                eventQ = 18;
            } else if (lastEvent.data.keyDown.chr==vchrRockerLeft) {
                eventQ = 16;
            } else if (lastEvent.data.keyDown.chr==vchrRockerRight) {
                eventQ = 17;
            } else {
                eventQ = 1; // TODO filter
            }
            eventQv1 = lastEvent.data.keyDown.chr; // TODO filter
        } else if (enableResize && (lastEvent.eType == winDisplayChangedEvent || lastEvent.eType == frmOpenEvent)) {
            // generate display change for open event
            eventQ = 24;
            WinGetDisplayExtent(&eventQv1, &eventQv2);
        } else {
            goto noevent;
        }
            
        if (eventQ <= 4 && eventQ >= 2) {
            eventQv1 = lastEvent.screenX;
            eventQv2 = lastEvent.screenY;
            //if (eventQv2 >= 160) eventQ=0;
        }
        
        eventQueue.AddEvent(eventQ, eventQv1, eventQv2);
        
noevent:
        //if (stopApp) return true;
        // TODO: not sure this is needed!
        //if (killVM) {
        //	ErrThrow(3);
        //}
        if (justOne)
            return false;
    } while (true);
}

/***********************************************************************
 *
 * FUNCTION:     AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 romVersion;
UInt32 density;
bool bHighDensity = false;
UInt16 MathLibRef = 0xffff;
bool hideSecrets;

static Err AppStart(void)
{
    nCards = MemNumCards();
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    UInt32 wver = 0;
    FtrGet(sysFtrCreator, sysFtrNumWinVersion, &wver);
    if (romVersion >= ver50 || wver >= 4) {
        WinScreenGetAttribute(winScreenDensity, &density);
        bHighDensity = true;
    } else {
        density = 72;
    }
    
    Err merror;
    merror = SysLibFind(MathLibName, &MathLibRef);
    if (merror == sysErrLibNotFound)
        merror = SysLibLoad(LibType, MathLibCreator, &MathLibRef);
    bool mathLibLoaded = (merror == 0);
    if (mathLibLoaded) {
        merror = MathLibOpen(MathLibRef, MathLibVersion);
        if (merror) mathLibLoaded = false;
    }
    
    SystemPreferencesType prefs;
    PrefGetPreferences(&prefs);
    hideSecrets = prefs.hideSecretRecords;
    
    InitDIA();
    
    return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void AppStop(void)
{
    UInt16 usecount;
    if (MathLibRef != 0xffff) {
        MathLibClose(MathLibRef, &usecount);
        if (usecount == 0) SysLibRemove(MathLibRef);
    }
    CloseDIA();
}


/***********************************************************************
 *
 * FUNCTION:    StarterPalmMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 *
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
#define sysAppLaunchCmdStartApplet sysAppLaunchCmdCustomBase
#define sysAppLaunchCmdStartAppletWithArg sysAppLaunchCmdCustomBase + 1

static UInt32 OrbFormsPalmMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    Err error;
    const char* args = NULL;

    switch (cmd)
        {
#ifdef ORBFORMS_FAT
        case sysAppLaunchCmdNormalLaunch:
#else
        case sysAppLaunchCmdStartApplet:
#endif
        case sysAppLaunchCmdStartAppletWithArg:
            error = RomVersionCompatible (ourMinVersion, launchFlags);
            if (error) return (error);
            error = AppStart();
            if (error)
                return error;
            if (cmd == sysAppLaunchCmdStartAppletWithArg)
#ifdef ORBFORMS_FAT
            	args = (const char*)cmdPBP;
#else
            	args = (const char*)cmdPBP + 32;
#endif
            Execute((const char*)cmdPBP, args);
            AppStop();
            WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);
            if (bForceExit) {
                EventType event;
                event.eType=keyDownEvent;
                event.data.keyDown.chr=launchChr;
                event.data.keyDown.keyCode=launchChr;
                event.data.keyDown.modifiers=commandKeyMask;
                SysHandleEvent(&event); // let the system run the launcher
            }

            break;		        

#ifndef ORBFORMS_FAT
        case sysAppLaunchCmdNormalLaunch:
            error = RomVersionCompatible (ourMinVersion, launchFlags);
            if (error) return (error);
            error = AppStart();
            if (error) 
                return error;
                
            FrmGotoForm(OutputForm);
            AppEventLoop(false, false);
            FrmCloseAllForms();
            AppStop();
            break;
#endif
        case sysAppLaunchCmdNotify:
            HandleResizeNotification(((SysNotifyParamType*)cmdPBP)->notifyType);
            break;
            
        default:
            break;

        }
    
    return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
UInt32 PilotMain( UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    return OrbFormsPalmMain(cmd, cmdPBP, launchFlags);
}
