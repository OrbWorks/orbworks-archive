int popForm(VM* vm, word &id) {
    id = popID(vm);
    int index = FindForm(id);
    if (index == -1)
        vm->vm_error("form not found");
    return index;
}	

int popCtrl(VM* vm, word &id, word& fid) {
    id = popID(vm);
    int index = FindActiveCtrl(id);
    if (index == -1)
        vm->vm_error("control not found in active form");
    fid = ctrls[index].fid;
    return index;
}

void get_string_text(VM* vm, int) {
    word id = popID(vm);
    MemHandle hStr = DmGetResource('tSTR', id);
    if (hStr) {
        char* pStr = (char*)MemHandleLock(hStr);
        vm->retVal.copyString(pStr);
        MemHandleUnlock(hStr);
    } else {
        vm->retVal.newConstString("");
    }
}

void lib_UIForm_load(VM* vm, int) {
    word id;
    int index = popForm(vm, id);
    // TODO:if this form exists, we don't want to re-create it!!
    //FormPtr pForm = CreateForm(index);
    FrmGotoForm(id);
}

void lib_UIForm_domodal(VM* vm, int) {
    parentForm = FrmGetActiveFormID();
    word id;
    int index = popForm(vm, id);
    //FormPtr pForm = CreateForm(index);
    FrmPopupForm(id);
}

void lib_UIForm_dodialog(VM* vm, int) {
    word id;
    int index = popForm(vm, id);
    FormPtr pForm = FrmInitForm(id);
    word buttonID = FrmDoDialog(pForm);
    FrmDeleteForm(pForm);
    vm->retVal.iVal = buttonID;
    vm->killVM = false;
}

void lib_UIForm_close(VM* vm, int) {
    //if (lastForm == -1)
    //	vm->vm_error("form.close may only be called from a modal form");
    // TODO: cannot close only form
    word id;
    int index = popForm(vm, id);
    CloseOrbForm(id, false, true); // free ui data, but OS deletes the form
    FrmReturnToForm(0);
    vm->killVM = false;
    parentForm = 0;
}

void set_UIForm_menu(VM* vm, int) {
    Value* p = vm->pop();
    word id = p->iVal;
    word fid;
    int index = popForm(vm, fid);
    FormPtr pForm = FrmGetFormPtr(fid);
    FrmSetMenu(pForm, id);
    if (!id)
        MenuSetActiveMenuRscID(0);
}

void get_UIForm_obscured(VM* vm, int) {
    vm->pop(); // ignore form
    //vm->retVal.iVal = MenuGetActiveMenu() ? 1 : 0;
    vm->retVal.iVal = (FrmGetWindowHandle(FrmGetActiveForm()) == WinGetActiveWindow()) ? 0 : 1;
}

void get_UIForm_title(VM* vm, int) {
    word id;
    int index = popForm(vm, id);
    FormPtr pForm = FrmGetFormPtr(id);
    const char* str = FrmGetTitle(pForm);
    vm->retVal.copyString(str);
}

void set_UIForm_title(VM* vm, int) {
    Value* p = vm->pop();
    word id;
    int index = popForm(vm, id);
    FormPtr pForm = FrmGetFormPtr(id);
    void*& pdata = forms[index].pdata;

    // copy the text safely
    int len = forms[index].ldata;
    // assert(len); this CAN be 0
    char* str = p->lock();
    if (strlen(str) < len && pdata == NULL) {
        FrmCopyTitle(pForm, str);
    } else if (strlen(str) < len) {
        strcpy((char*)pdata, str);
        FrmSetTitle(pForm, (char*)pdata);
    } else {
        if (pdata) {
            free(pdata);
            pdata = NULL;
        }
        len = strlen(str) + 1;
        pdata = new char[len];
        if (pdata) {
            forms[index].ldata = len;
            strcpy((char*)pdata, str);
            FrmSetTitle(pForm, (char*)pdata);
        } else {
            forms[index].ldata = 0;
            FrmCopyTitle(pForm, "");
        }
    }

    p->unlock();
    vm->moveVal(&vm->retVal, p);
}

void lib_UIForm_select(VM* vm, int) {
    word id, fid;
    Value* vid = vm->pop();
    assert(vid->type == vtInt);
    id = vid->iVal;
    Value* p = vm->pop();
    assert(p->type == vtInt);
    fid = popID(vm);
    FormPtr pForm = FrmGetFormPtr(fid);
    FrmSetControlGroupSelection(pForm, p->iVal, id);
}

void lib_UIForm_timer(VM* vm, int) {
    word fid;
    Value* p = vm->pop();
    assert(p->type == vtInt);
    fid = popID(vm);
    timer.SetTimer(fid, p->iVal);
}

void lib_UIForm_help(VM* vm, int) {
    Value* straddr = vm->pop();
    Value* id = vm->deref(straddr->iVal);
    vm->pop(); // UIForm
    FrmHelp(id->iVal);
}

void lib_UIForm_redraw(VM* vm, int) {
    word fid;
    Value* vCode = vm->pop();
    fid = popID(vm);
    FrmUpdateForm(fid, vCode->iVal);
}

void lib_UIForm_activefield(VM* vm, int) {
    word id;
    word fid = popID(vm);
    FormPtr pForm = FrmGetFormPtr(fid);
    word iObj =  FrmGetFocus(pForm);
    if (iObj != noFocus && FrmGetObjectType(pForm,iObj) == frmFieldObj) {
        id = FrmGetObjectId(pForm, iObj);
        word index = FindCtrl(id);
        vm->retVal.iVal = ctrls[index].addr;
    }
}

void lib_UIForm_keyboard(VM* vm, int) {
    vm->pop(); // UIForm
    SysKeyboardDialog(kbdDefault);
}

void set_UIFBitmap_bmpid(VM* vm, int) {
    Value* v = vm->pop();
    assert(v->type ==vtInt);
    word id, fid;
    int index = popCtrl(vm, id, fid);
    // TODO: bitmaps don't have IDs in the form list
    /*
    FormPtr pForm = FrmGetFormPtr(fid);
    word iObj =  FrmGetObjectIndex(pForm, id);
    FormBitmapType* pBmp = (FormBitmapType*)FrmGetObjectPtr(pForm, iObj);
    pBmp->rscID = v->iVal;
    FrmHideObject(pForm, iObj);
    FrmShowObject(pForm, iObj);
    */
    vm->retVal.iVal = v->iVal;
}

void get_UILabel_text(VM* vm, int) {
    word id, fid;
    int index = popCtrl(vm, id, fid);
    FormPtr pForm = FrmGetFormPtr(fid);
    const char* str = FrmGetLabel(pForm, id);
    vm->retVal.copyString(str);
}

void set_UILabel_text(VM* vm, int) {
    Value* p = vm->pop();
    word id, fid;
    int index = popCtrl(vm, id, fid);
    FormPtr pForm = FrmGetFormPtr(fid);

    //dword total, block;
    //MemHeapFreeBytes(MemPtrHeapID(vm), &total, &block);
    // copy the text safely
    int len = ctrls[index].ldata;
    assert(len);
    char* data = new char[len];
    strncpy(data, p->lock(), len);
    p->unlock();
    data[len-1] = '\0';

    // hide
    word iObj =  FrmGetObjectIndex(pForm, id);
    FrmHideObject(pForm, iObj);

    FrmCopyLabel(pForm, id, data);
    free(data);

    // show
    if (ctrls[index].visible)
        FrmShowObject(pForm, iObj);
    vm->moveVal(&vm->retVal, p);
}

void get_control_text(VM* vm, int) {
    word id, fid;
    int index = popCtrl(vm, id, fid);
    ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
    const char* str = CtlGetLabel(pCtrl);
    vm->retVal.copyString(str);
}

void set_control_text(VM* vm, int) {
    Value* p = vm->pop();
    word id, fid;
    int index = popCtrl(vm, id, fid);
    ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
    char* str = p->lock();
    void*& cstr = ctrls[index].pdata;
    if (cstr) {
        MemPtrFree(cstr);
        cstr = NULL;
    }
    cstr = MemPtrNew(strlen(str)+1);
    if (cstr) {
        strcpy((char*)cstr, str);
        CtlSetLabel(pCtrl, (char*)cstr);
    }
    p->unlock();
    vm->moveVal(&vm->retVal, p);
}

void get_control_value(VM* vm, int) {
    word id, fid;
    int index = popCtrl(vm, id, fid);
    ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
    vm->retVal.iVal = CtlGetValue(pCtrl);
}

void set_control_value(VM* vm, int) {
    Value* v = vm->pop();
    int val = v->iVal;
    word id, fid;
    int index = popCtrl(vm, id, fid);
    ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
    CtlSetValue(pCtrl, val);
    vm->retVal.iVal = val;
}

void set_control_bmp(VM* vm, int) {
    Value* bmpid = vm->pop();
    word id, fid;
    int index = popCtrl(vm, id, fid);
    if (romVersion >= ver35) {
        ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
        CtlSetGraphics(pCtrl, bmpid->iVal, NULL);
    }
    vm->retVal.iVal = bmpid->iVal;
}

void set_control_sbmp(VM* vm, int) {
    Value* bmpid = vm->pop();
    word id, fid;
    int index = popCtrl(vm, id, fid);
    if (romVersion >= ver35) {
        ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
        CtlSetGraphics(pCtrl, NULL, bmpid->iVal);
    }
    vm->retVal.iVal = bmpid->iVal;
}

void set_control_visible(VM* vm, int) {
    Value* v = vm->pop();
    int val = v->iVal;
    word id, fid;
    int index = popCtrl(vm, id, fid);
    FormPtr pForm = FrmGetFormPtr(fid);
    word iObj =  FrmGetObjectIndex(pForm, id);
    if (FrmGetObjectType(pForm, iObj) == frmGadgetObj) {
        if (val) {
            if (!ctrls[index].visible) {
                CallHandler(ctrls[index].addr, ctrls[index].handlers[2]);
                vm->killVM = false;
                ctrls[index].visible = true;
            }
        } else {
            if (ctrls[index].visible) {
                RectangleType rect;
                RctSetRectangle(&rect, ctrls[index].x, ctrls[index].y,
                    ctrls[index].w, ctrls[index].h);
                WinEraseRectangle(&rect, 0);
                ctrls[index].visible = false;
            }
        }
    } else {
        if (val) {
            FrmShowObject(pForm, iObj);
            ctrls[index].visible = true;
        } else {
            FrmHideObject(pForm, iObj);
            ctrls[index].visible = false;
        }
    }
    vm->retVal.iVal = val;
}

void get_control_visible(VM* vm, int) {
    word id, fid;
    int index = popCtrl(vm, id, fid);
    vm->retVal.iVal = ctrls[index].visible;
}

void set_control_enabled(VM* vm, int) {
    Value* v = vm->pop();
    int val = v->iVal;
    word id, fid;
    int index = popCtrl(vm, id, fid);
    FormPtr pForm = FrmGetFormPtr(fid);
    word iObj =  FrmGetObjectIndex(pForm, id);
    ctrls[index].enabled = val;
    if (FrmGetObjectType(pForm, iObj) == frmGadgetObj) {
        // nothing needed
    } else if (FrmGetObjectType(pForm, iObj) == frmControlObj || FrmGetObjectType(pForm, iObj) == frmPopupObj) {
        ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
        CtlSetEnabled(pCtrl, val);
    }
    vm->retVal.iVal = val;
}

void get_control_enabled(VM* vm, int) {
    word id, fid;
    int index = popCtrl(vm, id, fid);
    vm->retVal.iVal = ctrls[index].enabled;
}

const int x_bounds_func = 13; // BEWARE FUNC INDEX
const int x_fbounds_func = 17;

void get_control_bounds(VM* vm, int func) {
    assert(func >= x_bounds_func && func <= x_bounds_func + 3);
    word id, fid, iObj;
    int index = popCtrl(vm, id, fid);
    FormPtr pForm = FrmGetFormPtr(fid);
    iObj = FrmGetObjectIndex(pForm, id);
    RectangleType rect;
    FrmGetObjectBounds(pForm, iObj, &rect);
    int val;
    switch (func - x_bounds_func) {
        case 0: val = rect.topLeft.x; break;
        case 1: val = rect.topLeft.y; break;
        case 2: val = rect.extent.x; break;
        case 3: val = rect.extent.y; break;
    }
    vm->retVal.iVal = val;
}

void get_form_bounds(VM* vm, int func) {
    assert(func >= x_fbounds_func && func <= x_fbounds_func + 3);
    word id;
    int index = popForm(vm, id);
    FormPtr pForm = FrmGetFormPtr(id);
    RectangleType rect;
    FrmGetFormBounds(pForm, &rect);
    int val;
    switch (func - x_fbounds_func) {
        case 0: val = rect.topLeft.x; break;
        case 1: val = rect.topLeft.y; break;
        case 2: val = rect.extent.x; break;
        case 3: val = rect.extent.y; break;
    }
    vm->retVal.iVal = val;
}

const int x_bounds_func_set = 271; // BEWARE FUNC INDEX

void set_control_bounds(VM* vm, int func) {
    assert(func >= x_bounds_func_set && func <= x_bounds_func_set + 3);
    word id, fid, iObj;
    Value* v = vm->pop();
    int index = popCtrl(vm, id, fid);
    FormPtr pForm = FrmGetFormPtr(fid);
    iObj = FrmGetObjectIndex(pForm, id);
    RectangleType rect;
    FrmGetObjectBounds(pForm, iObj, &rect);

    int val = v->iVal;
    switch (func - x_bounds_func_set) {
        case 0: rect.topLeft.x = val; break;
        case 1: rect.topLeft.y = val; break;
        case 2: rect.extent.x = val; break;
        case 3: rect.extent.y = val; break;
    }

    // if this is a gadget, we need to update the bounds in our list
    //if (FrmGetObjectType(pForm, iObj) == frmGadgetObj) {
    {
        switch (func - x_bounds_func_set) {
            case 0: ctrls[index].x = val; break;
            case 1: ctrls[index].y = val; break;
            case 2: ctrls[index].w = val; break;
            case 3: ctrls[index].h = val; break;
        }
    }
    FrmSetObjectBounds(pForm, iObj, &rect);
    vm->retVal.iVal = val;
}

// TODO: consider making this one function

void get_Event_x(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.x;
}

void get_Event_y(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.y;
}

void get_Event_nx(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.nx;
}

void get_Event_ny(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.ny;
}

void get_Event_inside(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.inside;
}

void get_Event_value(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.value;
}

void get_Event_prev(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.prev;
}

void get_Event_key(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.type = vtChar;
    vm->retVal.cVal = curEvent.key;
}

void get_Event_code(VM* vm, int) {
    vm->pop(); // ignore event address
    vm->retVal.iVal = curEvent.code;
}

enum { funcSliderGetValue = 53, funcSliderGetMin,
       funcSliderGetMax, funcSliderGetPage,
       funcSliderSetValue, funcSliderSetMin,
       funcSliderSetMax, funcSliderSetPage };

void get_Slider_val(VM* vm, int iFunc) {
    word val, *pValue=NULL, *pMin=NULL, *pMax=NULL, *pPage=NULL;
    word id, fid;
    int index = popCtrl(vm, id, fid);
    ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
    switch (iFunc) {
        case funcSliderGetValue: pValue = &val; break;
        case funcSliderGetMin: pMin = &val; break;
        case funcSliderGetMax: pMax = &val; break;
        case funcSliderGetPage: pPage = &val; break;
    }
    CtlGetSliderValues(pCtrl, pMin, pMax, pPage, pValue);
    vm->retVal.iVal = val;
}

void set_Slider_val(VM* vm, int iFunc) {
    word val, *pValue=NULL, *pMin=NULL, *pMax=NULL, *pPage=NULL;
    word id, fid;
    Value* v = vm->pop();
    assert(v->type == vtInt);
    val = v->iVal;
    int index = popCtrl(vm, id, fid);
    ControlPtr pCtrl = (ControlPtr)GetObjectPtr(id);
    switch (iFunc) {
        case funcSliderSetValue: pValue = &val; break;
        case funcSliderSetMin: pMin = &val; break;
        case funcSliderSetMax: pMax = &val; break;
        case funcSliderSetPage: pPage = &val; break;
    }
    CtlSetSliderValues(pCtrl, pMin, pMax, pPage, pValue);
    vm->retVal.iVal = val;
}

enum { funcScrollGetValue = 246, funcScrollGetMin,
       funcScrollGetMax, funcScrollGetPage,
       funcScrollSetValue, funcScrollSetMin,
       funcScrollSetMax, funcScrollSetPage };

void get_scroll_val(VM* vm, int iFunc) {
    short value, min, max, page;
    short *pVal = &value;
    word id, fid;
    int index = popCtrl(vm, id, fid);
    ScrollBarPtr pCtrl = (ScrollBarPtr)GetObjectPtr(id);
    switch (iFunc) {
        case funcScrollGetValue: pVal = &value; break;
        case funcScrollGetMin: pVal = &min; break;
        case funcScrollGetMax: pVal = &max; break;
        case funcScrollGetPage: pVal = &page; break;
    }
    SclGetScrollBar(pCtrl, &value, &min, &max, &page);
    vm->retVal.iVal = *pVal;
}

void set_scroll_val(VM* vm, int iFunc) {
    short value, min, max, page;
    short *pVal = &value;
    word id, fid;
    Value* v = vm->pop();
    assert(v->type == vtInt);

    int index = popCtrl(vm, id, fid);
    ScrollBarPtr pCtrl = (ScrollBarPtr)GetObjectPtr(id);
    SclGetScrollBar(pCtrl, &value, &min, &max, &page);
    
    switch (iFunc) {
        case funcScrollSetValue: pVal = &value; break;
        case funcScrollSetMin: pVal = &min; break;
        case funcScrollSetMax: pVal = &max; break;
        case funcScrollSetPage: pVal = &page; break;
    }

    *pVal = v->iVal;
    SclSetScrollBar(pCtrl, value, min, max, page);
    vm->retVal.iVal = *pVal;
}

void lib_scroll_update(VM* vm, int) {
    word id, fid;
    popCtrl(vm, id, fid);
    FieldPtr pFld = (FieldPtr)GetObjectPtr(id);
    popCtrl(vm, id, fid);
    ScrollBarPtr pScl = (ScrollBarPtr)GetObjectPtr(id);
    
    word scrollPos, textHeight, fieldHeight;
    short maxValue;
    
    FldGetScrollValues (pFld, &scrollPos, &textHeight,  &fieldHeight);

    if (textHeight > fieldHeight) maxValue = textHeight - fieldHeight;
    else if (scrollPos) maxValue = scrollPos;
    else maxValue = 0;

    SclSetScrollBar (pScl, scrollPos, 0, maxValue, fieldHeight-1);
}

void lib_app_hookhard(VM* vm, int) {
    Value* vBool = vm->pop();
    vm->pop(); // app
    
    bHookHard = vBool->iVal != 0;
}

void lib_app_exit(VM* vm, int) {
    ErrThrow(5);
}

void lib_app_getSysPref(VM* vm, int) {
    int id = vm->pop()->iVal;
    vm->pop(); // eat app
    vm->retVal.iVal = PrefGetPreference((SystemPreferencesChoice)id);
}

void get_app_launchArgs(VM* vm, int) {
    vm->pop(); // eat app
    vm->retVal.copyString(g_appArgs);
}

void lib_keystate(VM* vm, int) {
    vm->retVal.iVal = KeyCurrentState();
}

void lib_penstate(VM* vm, int) {
    Int16 x, y;
    Boolean b;
    EvtGetPen(&x, &y, &b);
    vm->retVal.iVal = b;
}

#define alertMsgXPos			32
#define alertXMargin			8
#define alertYMargin			7
#define minButtonWidth			36
#define alertButtonMargin		4
#define alertMaxTextLines 		10
#define alertTextLinesForField	2
#define gsiWidth				12
#define frameWidth				0
#define maxTextInput			256

// System bitmaps
#define palmLogoBitmap					10000
#define InformationAlertBitmap 			10004
#define ConfirmationAlertBitmap			10005
#define WarningAlertBitmap				10006
#define ErrorAlertBitmap				10007

// Custom Alert Dialog
#define CustomAlertDialog					12000
#define CustomAlertTextEntryField			12001
#define CustomAlertBitmap					12002
#define CustomAlertField					12003
#define CustomAlertFirstButton				12004

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

FormPtr BuildAlert(const char* title, const char* text, 
    const char* buttonStr, bool bInput, int alertType)
{
    Int16 alertMaxMsgLines = alertMaxTextLines - (bInput ? alertTextLinesForField : 0);
    Coord x, y;
    Coord alertHeight;
    Coord displayWidth;
    Coord displayHeight;
    Coord alertMsgYPos;
    UInt16 i;
    Int16 msgHeight;
    FontID curFont;
    FormPtr pForm;
    UInt16 bitmapID;
    FieldPtr pField;
    ControlPtr pCtrl;
    Int16 spaceLeft;
    Boolean smallButtons;

    // Compute the height the message string.
    WinGetDisplayExtent(&displayWidth, &displayHeight);
    alertMsgYPos = FntLineHeight() + 1 + alertYMargin; // title + margin
    
    // create the form
    pForm = FrmNewForm(CustomAlertDialog, title, 2, 2,
        displayWidth - 4, displayHeight - 4, true, CustomAlertFirstButton, 0, 0);
    
    // add the alert icon
    switch (alertType) {
        case informationAlert: bitmapID = InformationAlertBitmap; break;
        case confirmationAlert: bitmapID = ConfirmationAlertBitmap; break;
        case warningAlert: bitmapID = WarningAlertBitmap; break;
        case errorAlert: bitmapID = ErrorAlertBitmap; break;
    }
    FrmNewBitmap (&pForm, CustomAlertBitmap, bitmapID, alertXMargin, alertMsgYPos);
    
    // add a field for the message
    curFont = FntSetFont(boldFont);
    pField = FldNewField((void**)&pForm, CustomAlertField, alertMsgXPos, alertMsgYPos, displayWidth - alertMsgXPos - 6, 
        FntLineHeight() * alertMaxMsgLines, boldFont, 0, false, false, 
        false, false, leftAlign, false, false, false);
    FldSetTextPtr(pField, (char*)text);
    FldRecalculateField(pField, true);
    msgHeight = max (FldGetTextHeight(pField), (FntLineHeight() * 2));
    RectangleType rect;
    FldGetBounds(pField, &rect);
    rect.extent.y = msgHeight;
    FldSetBounds(pField, &rect);
    
    // add the text entry field and graffiti indicator
    x = alertButtonMargin + 1;								  // 1 for the frame
    y = alertMsgYPos + msgHeight + alertYMargin + 2;
    FntSetFont (stdFont);

    if (bInput) {
        pField = FldNewField((void**)&pForm, CustomAlertTextEntryField, x, y,
            displayWidth - ((alertButtonMargin + 1) * 2) - 1, FntLineHeight(),
            stdFont, maxTextInput - 1, true, true, true, false,
            leftAlign, false, false, false);
        
        y += FntLineHeight() + alertYMargin;
        if (romVersion >= ver35)
            FrmNewGsi(&pForm, displayWidth - (gsiWidth + frameWidth + alertButtonMargin),y + alertButtonMargin);
    }
    
    // add the buttons
    // split the buttons out by : separators
    const int maxButtons = 6;
    const int maxButtonText = 64;
    const char buttonSep = ':';

    int nButtons = 0;
    char buttons[maxButtons][maxButtonText];
    char* iter = (char*)buttonStr;
    int ich = 0;
    while (*iter && nButtons < maxButtons) {
        if (*iter != ':') {
            if (ich < maxButtonText - 1) {
                buttons[nButtons][ich++] = *iter;
            }
        } else {
            buttons[nButtons][ich] = 0;
            ich = 0;
            nButtons++;
        }
        iter++;
    }
    
    if (nButtons < maxButtons) {
        buttons[nButtons][ich] = 0;
        nButtons++;
    }
    
    smallButtons = false;
    for (i = 0; i < nButtons; i++) {
        pCtrl = CtlNewControl((void**)&pForm, CustomAlertFirstButton + i, buttonCtl,
            buttons[i], x, y, 0, 0, stdFont, 0, true);
        
        UInt16 index = FrmGetObjectIndex(pForm, CustomAlertFirstButton + i);
        FrmGetObjectBounds(pForm, index, &rect);
        x += rect.extent.x + alertButtonMargin + 2;  // 2 for frames
        if (rect.extent.x < minButtonWidth)
            smallButtons = true; //we may have to come back and enlarge
    }
    
    //Add width to small buttons if there's room
    spaceLeft = displayWidth - (alertButtonMargin + x + frameWidth);
    if (bInput)
        spaceLeft -= gsiWidth;
    if (spaceLeft > 0 && smallButtons) {
        Int16	spaceAdded = 0;
        Int16	oldWidth;
        
        for (i = 0; i < nButtons; i++) {
            UInt16 index = FrmGetObjectIndex(pForm, CustomAlertFirstButton + i);
            FrmGetObjectBounds(pForm, index, &rect);
            if (spaceAdded)
                rect.topLeft.x += spaceAdded;
            if (spaceLeft && rect.extent.x < minButtonWidth) {
                oldWidth = rect.extent.x;
                rect.extent.x = min(oldWidth + spaceLeft, minButtonWidth);
                spaceAdded += rect.extent.x - oldWidth;
                spaceLeft -= rect.extent.x - oldWidth;
            }
            FrmSetObjectBounds(pForm, index, &rect);
        }
    }
    
    // Resize the form vertically now that we know the message height.
    alertHeight = y + 17;	// 17 = button height + 4 pixels of white space, that is,
                                    // FntLineHeight(stdFont) + 5, for proper spacing from bottom of form.
    WinHandle wh = FrmGetWindowHandle(pForm);
    if (romVersion < ver40) {
        WinHandle prev = WinSetDrawWindow(wh);
        WinGetDrawWindowBounds(&rect);
        WinSetDrawWindow(prev);
    } else {
        WinGetBounds(wh, &rect);
    }
    rect.topLeft.y = displayHeight - alertHeight - 2;
    rect.extent.y = alertHeight;
    WinSetBounds(wh, &rect);
    
    
    FntSetFont(curFont);						// restore font
    
    return pForm;
}

SndSysBeepType alertTypeToBeep[] = { sndInfo, sndConfirmation, sndWarning, sndError };

void lib_customBase(VM* vm, bool bInput, bool bDefault) {
    long addr;
    MemHandle hText = NULL;
    Value* res;
    Value* vDef = NULL;
    if (bInput) {
        addr = vm->pop()->iVal;
        res = vm->deref(addr);
        if (bDefault)
            vDef = vm->pop();
    }
    int type = vm->pop()->iVal;
    Value* vButtons = vm->pop();
    Value* vMessage = vm->pop();
    Value* vTitle = vm->pop();
    char* buttons = vButtons->lock();
    char* title = vTitle->lock();
    Value vMessageCopy;
    vMessageCopy.acquire(vMessage);
    vMessage->release();
    char* message = vMessageCopy.lock();
    
    SetDIAState(DIA_STATE_MAX);
    
    FormPtr pForm = BuildAlert(title, message, buttons, bInput, type);
    vButtons->unlockRelease();
    vTitle->unlockRelease();

    FormActiveStateType	curFrmState;
    MenuEraseStatus(0);
    FrmSaveActiveState(&curFrmState);
    FrmSetActiveForm(pForm);
    FieldPtr pField;
    if (bInput) {
        pField = (FieldPtr)FrmGetObjectPtr(pForm, FrmGetObjectIndex(pForm, CustomAlertTextEntryField));
        FrmSetFocus(pForm, FrmGetObjectIndex (pForm, CustomAlertTextEntryField));
        if (vDef) {
            FldSetTextHandle(pField, NULL);
            MemHandle hText = h_malloc(maxTextInput);
            char* def = (char*)MemHandleLock(hText);
            strncpy(def, vDef->lock(), maxTextInput - 1);
            def[maxTextInput - 1] = '\0';
            MemHandleUnlock(hText);
            vDef->unlockRelease();
            FldSetTextHandle(pField, hText);
        }
    }
    FrmDrawForm(pForm);
    if (type >= 0 && type <= 3)
        SndPlaySystemSound(alertTypeToBeep[type]);
    UInt16 buttonHit = FrmDoDialog(pForm);
    if (bInput) {
        char* str = FldGetTextPtr(pField);
        res->release();
        res->copyString(str);
    }
    FrmDeleteForm (pForm);
    FrmRestoreActiveState(&curFrmState);				// restore active form/window state
    SetDIAState(DIA_STATE_USER);
    vMessageCopy.unlockRelease();
    vm->retVal.iVal = (buttonHit - CustomAlertFirstButton);
    vm->killVM = false;
}

void lib_customAlert(VM* vm, int) {
    lib_customBase(vm, false, false);
}

void lib_customPrompt(VM* vm, int) {
    lib_customBase(vm, true, false);
}

void lib_customPromptDefault(VM* vm, int) {
    lib_customBase(vm, true, true);
}
