#include "OrbFormsRT.h"
#include "LstGlue.h"

#ifndef vskResizeVertically
#  define vskResizeVertically     1
#endif
#ifndef vskResizeHorizontally
#  define vskResizeHorizontally   2
#endif

void SetDIAAllowResize(bool allow);
void SetFormDIAPolicy(UInt16 formID);
DIAtype GetDIAType(void);

#define STD_EXTENT_X 160
#define STD_EXTENT_Y 160

DIAtype diaType = DIA_TYPE_NONE;
static UInt32 displayChangeNotification = 0;
static bool diaNotificationRegistered;
static UInt16 sonyRefNum;

static DIAtype InitSony(void)
{
    Err    err;
    UInt32 version;

    err = SysLibFind(sonySysLibNameSilk, &sonyRefNum);
    if (err == sysErrLibNotFound) {
        err = SysLibLoad('libr', sonySysFileCSilkLib, &sonyRefNum);
    }
    
    if (err != errNone) return DIA_TYPE_NONE;
    
    if (errNone == FtrGet(sonySysFtrCreator, sonySysFtrNumVskVersion, &version)) {
        err = VskOpen(sonyRefNum);
        if (errNone == err)
            return DIA_TYPE_SONY2;
    }
    return DIA_TYPE_NONE;
}


static DIAtype InitPalm(void)
{
    UInt32 version;
    Err    err;
    err = FtrGet(pinCreator, pinFtrAPIVersion, &version);
    if (err != errNone)
        return DIA_TYPE_NONE;
    if (pinAPIVersion1_1 <= version)
        return DIA_TYPE_PALM11;
    else if (pinAPIVersion1_0 <= version)
        return DIA_TYPE_PALM10;
    else
        return DIA_TYPE_NONE;
}


static DIAtype InitUnknown(void)
{
    Coord extentX;
    Coord extentY;
    WinGetDisplayExtent(&extentX, &extentY);
    if (extentX != STD_EXTENT_X || extentY != STD_EXTENT_Y)
        return DIA_TYPE_UNKNOWN;
    else
        return DIA_TYPE_NONE;
}


static void RegisterNotification(void)
{
    diaNotificationRegistered = false;
    switch (diaType) {
        case DIA_TYPE_SONY2:
            displayChangeNotification = sysNotifyDisplayChangeEvent;
            break;
        case DIA_TYPE_PALM10:
        case DIA_TYPE_PALM11:
            displayChangeNotification = sysNotifyDisplayResizedEvent;
            break;
        default:
            return;
    }
    UInt16 card;
    LocalID db;
    Err err = SysCurAppDatabase(&card, &db);
    if (err != errNone)
        return;
    err = SysNotifyRegister(card, db, displayChangeNotification, NULL,
        sysNotifyNormalPriority, 0);
    diaNotificationRegistered = (err == errNone);
}


static void UnregisterNotification(void) {
    UInt16 card;
    LocalID db;
    SysCurAppDatabase(&card, &db);
    
    if (diaNotificationRegistered) {
        SysNotifyUnregister(card, db, displayChangeNotification,
            sysNotifyNormalPriority);
        diaNotificationRegistered = false;
    }
}


DIAtype InitDIA(void) {
    diaType = InitPalm();
    if (diaType == DIA_TYPE_NONE)
        diaType = InitSony();
    if (diaType == DIA_TYPE_NONE)
        diaType = InitUnknown();
    RegisterNotification();

    return diaType;
}


void CloseDIA(void)
{
    UnregisterNotification();
    if (GetDIAState() == DIA_STATE_NO_STATUS_BAR)
        SetDIAState(DIA_STATE_MIN);
    if (diaType == DIA_TYPE_SONY2)
        VskClose(sonyRefNum);
    diaType = DIA_TYPE_NONE;
}


void SetDIAState(DIAState state)
{
    switch (diaType) {
        case DIA_TYPE_SONY2:
            switch (state) {
                case DIA_STATE_MAX:
                    VskSetState(sonyRefNum, vskStateResize, vskResizeMax);
                    break;
                case DIA_STATE_MIN:
                    VskSetState(sonyRefNum, vskStateResize, vskResizeMin);
                    break;
                case DIA_STATE_NO_STATUS_BAR:
                    VskSetState(sonyRefNum, vskStateResize, vskResizeNone);
                    break;
                default:
                    break;
            }
            break;
        case DIA_TYPE_PALM10:
        case DIA_TYPE_PALM11:
            switch (state) {
                case DIA_STATE_MAX:
                    PINSetInputAreaState(pinInputAreaOpen);
                    break;
                case DIA_STATE_MIN:
                case DIA_STATE_NO_STATUS_BAR:
                    PINSetInputAreaState(pinInputAreaClosed);
                    break;
                case DIA_STATE_USER:
                	PINSetInputAreaState(pinInputAreaUser);
                	break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}


DIAState GetDIAState(void)
{
    switch (diaType) {
        case DIA_TYPE_SONY2: {
            UInt16 state;
            Err    err;
            err = VskGetState(sonyRefNum, vskStateResize, &state);
            if (err != errNone)
                return DIA_STATE_UNDEFINED;
            switch (state) {
                case vskResizeMax:
                    return DIA_STATE_MAX;
                case vskResizeMin:
                    return DIA_STATE_MIN;
                case vskResizeNone:
                    return DIA_STATE_NO_STATUS_BAR;
                default:
                    return DIA_STATE_UNDEFINED;
            }
        }
        case DIA_TYPE_PALM10:
        case DIA_TYPE_PALM11:
            switch (PINGetInputAreaState()) {
                case pinInputAreaOpen:
                    return DIA_STATE_MAX;
                case pinInputAreaClosed:
                case pinInputAreaNone:
                    return DIA_STATE_MIN;
                default:
                    return DIA_STATE_UNDEFINED;
            }
        default:
            return DIA_STATE_MAX;
    }
}

/*
bool GetDIARotation() {
}

void SetDIARotation(bool landscape) {
}
*/

void SetDIAAllowResize(bool allow)
{
    switch (diaType) {
        case DIA_TYPE_SONY2:
            if (allow) {
                /* If available, enable horizontal resize */
                if (0x03 <= VskGetAPIVersion(sonyRefNum))
                    VskSetState(sonyRefNum, vskStateEnable,
                        vskResizeHorizontally);
                /* Enable vertical resize */
                VskSetState(sonyRefNum, vskStateEnable,
                    vskResizeVertically);
            }
            else {
                VskSetState(sonyRefNum, vskStateEnable, 0);
            }
            break;
        case DIA_TYPE_PALM11:
        case DIA_TYPE_PALM10:
            PINSetInputTriggerState(allow ? pinInputTriggerEnabled :
                                                 pinInputTriggerDisabled);
            SysSetOrientationTriggerState(allow ?
                sysOrientationTriggerEnabled : sysOrientationTriggerDisabled);
            break;
        default:
            break;
    }
}


void InitFormDIA(word formID) {
    int index = FindForm(formID);
    if (index == -1) {
        SetDIAAllowResize(false);
        SetDIAState(DIA_STATE_MAX);
    } else {
        if (forms[index].flags & OFORM_RESIZABLE) {
            SetDIAAllowResize(true);
            SetDIAState(DIA_STATE_USER);
            if (diaType == DIA_TYPE_PALM10 || diaType == DIA_TYPE_PALM11) {
                WinSetConstraintsSize(WinGetWindowHandle(FrmGetFormPtr(formID)),
                    forms[index].oh, 0x7fff, 0x7fff,
                    forms[index].ow, 0x7fff, 0x7fff);
            }
            // reset the current positions
            OForm& form = forms[index];
            form.x = form.ox;
            form.y = form.oy;
            form.w = form.ow;
            form.h = form.oh;
            
            word ic = form.firstCtrl;
            for (word i=0; i<form.nCtrls; i++) {
            	OControl& ctrl = ctrls[ic];
                ctrl.x = ctrl.ox;
                ctrl.y = ctrl.oy;
                ctrl.w = ctrl.ow;
                ctrl.h = ctrl.oh;
            	ic++;
            }
            
        } else {
            SetDIAAllowResize(false);
            SetDIAState(DIA_STATE_MAX);
        }
    }
}


static bool MatchLastExtents(void)
{
    static Coord lastX;
    static Coord lastY;
    Coord extentX;
    Coord extentY;

    extentX = lastX;
    extentY = lastY;

    WinGetDisplayExtent(&lastX, &lastY);

    return extentX == lastX && extentY == lastY;
}


bool HandleResizeNotification(UInt32 notificationType)
{
    if (notificationType != displayChangeNotification)
        return false;
    if (MatchLastExtents())
        return false;

    if (diaType != DIA_TYPE_PALM11) {
        EventType e;
        MemSet(&e, sizeof(EventType), 0);
        e.eType = (eventsEnum)winDisplayChangedEvent;
        EvtAddUniqueEventToQueue(&e, 0, true);
    }
    return true;
}


void SetFormDIAPolicy(UInt16 formID)
{
    if (diaType == DIA_TYPE_PALM10 || diaType == DIA_TYPE_PALM11) {
        FormType* pForm;
        pForm = FrmGetFormPtr(formID);
        if (pForm != NULL) {
            FrmSetDIAPolicyAttr(pForm, frmDIAPolicyCustom);
        }
    }
}


DIAtype GetDIAType(void)
{
    return diaType;
}


//
// resize code
//

#define StandardExtent() 160


static UInt16 MyFrmGetObjectIndex(FormPtr pForm, word id)
{
    if (id != 0) return FrmGetObjectIndex(pForm, id);
    
    for (word i = FrmGetNumberOfObjects(pForm) - 1 ; 0 < i ; i --) {
        if (FrmGetObjectType(pForm, i) == frmGraffitiStateObj)
            return i;
    }
    return 0;
}


static bool SameBounds(RectangleType* a, RectangleType* b)
{
    return (a->extent.y == b->extent.y && a->extent.x == b->extent.x && 
    	a->topLeft.y == b->topLeft.y && a->topLeft.x == b->topLeft.x);
}

static void AdjustBounds(RectangleType* orig, RectangleType* adjusted,
    int dw, int dh, word flags)
{
    *adjusted = *orig;
    
    switch (flags & RESIZE_X_MASK) {
        case RESIZE_X_ATTACH_LEFT:
            break;
        case RESIZE_X_ATTACH_RIGHT:
            adjusted->topLeft.x += dw;
            break;
        case RESIZE_X_STRETCH:
            adjusted->extent.x += dw;
            break;
    }
    
    switch (flags & RESIZE_Y_MASK) {
        case RESIZE_Y_ATTACH_TOP:
            break;
        case RESIZE_Y_ATTACH_BOTTOM:
            adjusted->topLeft.y += dh;
            break;
        case RESIZE_Y_STRETCH:
            adjusted->extent.y += dh;
            break;
    }
}


// returns true if something changed
bool LayoutForm(word formID)
{
    Coord extentX;
    Coord extentY;
    UInt16 focus;
    FormType* pForm;

    RectangleType newBounds, curBounds, origBounds;
    
    pForm = FrmGetFormPtr(formID);
    if (pForm == NULL)
        return false;
        
    int index = FindForm(formID);
    if (index == -1) return false;
    
    OForm& form = forms[index];

    WinGetDisplayExtent(&extentX, &extentY);

    RctSetRectangle(&origBounds, form.ox, form.oy, form.ow, form.oh);
    RctSetRectangle(&curBounds, form.x, form.y, form.w, form.h);
    
    AdjustBounds(&origBounds, &newBounds,
    	extentX - StandardExtent(),
    	extentY - StandardExtent(),
    	forms[index].flags);
    	
    if (SameBounds(&curBounds, &newBounds)) return false;

    form.x = newBounds.topLeft.x;
    form.y = newBounds.topLeft.y;
    form.w = newBounds.extent.x;
    form.h = newBounds.extent.y;
    WinSetBounds(FrmGetWindowHandle(pForm), &newBounds);
    
    // stop right there if the size of the form didn't change
    if (newBounds.extent.x == curBounds.extent.x &&
    	newBounds.extent.y == curBounds.extent.y) return true;

    // The rest is resized relative to the form itself
    extentX = newBounds.extent.x;
    extentY = newBounds.extent.y;
    focus   = FrmGetFocus(pForm);
    int dw = newBounds.extent.x - origBounds.extent.x;
    int dh = newBounds.extent.y - origBounds.extent.y;
    
    word ic = form.firstCtrl;
    for (word i = 0; i < form.nCtrls; i++) {
    	OControl& ctrl = ctrls[ic];
        RctSetRectangle(&origBounds, ctrl.ox, ctrl.oy, ctrl.ow, ctrl.oh);
        RctSetRectangle(&curBounds, ctrl.x, ctrl.y, ctrl.w, ctrl.h);
        AdjustBounds(&origBounds, &newBounds, dw, dh, ctrl.flags);
        // if we aren't stretching, leave the extents the same
        if ((ctrl.flags & RESIZE_X_MASK) != RESIZE_X_STRETCH)
        	newBounds.extent.x = curBounds.extent.x;
        if ((ctrl.flags & RESIZE_Y_MASK) != RESIZE_Y_STRETCH)
        	newBounds.extent.y = curBounds.extent.y;
        if (!SameBounds(&curBounds, &newBounds)) {
        	word objindex = MyFrmGetObjectIndex(pForm, ctrl.id); // assumed graffiti has id 0
        	FormObjectKind type = FrmGetObjectType(pForm, objindex);
        	// handle integral height objects
        	if ((ctrl.flags & RESIZE_Y_MASK) == RESIZE_Y_STRETCH) {
            	if (type == frmFieldObj) {
                    FieldType* pField;
                    FontID oldFont;
                    word  newHeight;

                    pField = (FieldType*) FrmGetObjectPtr(pForm, objindex);
                    oldFont = FntSetFont(FldGetFont(pField));
                    newHeight = newBounds.extent.y / FntLineHeight() *
                                 FntLineHeight();
                    FntSetFont( oldFont );

                    if (newHeight > 0)
                        newBounds.extent.y = newHeight;
            	} else if (type == frmListObj) {
                    ListType* pList;
                    FontID oldFont;
                    word  newHeight;

                    pList = (ListType*) FrmGetObjectPtr(pForm, objindex);
                    oldFont = FntSetFont(LstGlueGetFont(pList));
                    newHeight = newBounds.extent.y / FntLineHeight() *
                                 FntLineHeight();
                    int nVisible = newHeight / FntLineHeight();
                    FntSetFont( oldFont );
                    int nItems = LstGetNumberOfItems(pList);
                    int iTop = LstGetTopItem(pList);
                    if (iTop > 0 && iTop + nVisible > nItems) {
                        iTop = nItems - nVisible;
                        if (iTop < 0) iTop = 0;
                        LstSetTopItem(pList, iTop);
                    }

                    if (newHeight > 0)
                        newBounds.extent.y = newHeight;
            	}
            }
            ctrl.x = newBounds.topLeft.x;
            ctrl.y = newBounds.topLeft.y;
            ctrl.w = newBounds.extent.x;
            ctrl.h = newBounds.extent.y;
            if (ctrl.type == rtBitmap) {
            	FrmSetObjectPosition(pForm, objindex, newBounds.topLeft.x, newBounds.topLeft.y);
            } else {
        		FrmSetObjectBounds(pForm, objindex, &newBounds);
        	}
        	
            if (FrmGetObjectType(pForm, objindex) == frmFieldObj) {
                FieldType* pField;
                word insPt;

                pField = (FieldType*) FrmGetObjectPtr(pForm, objindex);
                insPt = FldGetInsPtPosition(pField);
                FldRecalculateField(pField, true);
                FldSetInsPtPosition(pField, insPt);
                FldSendChangeNotification(pField);
            }
        }
    	ic++;
    }

    if (FrmGetFocus(pForm) != focus)
        FrmSetFocus(pForm, focus);

    return true;
}

// resize support for standard (non-generic) forms
#include "OrbFormsRTRsc.h"
struct ResizeData {
    word id;
    word flags;
    int nCtrls;
    int x, y, w, h;
    int ox, oy, ow, oh;
};

ResizeData resizeData[] = {
    { OutputForm, RESIZE_STRETCH, 2 },
    { OutputOutputField, RESIZE_STRETCH },
    { OutputScroll, RESIZE_Y_STRETCH | RESIZE_X_ATTACH_RIGHT },
    { GraphForm, RESIZE_STRETCH, 0 }
};
int cResizeData = sizeof(resizeData) / sizeof(resizeData[0]);

int FindObject(word id) {
    for (int i=0; i<cResizeData; i++) {
        if (resizeData[i].id == id)
            return i;
    }
    return -1;
}

bool LayoutStandardForm(word formID) {
    Coord extentX;
    Coord extentY;
    UInt16 focus;
    FormType* pForm;

    RectangleType newBounds, curBounds, origBounds;
    
    pForm = FrmGetFormPtr(formID);
    if (pForm == NULL)
        return false;
        
    int index = FindObject(formID);
    if (index == -1) return false;
    
    ResizeData& form = resizeData[index];

    WinGetDisplayExtent(&extentX, &extentY);

    RctSetRectangle(&origBounds, form.ox, form.oy, form.ow, form.oh);
    RctSetRectangle(&curBounds, form.x, form.y, form.w, form.h);
    
    AdjustBounds(&origBounds, &newBounds,
    	extentX - StandardExtent(),
    	extentY - StandardExtent(),
    	form.flags);
    	
    if (SameBounds(&curBounds, &newBounds)) return false;

    form.x = newBounds.topLeft.x;
    form.y = newBounds.topLeft.y;
    form.w = newBounds.extent.x;
    form.h = newBounds.extent.y;
    WinSetBounds(FrmGetWindowHandle(pForm), &newBounds);
    
    // stop right there if the size of the form didn't change
    if (newBounds.extent.x == curBounds.extent.x &&
    	newBounds.extent.y == curBounds.extent.y) return true;

    // The rest is resized relative to the form itself
    extentX = newBounds.extent.x;
    extentY = newBounds.extent.y;
    focus   = FrmGetFocus(pForm);
    int dw = newBounds.extent.x - origBounds.extent.x;
    int dh = newBounds.extent.y - origBounds.extent.y;
    
    word ic = index + 1;
    for (word i = 0; i < form.nCtrls; i++) {
    	ResizeData& ctrl = resizeData[ic];
        RctSetRectangle(&origBounds, ctrl.ox, ctrl.oy, ctrl.ow, ctrl.oh);
        RctSetRectangle(&curBounds, ctrl.x, ctrl.y, ctrl.w, ctrl.h);
        AdjustBounds(&origBounds, &newBounds, dw, dh, ctrl.flags);
        // if we aren't stretching, leave the extents the same
        if ((ctrl.flags & RESIZE_X_MASK) != RESIZE_X_STRETCH)
        	newBounds.extent.x = curBounds.extent.x;
        if ((ctrl.flags & RESIZE_Y_MASK) != RESIZE_Y_STRETCH)
        	newBounds.extent.y = curBounds.extent.y;
        if (!SameBounds(&curBounds, &newBounds)) {
        	word objindex = MyFrmGetObjectIndex(pForm, ctrl.id); // assumed graffiti has id 0
        	FormObjectKind type = FrmGetObjectType(pForm, objindex);
        	// handle integral height objects
        	if ((ctrl.flags & RESIZE_Y_MASK) == RESIZE_Y_STRETCH) {
            	if (type == frmFieldObj) {
                    FieldType* pField;
                    FontID oldFont;
                    word  newHeight;

                    pField = (FieldType*) FrmGetObjectPtr(pForm, objindex);
                    oldFont = FntSetFont(FldGetFont(pField));
                    newHeight = newBounds.extent.y / FntLineHeight() *
                                 FntLineHeight();
                    FntSetFont( oldFont );

                    if (newHeight > 0)
                        newBounds.extent.y = newHeight;
            	} else if (type == frmListObj) {
                    ListType* pList;
                    FontID oldFont;
                    word  newHeight;

                    pList = (ListType*) FrmGetObjectPtr(pForm, objindex);
                    oldFont = FntSetFont(LstGlueGetFont(pList));
                    newHeight = newBounds.extent.y / FntLineHeight() *
                                 FntLineHeight();
                    int nVisible = newHeight / FntLineHeight();
                    FntSetFont(oldFont);
                    int nItems = LstGetNumberOfItems(pList);
                    int iTop = LstGetTopItem(pList);
                    if (iTop > 0 && iTop + nVisible > nItems) {
                        iTop = nItems - nVisible;
                        if (iTop < 0) iTop = 0;
                        LstSetTopItem(pList, iTop);
                    }

                    if (newHeight > 0)
                        newBounds.extent.y = newHeight;
            	}
            }
            ctrl.x = newBounds.topLeft.x;
            ctrl.y = newBounds.topLeft.y;
            ctrl.w = newBounds.extent.x;
            ctrl.h = newBounds.extent.y;
            FrmSetObjectBounds(pForm, objindex, &newBounds);
        	
            if (FrmGetObjectType(pForm, objindex) == frmFieldObj) {
                FieldType* pField;
                word insPt;

                pField = (FieldType*) FrmGetObjectPtr(pForm, objindex);
                insPt = FldGetInsPtPosition(pField);
                FldRecalculateField(pField, true);
                FldSetInsPtPosition(pField, insPt);
                FldSendChangeNotification(pField);
            }
        }
    	ic++;
    }

    if (FrmGetFocus(pForm) != focus)
        FrmSetFocus(pForm, focus);

    return true;
}

void FillResizeDataBounds(int index, RectangleType* bounds) {
    resizeData[index].ox = resizeData[index].x = bounds->topLeft.x;
    resizeData[index].oy = resizeData[index].y = bounds->topLeft.y;
    resizeData[index].ow = resizeData[index].w = bounds->extent.x;
    resizeData[index].oh = resizeData[index].h = bounds->extent.y;
}

void InitStandardFormDIA(word formID) {
    if (romVersion >= ver40) {
        int index = FindObject(formID);
        if (index == -1) {
            SetDIAAllowResize(false);
            SetDIAState(DIA_STATE_MAX);
        } else {
            // get original bounds of controls
            int nCtrls = resizeData[index].nCtrls;
            FormPtr pForm = FrmGetFormPtr(formID);
            RectangleType bounds;
            WinGetBounds(FrmGetWindowHandle(pForm), &bounds);
            FillResizeDataBounds(index, &bounds);

            for (int iCtrl=0; iCtrl<nCtrls; iCtrl++) {
            	word objindex = MyFrmGetObjectIndex(pForm, resizeData[index + iCtrl + 1].id); // assumed graffiti has id 0
                FrmGetObjectBounds(pForm, objindex, &bounds);
                FillResizeDataBounds(index + iCtrl + 1, &bounds);
            }
            
            SetDIAAllowResize(true);
            SetDIAState(DIA_STATE_USER);
            if (diaType == DIA_TYPE_PALM10 || diaType == DIA_TYPE_PALM11) {
                WinSetConstraintsSize(WinGetWindowHandle(FrmGetFormPtr(formID)),
                    resizeData[index].oh, 0x7fff, 0x7fff,
                    resizeData[index].ow, 0x7fff, 0x7fff);
            }
        }
    }
}

