#include <PalmOS.h>
#include <Extensions\ExpansionMgr\VFSMgr.h>
#include "..\..\PcNative.h"

extern "C" {
void* __Startup__(PocketCInterface*, void*, UInt16);
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

#define MAX_HANDLES 32
class HandleTable {
public:
    HandleTable();
    void* find(PocketCInterface* pci, int id);
    int alloc(void* data);
    void clear(int id);
    void* findNext();

private:
    void* data[MAX_HANDLES];
    int nextId;
};

HandleTable::HandleTable() : nextId(-1) {
    for (int i=0;i<MAX_HANDLES;i++)
        data[i] = 0;
}

void* HandleTable::find(PocketCInterface* pci, int id) {
    if (id < 1 || id > MAX_HANDLES) {
        pci->vm_error("invalid handle value");
    }
    return data[id-1];
}

int HandleTable::alloc(void* d) {
    for (int i=0;i<MAX_HANDLES;i++) {
        if (data[i] == NULL) {
            data[i] = d;
            return i+1;
        }
    }
    return 0;
}

void* HandleTable::findNext() {
    while (++nextId < MAX_HANDLES) {
        if (data[nextId] != NULL) return data[nextId];
    }
    return NULL;
}

void HandleTable::clear(int id) {
    if (id < 1 || id > MAX_HANDLES) {
        //pci->vm_error("invalid handle value");
        return;
    }
    data[id-1] = NULL;
}

/*

fileclose(int id);
fileread(int id, pointer data, string format, int count);
filewrite(int id, pointer data, string format, int count);
filesetdate(int id, int which, int date);
filegetdate(int id, int which);
fileseek(int id, int offset);
filetell(int id);
fileeof(int id);
filesize(int id);
fileresize(int id, int size);
filegetattribs(int id);
filesetattribs(int id, int attribs);

dirclose(int id);
dirsetdate(int id, int which, int date);
dirgetdate(int id, int which);
dirgetattribs(int id);
dirsetattribs(int id, int attribs);
direnum(int id, int first, pointer name, pointer attribs);

volclose(int volid);
volopenfile(int volid, string path, int mode, pointer fileid);
volopendir(int volid, string path, pointer dirid);
volcreatefile(int volid, string path);
volcreatedir(int volid, string path);
voldelete(int volid, string path);
volrename(int volid, string path, string newname);
volexport(int volid, string name, string path);
volimport(int volid, string path, pointer pname);
volgetdefdir(int volid, string type);
vollabel(int volid);

enumvols(int first, pointer volid);



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
    HandleTable ht;
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

void Volume_open(PocketCInterface* pci, VFSData* vfsData, bool isDir) {
    FileData* fd = new FileData;
    fd->fileRef = 0;
    fd->dirIter = vfsIteratorStart;
    int newid = vfsData->ht.alloc(fd);
    
    Value* vaddr = pci->vm_deref(pci->vm_pop()->iVal);
    UInt16 mode = vfsModeRead;
    if (!isDir) {
        mode = pci->vm_pop()->iVal;
    }
    Value* vpath = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    char* path = pci->val_lock(vpath);

    Err err = VFSFileOpen(volRef, path, mode, &fd->fileRef);
    pci->val_unlockRelease(vpath);
    
    if (err == 0) {
        vaddr->iVal = newid;
    } else {
        vfsData->ht.clear(newid);
        delete fd;
    }
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Volume_createFile(PocketCInterface* pci, VFSData* vfsData) {
    Value* vpath = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    char* path = pci->val_lock(vpath);
    Err err = VFSFileCreate(volRef, path);
    pci->val_unlockRelease(vpath);
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Volume_createDir(PocketCInterface* pci, VFSData* vfsData) {
    Value* vpath = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    char* path = pci->val_lock(vpath);
    Err err = VFSDirCreate(volRef, path);
    pci->val_unlockRelease(vpath);
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Volume_del(PocketCInterface* pci, VFSData* vfsData) {
    Value* vpath = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    char* path = pci->val_lock(vpath);
    Err err = VFSFileDelete(volRef, path);
    pci->val_unlockRelease(vpath);
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Volume_rename(PocketCInterface* pci, VFSData* vfsData) {
    Value* vname = pci->vm_pop();
    Value* vpath = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    char* path = pci->val_lock(vpath);
    char* name = pci->val_lock(vname);
    Err err = VFSFileRename(volRef, path, name);
    pci->val_unlockRelease(vpath);
    pci->val_unlockRelease(vname);
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
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

void Volume_export(PocketCInterface* pci, VFSData* vfsData) {
    Value* vpath = pci->vm_pop();
    Value* vname = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = 0;

    char* path = pci->val_lock(vpath);
    char* name = pci->val_lock(vname);
    UInt16 card;
    LocalID lid = MyFindDatabase(name, card);
    if (!lid) {
        pci->vm_retVal->iVal = vfsErrFileNotFound;
        goto end;
    }
    
    Err err = VFSExportDatabaseToFile(volRef, path, card, lid);
    pci->vm_retVal->iVal = err;

end:
    pci->val_unlockRelease(vname);
    pci->val_unlockRelease(vpath);
}

void Volume_import(PocketCInterface* pci, VFSData* vfsData) {
    long addrName = pci->vm_pop()->iVal;
    Value* vpath = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    char* path = pci->val_lock(vpath);
    UInt16 card;
    LocalID lid;
    Err err = VFSImportDatabaseFromFile(volRef, path, &card, &lid);
    if (err == errNone) { // or dmErrAlreadyExists??
        // get name
        char buff[32];
        buff[0] = 0;
        DmDatabaseInfo(card, lid, buff, 0,0,0,0,0,0,0,0,0,0);
        Value* vname = pci->vm_deref(addrName);
        pci->val_release(vname);
        pci->val_copyString(vname, buff);
    }

    pci->val_unlockRelease(vpath);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Volume_getDefaultDir(PocketCInterface* pci, VFSData* vfsData) {
    Value* vtype = pci->vm_pop();
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;

    char buff[256];
    buff[0] = 0;
    UInt16 len = 256;
    char* type = pci->val_lock(vtype);
    Err err = VFSGetDefaultDirectory(volRef, type, buff, &len);
    pci->val_unlockRelease(vtype);
    
    if (err == errNone) {
        pci->val_copyString(pci->vm_retVal, buff);
    } else {
        pci->val_newConstString(pci->vm_retVal, "");
    }
}

void Volume_get_label(PocketCInterface* pci, VFSData* vfsData) {
    UInt16 volRef = (UInt16)pci->vm_pop()->iVal;
    char buff[256];
    buff[0] = 0;
    
    VFSVolumeGetLabel(volRef, buff, 256);
    pci->val_copyString(pci->vm_retVal, buff);
}

void File_close(PocketCInterface* pci, VFSData* vfsData) {
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    FileClose(fd);
    vfsData->ht.clear(id);
    delete fd;
}

bool read_value(PocketCInterface* pci, Value* val, VarType type, int len, void* context) {
    FileData* fd = (FileData*)context;
    
    Err err;
    UInt32 rlen;
    char buffer[512] = {0};
    if (len > 511) pci->vm_error("File.read() string too long");
    
    switch (type) {
        case vtInt:
        case vtFloat:
            val->iVal = 0;
            err = VFSFileRead(fd->fileRef, len, (char*)(&val->iVal)+4-len, &rlen);
            if (err != errNone) goto Error;
            break;
        case vtChar:
            val->iVal = 0;
            err = VFSFileRead(fd->fileRef, len, &val->cVal, &rlen);
            if (err != errNone) goto Error;
            break;
        case vtString:
            if (len) {
                err = VFSFileRead(fd->fileRef, len, buffer, &rlen);
                if (err != errNone && err != vfsErrFileEOF) goto Error;
                buffer[rlen] = 0;
                pci->val_release(val); // release the old value
                if (!pci->val_copyString(val, buffer)) goto Error;
            } else {
                int i = 0;
                // read one byte at a time until we get a NULL
                while (i < 511) {
                    // simplify by keeping the same timout
                    err = VFSFileRead(fd->fileRef, 1, &buffer[i], &rlen);
                    if (err == vfsErrFileEOF) break;
                    if (err != errNone) goto Error;
                    if (buffer[i] == 0) break;
                    i++;
                }
                buffer[i] = 0;
                pci->val_release(val); // release the old value
                if (!pci->val_copyString(val, buffer)) goto Error;
            }
            break;
    }

    return true;
Error:
    return false;
}

void File_read(PocketCInterface* pci, VFSData* vfsData) {
    long count = pci->vm_pop()->iVal;
    Value* vtype = pci->vm_pop();
    char* strFormat = pci->val_lock(vtype);
    long addr = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = 0;
    
    if (fd->fileRef) {
        pci->vm_retVal->iVal = pci->ts_iterate(addr, strFormat, count, read_value, fd);
    }
    
    pci->val_unlockRelease(vtype);
}

bool write_value(PocketCInterface* pci, Value* val, VarType type, int len, void* context) {
    FileData* sd = (FileData*)context;
    char* str;
    
    switch (type) {
        case vtInt:
        case vtFloat:
            MemMove(sd->bytes + sd->offset, (char*)(&val->iVal)+4-len, len);
            sd->offset += len;
            break;
        case vtChar:
            *(sd->bytes + sd->offset) = val->cVal;
            sd->offset += len;
            break;
        case vtString:
            str = pci->val_lock(val);
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
            pci->val_unlock(val);
            break;
    }
    
    // always continue
    return true;
}

void File_write(PocketCInterface* pci, VFSData* vfsData) {
    long count = pci->vm_pop()->iVal;
    Value* vtype = pci->vm_pop();
    char* strFormat = pci->val_lock(vtype);
    long addr = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = 0;

    // calculate the total size for the buffer
    long size = pci->ts_data_size(addr, strFormat, count);
    if (size) {
        fd->bytes = new char[size];
        if (fd->bytes) {
            fd->offset = 0;
            long wCount = pci->ts_iterate(addr, strFormat, count, write_value, fd);
            if (fd->offset == size) {
                // we packed everything, now send
                Err err = 0;
                UInt32 wlen;
                err = VFSFileWrite(fd->fileRef, size, fd->bytes, &wlen);
                if (err == 0)
                    pci->vm_retVal->iVal = wCount;
            }
            delete fd->bytes;
        }
    }
    
    pci->val_unlockRelease(vtype);
}

void File_setDate(PocketCInterface* pci, VFSData* vfsData) {
    UInt32 date = pci->vm_pop()->iVal + 2147483648UL;
    int which = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    VFSFileSetDate(fd->fileRef, which, date);
}

void File_getDate(PocketCInterface* pci, VFSData* vfsData) {
    int which = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    
    UInt32 date = 0;
    VFSFileGetDate(fd->fileRef, which, &date);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = date - 2147483648UL;
}

void File_get_offset(PocketCInterface* pci, VFSData* vfsData) {
    UInt32 offset = 0;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    VFSFileTell(fd->fileRef, &offset);
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = offset;
}

void File_set_offset(PocketCInterface* pci, VFSData* vfsData) {
    UInt32 offset = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    VFSFileSeek(fd->fileRef, vfsOriginBeginning, offset);
    VFSFileTell(fd->fileRef, &offset);
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = offset;
}

void File_get_eof(PocketCInterface* pci, VFSData* vfsData) {
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    Err err = VFSFileEOF(fd->fileRef);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err == vfsErrFileEOF;
}

void File_get_size(PocketCInterface* pci, VFSData* vfsData) {
    UInt32 size = 0;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    VFSFileSize(fd->fileRef, &size);
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = size;
}

void File_set_size(PocketCInterface* pci, VFSData* vfsData) {
    UInt32 size = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    VFSFileResize(fd->fileRef, size);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = size;
}

void File_get_attribs(PocketCInterface* pci, VFSData* vfsData) {
    UInt32 attribs = 0;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    VFSFileGetAttributes(fd->fileRef, &attribs);
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = attribs;
}

void File_set_attribs(PocketCInterface* pci, VFSData* vfsData) {
    UInt32 attribs = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    VFSFileSetAttributes(fd->fileRef, attribs);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = attribs;
}

void Dir_enum(PocketCInterface* pci, VFSData* vfsData) {
    long addrAttrib = pci->vm_pop()->iVal;
    long addrName = pci->vm_pop()->iVal;
    bool first = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    FileData* fd = (FileData*)vfsData->ht.find(pci, id);
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = 0;

    if (first) {
        fd->dirIter = vfsIteratorStart;
    } else if (fd->dirIter == vfsIteratorStop) {
        return;
    }
    FileInfoType info;
    char buff[256];
    info.nameP = buff;
    info.nameBufLen = 256;
    Err err = VFSDirEntryEnumerate(fd->fileRef, &fd->dirIter, &info);
    if (err == errNone) {
        Value* pname = pci->vm_deref(addrName);
        pci->val_release(pname);
        pci->val_copyString(pname, buff);
        Value* pattrib = pci->vm_deref(addrAttrib);
        pattrib->iVal = info.attributes;
        pci->vm_retVal->iVal = 1;
    }
}

void VolumeMgr_enum(PocketCInterface* pci, VFSData* data) {
    Value* volId = pci->vm_deref(pci->vm_pop()->iVal);
    bool first = pci->vm_pop()->iVal;
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = 0;
    
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
    UInt16 volRef;
    Err err = VFSVolumeEnumerate(&volRef, &data->volIter);
    if (err == errNone) {
        volId->iVal = volRef;
        pci->vm_retVal->iVal = 1;
    }
}

void Volume_close(PocketCInterface* pci, VFSData* vfsData) {
    pci->vm_pop();
}


void* __Startup__(PocketCInterface* pci, void* data, UInt16 iFunc)
{
    if (iFunc == NATIVE_STARTUP) {
        VFSData* vfsData = new VFSData;
        vfsData->volIter = 0;
        return vfsData;
    } else if (iFunc == NATIVE_SHUTDOWN) {
        VFSData* vfsData = (VFSData*)data;
        FileData* fd;
        while (NULL != (fd = (FileData*)vfsData->ht.findNext())) {
            FileClose(fd);
            delete fd;
        }
        delete vfsData;
        return NULL;
    }

    switch (iFunc) {
        case 0: File_close(pci, (VFSData*)data); break;
        case 1: File_read(pci, (VFSData*)data); break;
        case 2: File_write(pci, (VFSData*)data); break;
        case 3: File_setDate(pci, (VFSData*)data); break;
        case 4: File_getDate(pci, (VFSData*)data); break;
        case 5: File_set_offset(pci, (VFSData*)data); break;
        case 6: File_get_offset(pci, (VFSData*)data); break;
        case 7: File_get_eof(pci, (VFSData*)data); break;
        case 8: File_get_size(pci, (VFSData*)data); break;
        case 9: File_set_size(pci, (VFSData*)data); break;
        case 10: File_get_attribs(pci, (VFSData*)data); break;
        case 11: File_set_attribs(pci, (VFSData*)data); break;

        case 12: File_close(pci, (VFSData*)data); break;
        case 13: File_setDate(pci, (VFSData*)data); break;
        case 14: File_getDate(pci, (VFSData*)data); break;
        case 15: File_get_attribs(pci, (VFSData*)data); break;
        case 16: File_set_attribs(pci, (VFSData*)data); break;
        case 17: Dir_enum(pci, (VFSData*)data); break;

        case 18: Volume_close(pci, (VFSData*)data); break;
        case 19: Volume_open(pci, (VFSData*)data, false); break;
        case 20: Volume_open(pci, (VFSData*)data, true); break;
        case 21: Volume_createFile(pci, (VFSData*)data); break;
        case 22: Volume_createDir(pci, (VFSData*)data); break;
        case 23: Volume_del(pci, (VFSData*)data); break;
        case 24: Volume_rename(pci, (VFSData*)data); break;
        case 25: Volume_export(pci, (VFSData*)data); break;
        case 26: Volume_import(pci, (VFSData*)data); break;
        case 27: Volume_getDefaultDir(pci, (VFSData*)data); break;
        case 28: Volume_get_label(pci, (VFSData*)data); break;

        case 29: VolumeMgr_enum(pci, (VFSData*)data); break;
    }
    return NULL;
}
