static FieldPtr getField(VM* vm, word& id) {
    word fid;
    int index = popCtrl(vm, id, fid);
    return (FieldPtr)GetObjectPtr(id);
}

void get_field_text(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    const char* str = FldGetTextPtr(pCtrl);
    if (!str) str = "";
    char* vbuff = vm->retVal.newString(strlen(str));
    strcpy(vbuff, str);
    vm->retVal.unlock();
}

void set_field_text(VM* vm, int) {
    Value* p = vm->pop();
    assert(p->type == vtString);
    word id;
    FieldPtr pCtrl = getField(vm, id);
    MemHandle hOld = FldGetTextHandle(pCtrl);
    char* pText = p->lock();
    MemHandle hNew = MemHandleNew(strlen(pText)+1);
    char* pNew = (char*)MemHandleLock(hNew);
    strcpy(pNew, pText);
    MemHandleUnlock(hNew);
    FldSetTextHandle(pCtrl, hNew);
    if (FrmVisible(FrmGetActiveForm()))
        FldDrawField(pCtrl);
    if (hOld)
        MemHandleFree(hOld);
    p->unlock();
    vm->moveVal(&vm->retVal, p);
}

void get_field_inspt(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = FldGetInsPtPosition(pCtrl);
}

void set_field_inspt(VM* vm, int) {
    Value* p = vm->pop();
    assert(p->type == vtInt);
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = p->iVal;
    FldSetInsPtPosition(pCtrl, p->iVal);
}

void get_field_seltext(VM* vm, int) {
    word id, start, end;
    FieldPtr pCtrl = getField(vm, id);
    FldGetSelection(pCtrl, &start, &end);
    if (start == end) {
        vm->retVal.newConstString("");
    } else {
        char* pText = FldGetTextPtr(pCtrl);
        char* pstr = vm->retVal.newString(end - start);
        strncpy(pstr, &pText[start], end - start);
        pstr[end - start] = '\0';
        vm->retVal.unlock();
    }
}

static void field_insert(VM* vm, bool setRet) {
    Value* p = vm->pop();
    word id;
    FieldPtr pCtrl = getField(vm, id);
    char* pText = p->lock();
    FldInsert(pCtrl, pText, strlen(pText));
    p->unlock();
    if (setRet) {
        vm->moveVal(&vm->retVal, p);
    } else {
        vm->cleanUp(p);
    }
}

void set_field_seltext(VM* vm, int) {
    field_insert(vm, true);
}

void get_field_fontid(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = (long)FldGetFont(pCtrl);
}

void set_field_fontid(VM* vm, int) {
    Value* p = vm->pop();
    assert(p->type == vtInt);
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = p->iVal;
    FldSetFont(pCtrl, (FontID)p->iVal);
}

void get_field_scrollpos(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = FldGetScrollPosition(pCtrl);
}

void set_field_scrollpos(VM* vm, int) {
    Value* p = vm->pop();
    assert(p->type == vtInt);
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = p->iVal;
    FldSetScrollPosition(pCtrl, p->iVal);
}

void lib_field_scroll(VM* vm, int) {
    Value* p = vm->pop();
    word id;
    FieldPtr pCtrl = getField(vm, byref id);
    if (p->iVal > 0) {
        FldScrollField(pCtrl, p->iVal, winDown);
    } else {
        FldScrollField(pCtrl, -p->iVal, winUp);
    }
}

void get_field_dirty(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = FldDirty(pCtrl);
}

void set_field_dirty(VM* vm, int) {
    Value* p = vm->pop();
    assert(p->type == vtInt);
    word id;
    FieldPtr pCtrl = getField(vm, id);
    vm->retVal.iVal = p->iVal;
    FldSetDirty(pCtrl, p->iVal);
}

void lib_field_insert(VM* vm, int) {
    field_insert(vm, false);
}

void lib_field_select(VM* vm, int) {
    Value* end = vm->pop();
    Value* start = vm->pop();
    assert(end->type == vtInt);
    assert(start->type == vtInt);
    word id;
    FieldPtr pCtrl = getField(vm, id);
    FldSetSelection(pCtrl, start->iVal, end->iVal);
}

void lib_field_cut(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    FldCut(pCtrl);
}

void lib_field_copy(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    FldCopy(pCtrl);
}

void lib_field_paste(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    FldPaste(pCtrl);
}

void lib_field_undo(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    FldUndo(pCtrl);
}

void lib_field_del(VM* vm, int) {
    Value* end = vm->pop();
    Value* start = vm->pop();
    assert(end->type == vtInt);
    assert(start->type == vtInt);
    word id;
    FieldPtr pCtrl = getField(vm, id);
    FldDelete(pCtrl, start->iVal, end->iVal);
}

void lib_field_grabfocus(VM* vm, int) {
    word id;
    word fid;
    int index = popCtrl(vm, id, fid);
    FormPtr pForm = FrmGetFormPtr(fid);
    word iObj =  FrmGetObjectIndex(pForm, id);
    FrmSetFocus(pForm, iObj);
}

void lib_field_relfocus(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    FldReleaseFocus(pCtrl);
}

void lib_field_phonelookup(VM* vm, int) {
    word id;
    FieldPtr pCtrl = getField(vm, id);
    PhoneNumberLookup(pCtrl);
}

void set_field_editable(VM* vm, int) {
    word id;
    Value* edit = vm->pop();
    FieldPtr fp = getField(vm, id);
    FieldAttrType fAttr;
    FldGetAttributes(fp, &fAttr);
    fAttr.editable = (bool)edit->iVal;
    FldSetAttributes(fp, &fAttr);
    vm->retVal.iVal = edit->iVal;
}

void set_field_numeric(VM* vm, int) {
    word id;
    Value* numeric = vm->pop();
    FieldPtr fp = getField(vm, id);
    FieldAttrType fAttr;
    FldGetAttributes(fp, &fAttr);
    fAttr.numeric = (bool)numeric->iVal;
    FldSetAttributes(fp, &fAttr);
    vm->retVal.iVal = numeric->iVal;
}

void set_field_underline(VM* vm, int) {
    word id;
    Value* underline = vm->pop();
    FieldPtr fp = getField(vm, id);
    FieldAttrType fAttr;
    FldGetAttributes(fp, &fAttr);
    fAttr.underlined = (0x3 & underline->iVal);
    FldSetAttributes(fp, &fAttr);
    vm->retVal.iVal = underline->iVal;
}
