static ListPtr getList(VM* vm, word& id) {
    word fid;
    int index = popCtrl(vm, id, fid);
    return (ListPtr)GetObjectPtr(id);
}

void get_UIList_selitem(VM* vm, int) {
    word id;
    ListPtr pList = getList(vm, id);
    vm->retVal.iVal = LstGetSelection(pList);
}

void set_UIList_selitem(VM* vm, int) {
    word id;
    Value* v = vm->pop();
    assert(v->type == vtInt);
    ListPtr pList = getList(vm, id);
    if (LstGetNumberOfItems(pList) > 0)
        LstSetSelection(pList, v->iVal);
    vm->retVal.iVal = v->iVal;
}

void lib_UIList_gettext(VM* vm, int) {
    word id;
    Value* v = vm->pop();
    assert(v->type == vtInt);
    ListPtr pList = getList(vm, id);
    vm->retVal.copyString(LstGetSelectionText(pList, v->iVal));	
}

void set_UIList_topitem(VM* vm, int) {
    word id;
    Value* v = vm->pop();
    assert(v->type == vtInt);
    ListPtr pList = getList(vm, id);
    LstSetTopItem(pList, v->iVal);
    vm->retVal.iVal = v->iVal;
}

void get_UIList_count(VM* vm, int) {
    word id;
    ListPtr pList = getList(vm, id);
    vm->retVal.iVal = LstGetNumberOfItems(pList);
}

void lib_UIList_makevisible(VM* vm, int) {
    word id;
    Value* v = vm->pop();
    assert(v->type == vtInt);
    ListPtr pList = getList(vm, id);
    if (v->iVal < LstGetNumberOfItems(pList))
        LstMakeItemVisible(pList, v->iVal);
}

void lib_UIList_scroll(VM* vm, int) {
    word id;
    Value* v = vm->pop();
    assert(v->type == vtInt);
    ListPtr pList = getList(vm, id);
    vm->retVal.iVal = LstScrollList(pList, v->iVal > 0 ? winDown : winUp, v->iVal < 0 ? -v->iVal : v->iVal);
}

void lib_UIList_popup(VM* vm, int) {
    word id;
    ListPtr pList = getList(vm, id);
    vm->retVal.iVal = LstPopupList(pList);
}

void lib_UIList_redraw(VM* vm, int) {
    word id;
    ListPtr pList = getList(vm, id);
    LstDrawList(pList);
}

void lib_UIList_setitems(VM* vm, int) {
    word id, fid;
    Value* p = vm->pop();
    Value* n = vm->pop();
    assert(n->type == vtInt);
    assert(p->type == vtInt);
    int index = popCtrl(vm, id, fid);
    ListPtr pList = (ListPtr)GetObjectPtr(id);
    void*& pdata = ctrls[index].pdata;
    if (pdata) {
        MemPtrFree(pdata);
        pdata = NULL;
    }
    if (n->iVal != 0) {
        dword size = n->iVal * sizeof(char*);
        for (int i=0;i<n->iVal;i++) {
            Value* v = vm->deref(p->iVal + i);
            if (v->type != vtString)
                vm->vm_error("UIList.setitems(): not a string");
            size += strlen(v->lock()) + 1;
            v->unlock();
        }
        pdata = MemPtrNew(size);
        if (pdata == NULL)
            return;
        dword off = n->iVal * sizeof(char*);
        for (int i=0;i<n->iVal;i++) {
            ((char**)pdata)[i] = (char*)(((char*)pdata) + off);
            Value* v = vm->deref(p->iVal + i);
            char* str = v->lock();
            strcpy(((char*)pdata) + off, str);
            off += strlen(str) + 1;
            v->unlock();
        }
    }
    LstSetListChoices(pList, (char**)pdata, n->iVal);
    
    vm->retVal.iVal = 1;
}

void lib_UIList_setitemslist(VM* vm, int) {
    word id, fid;
    NativeStringList* pStrList = (NativeStringList*)PopNO(vm);
    int index = popCtrl(vm, id, fid);
    ListPtr pList = (ListPtr)GetObjectPtr(id);
    void*& pdata = ctrls[index].pdata;
    if (pdata) {
        MemPtrFree(pdata);
        pdata = NULL;
    }
    int n = pStrList->strs.size();
    
    if (n != 0) {
        dword size = n * sizeof(char*);
        for (int i=0;i<n;i++) {
            size += strlen(pStrList->strs[i]) + 1;
        }
        pdata = MemPtrNew(size);
        if (pdata == NULL)
            return;
        dword off = n * sizeof(char*);
        for (int i=0;i<n;i++) {
            ((char**)pdata)[i] = (char*)(((char*)pdata) + off);
            char* str = pStrList->strs[i];
            strcpy(((char*)pdata) + off, str);
            off += strlen(str) + 1;
        }
    }
    LstSetListChoices(pList, (char**)pdata, n);
    
    vm->retVal.iVal = 1;
}

struct CustomList {
    int index;
    long addr;
};

vector<CustomList> customLists;

void CustomDrawList(Int16 itemNum, RectangleType *bounds, Char **itemsText) {
    long addr = 0;
    int i;
    for (i=0;i<customLists.size();i++) {
        if (ctrls[customLists[i].index].pdata == itemsText) {
            addr = customLists[i].addr;
            break;
        }
    }
    assert(addr != 0);
    if (addr == 0) return;
    // void drawfunc(UIList list, int item, int x, int y, int w, int h, string text)
    Value val;
    val.type = vtInt;
    //val.iVal = ctrls[i].addr;
    //vm->push(&val);
    val.iVal = itemNum;
    vm->push(&val);
    val.iVal = bounds->topLeft.x;
    vm->push(&val);
    val.iVal = bounds->topLeft.y;
    vm->push(&val);
    val.iVal = bounds->extent.x;
    vm->push(&val);
    val.iVal = bounds->extent.y;
    vm->push(&val);
    val.copyString(itemsText[itemNum]);
    vm->push(&val);
    vm->Call(addr, true, false);
    vm->killVM = false;
}

void lib_UIList_setdrawfunc(VM* vm, int) {
    long addr = vm->pop()->iVal;
    word id, fid;
    int index = popCtrl(vm, id, fid);
    ListPtr pList = (ListPtr)GetObjectPtr(id);
    int i;
    for (i=0;i<customLists.size();i++) {
        if (index == customLists[i].index) break;
    }
    if (i < customLists.size()) {
        customLists[i].addr = addr;
    } else {
        CustomList cl;
        cl.index = index;
        cl.addr = addr;
        customLists.add(cl);
    }
    LstSetDrawFunction(pList, addr ? CustomDrawList : NULL);
}
