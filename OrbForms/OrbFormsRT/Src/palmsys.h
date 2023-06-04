// system routines
void lib_resetaot(VM* vm, int) {
    EvtResetAutoOffTimer();
}

void lib_sleep(VM* vm, int) {
    Value* ms = vm->pop();
    UInt32 nTicks = ms->iVal / (1000 / SysTicksPerSecond());
    SysTaskDelay(nTicks);
}

void lib_launch(VM* vm, int) {
    UInt16 cardNo;
    LocalID dbID;
    DmSearchStateType searchState;
    Err err;
    
    UInt32 appCreator;
    Value* id = vm->pop();
    
    appCreator = CIDstoi(id->lock());
    id->unlockRelease();
    
    DmGetNextDatabaseByTypeCreator(true, &searchState, sysFileTApplication,
        appCreator, true, &cardNo, &dbID);

    err = 1;
    if (dbID) err = SysUIAppSwitch(cardNo, dbID, 0, 0);
    if (!err) vm->retVal.iVal = 1;
}

void lib_launch_goto(VM* vm, int) {
    UInt16 cardNo;
    LocalID dbID;
    DmSearchStateType searchState;
    Err err;
    
    UInt32 matchCustom = vm->pop()->iVal;
    UInt16 matchField = vm->pop()->iVal;
    UInt16 matchPos = vm->pop()->iVal;
    UInt16 searchStrLen = vm->pop()->iVal;
    UInt16 recId = vm->pop()->iVal;
    Value* vDB = vm->pop();
    UInt32 appCreator;
    Value* id = vm->pop();
    
    appCreator = CIDstoi(id->lock());
    id->unlockRelease();
    
    DmGetNextDatabaseByTypeCreator(true, &searchState, sysFileTApplication,
        appCreator, true, &cardNo, &dbID);

    err = 1;
    if (dbID) {
        GoToParamsPtr gp = (GoToParamsPtr)MemPtrNew(sizeof(GoToParamsType));
        MemSet(gp, sizeof(GoToParamsType), 0);
        MemPtrSetOwner(gp, 0);
        gp->dbID = DmFindDatabase(0, vDB->lock());
        vDB->unlock();
        gp->recordNum = recId;
        gp->matchCustom = matchCustom;
        gp->matchPos = matchPos;
        gp->matchFieldNum = matchField;
        gp->searchStrLen = searchStrLen;
        err = SysUIAppSwitch(0, dbID, sysAppLaunchCmdGoTo, (MemPtr)gp);
    }
    vDB->release();
    if (!err) vm->retVal.iVal = 1;
}

void lib_launch_string(VM* vm, int) {
    UInt16 cardNo;
    LocalID dbID;
    DmSearchStateType searchState;
    Err err;
    
    Value* vParam = vm->pop();
    UInt16 code = vm->pop()->iVal;
    UInt32 appCreator;
    Value* id = vm->pop();
    
    appCreator = CIDstoi(id->lock());
    id->unlockRelease();
    
    DmGetNextDatabaseByTypeCreator(true, &searchState, sysFileTApplication,
        appCreator, true, &cardNo, &dbID);

    err = 1;
    if (dbID) {
        char* str = vParam->lock();
        char* param = (char*)MemPtrNew(strlen(str) + 1);
        strcpy(param, str);
        vParam->unlock();
        MemPtrSetOwner(param, 0);
        err = SysUIAppSwitch(0, dbID, code, (MemPtr)param);
    }
    vParam->release();
    if (!err) vm->retVal.iVal = 1;
}

void lib_username(VM* vm, int) {
    char name[41];
    DlkGetSyncInfo(NULL, NULL, NULL, name, NULL, NULL);
    vm->retVal.copyString(name);
}

void lib_serialnum(VM* vm, int) {
    char* sn = NULL;
    UInt16 len = 0;
    Err ret = SysGetROMToken(0, sysROMTokenSnum, (UInt8**)&sn, &len);
    if (ret == 0 && len && sn && (UInt8)*sn != 0xff) {
        char* data = vm->retVal.newString(len);
        if (data) {
            strncpy(data, sn, len);
            data[len] = 0;
            vm->retVal.unlock();
        } else {
            vm->retVal.newConstString("");
        }
    } else {
        vm->retVal.newConstString("");
    }
}

void lib_osver(VM* vm, int) {
    vm->retVal.iVal = romVersion;
}

void lib_osvers(VM* vm, int) {
    char* strOS = SysGetOSVersionString();
    vm->retVal.copyString(strOS);
    MemPtrFree(strOS);
}

void lib_orbver(VM* vm, int) {
    vm->retVal.iVal = appVersionNum;
}

void lib_getclip(VM* vm, int) {
    word len;
    MemHandle clp = ClipboardGetItem(clipboardText, &len);
    if (clp && len) {
        char* src = (char*)MemHandleLock(clp);
        char* dest = vm->retVal.newString(len);
        if (dest) {
            for (word i=0;i<len;i++)
                dest[i] = src[i];
            dest[len] = 0;
        }
        vm->retVal.unlock();
        MemHandleUnlock(clp);
    }
    else vm->retVal.newConstString("");
}

void lib_setclip(VM* vm, int) {
    Value* text = vm->pop();
    char* pt = text->lock();
    ClipboardAddItem(clipboardText, pt, strlen(pt));
    text->unlockRelease();
}

void lib_memcpy(VM* vm, int) {
    Value* vLen = vm->pop();
    Value* vpSrc = vm->pop();
    Value* vpDst = vm->pop();
    
    long dst = vpDst->iVal;
    long src = vpSrc->iVal;
    long len = vLen->iVal;
    
    for (long i=0;i<len;i++) {
        Value* vSrc = vm->deref(src++);
        Value* vDst = vm->deref(dst++);
        //if (vSrc->type != vDst->type) {
        //	vm->vm_error("memcpy to a location of different type");
        //}
        vm->cleanUp(vDst);
        vm->copyVal(vDst,vSrc);
    }
}

void lib_malloct(VM* vm, int) {
    Value* size = vm->pop();
    Value* type = vm->pop();
    vm->retVal.iVal = vm->globs.alloc(size->iVal, type->lock());
    vm->gsize = vm->globs.size();
    type->unlockRelease();
}

void lib_free(VM* vm, int) {
    Value* vPtr = vm->pop();
    if (vPtr->iVal) {
        vm->globs.release(vPtr->iVal);
        vm->gsize = vm->globs.size();
    }
}

void lib_freemem(VM* vm, int) {
    word heapid;
    dword cbFree;
    dword cbMax;
    heapid = MemPtrHeapID(vm);
    MemHeapFreeBytes(heapid, &cbFree, &cbMax);
    vm->retVal.iVal = cbFree;
}

void lib_battery(VM* vm, int) {
    word volts;
    byte percent;
    volts = SysBatteryInfo(false, NULL, NULL, NULL, NULL, NULL, &percent);
    if (vm->pop()->iVal)
        vm->retVal.iVal = volts;
    else
        vm->retVal.iVal = percent;
}	

struct PrefsContext {
    int size;
    int offset;
    byte* pData;
    bool bDone;
};

bool PrefReadVal(Value* pVal, VarType type, int len, void* pContext);
bool PrefWriteVal(Value* pVal, VarType type, int len, void* pContext);

void lib_prefs_read(VM* vm, int) {
    Value* vCount = vm->pop();
    Value* vFormat = vm->pop();
    Value* vPtr = vm->pop();
    Value* vSaved = vm->pop();
    Value* vCreator = vm->pop();
    vm->pop(); // Preferences object
    
    char* strFormat = vFormat->lock();
    UInt32 cid = CIDstoi(vCreator->lock());
    vCreator->unlockRelease();
    
    word len = 0, ver;
    PrefsContext pc = { 0, };
    
    if (!vCount->iVal) goto done;
    
    if (noPreferenceFound != PrefGetAppPreferences(cid, 0, NULL, &len, vSaved->iVal)) {
        void* pData = malloc(len);
        if (!pData) goto done;
        
        ver = PrefGetAppPreferences(cid, 0, pData, &len, vSaved->iVal);
        
        pc.size = len;
        pc.pData = (byte*)pData;
        pc.bDone = true; // set to false on error
        
        IterateValues(vPtr->iVal, strFormat, vCount->iVal, &PrefReadVal, &pc);

        if (pc.bDone)
            vm->retVal.iVal = 1;
            
        free(pData);
    }
done:
    vFormat->unlockRelease();
}

void lib_prefs_write(VM* vm, int) {
    Value* vCount = vm->pop();
    Value* vFormat = vm->pop();
    Value* vPtr = vm->pop();
    Value* vSaved = vm->pop();
    Value* vCreator = vm->pop();
    vm->pop(); // Preferences object
    
    char* strFormat = vFormat->lock();
    UInt32 cid = CIDstoi(vCreator->lock());
    vCreator->unlockRelease();
    
    word len = 0;
    PrefsContext pc = { 0, };

    if (!vCount->iVal) goto done;
    len = TypeDataSize(vPtr->iVal, strFormat, vCount->iVal);
    
    if (len) {
        void* pData = malloc(len);
        if (!pData) goto done;
        
        pc.size = len;
        pc.pData = (byte*)pData;
        pc.bDone = true; // set to false on error
        
        IterateValues(vPtr->iVal, strFormat, vCount->iVal, &PrefWriteVal, &pc);
        
        if (pc.bDone) {
            PrefSetAppPreferences(cid, 0, 0, pData, len, vSaved->iVal);
            vm->retVal.iVal = 1;
        }
        
        free(pData);
    }
    
done:
    vFormat->unlockRelease();
}

void lib_prefs_del(VM* vm, int) {
    Value* vSaved = vm->pop();
    Value* vCreator = vm->pop();
    vm->pop(); // Preferences object
    
    UInt32 cid = CIDstoi(vCreator->lock());
    vCreator->unlockRelease();
    
    PrefSetAppPreferences(cid, 0 ,0, NULL, 0, vSaved->iVal);
}


bool PrefReadVal(Value* pVal, VarType type, int len, void* pContext) {
    PrefsContext* pPC = (PrefsContext*)pContext;
    bool bsz = false;
    if (pPC->offset + (len ? len : 1) > pPC->size) {
        pPC->bDone = false;
        return false;
    }

    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            pVal->iVal = 0;
            //memcpy(&pVal->iVal, pRec+offset, len);
            memcpy((byte*)(&pVal->iVal) + 4 - len, pPC->pData + pPC->offset, len);
            pPC->offset += len;
            break;
        case vtString: {
            if (!len) {
                bsz = true;
                len = strlen((char*)pPC->pData + pPC->offset);
            }
            pVal->release();
            char* data = pVal->newString(len);
            if (bsz) {
                strcpy(data, (char*)(pPC->pData + pPC->offset));
            } else {
                memcpy(data, pPC->pData + pPC->offset, len);
                data[len] = 0;
            }
            pVal->unlock();
            pPC->offset += len + (bsz ? 1 : 0);
            break;
        }
    }

    return true;
}

bool PrefWriteVal(Value* pVal, VarType type, int len, void* pContext) {
    PrefsContext* pPC = (PrefsContext*)pContext;
    bool bsz = type == vtString && len == 0;
    
    if (bsz) {
        len = strlen(pVal->lock()) + 1;
        pVal->unlock();
    }
    
    if (pPC->offset + len > pPC->size) {
        pPC->bDone = false;
        return false;
    }

    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            //DmWrite(pRec, offset, &pVal->iVal, len);
            //DmWrite(pRec, offset, (byte*)(&pVal->iVal)+4-len, len);
            memcpy(pPC->pData + pPC->offset, (byte*)(&pVal->iVal)+4-len, len);
            pPC->offset += len;
            break;
        case vtString:
            if (bsz) {
                //DmStrCopy(pRec, offset, pVal->lock());
                strcpy((char*)(pPC->pData + pPC->offset), pVal->lock());
                pVal->unlock();
            } else {
                char* str = pVal->lock();
                int slen = strlen(str);
                if (slen > len) {
                    //DmWrite(pRec, offset, str, len);
                    memcpy(pPC->pData + pPC->offset, str, len);
                } else {
                    //DmWrite(pRec, offset, str, slen);
                    memcpy(pPC->pData + pPC->offset, str, slen);
                    if (len > slen) {
                        //DmSet(pRec, offset + slen, len - slen, 0);
                        memset(pPC->pData + pPC->offset + slen, len - slen, 0);
                    }
                }
                pVal->unlock();
            }
            pPC->offset += len;
            break;
    }

    return true;
}

/*
    //int getRotation() @ 0;
    //bool setRotation(int) @ 0;
    //int getInputState() @ 0;
    //bool setInputState(int) @ 0;

void lib_app_getInputState(VM* vm, int) {
    vm->pop(); // ignore app
    DIAState state = GetDIAState();
    if (state == DIA_STATE_UNDEFINED)
        state = DIA_STATE_MAX; // best guess
    vm->retVal.iVal = state;
}

void lib_app_setInputState(VM* vm, int) {
    DIAState state = vm->pop()->iVal;
    vm->pop(); // ignore app
    
    SetDIAState(state);
}

void lib_app_getRotation(VM* vm, int) {
    vm->pop(); // ignore app
    vm->retVal.iVal = GetDIARotation();
}

void lib_app_setRotation(VM* vm, int) {
    bool landscape = vm->pop()->iVal;
    vm->pop(); // ignore app
    
    SetDIARotation(landscape);
}
*/
