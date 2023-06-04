#include "OrbFormsRT.h"
#include "OrbFormsRTRsc.h"

// NativeMemo implementation
DmOpenRef NativeMemo::dbMemo = NULL;
int NativeMemo::nMemos = 0;

NativeMemo::NativeMemo() : offset(0), recid(-1), hRec(NULL), dirty(false), end(false) {
    assert(!nMemos || dbMemo);
    nMemos++;
    if (!dbMemo) {
        dbMemo = DmOpenDatabaseByTypeCreator('DATA', 'PMem', dmModeReadWrite);
        if (!dbMemo)
            dbMemo = DmOpenDatabaseByTypeCreator('DATA', 'memo', dmModeReadWrite);
        assert(dbMemo);
    }
}

NativeMemo::~NativeMemo() {
    if (hRec) {
        Close();
    }
    if (!--nMemos) {
        DmCloseDatabase(dbMemo);
        dbMemo = NULL;
    }
}

void NativeMemo::Close() {
    assert(dbMemo);
    assert(recid != -1);
    DmReleaseRecord(dbMemo, recid, dirty);
    hRec = NULL;
    recid = -1;
    dirty = false;
    end = false;
}

void NativeMemo::Puts(const char* text) {
    int size1 = MemHandleSize(hRec);
    int size2 = strlen(text);
    if (size2) {
        MemHandle hRecNew = DmResizeRecord(dbMemo, recid, size1 + size2);
        if (!hRecNew) oom();
        hRec = hRecNew;
        DmStrCopy(MemHandleLock(hRec), size1-1, text);
        MemHandleUnlock(hRec);
        dirty = true;
        Rewind();
    }
}

void NativeMemo::EnsureOpen(VM* vm) {
    if (!hRec)
        vm->vm_error("Memo must be opened/created first");
}

void NativeMemo::Erase() {
    hRec = DmResizeRecord(dbMemo, recid, 1);
    DmSet(MemHandleLock(hRec), 0, 1, 0);
    MemHandleUnlock(hRec);
    offset = 0;
    dirty = true;
    end = true;
}

void NativeMemo::SetText(const char* text) {
    MemHandle hRecNew = DmResizeRecord(dbMemo, recid, strlen(text) + 1);
    if (!hRecNew) oom();
    hRec = hRecNew;
    DmStrCopy(MemHandleLock(hRec), 0, text);
    MemHandleUnlock(hRec);
    dirty = true;
    Rewind();
}

void NativeMemo::Rewind() {
    offset = 0;
    end = false;
}

bool NativeMemo::Find(const char* name) {
    assert(!hRec);
    
    int nRecs = DmNumRecords(dbMemo);
    MemHandle h_memo = NULL;

    for (UInt16 i=0;i<nRecs;i++) {
        word attr; // It's supposed to be a Byte
        dword uid;
        LocalID lid;

        DmRecordInfo(dbMemo, i, &attr, &uid, &lid);
        if ((attr & dmRecAttrDelete) || (hideSecrets && (attr & dmRecAttrSecret))) continue;
        h_memo = DmQueryRecord(dbMemo, i);

        char* memo = (char*)MemHandleLock(h_memo);
        char* t = memo;

        while (*t && *t != '\n') t++;
        
        if (!strncmp(name, memo, (t-memo))) {
            hRec = DmGetRecord(dbMemo, i);
            recid = i;
            break;
        }
        MemHandleUnlock(h_memo);
        h_memo = NULL;
    }
    
    if (h_memo) MemHandleUnlock(h_memo);
    end = false;
    offset = 0;
    return hRec != NULL;
}

bool NativeMemo::Open(UInt16 id) {
    assert(!hRec);
    recid = -1;
    if (DmNumRecords(dbMemo) > id) {
        word attr; // It's supposed to be a Byte

        DmRecordInfo(dbMemo, id, &attr, NULL, NULL);
        if (!(attr & dmRecAttrDelete) && !(attr & dmRecAttrBusy) && !(hideSecrets && (attr & dmRecAttrSecret)))
            recid = id;
    }
    if (recid == id)
        hRec = DmGetRecord(dbMemo, recid);
    offset = 0;
    end = false;
    return hRec != NULL;
}

bool NativeMemo::Create() {
    assert(!hRec);

    hRec = DmNewRecord(dbMemo, &recid, 1);
    if (hRec) {
        DmSet(MemHandleLock(hRec), 0, 1, 0);
        MemHandleUnlock(hRec);
    }
    
    offset = 0;
    end = true;	
    return hRec != NULL;
}


// NativeDatabase implementation

NativeDatabase::NativeDatabase() {
    dbRef = NULL;
    bRes = false;
}

NativeDatabase::~NativeDatabase() {
    if (dbRef) {
        DmCloseDatabase(dbRef);
        dbRef = NULL;
    }
}

void NativeDatabase::EnsureOpen(VM* vm) {
    if (!dbRef)
        vm->vm_error("Database must be opened/created first");
}

bool NativeDatabase::Open(char* name, bool ro) {
    if (dbRef) return false;
    
    lid = MyFindDatabase(name, byref card);
    if (lid) {
        dbRef = DmOpenDatabase(card, lid, ro ? dmModeReadOnly : dmModeReadWrite);
        if (dbRef) {
            Boolean isRes = false;
            DmOpenDatabaseInfo(dbRef, NULL, NULL, NULL, NULL, &isRes);
            bRes = isRes;
            return true;
        }
    }
    return false;
}

bool NativeDatabase::OpenTC(UInt32 type, UInt32 cid, bool ro) {
    if (dbRef) return false;

    dbRef = DmOpenDatabaseByTypeCreator(type, cid, ro ? dmModeReadOnly : dmModeReadWrite);
    if (dbRef) {
        Boolean isRes = false;
        DmOpenDatabaseInfo(dbRef, &lid, NULL, NULL, &card, &isRes);
        bRes = isRes;
    }
    return (dbRef != NULL);
}

bool NativeDatabase::Create(char* name, UInt32 type, UInt32 cid, bool res) {
    if (dbRef) return false;
    
    Err err = DmCreateDatabase(0, name, cid, type, res);
    if (err == errNone) {
        lid = DmFindDatabase(0, name);
        if (lid) {
            bRes = res;
            card = 0;
            dbRef = DmOpenDatabase(0, lid, dmModeReadWrite);
            if (dbRef) return true;
        }
    }
    return false;
}

void NativeDatabase::Close() {
    if (dbRef) {
        DmCloseDatabase(dbRef);
        dbRef = NULL;
    }
}

void NativeDatabase::Delete() {
    DmCloseDatabase(dbRef);
    dbRef = NULL;
    DmDeleteDatabase(card, lid);
}

void NativeDatabase::GetCatName(int iCat, Value* vName) {
    assert(!bRes);
    char* name = vName->newString(dmCategoryLength);
    if (!name) oom();
    CategoryGetName(dbRef, iCat & 0x0f, name);
    vName->unlock();
}

void NativeDatabase::SetCatName(int iCat, char* name) {
    assert(!bRes);
    CategorySetName(dbRef, iCat & 0x0f, name);
}

void NativeDatabase::MoveRec(int iFrom, int iTo) {
    assert(!bRes);
    DmMoveRecord(dbRef, iFrom, iTo);
}

bool NativeDatabase::GetRec(UInt16 iRec, NativeDBRecord* pRec, bool ro) {
    assert(!bRes);
    return pRec->Open(iRec, this, ro);
}

bool NativeDatabase::NewRec(UInt16* piRec, NativeDBRecord* pRec) {
    assert(!bRes);
    return pRec->Create(piRec, this);
}

bool NativeDatabase::GetRes(UInt32 type, UInt32 id, NativeDBRecord* pRec) {
    assert(bRes);
    return pRec->OpenRes(type, id, this);
}

bool NativeDatabase::NewRes(UInt32 type, UInt32 id, NativeDBRecord* pRec) {
    assert(bRes);
    return pRec->CreateRes(type, id, this);
}

bool NativeDatabase::GetResInfo(UInt16 index, UInt32* type, UInt16* id) {
    if (index < DmNumResources(dbRef)) {
        Err err = DmResourceInfo(dbRef, index, type, id, NULL);
        return err == 0;
    }
    return false;
}

long NativeDatabase::FindRec(UInt32 uniqueId) {
    assert(!bRes);
    word index = -1;
    if (DmFindRecordByID(dbRef, uniqueId, &index) != 0)
        return -1;
    return index;
}

void NativeDatabase::RemoveRec(int iRec) {
    assert(!bRes);
    DmRemoveRecord(dbRef, iRec);
}

void NativeDatabase::RemoveRes(UInt32 type, UInt32 id) {
    assert(bRes);
    UInt16 index;
    index = DmFindResource(dbRef, type, id, NULL);
    if (index != -1) {
        DmRemoveResource(dbRef, index);
    }
}

void NativeDatabase::DelRec(int iRec) {
    assert(!bRes);
    DmDeleteRecord(dbRef, iRec);
}

void NativeDatabase::ArchiveRec(int iRec) {
    assert(!bRes);
    DmArchiveRecord(dbRef, iRec);
}

bool NativeDatabase::HasAppInfo() {
    assert(!bRes);
    return DmGetAppInfoID(dbRef) != NULL;
}

long NativeDatabase::GetAppInfo() {
    assert(!bRes);
    long res = -1;
    UInt32 size = 0;
    LocalID ailid = DmGetAppInfoID(dbRef);
    if (ailid) {
        void* pv = MemLocalIDToLockedPtr(ailid, card);
        UInt16 iRec = -1;
        size = MemPtrSize(pv);
        MemHandle hMem = DmNewRecord(dbRef, &iRec, size);
        if (hMem) {
            void* pRec = MemHandleLock(hMem);
            DmWrite(pRec, 0, pv, size);
            MemHandleUnlock(hMem);
            DmReleaseRecord(dbRef, iRec, true);
            res = iRec;
        }
        MemPtrUnlock(pv);
    }
    return res;
}

bool NativeDatabase::SetAppInfo(UInt16 index) {
    MemHandle hRec;
    if (0 == DmDetachRecord(dbRef, index, &hRec)) {
        LocalID rlid = MemHandleToLocalID(hRec);
        if (rlid) {
            LocalID ailid = DmGetAppInfoID(dbRef);
            DmSetDatabaseInfo(card, lid, 0,0,0,0,0,0,0, &rlid, 0,0,0);
            if (ailid) {
                MemHandle h = (MemHandle)MemLocalIDToGlobal(ailid, card);
                MemHandleFree(h);
            }
            return true;
        }
    }
    return false;
}

bool NativeDatabase::CreateAppInfo() {
    MemHandle hAI = DmNewHandle(dbRef, sizeof(AppInfoType));
    if (hAI) {
        void* pAI = MemHandleLock(hAI);
        DmSet(pAI, 0, sizeof(AppInfoType), 0);
        MemHandleUnlock(hAI);
        LocalID rlid = MemHandleToLocalID(hAI);
        if (rlid) {
            LocalID ailid = DmGetAppInfoID(dbRef);
            DmSetDatabaseInfo(card, lid, 0,0,0,0,0,0,0, &rlid, 0,0,0);
            if (ailid) {
                MemHandle h = (MemHandle)MemLocalIDToGlobal(ailid, card);
                MemHandleFree(h);
            }
            return true;
        }
    }
    return false;
}

dword NativeDatabase::GetDate(int which) {
    dword* cr = NULL, *mod = NULL, *bu = NULL;
    dword date;
    if (which == 0) cr = &date;
    else if (which == 1) mod = &date;
    else bu = &date;
    DmDatabaseInfo(card, lid, 0,0,0, cr, mod, bu, 0,0,0,0,0);
    return date;
}

long NativeDatabase::get_nrecs() {
    if (!bRes)
        return DmNumRecords(dbRef);
    else
        return DmNumResources(dbRef);
}

bool NativeDatabase::get_bit(word bit) {
    UInt16 attr;
    DmDatabaseInfo(card, lid, 0, &attr, 0,0,0,0,0,0,0,0,0);
    return (attr & bit) != 0;
}

void NativeDatabase::set_bit(word bit, bool set) {
    UInt16 attr;
    DmDatabaseInfo(card, lid, 0, &attr, 0,0,0,0,0,0,0,0,0);
    attr &= ~bit;
    if (set)
        attr |= bit;
    DmSetDatabaseInfo(card, lid, 0, &attr, 0,0,0,0,0,0,0,0,0);
}

long NativeDatabase::get_version() {
    UInt16 ver = 0;
    DmDatabaseInfo(card, lid, 0, 0, &ver, 0,0,0,0,0,0,0,0);
    return ver;
}

void NativeDatabase::set_version(long version) {
    UInt16 ver = version;
    DmSetDatabaseInfo(card, lid, 0,0, &ver, 0,0,0,0,0,0,0,0);
}

long NativeDatabase::get_size() {
    UInt32 size;
    DmDatabaseSize(card, lid, NULL, &size, NULL);
    return size;
}

bool NativeDatabase::get_res() {
    return bRes;
}

void NativeDatabase::get_type(Value* vType) {
    UInt32 data;
    DmDatabaseInfo(card, lid, 0,0,0,0,0,0,0,0,0, &data, 0);
    CIDitos(data, vType);
}

void NativeDatabase::get_cid(Value* vCid) {
    UInt32 data;
    DmDatabaseInfo(card, lid, 0,0,0,0,0,0,0,0,0,0, &data);
    CIDitos(data, vCid);
}

void NativeDatabase::get_name(Value* vName) {
    char* data = vName->newString(32);
    if (!data) oom();
    DmDatabaseInfo(card, lid, data,0,0,0,0,0,0,0,0,0,0);
    vName->unlock();
}

void NativeDatabase::set_name(char* name) {
    char buff[32] = {0};
    strncpy(buff, name, 31);
    DmSetDatabaseInfo(card, lid, buff,0,0,0,0,0,0,0,0,0,0);
}

bool NativeDatabase::get_inrom() {
    void* ptr = MemLocalIDToPtr(lid, card);
    word heapId = MemPtrHeapID(ptr);
    return (MemHeapFlags(heapId) & 1) == 1;
}

// NativeDBRecord implementation

NativeDBRecord::NativeDBRecord() {
    hRec = NULL;
    isDirty = isEmpty = false;
    offset = size = 0;
    pDB = NULL;
    iRec = -1;
    bRes = false;
    readOnly = false;
    fileRef = NULL;
}

NativeDBRecord::~NativeDBRecord() {
    if (hRec)
        Close();
}

void NativeDBRecord::Opened(NativeDatabase* pDB) {
    this->pDB = pDB;
    if (pDB) {
        pDB->AddRef(true);
        fileRef = NULL;
    }
    offset = 0;
    isEmpty = false;
    isDirty = false;
    size = MemHandleSize(hRec);
}

bool NativeDBRecord::Open(UInt16 index, NativeDatabase* pDB, bool ro) {
    if (hRec) {
        vm->vm_error("DBRecord must be closed before being reused");
    }
    if (index < DmNumRecords(pDB->dbRef)) {
        UInt16 attr;
        DmRecordInfo(pDB->dbRef, index, &attr, NULL, NULL);
        if (attr & dmRecAttrDelete) return false;
        if (hideSecrets && (attr & dmRecAttrSecret)) return false;
        hRec = DmGetRecord(pDB->dbRef, index);
        if (hRec) {
            Opened(pDB);
            DmRecordInfo(pDB->dbRef, index, NULL, &uid, NULL);
            bRes = false;
            iRec = index;
            readOnly = ro;
            return true;
        }
    }
    return false;
}

bool NativeDBRecord::OpenRes(UInt32 type, UInt32 id, NativeDatabase* pDB) {
    if (hRec) {
        vm->vm_error("DBRecord must be closed before being reused");
    }
    UInt16 index;
    index = DmFindResource(pDB->dbRef, type, id, NULL);
    if (index != -1) {
        hRec = DmGetResourceIndex(pDB->dbRef, index);
        if (hRec) {
            Opened(pDB);
            bRes = true;
            readOnly = false;
            return true;
        }
    }
    return false;
}

long NativeDBRecord::OpenFromFile(FileRef fileRef, UInt16 index) {
    if (hRec) {
        vm->vm_error("DBRecord must be closed before being reused");
    }
    byte attr; // byte here...
    Err err = VFSFileDBGetRecord(fileRef, index, &hRec, &attr, &uid);
    if (err == errNone) {
        Opened(NULL);
        this->fileRef = fileRef;
        bRes = false;
        readOnly = true;
        return 0;
    }
    return err;
}

long NativeDBRecord::OpenResFromFile(FileRef fileRef, UInt32 type, UInt32 index) {
    if (hRec) {
        vm->vm_error("DBRecord must be closed before being reused");
    }
    Err err = VFSFileDBGetResource(fileRef, type, index, &hRec);
    if (err == errNone) {
        Opened(NULL);
        this->fileRef = fileRef;
        bRes = true;
        readOnly = true;
        return 0;
    }
    return err;
}

void NativeDBRecord::Created(NativeDatabase* pDB) {
    this->pDB = pDB;
    pDB->AddRef(true);
    isEmpty = true;
    isDirty = true;
    offset = 0;
    size = 1;
    fileRef = NULL;
    readOnly = false;
}

bool NativeDBRecord::Create(UInt16* pIndex, NativeDatabase* pDB) {
    if (hRec) {
        vm->vm_error("DBRecord must be closed before being reused");
    }
    hRec = DmNewRecord(pDB->dbRef, pIndex, 1);
    if (hRec) {
        Created(pDB);
        DmRecordInfo(pDB->dbRef, *pIndex, NULL, &uid, NULL);
        bRes = false;
        iRec = *pIndex;
        return true;
    }
    return false;
}

bool NativeDBRecord::CreateRes(UInt32 type, UInt32 id, NativeDatabase* pDB) {
    if (hRec) {
        vm->vm_error("DBRecord must be closed before being reused");
    }
    hRec = DmNewResource(pDB->dbRef, type, id, 1);
    if (hRec) {
        Created(pDB);
        bRes = true;
        return true;
    }
    return false;
}

struct DBRecordContext {
    NativeDBRecord* pRec;
    bool bRead;
    byte* pData;
    word offset;
};

bool DBRecordIterFunc(Value* pVal, VarType type, int len, void* pContext) {
    DBRecordContext* pDBRContext = (DBRecordContext*)pContext;
    if (pDBRContext->bRead)
        return pDBRContext->pRec->ReadVal(pVal, type, len);
    else
        return pDBRContext->pRec->WriteVal(pVal, type, len, pDBRContext->pData, pDBRContext->offset);
}

long NativeDBRecord::Read(long addr, char* strFormat, int count) {
    DBRecordContext dbc = { this, true };
    pRec = (byte*)MemHandleLock(hRec);
    long res = IterateValues(addr, strFormat, count, &DBRecordIterFunc, &dbc);
    MemHandleUnlock(hRec);
    return res;
}

long NativeDBRecord::Write(long addr, char* strFormat, int count) {
    DBRecordContext dbc = { this, false };
    
    if (readOnly)
        return false;
    
    // allocate a buffer to write to temporarily
    word len = TypeDataSize(addr, strFormat, count);
    if (len) {
        dbc.pData = new byte[len];
    }
    long res = IterateValues(addr, strFormat, count, &DBRecordIterFunc, &dbc);
    
    // resize record if necessary
    word actualLen = dbc.offset;
    if (offset + actualLen > size) {
        MemHandle hNewRec = NULL;
        if (bRes) {
            hNewRec = DmResizeResource(hRec, offset + actualLen);
        } else {
            if (MemHandleResize(hRec, offset + actualLen) == 0)
                hNewRec = hRec;
        }
        if (!hNewRec) {
            return false;
        }
        hRec = hNewRec;
        size = offset + actualLen;
    }
    
    // write the buffer to the record
    byte* pRec = (byte*)MemHandleLock(hRec);
    DmWrite(pRec, offset, dbc.pData, actualLen);
    delete dbc.pData;
    offset += actualLen;
    MemHandleUnlock(hRec);
    return res;
}

bool NativeDBRecord::ReadVal(Value* pVal, VarType type, int len) {
    bool skipNull = false;
    if (offset + (len ? len : 1) > size) return false;
    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            pVal->iVal = 0;
            memcpy((byte*)(&pVal->iVal) + 4 - len, pRec+offset, len);
            offset += len;
            break;
        case vtString: {
            if (!len) {
                skipNull = true;
                while (*(pRec+offset+len)) {
                    len++;
                    if (len + offset == size) {
                        skipNull = false;
                        break;
                    }
                }
            }
            pVal->release();
            char* data = pVal->newString(len);
            memcpy(data, pRec+offset, len);
            data[len] = 0;
            pVal->unlock();
            offset += len + (skipNull ? 1 : 0);
            break;
        }
    }
    return true;
}

bool NativeDBRecord::WriteVal(Value* pVal, VarType type, int len, byte* pData, word& doffset) {
    bool bsz = type == vtString && len == 0;
    if (bsz) {
        len = strlen(pVal->lock()) + 1;
        pVal->unlock();
    }
    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            memcpy(pData + doffset, (byte*)(&pVal->iVal)+4-len, len);
            doffset += len;
            break;
        case vtString:
            if (bsz) {
                memcpy(pData + doffset, pVal->lock(), len);
                pVal->unlock();
            } else {
                char* str = pVal->lock();
                int slen = strlen(str);
                if (slen > len) {
                    memcpy(pData + doffset, str, len);
                } else {
                    memcpy(pData + doffset, str, len);
                    if (len > slen) {
                        memset(pData + doffset + slen, len - slen, 0);
                    }
                }
                pVal->unlock();
            }
            doffset += len;
            break;
    }

    isEmpty = false;
    isDirty = true;
    return true;
}

#if 0
bool NativeDBRecord::WriteVal(Value* pVal, VarType type, int len) {
    bool bsz = type == vtString && len == 0;
    if (bsz) {
        len = strlen(pVal->lock()) + 1;
        pVal->unlock();
    }
    if (offset + len > size) {
        MemHandle hNewRec = NULL;
        if (bRes) {
            hNewRec = DmResizeResource(hRec, offset + len);
        } else {
            //hNewRec = DmResizeRecord(pDB->dbRef, iRec, offset + len);
            if (MemHandleResize(hRec, offset + len) == 0)
                hNewRec = hRec;
        }
        if (!hNewRec) {
            return false;
        }
        hRec = hNewRec;
        size = offset + len;
    }
    byte* pRec = (byte*)MemHandleLock(hRec);
    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            DmWrite(pRec, offset, (byte*)(&pVal->iVal)+4-len, len);
            offset += len;
            break;
        case vtString:
            if (bsz) {
                DmWrite(pRec, offset, pVal->lock(), len);
                pVal->unlock();
            } else {
                char* str = pVal->lock();
                int slen = strlen(str);
                if (slen > len) {
                    DmWrite(pRec, offset, str, len);
                } else {
                    DmWrite(pRec, offset, str, slen);
                    if (len > slen) {
                        DmSet(pRec, offset + slen, len - slen, 0);
                    }
                }
                pVal->unlock();
            }
            offset += len;
            break;
    }
    MemHandleUnlock(hRec);

    isEmpty = false;
    isDirty = true;
    return true;
}
#endif

void NativeDBRecord::Erase() {
    if (!readOnly) {
        if (bRes) {
            hRec = DmResizeResource(hRec, 1);
        } else {
            MemHandleResize(hRec, 1);
        }
        size = 1;
        offset = 0;
        isDirty = true;
        isEmpty = true;
    }
}

void NativeDBRecord::Close() {
    if (fileRef) {
        MemHandleFree(hRec);
        fileRef = NULL;
    } else {
        if (bRes)
            DmReleaseResource(hRec);
        else {
            DmReleaseRecord(pDB->dbRef, iRec, isDirty);
        }
        pDB->Release(true);
        pDB = NULL;
    }

    hRec = NULL;
}

void NativeDBRecord::EnsureOpen(VM* vm) {
    if (!hRec)
        vm->vm_error("Record must be created/opened first");
}

long NativeDBRecord::get_offset() {
    return offset;
}

long NativeDBRecord::set_offset(long o) {
    offset = o;
    if (offset > size)
        offset = size;
    return offset;
}

long NativeDBRecord::get_id() {
    assert(!bRes);
    return iRec;
}

long NativeDBRecord::get_uniqueid() {
    assert(!bRes);
    return uid;
}

long NativeDBRecord::get_size() {
    if (isEmpty) return 0;
    return MemHandleSize(hRec);
}

int NativeDBRecord::get_cat() {
    assert(!bRes);
    if (fileRef) {
        return 0;
    } else {
        word attr;
        DmRecordInfo(pDB->dbRef, iRec, &attr, NULL, NULL);
        return attr & 0x0f;
    }
}

int NativeDBRecord::set_cat(int cat) {
    assert(!bRes);
    if (!readOnly) {
        word attr;
        DmRecordInfo(pDB->dbRef, iRec, &attr, NULL, NULL);
        attr &= 0xFFF0;
        attr |= cat & 0x0f;
        DmSetRecordInfo(pDB->dbRef, iRec, &attr, NULL);
    }
    return cat & 0x0f;
}


// NativeMemo interface
NATIVE_INIT(lib_memo_init, NativeMemo);

void lib_memo_find(VM* vm, int) {
    Value* vName = vm->pop();
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    if (pMemo->hRec)
        pMemo->Close();
    vm->retVal.iVal = pMemo->Find(vName->lock());
    vName->unlockRelease();
}

void lib_memo_open(VM* vm, int) {
    Value* vID = vm->pop();
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    if (pMemo->hRec)
        pMemo->Close();
    vm->retVal.iVal = pMemo->Open(vID->iVal);
}

void lib_memo_create(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    if (pMemo->hRec)
        pMemo->Close();
    vm->retVal.iVal = pMemo->Create();
}

void lib_memo_line(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    char* pRec = (char*)MemHandleLock(pMemo->hRec);
    int mlen = strlen(pRec);
    if (pMemo->offset < mlen) {
        char* text = &pRec[pMemo->offset];
        while (*text && *text != '\n') text++;
        if (!*text || !text[1]) pMemo->end = true;
        int len = (text - pRec) - pMemo->offset;
        if (!len) {
            if (*text) pMemo->offset ++; // eat the \n
            goto retEmpty;
        }
        char* res = vm->retVal.newString(len);
        if (!res) oom();
        strncpy(res, &pRec[pMemo->offset], len);
        res[len] = '\0';
        vm->retVal.unlock();
        if (!*text) {
            pMemo->offset += len;
        } else {
            pMemo->offset += len+1; // skip the \n
        }
        
    } else {
        pMemo->end = true;
retEmpty:
        vm->retVal.newConstString("");
    }
    MemHandleUnlock(pMemo->hRec);
}

void lib_memo_end(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    vm->retVal.iVal = pMemo->end;
}

void lib_memo_rewind(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    pMemo->Rewind();
}

void lib_memo_close(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    if (pMemo->hRec)
        pMemo->Close();
}

void lib_memo_remove(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    int recid = pMemo->recid;
    pMemo->Close();
    DmRemoveRecord(NativeMemo::dbMemo, recid);
}

void lib_memo_delete(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    int recid = pMemo->recid;
    pMemo->Close();
    DmDeleteRecord(NativeMemo::dbMemo, recid);
}

void lib_memo_archive(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    int recid = pMemo->recid;
    pMemo->Close();
    DmArchiveRecord(NativeMemo::dbMemo, recid);
}

void lib_memo_erase(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    pMemo->Erase();
}

void lib_memo_puts(VM* vm, int) {
    Value* vText = vm->pop();
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    char* text = vText->lock();
    pMemo->Puts(text);
    vText->unlockRelease();
}

void get_memo_nrecs(VM* vm, int) {
    vm->pop();
    vm->retVal.iVal = DmNumRecords(NativeMemo::dbMemo);
}

void get_memo_text(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    vm->retVal.copyString((char*)MemHandleLock(pMemo->hRec));
    MemHandleUnlock(pMemo->hRec);
}

void set_memo_text(VM* vm, int) {
    Value* vText = vm->pop();
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    char* text = vText->lock();
    pMemo->SetText(text);
    vText->unlock();
    vm->moveVal(&vm->retVal, vText);
}

void get_memo_index(VM* vm, int) {
    NativeMemo* pMemo = (NativeMemo*)PopNO(vm);
    RUNTIME_CHECK(pMemo->EnsureOpen(vm));
    
    vm->retVal.iVal = pMemo->recid;
}

// NativeDatabase interface
UInt32 CIDstoi(char* str) {
    UInt32 ret = 0;
    int i = 0;
    char* pret = (char*)&ret;
    
    while (*str && i < 4) {
        pret[i] = str[i];
        i++;
    }
    
    return ret;
}

void CIDitos(UInt32 i, Value* pVal) {
    char buff[5];
    memcpy(buff, &i, 4);
    buff[4] = 0;
    pVal->copyString(buff);
}

NATIVE_INIT(lib_database_init, NativeDatabase);

void lib_database_open(VM* vm, int) {
    Value* vReadOnly = vm->pop();
    Value* vName = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    vm->retVal.iVal = pDB->Open(vName->lock(), vReadOnly->iVal);
    vName->unlockRelease();
}

void lib_database_opentc(VM* vm, int) {
    Value* vReadOnly = vm->pop();
    Value* vCID = vm->pop();
    Value* vType = vm->pop();

    UInt32 cid, type;
    cid = CIDstoi(vCID->lock());
    type = CIDstoi(vType->lock());
    vCID->unlockRelease();
    vType->unlockRelease();
    
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    vm->retVal.iVal = pDB->OpenTC(type, cid, vReadOnly->iVal);
}

void lib_database_create(VM* vm, int) {
    Value* vRes = vm->pop();
    Value* vCID = vm->pop();
    Value* vType = vm->pop();
    Value* vName = vm->pop();

    UInt32 cid, type;
    cid = CIDstoi(vCID->lock());
    type = CIDstoi(vType->lock());
    vCID->unlockRelease();
    vType->unlockRelease();

    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    vm->retVal.iVal = pDB->Create(vName->lock(), type, cid, vRes->iVal);
    vName->unlockRelease();
}

void lib_database_close(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    pDB->Close();
}

void lib_database_delete(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->Delete();
}

void lib_database_setcatname(VM* vm, int) {
    Value* vName = vm->pop();
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->SetCatName(vIndex->iVal, vName->lock());
    vName->unlockRelease();
}

void lib_database_getcatname(VM* vm, int) {
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->GetCatName(vIndex->iVal, &vm->retVal);
}

void lib_database_moverec(VM* vm, int) {
    Value* vTo = vm->pop();
    Value* vFrom = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->MoveRec(vFrom->iVal, vTo->iVal);
}

void lib_database_getrec(VM* vm, int) {
    Value* vRO = vm->pop();
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->GetRec(vIndex->iVal, pRec, vRO->iVal);
}

void lib_database_newrec(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    Value* vpIndex = vm->pop();
    Value* vIndex = vm->deref(vpIndex->iVal);
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    UInt16 index = vIndex->iVal;
    vm->retVal.iVal = pDB->NewRec(&index, pRec);
    vIndex->iVal = index;
}

void lib_database_getres(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    Value* vIndex = vm->pop();
    Value* vType = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);

    UInt32 type;
    type = CIDstoi(vType->lock());
    vType->unlockRelease();

    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->GetRes(type, vIndex->iVal, pRec);
}

void lib_database_getresinfo(VM* vm, int) {
    Value* vId = vm->deref(vm->pop()->iVal);
    Value* vType = vm->deref(vm->pop()->iVal);
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);

    UInt32 type = 0;
    UInt16 id = 0;

    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->GetResInfo(vIndex->iVal, &type, &id);
    vId->iVal = id;
    CIDitos(type, vType);	
}

void lib_database_findrec(VM* vm, int) {
    Value* vId = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->FindRec(vId->iVal);
}

void lib_database_newres(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    Value* vIndex = vm->pop();
    Value* vType = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);

    UInt32 type;
    type = CIDstoi(vType->lock());
    vType->unlockRelease();

    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->NewRes(type, vIndex->iVal, pRec);
}

void lib_database_removerec(VM* vm, int) {
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->RemoveRec(vIndex->iVal);
}

void lib_database_removeres(VM* vm, int) {
    Value* vIndex = vm->pop();
    Value* vType = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);

    UInt32 type;
    type = CIDstoi(vType->lock());
    vType->unlockRelease();

    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->RemoveRes(type, vIndex->iVal);
}

void lib_database_delrec(VM* vm, int) {
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->DelRec(vIndex->iVal);
}

void lib_database_archiverec(VM* vm, int) {
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->ArchiveRec(vIndex->iVal);
}

void lib_database_hasappinfo(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->HasAppInfo();
}

void lib_database_getappinfo(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->GetAppInfo();
}

void lib_database_setappinfo(VM* vm, int) {
    Value* vIndex = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->SetAppInfo(vIndex->iVal);
}

void lib_database_createappinfo(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->CreateAppInfo();
}

void lib_database_getdate(VM* vm, int) {
    int which = vm->pop()->iVal;
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    long addrDate = vm->pop()->iVal;
    
    Value* vdate = vm->deref(addrDate);
    vdate->iVal = pDB->GetDate(which);

    vm->retVal.type = vtInt;
    vm->retVal.iVal = addrDate;
}

void get_database_nrecs(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->get_nrecs();
}

void get_database_backup(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->get_bit(dmHdrAttrBackup);
}

void set_database_backup(VM* vm, int) {
    Value* vBackup = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->set_bit(dmHdrAttrBackup, vBackup->iVal);
    vm->retVal.iVal = vBackup->iVal;
}

void get_database_nobeam(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->get_bit(dmHdrAttrCopyPrevention);
}

void set_database_nobeam(VM* vm, int) {
    Value* vBackup = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->set_bit(dmHdrAttrCopyPrevention, vBackup->iVal);
    vm->retVal.iVal = vBackup->iVal;
}

void get_database_version(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->get_version();
}

void set_database_version(VM* vm, int) {
    Value* vVersion = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->set_version(vVersion->iVal);
    vm->retVal.iVal = vVersion->iVal;
}

void get_database_size(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->get_size();
}

void get_database_res(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->get_res();
}

void get_database_type(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->get_type(&vm->retVal);
}

void get_database_cid(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->get_cid(&vm->retVal);
}

void get_database_name(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    pDB->get_name(&vm->retVal);
}

void set_database_name(VM* vm, int) {
    Value* vName = vm->pop();
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    char* name = vName->lock();
    pDB->set_name(name);
    vName->unlock();
    vm->moveVal(&vm->retVal, vName);
}

void get_database_dbref(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    //RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = (long)pDB->dbRef;
}

void get_database_dbid(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    //RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = (long)pDB->lid;
}

void get_database_card(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    //RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = (long)pDB->card;
}

void get_database_inrom(VM* vm, int) {
    NativeDatabase* pDB = (NativeDatabase*)PopNO(vm);
    RUNTIME_CHECK(pDB->EnsureOpen(vm));
    vm->retVal.iVal = pDB->get_inrom();
}

// NativeDBRecord interface

NATIVE_INIT(lib_dbrecord_init, NativeDBRecord);

void lib_dbrecord_read(VM* vm, int) {
    Value* vCount = vm->pop();
    Value* vFormat = vm->pop();
    Value* vPtr = vm->pop();
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->Read(vPtr->iVal, vFormat->lock(), vCount->iVal);
    vFormat->unlockRelease();
}

void lib_dbrecord_write(VM* vm, int) {
    Value* vCount = vm->pop();
    Value* vFormat = vm->pop();
    Value* vPtr = vm->pop();
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->Write(vPtr->iVal, vFormat->lock(), vCount->iVal);
    vFormat->unlockRelease();
}

void lib_dbrecord_erase(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    pRec->Erase();
}

void lib_dbrecord_close(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    pRec->Close();
}

void get_dbrecord_offset(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->get_offset();
}

void set_dbrecord_offset(VM* vm, int) {
    Value* offset = vm->pop();
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->set_offset(offset->iVal);
}

void get_dbrecord_id(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->get_id();
}

void get_dbrecord_cat(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->get_cat();
}

void set_dbrecord_cat(VM* vm, int) {
    Value* cat = vm->pop();
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->set_cat(cat->iVal);
}

void get_dbrecord_size(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->get_size();
}

void get_dbrecord_handle(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = (long)pRec->hRec;
}

void get_dbrecord_uniqueid(VM* vm, int) {
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    RUNTIME_CHECK(pRec->EnsureOpen(vm));
    vm->retVal.iVal = pRec->get_uniqueid();
}

void lib_dbrecord_fromfile(VM* vm, int) {
    Value* index = vm->pop();
    Value* fileRef = vm->pop();
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    vm->retVal.iVal = pRec->OpenFromFile((FileRef)fileRef->iVal, index->iVal);
}

void lib_dbrecord_resfromfile(VM* vm, int) {
    Value* vID = vm->pop();
    Value* vType = vm->pop();
    Value* fileRef = vm->pop();
    
    UInt32 type = CIDstoi(vType->lock());
    vType->unlockRelease();
    NativeDBRecord* pRec = (NativeDBRecord*)PopNO(vm);
    vm->retVal.iVal = pRec->OpenResFromFile((FileRef)fileRef->iVal, type, vID->iVal);
}

void lib_dbmanager_dbenum(VM* vm, int) {
    static DmSearchStateType srch;
    Value* vCreator = vm->pop();
    Value* vType = vm->pop();
    Value* vFirst = vm->pop();
    vm->pop(); // dbmanager address

    UInt32 t, c;
    UInt16 card;
    LocalID lid;

    t = CIDstoi(vType->lock());
    c = CIDstoi(vCreator->lock());
    vType->unlockRelease();
    vCreator->unlockRelease();
    
    if (vFirst->iVal)
        MemSet(&srch, sizeof(DmSearchStateType), 0);

    Err err = DmGetNextDatabaseByTypeCreator(
        vFirst->iVal, &srch, t, c, false, &card, &lid);

    if (err) {
        vm->retVal.newConstString("");
    } else {
        char* c = vm->retVal.newString(32);
        if (c) {
            DmDatabaseInfo(card, lid, c, 0,0,0,0,0,0,0,0,0,0);
            vm->retVal.unlock();
        } else {
            vm->retVal.newConstString("");
        }
    }
}

LocalID MyFindDatabase(char* name, UInt16& card) {
    UInt16 nCards = MemNumCards();
    for (card=0;card<nCards;card++) {
        LocalID lid = DmFindDatabase(card, name);
        if (lid)
            return lid;
    }
    return NULL;
}

