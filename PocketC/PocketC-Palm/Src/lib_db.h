#if 0
void _____Database_____() {}
#endif
/////////////////////////////////////////////////////////
//
// Database I/O
//

Format::Format(char* str, short _count, bool pack, char* _err) : orig(str), count(_count),
    len(0), scount(0), format(str), bPack(pack), err(_err)
{
    if (count > 0)
        count--;
}

bool Format::Next(VarType& _type, long& _len) {
    if (scount) {
        // same as last time
        scount--;
        _type = type;
        _len = len;
        return true;
    }
    
    if (*format == '\0' && !count)
        return false;
    
    if (*format == '\0') {
        count--;
        format = orig;
    }
    
tryagain:
    switch (*format) {
        case 'i':
        case 'p':
            type = vtInt;
            len = 4;
            format++;
            if (bPack) {
                if (*format == '4') {
                    format++;
                } else if (*format == '2') {
                    len = 2;
                    format++;
                }
                if (*format == '.')
                    format++;
            }
            break;
        case 'c':
            type = vtChar;
            len = 1;
            format++;
            break;
        case 'f':
            type = vtFloat;
            len = 4;
            format++;
            break;
        case 's':
            type = vtString;
            len = 0;
            format++;
            if (bPack) {
                if (*format == 'z') {
                    format++;
                } else {
                    len = 0;
                    while (isdigit(*format)) {
                        len*=10;
                        len+=(*format++) -'0';
                    }
                    if (*format == '.')
                        format++;
                }
            }
            break;
        default:
            if (!isdigit(*format)) {
                vm_error(err, pc);
            }
            scount = 0;
            while (isdigit(*format)) {
                scount *= 10;
                scount += (*format++) - '0';
            }
            scount--; // we are about to grab the first one
            goto tryagain;
    }
    _type = type;
    _len = len;
    return true;
}


LocalID currdb; // currently open database, be sure to clear before exec
long dbloc, dbrec;

void lib_dbopen(short) {
    short ret = 0;
    Value name;
    pop(name);
    dbloc=dbrec=0;

    currdb = DmFindDatabase(0, LockString(&name));
    if (currdb) {
        DmOpenRef db = DmOpenDatabase(0, currdb, dmModeReadWrite);
        if (db) {
            Boolean isRes;
            DmOpenDatabaseInfo(db, 0, 0, 0, 0, &isRes);
            if (isRes) {
                DmCloseDatabase(db);
                db = NULL;
            } else {
                if (!DmNumRecords(db)) dbloc=-1;
                DmCloseDatabase(db);
                ret=1;
            }
        }
    }
    RetInt(ret);
    UnlockReleaseString(&name);
}

void lib_dbclose(short) {
    currdb = 0;
} // cleanup?

void dbcreatex(ULong cID, ULong tID) {
    short ret = 0;
    Value n;
    char* name;
    pop(n);
    dbloc=-1;

    name = LockString(&n);
    currdb = DmFindDatabase(0, name);
    if (currdb) {
        ULong creator, type;
        DmDatabaseInfo(0, currdb, 0,0,0,0,0,0,0,0,0, &type, &creator);
        if (creator==cID && type==tID) {
            // Database exists and is a PocketC db, erase it
            DmDeleteDatabase(0, currdb);
            currdb = 0;
        } else {
            currdb = 0;
            goto done;
        }
    }
    if (!DmCreateDatabase(0, name, cID, tID, false)) {
        // Creation successful
        currdb = DmFindDatabase(0, name);
        if (currdb) ret = 1;
    }	

done:
    UnlockReleaseString(&n);
    RetInt(ret);
}

void lib_dbcreate(short) {
    dbcreatex('PktC', 'user');
}

static ULong getTC(Value* v) {
    ULong ret = 0;
    char* c = LockString(v);
    char* pret = (char*)&ret;
    if (*c) {
        for (short i=0;i<4;i++) {
            pret[i] = c[i];
        }
    }
    UnlockString(v);
    return ret;
}

void lib_dbcreatex(short) {
    Value c, t;
    pop(t);
    pop(c);
    
    dbcreatex(getTC(&c), getTC(&t));
    cleanup(c);
    cleanup(t);
}
    

DmOpenRef dbRef;
VoidHand hRec;
bool openDB(bool readOnly) {
    if (!currdb) return false;
    dbRef = DmOpenDatabase(0, currdb, (readOnly ? dmModeReadOnly : dmModeReadWrite));
    if (!dbRef) return false;
    return true;
}

void closeDB() {
    DmCloseDatabase(dbRef);
}

extern Boolean HideSecretRecords;
bool writeValue(Value* v, long len, long totalLen) {
    if (!len) {
        len = strlen(LockString(v)) + 1;
        UnlockString(v);
    }

    if (DmNumRecords(dbRef) > (UInt16)dbrec) {
        // The record exists, try to open it
        Word attr;
        DmRecordInfo(dbRef, dbrec, &attr, NULL, NULL);
        if (attr & dmRecAttrDelete) return false;
        if (HideSecretRecords && (attr & dmRecAttrSecret)) return false;
        hRec = DmGetRecord(dbRef, dbrec);
    } else {
        // Create a record of appropriate size
        UInt num = dbrec;
        hRec = DmNewRecord(dbRef, &num, len);
        dbloc = 0; dbrec = num;
    }
    
    if (!hRec) return false;
    
    //  Check size
    long size = MemHandleSize(hRec);
    if (totalLen < len) totalLen = len;
    if (dbloc<0) dbloc=size;
    if (dbloc + totalLen > size) {
        hRec = DmResizeRecord(dbRef, dbrec, dbloc + totalLen);
        if (!hRec) return false;
    }
    
    // Write the data
    void* recp = MemHandleLock(hRec);
    // FIX THIS
    if (v->type == vtString) {
        DmSet(recp, dbloc, len, 0);
        char* s = LockString(v);
        if (strlen(s) >= len)
            DmWrite(recp, dbloc, s, len);
        else
            DmStrCopy(recp, dbloc, s);
        UnlockString(v); 
    } else if (len==4) {
        DmWrite(recp, dbloc, &v->iVal, 4);
    } else if (len==2) {
        short i = v->iVal;
        DmWrite(recp, dbloc, &i, 2);
    } else
        DmWrite(recp, dbloc, &v->cVal, 1);
    
    // Close stuff
    MemHandleUnlock(hRec);
    DmReleaseRecord(dbRef, dbrec, true);
    dbloc += len;
    return true;
}

bool readValue(Value* v, long len) {
    if (dbloc<0) return false;

    if (DmNumRecords(dbRef) > (UInt16)dbrec) {
        // The record exists, try to open it
        Word attr;
        DmRecordInfo(dbRef, dbrec, &attr, NULL, NULL);
        if (attr & dmRecAttrDelete) return false;
        if (HideSecretRecords && (attr & dmRecAttrSecret)) return false;
        hRec = DmQueryRecord(dbRef, dbrec);
    } else {
        return false;
    }
    
    if (!hRec) return false;
    
    //  Check size
    long size = MemHandleSize(hRec);
    if (dbloc + (len ? len : 1) > size)
        return false;
    
    // Read the data
    void* recp = MemHandleLock(hRec);
    if (v->type == vtString) {
        char* s;
        if (len) {
            s = NewString(v, len+1);
            MemMove(s, (char*)recp+dbloc, len);
            s[len] = 0;
            UnlockString(v);
        } else {
            len = strlen((char*)recp+dbloc) + 1;
            s = NewString(v, len);
            strcpy(s, (char*)recp+dbloc);
            UnlockString(v);
        }
    } else if (len==4) {
        MemMove(&v->iVal, (char*)recp+dbloc, 4);
    } else if (len==2) {
        Word w;
        MemMove(&w, (char*)recp+dbloc, 2);
        v->iVal = w;
    } else
        v->cVal = ((char*)recp)[dbloc];
    
    // Close stuff
    MemHandleUnlock(hRec);
    dbloc += len;
    if (dbloc >= size) dbloc = -1;
    return true;
}

void lib_dbwrite(short) {
    Value val;
    pop(val);
    
    short len = 0;
    dbloc = -1;
    if (val.type==vtInt || val.type==vtFloat) len = 4;
    if (val.type==vtChar) len = 1;
    
    if (openDB(false)) {
        if (writeValue(&val, len, 0)) retVal.iVal = 1;
        closeDB();
    }
    cleanup(val);
}

void lib_dbread(short) {
    Value type;
    pop(type);
    
    short len;
    
    switch (type.cVal) {
        case 's': retVal.type = vtString; len = 0; break;
        case 'f': retVal.type = vtFloat; len = 4; break;
        case 'i': retVal.type = vtInt; len = 4; break;
        case 'c': retVal.type = vtChar; len = 1; break;
        default: vm_error("Invalid type in dbread()", 0);
    }
    
    if (openDB(true)) {
        if (!readValue(&retVal, len)) {
            retVal.type = vtInt;
            RetInt(0);
        }
        closeDB();
    }
}

void lib_dbpos(short) {
    RetInt(dbloc);
}

void lib_dbseek(short) {
    Value pos;
    pop(pos);
    
    dbloc = pos.iVal;
}

void lib_dbbackup(short) {
    Value fbu;
    pop(fbu);
    
    if (!currdb) return;
    
    UInt attr;
    DmDatabaseInfo(0, currdb, 0, &attr, 0,0,0,0,0,0,0,0,0);
    if (fbu.iVal==0)
        attr &= ~dmHdrAttrBackup;
    else if (fbu.iVal==1)
        attr |= dmHdrAttrBackup;
    DmSetDatabaseInfo(0, currdb, 0, &attr, 0,0,0,0,0,0,0,0,0);
    RetInt(((attr&dmHdrAttrBackup)?1:0));
}

void lib_dbdelete(short) {
    if (currdb) DmDeleteDatabase(0, currdb);
    currdb=0;
}

void delrec(short op) {
    Value id;
    pop(id);
    
    short res;
    
    if (openDB(false)) {
        if (id.iVal < DmNumRecords(dbRef)) {
            switch (op) {
                case 0: res = DmRemoveRecord(dbRef, id.iVal); break;
                case 1: res = DmArchiveRecord(dbRef, id.iVal); break;
                case 2: res = DmDeleteRecord(dbRef, id.iVal); break;
            }
        }
        closeDB();
        if (res==0) retVal.iVal = 1;
        if (id.iVal==dbrec) dbrec = 0;
    }
}

void lib_dbdelrec(short) {
    delrec(2);
}

void lib_dbarcrec(short) {
    delrec(1);
}

void lib_dbremrec(short) {
    delrec(0);
}

void lib_dberase(short) {
    if (openDB(false)) {
        if (DmNumRecords(dbRef) > (UInt16)dbrec) {
            // The record exists, try to open it
            Word attr;
            DmRecordInfo(dbRef, dbrec, &attr, NULL, NULL);
            if (attr & dmRecAttrDelete) goto noRecord;
            if (HideSecretRecords && (attr & dmRecAttrSecret)) goto noRecord;
            VoidHand v = DmResizeRecord(dbRef, dbrec, 0);
            if (v) {
                dbloc = 0;
                retVal.iVal = 1;
            }
            //DmReleaseRecord(dbRef, dbrec, true);
        }
noRecord:
        closeDB();
    }
}

DmSearchStateType srch;
void lib_dbenum(short) {
    Value first, type, creator;
    pop(creator);
    pop(type);
    pop(first);

    ULong t, c;
    UInt card;
    LocalID lid;

    t = getTC(&type);
    c = getTC(&creator);
    
    if (first.iVal)
        MemSet(&srch, sizeof(DmSearchStateType), 0);

    Err err = DmGetNextDatabaseByTypeCreator(
        first.iVal, &srch, t, c, false, &card, &lid);

    if (err) {
        retVal.iVal = 0;
    } else {
        char* c = NewString(&retVal, 33);
        DmDatabaseInfo(card, lid, c, 0,0,0,0,0,0,0,0,0,0);
        UnlockString(&retVal);
    }
    cleanup(type);
    cleanup(creator);
}

void lib_dbrec(short) {
    Value num;
    pop(num);
    dbrec = num.iVal;
    dbloc = 0;
}

void lib_dbnrecs(short) {
    if (openDB(true)) {
        retVal.iVal = DmNumRecords(dbRef);
        closeDB();
    }
}

#ifndef POCKETC_FAT
inline void moveVal(short* dest, short* src) {
    *dest++ = *src++;
    *dest++ = *src++;
    *dest++ = *src++;
}
#endif
/*
void dbreadx(short count) {
    Value ptrV, formatV;
    pop(formatV);
    pop(ptrV);
    long len, ptr = ptrV.iVal, nRead = 0;
    char* format, *orig_format = LockString(formatV.sVal);

    if (openDB(true)) {
        while (count--) {
            format = orig_format;
            while (*format) {
                Value v, *loc = deref(ptr++);
                switch (*format++) {
                    case 'c': v.type=vtChar; len=1; break;
                    case 'i': v.type=vtInt; len=(*format++) - '0'; if (len!=2 && len!=4) len=4; break;
                    case 'f': v.type=vtFloat; len=4; break;
                    case 's':
                        v.type=vtString;
                        if (*format == 'z') {
                            format++;
                            len = 0;
                        } else {
                            len = 0;
                            while (*format && isdigit(*format)) {
                                len*=10;
                                len+=(*format++) -'0';
                            }
                        }
                        break;
                    default:
                        vm_error("Invalid format in dbreadx()", 0);
                }
                if (readValue(&v, len)) nRead++;
                else break;
                typeCast(v, loc->type);
                cleanup(*loc);
                valcpy((short*)loc, (short*)&v);
            }
        }
        closeDB();
    }
    UnlockReleaseString(formatV.sVal);
    retVal.iVal = nRead;
}
*/

void dbreadx(short count) {
    Value ptrV, formatV;
    pop(formatV);
    pop(ptrV);
    long len, ptr = ptrV.iVal, nRead = 0;
    char* fstr = LockString(&formatV);
    Format format(fstr, count, true, "Invalid format in dbreadx/c()");

    if (openDB(true)) {
        Value v;
        while (format.Next(v.type, len)) {
            Value* loc = deref(ptr++);
            if (readValue(&v, len)) nRead++;
            else break;
            typeCast(v, loc->type);
            cleanup(*loc);
            moveVal((short*)loc, (short*)&v);
        }
        closeDB();
    }
    UnlockReleaseString(&formatV);
    retVal.iVal = nRead;
}

void lib_dbreadx(short) {
    dbreadx(1);
}

void lib_dbreadx2(short) {
    Value count;
    pop(count);
    dbreadx(count.iVal);
}

void copyVal(Value* dest, Value* src);
/*
void dbwritex(short count) {
    Value ptrV, formatV;
    pop(formatV);
    pop(ptrV);
    long len, ptr = ptrV.iVal, nWrite = 0;
    char* format, *orig_format = LockString(formatV.sVal);

    if (openDB(false)) {
        while (count--) {
            format = orig_format;
            while (*format) {
                Value v;
                copyVal(&v, deref(ptr++));
                switch (*format++) {
                    case 'c': len=1; typeCast(v, vtChar); break;
                    case 'i': len=(*format++) - '0'; if (len!=2 && len!=4) len=4; typeCast(v, vtInt); break;
                    case 'f': len=4; typeCast(v, vtFloat); break;
                    case 's':
                        typeCast(v, vtString);
                        if (*format == 'z') {
                            format++;
                            len = 0;
                        } else {
                            len = 0;
                            while (*format && isdigit(*format)) {
                                len*=10;
                                len+=(*format++) -'0';
                            }
                        }
                        break;
                    default:
                        vm_error("Invalid format in dbwritex()", 0);
                }
                if (writeValue(&v, len)) nWrite++;
                else break;
                cleanup(v);						
            }
        }
        closeDB();
    }
    UnlockReleaseString(formatV.sVal);
    retVal.iVal = nWrite;
}
*/

void dbwritex(short count) {
    Value ptrV, formatV;
    pop(formatV);
    pop(ptrV);
    long len, ptr = ptrV.iVal, nWrite = 0;
    char* fstr = LockString(&formatV);
    long size = 0;
    {
        Format format(fstr, count, true, "Invalid format in dbwritex/c()");
        VarType v;
        while (format.Next(byref v, byref len)) {
            size += len;
            if (len == 0) {
                Value* v = deref(ptr);
                char* str = LockString(v);
                len += strlen(str) + 1;
                UnlockString(v);
            }
            ptr++;
        }
    }
    ptr = ptrV.iVal;
    Format format(fstr, count, true, "Invalid format in dbwritex/c()");
    if (openDB(false)) {
        Value v;
        while (format.Next(byref v.type, byref len)) {
            moveVal((short*)&v, (short*)deref(ptr++));
            if (writeValue(&v, len, size)) nWrite++;
            else break;
            size = 0;
        }
        closeDB();
    }
    UnlockReleaseString(&formatV);
    retVal.iVal = nWrite;
}

void lib_dbwritex2(short) {
    Value count;
    pop(count);
    dbwritex(count.iVal);
}

void lib_dbwritex(short) {
    dbwritex(1);
}

bool hack_recinfo(Word* pAttr) {
    if (DmNumRecords(dbRef) > (UInt16)dbrec) {
        DmRecordInfo(dbRef, dbrec, pAttr, NULL, NULL);
        return true;
    }
    return false;
}

void lib_dbsize(short) {
    if (openDB(true)) {
        Word attr;
        if (hack_recinfo(&attr)) {
        //if (DmNumRecords(dbRef) >(UInt16) dbrec) {
            // The record exists, try to open it
            //DmRecordInfo(dbRef, dbrec, &attr, NULL, NULL);
            if (attr & dmRecAttrDelete) goto noRecord;
            if (HideSecretRecords && (attr & dmRecAttrSecret)) goto noRecord;
            hRec = DmGetRecord(dbRef, dbrec);
            retVal.iVal = MemHandleSize(hRec);
            DmReleaseRecord(dbRef, dbrec, false);
        }
noRecord:
        closeDB();
    }
}

void lib_dbgetcat(short) {
    if (openDB(true)) {
        Word attr;
        if (hack_recinfo(&attr)) {
        //if (DmNumRecords(dbRef) > dbrec) {
            //DmRecordInfo(dbRef, dbrec, &attr, NULL, NULL);
            if (attr & dmRecAttrDelete) goto noRecord;
            if (HideSecretRecords && (attr & dmRecAttrSecret)) goto noRecord;
            RetInt(attr & dmRecAttrCategoryMask);
        }
noRecord:
        closeDB();
    }
}

void lib_dbsetcat(short) {
    short res = 0;
    Value cat;
    pop(cat);

    if (openDB(false)) {
        Word attr;
        if (hack_recinfo(&attr)) {
        //if (DmNumRecords(dbRef) > dbrec) {
            //DmRecordInfo(dbRef, dbrec, &attr, NULL, NULL);
            if (attr & dmRecAttrDelete) goto noRecord;
            if (HideSecretRecords && (attr & dmRecAttrSecret)) goto noRecord;
            attr &= ~dmRecAttrCategoryMask;
            attr |= cat.iVal & dmRecAttrCategoryMask;
            DmSetRecordInfo(dbRef, dbrec, &attr, NULL);
            res = 1;
        }
noRecord:
        closeDB();
    }
    RetInt(res);
}

void lib_dbcatname(short) {
    Value cat;
    pop(cat);
    
    if (cat.iVal >= 0 && cat.iVal < 16 && openDB(true)) {
        char* pName = NewString(&retVal, 16);
        if (!pName) vm_error("Out of memory in dbcatname()", 0);
        pName[0] = '\0';
        CategoryGetName(dbRef, cat.iVal, pName);
        UnlockString(&retVal);
        closeDB();
    } else {
        emptyString(&retVal);
    }		
}

void lib_dbsetcatname(short) {
    Value cat;
    Value name;
    pop(name);
    pop(cat);
    
    char* pName = LockString(&name);
    
    if (cat.iVal >= 0 && cat.iVal < 16 && openDB(false)) {
        CategorySetName(dbRef, cat.iVal, pName);
        closeDB();
        retVal.iVal = 1;
    }		
    UnlockReleaseString(&name);
}

void lib_dbmoverec(short) {
    Value from, to;
    pop(to);
    pop(from);
    
    if (openDB(false)) {
        retVal.iVal = !DmMoveRecord(dbRef, from.iVal, to.iVal);
        closeDB();
    }
}

void lib_dbrename(short) {
    Value name;
    pop(name);
    char* strname = LockString(&name);
    
    if (currdb) {
        char buff[32] = {0, };
        strncpy(buff, strname, 31);
        if (0 == DmSetDatabaseInfo(0, currdb, buff, 0,0,0,0,0,0,0,0,0,0))
            retVal.iVal = 1;
    }
    UnlockReleaseString(&name);
}

void typeToString(Value* v, UInt32 i) {
    char buff[5];
    memcpy(buff, &i, 4);
    buff[4] = 0;
    NewStringFromConst(v, buff);
}

void lib_dbinfo(short) {
    Value name;
    Value pType, pCid;
    
    pop(pCid);
    pop(pType);
    pop(name);
    
    LocalID lid = DmFindDatabase(0, LockString(&name));
    
    if (lid) {
        UInt32 type, cid;
        if (errNone == DmDatabaseInfo(0, lid, NULL, NULL, NULL,
                NULL, NULL, NULL, NULL, NULL, NULL, &type, &cid)) {
                
            Value* pVal = deref(pType.iVal);
            cleanup(*pVal);
            typeToString(pVal, type);
            pVal = deref(pCid.iVal);
            cleanup(*pVal);
            typeToString(pVal, cid);
            retVal.iVal = 1;
        }
    }
    UnlockReleaseString(&name);
}

void lib_dbtotalsize(short) {
    Value name;
    pop(name);
    
    LocalID lid = DmFindDatabase(0, LockString(&name));
    UInt32 size = 0;
    
    if (lid) {
        DmDatabaseSize(0, lid, NULL, &size, NULL);
    }
    
    retVal.iVal = size;
    UnlockReleaseString(&name);
}

/*
bool NativeDatabase::SetAppInfo(UInt16 index) {
    MemHandle hRec;
    if (0 == DmDetachRecord(dbRef, index, &hRec)) {
        LocalID rlid = MemHandleToLocalID(hRec);
        if (rlid) {
            DmSetDatabaseInfo(card, lid, 0,0,0,0,0,0,0, &rlid, 0,0,0);
            return true;
        }
    }
    return false;
}
*/

void lib_dbgetappinfo(short) {
    long res = -1;
    UInt32 size = 0;
    dbrec = -1;
    dbloc = 0;

    if (openDB(false)) {
        LocalID ailid = DmGetAppInfoID(dbRef);
        if (ailid) {
            void* pv = MemLocalIDToLockedPtr(ailid, 0);
            size = MemPtrSize(pv);
            UInt16 iRec = -1;
            MemHandle hMem = DmNewRecord(dbRef, &iRec, size);
            dbrec = iRec;
            if (hMem) {
                void* pRec = MemHandleLock(hMem);
                DmWrite(pRec, 0, pv, size);
                MemHandleUnlock(hMem);
                DmReleaseRecord(dbRef, dbrec, true);
                res = dbrec;
            }
            MemPtrUnlock(pv);
        }
        closeDB();
    }		
    retVal.iVal = res;
}

void lib_dbsetappinfo(short) {
    MemHandle hRec;
    if (openDB(false)) {
        if (0 == DmDetachRecord(dbRef, dbrec, &hRec)) {
            LocalID rlid = MemHandleToLocalID(hRec);
            if (rlid) {
                DmSetDatabaseInfo(0, currdb, 0,0,0,0,0,0,0, &rlid, 0,0,0);
                retVal.iVal = 1;
            }
        }
        closeDB();
    }
    dbrec = -1;
    dbloc = 0;
}

void lib_dbmovecat(short) {
    Value to, from, dirty;
    pop(dirty);
    pop(to);
    pop(from);
    
    if (openDB(false)) {
        if (0 == DmMoveCategory(dbRef, to.iVal, from.iVal, dirty.iVal)) {
            retVal.iVal = 1;
        }
    }
}

#if 0
void _____Memo_Pad_____() {}
#endif
/////////////////////////////////////////////////////////
//
// Memo Pad I/O
//
short memorec, memoloc;
LocalID mmDB;
extern DmOpenRef memoDB;

void lib_mmcount(short) {
    memoDB = DmOpenDatabase(0, mmDB, dmModeReadOnly);
    retVal.iVal = DmNumRecords(memoDB);
    DmCloseDatabase(memoDB);
}

void lib_mmnew(short) {
    DmOpenRef db = DmOpenDatabase(0, mmDB, dmModeReadWrite);
    UInt mrec = -1;
    VoidHand nrec = DmNewRecord(db, &mrec, 1);
    if (nrec) {
        DmSet(MemHandleLock(nrec), 0, 1, 0);
        MemHandleUnlock(nrec);
        
        retVal.iVal = 1;
        memorec = mrec;
        DmReleaseRecord(db, memorec, true);
    }
    DmCloseDatabase(db);
}

void lib_mmfind(short) {
    Value name;
    pop(name);
    
    memoDB = DmOpenDatabase(0, mmDB, dmModeReadOnly);
    memorec = FindMemo(LockString(&name), false);
    DmCloseDatabase(memoDB);
    if (memorec >= 0) retVal.iVal = 1, memoloc = 0;
    UnlockReleaseString(&name);
}

void lib_mmfindx(short) {
    lib_mmfind(0);
    retVal.iVal = memorec;
}

void lib_mmopen(short) {
    Value num;
    pop(num);

    memoDB = DmOpenDatabase(0, mmDB, dmModeReadOnly);
    memorec = num.iVal;
    if (DmNumRecords(memoDB) > memorec) {
        Word attr; // It's supposed to be a Byte

        DmRecordInfo(memoDB, memorec, &attr, NULL, NULL);
        if (attr & dmRecAttrDelete) memorec = -1;
        else if (HideSecretRecords && (attr & dmRecAttrSecret)) memorec =-1;
        else retVal.iVal = 1, memoloc = 0;
    }
    DmCloseDatabase(memoDB);
}

void lib_mmclose(short) {
    memoloc = memorec = -1;
}

void lib_mmputs(short) {
    Value arg;
    pop(arg);
    char* src;
    
    memoloc = -1;	
    if (memorec < 0) goto clean;
    
    DmOpenRef db = DmOpenDatabase(0, mmDB, dmModeReadWrite);
    Handle rec = (Handle)DmGetRecord(db, memorec);
    
    src = LockString(&arg);
    short size1 = MemHandleSize(rec);
    short size2 = strlen(src);
    
    rec = (Handle)DmResizeRecord(db, memorec, size1 + size2 /*+ 1*/);
    if (!rec) goto clean1;
    DmStrCopy(MemHandleLock(rec), size1-1, src);
    MemHandleUnlock(rec);
    UnlockString(&arg);
    retVal.iVal = 1;

clean1:
    DmReleaseRecord(db, memorec, true);
    DmCloseDatabase(db);
clean:
    cleanup(arg);
}

void lib_mmgetl(short) {
    if (memorec < 0 || memoloc < 0) return;
    
    DmOpenRef db = DmOpenDatabase(0, mmDB, dmModeReadOnly);
    Handle rec = (Handle)DmQueryRecord(db, memorec);
    if (MemHandleSize(rec) < memoloc) {
        memoloc = -1;
        goto clean;
    }
    
    char* recp = (char*)MemHandleLock(rec);
    recp = &recp[memoloc];

    char* t = recp;
    short len = 0;
    while (*t && *t!='\n') t++, len++;

    if (len) {
        t = NewString(&retVal, len+1);
        if (!retVal.sVal) vm_error("Out of memory in mmgetl()", 0);
        MemMove(t, recp, len);
        t[len] = '\0';
        UnlockString(&retVal);
    } else if (*t=='\n') {
        emptyString(&retVal);
    }

    if (recp[len] && recp[len+1]) memoloc+=len + 1;
    else memoloc = -1;
    MemHandleUnlock(rec);

clean:
    DmCloseDatabase(db);
}

void lib_mmeor(short) {
    RetInt(memoloc == -1);
}

void lib_mmrewind(short) {
    memoloc = 0;
}

void lib_mmdelete(short) {
    if (memorec < 0) return;
    
    DmOpenRef db = DmOpenDatabase(0, mmDB, dmModeReadWrite);
    if (!DmRemoveRecord(db, memorec)) retVal.iVal = 1;
    DmCloseDatabase(db);
    memorec = -1;
}

