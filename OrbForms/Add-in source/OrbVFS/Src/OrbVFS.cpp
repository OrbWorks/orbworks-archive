#include <PalmOS.h>
#include <Extensions\ExpansionMgr\VFSMgr.h>
#include "OrbNative.h"

extern "C" {
void* __Startup__(OrbFormsInterface*, void*, UInt16);
}

// The following five functions would normally be defined
// in the runtime library that an application links with.
// However, since this is just a code resource, we cannot
// link to it, so must define these ourselves
void* operator new(unsigned long size) {
    return MemPtrNew(size);
}

void operator delete(void* p) {
    MemPtrFree(p);
}

void* operator new[](unsigned long size) {
    return MemPtrNew(size);
}

void operator delete[](void* p) {
    MemPtrFree(p);
}
// the compiler generates a call to this to copy a structure
extern "C" 
void *__copy(char *to,char *from,unsigned long size) {
    char *f,*t;

    if(to) for(f=(char *)from,t=(char *)to; size>0; size--) *t++=*f++;
    return to;
}

/*
object File {
    int id;
    void _init() @ library("OrbVFS", 25);
    void _destroy() @ 80;
    void _copy(File) @ 89;

    void close() @ library("OrbVFS", 10);
    int read(void* data, string type, int count) @ library("OrbVFS", 11);
    int write(void* data, string type, int count) @ library("OrbVFS", 12);
    void setDate(int which, Date date) @ library("OrbVFS", 13);
    Date getDate(int which) @ library("OrbVFS", 14);

    int offset @ library("OrbVFS", 15:16);
    bool eof @ library("OrbVFS", 17);
    int size @ library("OrbVFS", 18:19);
    int attribs @ library("OrbVFS", 20:21);
};

object Directory {
    int id;
    void _init() @ library("OrbVFS", 24);
    void _destroy() @ 80;
    void _copy(Directory) @ 89;

    void close() @ library("OrbVFS", 10);
    void setDate(int which, Date date) @ library("OrbVFS", 13);
    Date getDate(int which) @ library("OrbVFS", 14);
    bool enumerate(bool first, string* name, int* attribs) @ library("OrbVFS", 22);
    int attribs @ library("OrbVFS", 20:21);
};

object Volume {
    int id;
    void _init() @ library("OrbVFS", 23);
    void _destroy() @ 80;
    void _copy(Volume) @ 89;

    int open(string path, int mode, File file) @ library("OrbVFS", 1);
    int openDir(string path, Directory dir) @ library("OrbVFS", 25);
    int create(string path) @ library("OrbVFS", 2);
    int createDir(string path) @ library("OrbVFS", 3);
    int del(string path) @ library("OrbVFS", 4);
    int rename(string path, string newname) @ library("OrbVFS", 5);
    int export(string name, string path) @ library("OrbVFS", 6);
    int import(string path, string* name) @ library("OrbVFS", 7);
    string getDefaultDir(string type) @ library("OrbVFS", 8);
    string label @ library("OrbVFS", 9);
};

object VolumeMgr {
    bool enumerate(bool first, Volume vol) @ library("OrbVFS", 0);
};
*/

struct VFSData {
    UInt32 volIter;
};

struct VolumeData {
    UInt16 volRef;
};

struct FileData {
    FileRef fileRef;
    UInt32  dirIter;
    UInt32  offset;
    char*   bytes;
};

void FileClose(FileData* fd) {
    if (fd->fileRef) {
        VFSFileClose(fd->fileRef);
        fd->fileRef = NULL;
    }
}

void Volume_destroy(OrbFormsInterface* ofi, void* data, void* obj) {
    VolumeData* pd = (VolumeData*)obj;
    delete pd;
}

void Volume_init(OrbFormsInterface* ofi) {
    VolumeData* pd = new VolumeData;
    pd->volRef = 0;
    ofi->nobj_create(pd, Volume_destroy);
}

void Volume_open(OrbFormsInterface* ofi, bool isDir) {
    FileData* fd = (FileData*)ofi->nobj_pop();
    UInt16 mode = vfsModeRead;
    if (!isDir) {
        mode = ofi->vm_pop()->iVal;
    }
    Value* vpath = ofi->vm_pop();
    VolumeData* vd = (VolumeData*)ofi->nobj_pop();
    char* path = ofi->val_lock(vpath);
    FileClose(fd);
    Err err = VFSFileOpen(vd->volRef, path, mode, &fd->fileRef);
    ofi->val_unlockRelease(vpath);
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Volume_create(OrbFormsInterface* ofi) {
    Value* vpath = ofi->vm_pop();
    VolumeData* vd = (VolumeData*)ofi->nobj_pop();
    char* path = ofi->val_lock(vpath);
    Err err = VFSFileCreate(vd->volRef, path);
    ofi->val_unlockRelease(vpath);
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Volume_createDir(OrbFormsInterface* ofi) {
    Value* vpath = ofi->vm_pop();
    VolumeData* vd = (VolumeData*)ofi->nobj_pop();
    char* path = ofi->val_lock(vpath);
    Err err = VFSDirCreate(vd->volRef, path);
    ofi->val_unlockRelease(vpath);
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Volume_del(OrbFormsInterface* ofi) {
    Value* vpath = ofi->vm_pop();
    VolumeData* vd = (VolumeData*)ofi->nobj_pop();
    char* path = ofi->val_lock(vpath);
    Err err = VFSFileDelete(vd->volRef, path);
    ofi->val_unlockRelease(vpath);
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Volume_rename(OrbFormsInterface* ofi) {
    Value* vname = ofi->vm_pop();
    Value* vpath = ofi->vm_pop();
    VolumeData* vd = (VolumeData*)ofi->nobj_pop();
    char* path = ofi->val_lock(vpath);
    char* name = ofi->val_lock(vname);
    Err err = VFSFileRename(vd->volRef, path, name);
    ofi->val_unlockRelease(vpath);
    ofi->val_unlockRelease(vname);
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
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

void Volume_export(OrbFormsInterface* ofi) {
    Value* vpath = ofi->vm_pop();
    Value* vname = ofi->vm_pop();
    VolumeData* pd = (VolumeData*)ofi->nobj_pop();
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;

    char* path = ofi->val_lock(vpath);
    char* name = ofi->val_lock(vname);
    UInt16 card;
    LocalID lid = MyFindDatabase(name, card);
    if (!lid) {
        ofi->vm_retVal->iVal = vfsErrFileNotFound;
        goto end;
    }
    
    Err err = VFSExportDatabaseToFile(pd->volRef, path, card, lid);
    ofi->vm_retVal->iVal = err;

end:
    ofi->val_unlockRelease(vname);
    ofi->val_unlockRelease(vpath);
}

void Volume_import(OrbFormsInterface* ofi) {
    long addrName = ofi->vm_pop()->iVal;
    Value* vpath = ofi->vm_pop();
    VolumeData* pd = (VolumeData*)ofi->nobj_pop();
    char* path = ofi->val_lock(vpath);
    UInt16 card;
    LocalID lid;
    Err err = VFSImportDatabaseFromFile(pd->volRef, path, &card, &lid);
    if (err == errNone) { // or dmErrAlreadyExists??
        // get name
        char buff[32];
        buff[0] = 0;
        DmDatabaseInfo(card, lid, buff, 0,0,0,0,0,0,0,0,0,0);
        Value* vname = ofi->vm_deref(addrName);
        ofi->val_release(vname);
        ofi->val_copyString(vname, buff);
    }

    ofi->val_unlockRelease(vpath);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Volume_getDefaultDir(OrbFormsInterface* ofi) {
    Value* vtype = ofi->vm_pop();
    VolumeData* pd = (VolumeData*)ofi->nobj_pop();

    char buff[256];
    buff[0] = 0;
    UInt16 len = 256;
    char* type = ofi->val_lock(vtype);
    Err err = VFSGetDefaultDirectory(pd->volRef, type, buff, &len);
    ofi->val_unlockRelease(vtype);
    
    if (err == errNone) {
        ofi->val_copyString(ofi->vm_retVal, buff);
    } else {
        ofi->val_newConstString(ofi->vm_retVal, "");
    }
}

void Volume_get_label(OrbFormsInterface* ofi) {
    VolumeData* pd = (VolumeData*)ofi->nobj_pop();
    char buff[256];
    buff[0] = 0;
    
    VFSVolumeGetLabel(pd->volRef, buff, 256);
    ofi->val_copyString(ofi->vm_retVal, buff);
}

void File_destroy(OrbFormsInterface* ofi, void* data, void* obj) {
    FileData* pd = (FileData*)obj;
    FileClose(pd);
    delete pd;
}

void File_init(OrbFormsInterface* ofi) {
    FileData* pd = new FileData;
    pd->fileRef = 0;
    pd->dirIter = vfsIteratorStart;
    ofi->nobj_create(pd, File_destroy);
}

void File_close(OrbFormsInterface* ofi) {
    FileData* pd = (FileData*)ofi->nobj_pop();
    FileClose(pd);
}

void File_get_fileRef(OrbFormsInterface* ofi) {
    FileData* pd = (FileData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = pd->fileRef;
}

bool read_value(OrbFormsInterface* ofi, Value* val, VarType type, int len, void* context) {
    FileData* fd = (FileData*)context;
    
    Err err;
    UInt32 rlen;
    char buffer[512] = {0};
    if (len > 511) ofi->vm_error("File.read() string too long");
    
    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            val->iVal = 0;
            err = VFSFileRead(fd->fileRef, len, (char*)(&val->iVal)+4-len, &rlen);
            if (err != errNone) goto Error;
            break;
        case vtString:
            if (len) {
                err = VFSFileRead(fd->fileRef, len, buffer, &rlen);
                if (err != errNone && err != vfsErrFileEOF) goto Error;
                buffer[rlen] = 0;
                ofi->val_release(val); // release the old value
                if (!ofi->val_copyString(val, buffer)) goto Error;
            } else {
                int i = 0;
                // read one byte at a time until we get a NULL
                while (i < 511) {
                    err = VFSFileRead(fd->fileRef, 1, &buffer[i], &rlen);
                    if (err == vfsErrFileEOF) break;
                    if (err != errNone) goto Error;
                    if (buffer[i] == 0) break;
                    i++;
                }
                buffer[i] = 0;
                ofi->val_release(val); // release the old value
                if (!ofi->val_copyString(val, buffer)) goto Error;
            }
            break;
    }

    return true;
Error:
    return false;
}

void File_read(OrbFormsInterface* ofi) {
    long count = ofi->vm_pop()->iVal;
    Value* vtype = ofi->vm_pop();
    char* strFormat = ofi->val_lock(vtype);
    long addr = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    
    if (pd->fileRef) {
        ofi->vm_retVal->iVal = ofi->ts_iterate(addr, strFormat, count, read_value, pd);
    }
    
    ofi->val_unlockRelease(vtype);
}

bool write_value(OrbFormsInterface* ofi, Value* val, VarType type, int len, void* context) {
    FileData* sd = (FileData*)context;
    char* str;
    
    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            MemMove(sd->bytes + sd->offset, (char*)(&val->iVal)+4-len, len);
            sd->offset += len;
            break;
        case vtString:
            str = ofi->val_lock(val);
            if (len) {
                // fixed width string
                int slen = StrLen(str);
                if (slen < len) {
                    MemMove(sd->bytes + sd->offset, str, slen);
                    MemSet(sd->bytes + sd->offset + slen, len - slen, 0);
                } else {
                    MemMove(sd->bytes + sd->offset, str, len);
                }
            } else {
                // null-terminated string
                len = StrLen(str) + 1;
                MemMove(sd->bytes + sd->offset, str, len);
            }
            sd->offset += len;
            ofi->val_unlock(val);
            break;
    }
    
    // always continue
    return true;
}

/*
bool write_value(OrbFormsInterface* ofi, Value* val, VarType type, int len, void* context) {
    FileData* fd = (FileData*)context;
    char* str;
    UInt32 wlen;
    Err err;
    
    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            err = VFSFileWrite(fd->fileRef, len, (char*)(&val->iVal)+4-len, &wlen);
            if (err != errNone) goto Error;
            break;
        case vtString:
            str = ofi->val_lock(val);
            if (len) {
                // fixed width string
                int slen = StrLen(str);
                if (slen < len) {
                    err = VFSFileWrite(fd->fileRef, slen, str, &wlen);
                    if (err != errNone) goto Error;
                    for (int i=0;i<len-slen;i++) {
                        char null = '\0';
                        err = VFSFileWrite(fd->fileRef, 1, &null, &wlen);
                        if (err != errNone) goto Error;
                    }
                } else {
                    err = VFSFileWrite(fd->fileRef, len, str, &wlen);
                    if (err != errNone) goto Error;
                }
            } else {
                // null-terminated string
                len = StrLen(str) + 1;
                err = VFSFileWrite(fd->fileRef, len, str, &wlen);
                if (err != errNone) goto Error;
            }
            ofi->val_unlock(val);
            break;
    }

    return true;
Error:
    return false;
}

void File_write(OrbFormsInterface* ofi) {
    long count = ofi->vm_pop()->iVal;
    Value* vtype = ofi->vm_pop();
    char* strFormat = ofi->val_lock(vtype);
    long addr = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    
    if (pd->fileRef) {
        long size = ofi->ts_data_size(addr, strFormat, count);
        UInt32 offset = 0;
        VFSFileTell(pd->fileRef, &offset);
        UInt32 fileSize = 0;
        VFSFileSize(pd->fileRef, &fileSize);
        if (offset + size > fileSize) {
            VFSFileResize(pd->fileRef, offset + size);
            VFSFileSeek(pd->fileRef, vfsOriginBeginning, offset);
        }
        ofi->vm_retVal->iVal = ofi->ts_iterate(addr, strFormat, count, write_value, pd);
    }
    
    ofi->val_unlockRelease(vtype);
}
*/

void File_write(OrbFormsInterface* ofi) {
    long count = ofi->vm_pop()->iVal;
    Value* vtype = ofi->vm_pop();
    char* strFormat = ofi->val_lock(vtype);
    long addr = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;

    // calculate the total size for the buffer
    long size = ofi->ts_data_size(addr, strFormat, count);
    if (size) {
        pd->bytes = new char[size];
        if (pd->bytes) {
            pd->offset = 0;
            long wCount = ofi->ts_iterate(addr, strFormat, count, write_value, pd);
            if (pd->offset == size) {
                // we packed everything, now send
                Err err = 0;
                UInt32 wlen;
                err = VFSFileWrite(pd->fileRef, size, pd->bytes, &wlen);
                if (err == 0)
                    ofi->vm_retVal->iVal = wCount;
            }
            delete pd->bytes;
        }
    }
    
    ofi->val_unlockRelease(vtype);
}

void File_setDate(OrbFormsInterface* ofi) {
    long addrDate = ofi->vm_pop()->iVal;
    int which = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    UInt32 date = ofi->vm_deref(addrDate)->iVal;
    VFSFileSetDate(pd->fileRef, which, date);
}

void File_getDate(OrbFormsInterface* ofi) {
    int which = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    long addrDate = ofi->vm_pop()->iVal;
    
    UInt32 date = 0;
    VFSFileGetDate(pd->fileRef, which, &date);
    Value* vdate = ofi->vm_deref(addrDate);
    vdate->iVal = date;

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = addrDate;
}

void File_get_offset(OrbFormsInterface* ofi) {
    UInt32 offset = 0;
    FileData* pd = (FileData*)ofi->nobj_pop();
    VFSFileTell(pd->fileRef, &offset);
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = offset;
}

void File_set_offset(OrbFormsInterface* ofi) {
    UInt32 offset = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    VFSFileSeek(pd->fileRef, vfsOriginBeginning, offset);
    VFSFileTell(pd->fileRef, &offset);
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = offset;
}

void File_get_eof(OrbFormsInterface* ofi) {
    FileData* pd = (FileData*)ofi->nobj_pop();
    Err err = VFSFileEOF(pd->fileRef);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err == vfsErrFileEOF;
}

void File_get_size(OrbFormsInterface* ofi) {
    UInt32 size = 0;
    FileData* pd = (FileData*)ofi->nobj_pop();
    VFSFileSize(pd->fileRef, &size);
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = size;
}

void File_set_size(OrbFormsInterface* ofi) {
    UInt32 size = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    VFSFileResize(pd->fileRef, size);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = size;
}

void File_get_attribs(OrbFormsInterface* ofi) {
    UInt32 attribs = 0;
    FileData* pd = (FileData*)ofi->nobj_pop();
    VFSFileGetAttributes(pd->fileRef, &attribs);
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = attribs;
}

void File_set_attribs(OrbFormsInterface* ofi) {
    UInt32 attribs = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    VFSFileSetAttributes(pd->fileRef, attribs);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = attribs;
}

void File_enum(OrbFormsInterface* ofi) {
    long addrAttrib = ofi->vm_pop()->iVal;
    long addrName = ofi->vm_pop()->iVal;
    bool first = ofi->vm_pop()->iVal;
    FileData* pd = (FileData*)ofi->nobj_pop();
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;

    if (first) {
        pd->dirIter = vfsIteratorStart;
    } else if (pd->dirIter == vfsIteratorStop) {
        return;
    }
    FileInfoType info;
    char buff[256];
    info.nameP = buff;
    info.nameBufLen = 256;
    Err err = VFSDirEntryEnumerate(pd->fileRef, &pd->dirIter, &info);
    if (err == errNone) {
        Value* pname = ofi->vm_deref(addrName);
        ofi->val_release(pname);
        ofi->val_copyString(pname, buff);
        Value* pattrib = ofi->vm_deref(addrAttrib);
        pattrib->iVal = info.attributes;
        ofi->vm_retVal->iVal = 1;
    }
}

void VolumeMgr_enum(OrbFormsInterface* ofi, VFSData* data) {
    VolumeData* volume = (VolumeData*)ofi->nobj_pop();
    bool first = ofi->vm_pop()->iVal;
    ofi->vm_pop(); // ignore object pointer
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    
    if (first) {
        data->volIter = vfsIteratorStart;
        // ensure that the VFS manager exists
        UInt32 vfsMgrVersion;
        Err err = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &vfsMgrVersion);
        if (err) {
            return;
        }
    } else if (data->volIter == vfsIteratorStop) {
        return;
    }
    Err err = VFSVolumeEnumerate(&volume->volRef, &data->volIter);
    if (err == errNone) {
        ofi->vm_retVal->iVal = 1;
    }
}


void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
{
    if (iFunc == NATIVE_STARTUP) {
        VFSData* vfsData = new VFSData;
        vfsData->volIter = 0;
        return vfsData;
    } else if (iFunc == NATIVE_SHUTDOWN) {
        VFSData* vfsData = (VFSData*)data;
        delete vfsData;
        return NULL;
    }

    switch (iFunc) {
        case 0: VolumeMgr_enum(ofi, (VFSData*)data); break;
        case 1: Volume_open(ofi, false); break;
        case 2: Volume_create(ofi); break;
        case 3: Volume_createDir(ofi); break;
        case 4: Volume_del(ofi); break;
        case 5: Volume_rename(ofi); break;
        case 6: Volume_export(ofi); break;
        case 7: Volume_import(ofi); break;
        case 8: Volume_getDefaultDir(ofi); break;
        case 9: Volume_get_label(ofi); break;
        case 10: File_close(ofi); break;
        case 11: File_read(ofi); break;
        case 12: File_write(ofi); break;
        case 13: File_setDate(ofi); break;
        case 14: File_getDate(ofi); break;
        case 15: File_get_offset(ofi); break;
        case 16: File_set_offset(ofi); break;
        case 17: File_get_eof(ofi); break;
        case 18: File_get_size(ofi); break;
        case 19: File_set_size(ofi); break;
        case 20: File_get_attribs(ofi); break;
        case 21: File_set_attribs(ofi); break;
        case 22: File_enum(ofi); break;
        case 23: Volume_init(ofi); break;
        case 24: File_init(ofi); break;
        case 25: Volume_open(ofi, true); break;
        case 26: File_get_fileRef(ofi); break;
    }
    return NULL;
}
