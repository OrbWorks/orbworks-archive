#include <PalmOS.h>
#include <Extensions\ExpansionMgr\VFSMgr.h>
#include "..\..\PcNative.h"

typedef unsigned char byte;

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

extern "C"
asm long __lmul__(long left:__D0,long right:__D1):__D0;
asm long __lmul__(long left:__D0,long right:__D1):__D0
{				/* d0: operand a ; d1: operand b */
#if __CFM68K__
    move.l	(a1), a5
#endif

    movem.l	d2-d3,-(sp)
    move.l	d0,d2
    swap	d2
    mulu.w	d1,d2		/* HiWord(a)*LoWord(b)->d2 */
    move.l	d1,d3
    swap	d3
    mulu.w	d0,d3		/* LoWord(a)*HiWord(b)->d3 */
    add.w	d3,d2		/* add the two results */
    swap	d2		/* and put LoWord of result in HiWord(d2) */
    clr.w	d2		/* clr LoWord(d2) */
    mulu	d1,d0		/* LoWord(a)*LoWord(b)->d0 */
    add.l	d2,d0		/* result:=d0+d2 */
    movem.l	(sp)+,d2-d3
    rts
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
// address families
const int netAfInet = 2;
const int netAfInet6 = 23;

// socket types
const int netSockStream = 1;
const int netSockDgram = 2;

// protocol types
const int netProtoIpTcp = 6;
const int netProtoIpUdp = 17;

// directions for Socket.shutdown
const int netDirInput = 0;
const int netDirOutput = 1;
const int netDirBoth = 2;

// error codes
const int netErrInvalidAddrString = 1;
const int netErrOther = 2;

int sockopen(pointer id);
int sockclose(int id);
int sockaccept(int id, pointer newid);
int sockbind(int id, string localAddr);
int sockconnect(int id, string addr);
int socklisten(int id, int backlog);
int socksend(int id, pointer data, string format, int count);
int sockrecv(int id, pointer data, string format, int count);
int sockshutdown(int id, int dir);
string socklocaladdr(int id);
string sockremoteaddr(int id);
    
int netopen();
void netclose();
int nethostbyname(string name, pointer paddr);
int nethostbyaddr(string addr, pointer pname);
string netlocaladdr();
int netgettimeout();
void netsettimeout(int timeout);
*/

/*
void VMError(PocketCInterface* pci, char* str, UInt32 err) {
    char buff[256];
    StrPrintF(buff, "%s: %ld", str, err);
    pci->vm_error(buff);
}
*/

bool StringToAddr(char* str, NetSocketAddrINType* pAddrIn) {
    MemSet(pAddrIn, sizeof(*pAddrIn), 0);
    pAddrIn->family = netSocketAddrINET;
    
    int words = 0;
    while (*str >= '0' && *str <= '9' && words < 4) {
        UInt32 data = 0;
        while (*str >= '0' && *str <= '9') {
            data *= 10;
            data += (*str - '0');
            str++;
        }
        if (data > 255) return false;
        if (*str == '.') str++;
        words++;
        pAddrIn->addr <<= 8;
        pAddrIn->addr |= (data & 0xff);
    }

    if (*str == ':') {
        str++;
        UInt32 data = 0;
        while (*str >= '0' && *str <= '9') {
            data *= 10;
            data += (*str - '0');
            str++;
        }
        if (data > 0x0000ffff) return false;
        pAddrIn->port = data;
    }
    
    return true;
}

char* AppendAddrWord(char* str, unsigned int word) {
    char buff[8] = {0};
    int i = 0;
    bool first = true;
    while (first || word) {
        buff[i++] = (word % 10) + '0';
        word /= 10;
        first = false;
    }
    while (i--) {
        *str++ = buff[i];
    }
    return str;
}

void AddrToString(NetSocketAddrINType* pAddrIn, char* str) {
    for (int words=0; words < 4; words++) {
        str = AppendAddrWord(str, (unsigned int)(pAddrIn->addr >> (8 * (3 - words))) & 0xff);
        if (words != 3) *str++ = '.';
    }
    if (pAddrIn->port) {
        *str++ = ':';
        str = AppendAddrWord(str, pAddrIn->port);
    }
    *str = '\0';
}

class StringBuffer {
public:
    StringBuffer() {
        size = len = 0;
        buff = NULL;
    }
    ~StringBuffer() {
        if (buff) delete buff;
    }
    bool reserve(int _size) {
        size = _size;
        buff = new char[size];
        return (buff != NULL);
    }
    bool add(char c) {
        if (!buff) {
            if (!reserve(128)) return false;
        }
        if (len + 2 > size) {
            char* newbuff = new char[size * 2];
            if (newbuff) {
                MemMove(newbuff, buff, size);
                size *= 2;
                delete buff;
                buff = newbuff;
            } else return false;
        }
        buff[len++] = c;
        return true;
    }
    char* c_str() {
        if (buff) {
            buff[len] = 0;
            return buff;
        } else return "";
    }
    
    long size, len;
    char* buff;
};

struct SocketData;

struct NetworkData {
    UInt16 libRefNum;
    UInt32 timeout;
    int openCount;
    HandleTable ht;
};

struct SocketData {
    NetSocketRef socketRef;

    // send context
    UInt32 offset;
    char* bytes;

    // recv context
    Err err;
    NetSocketAddrINType* inAddr;
    NetworkData* nd;
};

Err SocketClose(SocketData* sd, UInt16 libRef, UInt32 timeout) {
    if (sd->socketRef) {
        Err err = 0;
        NetLibSocketClose(libRef, sd->socketRef, timeout, &err);
        if (err == 0) sd->socketRef = NULL;
        return err;
    }
    return 0;
}

void Socket_open(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    SocketData* sd = new SocketData;
    sd->socketRef = 0;
    Err err = 0;
    int id = networkData->ht.alloc(sd);

    if (id) {
        long addr = pci->vm_pop()->iVal;
        Value* vaddr = pci->vm_deref(addr);
        
        long protocol = netSocketProtoIPTCP;
        long type = netSocketTypeStream;
        long family = netSocketAddrINET;

        sd->socketRef = NetLibSocketOpen(libRef, (NetSocketAddrEnum)family, (NetSocketTypeEnum)type, protocol, timeout, &err);
        if (err != 0) {
            networkData->ht.clear(id);
            delete sd;
        } else {
            vaddr->iVal = id;
            NetSocketLingerType linger;
            linger.onOff = true;
            linger.time = 0;
            NetLibSocketOptionSet(libRef, sd->socketRef,
                netSocketOptLevelSocket, netSocketOptSockLinger,
                &linger, sizeof(linger), -1, &err);
        }
    } else {
        err = 2;
        delete sd;
    }

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Socket_close(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);

    Err err = SocketClose(sd, libRef, timeout);
    networkData->ht.clear(id);
    delete sd;

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Socket_accept(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    SocketData* sdNew = new SocketData;
    int newid = networkData->ht.alloc(sdNew);

    Value* vaddr = pci->vm_deref(pci->vm_pop()->iVal);

    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    Err err = 0;
    NetSocketAddrType addr;
    Int16 addrSize = sizeof(addr);
    
    if (newid) {
        SocketClose(sdNew, libRef, timeout);
        sdNew->socketRef = NetLibSocketAccept(libRef, sd->socketRef, &addr, &addrSize, timeout, &err);
        if (err != 0) {
            networkData->ht.clear(newid);
            delete sdNew;
        } else {
            vaddr->iVal = newid;
            NetSocketLingerType linger;
            linger.onOff = true;
            linger.time = 0;
            NetLibSocketOptionSet(libRef, sdNew->socketRef,
                netSocketOptLevelSocket, netSocketOptSockLinger,
                &linger, sizeof(linger), -1, &err);
        }
    } else {
        err = 0;
        delete sdNew;
    }

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Socket_bind(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    Value* vAddr = pci->vm_pop();
    char* sAddr = pci->val_lock(vAddr);
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    Err err = 0;
    NetSocketAddrType addr;
    Int16 addrSize = sizeof(addr);
    
    if (StringToAddr(sAddr, (NetSocketAddrINType*)&addr)) {
        NetLibSocketBind(libRef, sd->socketRef, &addr, addrSize, timeout, &err);
    } else {
        err = 1; // netErrInvalidAddrString
    }
    pci->val_unlockRelease(vAddr);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Socket_connect(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    Value* vAddr = pci->vm_pop();
    char* sAddr = pci->val_lock(vAddr);
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    Err err = 0;
    NetSocketAddrType addr;
    Int16 addrSize = sizeof(addr);
    
    if (StringToAddr(sAddr, (NetSocketAddrINType*)&addr)) {
        //VMError(pci, "addr", ((NetSocketAddrINType*)&addr)->port);
        NetLibSocketConnect(libRef, sd->socketRef, &addr, addrSize, timeout, &err);
    } else {
        err = 1; // netErrInvalidAddrString
    }
    pci->val_unlockRelease(vAddr);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Socket_listen(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    int backlog = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    Err err = 0;
    
    NetLibSocketListen(libRef, sd->socketRef, backlog, timeout, &err);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

// send helper
bool pack_buffer(PocketCInterface* pci, Value* val, VarType type, int len, void* context) {
    SocketData* sd = (SocketData*)context;
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

bool recv_value(PocketCInterface* pci, Value* val, VarType type, int len, void* context) {
    SocketData* sd = (SocketData*)context;
    
    Err err;
    int rlen;
    StringBuffer sb;
    if (type == vtString && len > 0) {
        if (!sb.reserve(len)) goto MemError;
    }
    UInt16 addrSize = sd->inAddr ? sizeof(NetSocketAddrINType) : 0;
    UInt16 oSize = addrSize;
    bool dataError = false;
    
    switch (type) {
        case vtInt:
        case vtFloat:
            val->iVal = 0;
            rlen = NetLibReceive(sd->nd->libRefNum, sd->socketRef, (char*)(&val->iVal)+4-len, len, 0, sd->inAddr, &addrSize, sd->nd->timeout, &err);
            if (rlen < len) goto DataError; // didn't get all data
            if (err != errNone) goto Error; // line or other error
            break;
        case vtChar:
            val->iVal = 0;
            rlen = NetLibReceive(sd->nd->libRefNum, sd->socketRef, &val->cVal, len, 0, sd->inAddr, &addrSize, sd->nd->timeout, &err);
            if (rlen < len) goto DataError; // didn't get all data
            if (err != errNone) goto Error; // line or other error
            break;
        case vtString:
            if (len) {
                rlen = NetLibReceive(sd->nd->libRefNum, sd->socketRef, sb.c_str(), len, 0, sd->inAddr, &addrSize, sd->nd->timeout, &err);
                if (rlen < len) dataError = true; // didn't get all data
                if (err != errNone) goto Error; // line or other error
                pci->val_release(val); // release the old value
                if (!pci->val_copyString(val, sb.c_str())) goto Error;
                if (dataError) goto DataError;
            } else {
                int i = 0;
                // receive one byte at a time until we get a NULL
                while (true) {
                    char c;
                    // simplify by keeping the same timout
                    addrSize = oSize;
                    rlen = NetLibReceive(sd->nd->libRefNum, sd->socketRef, &c, sizeof(c), 0, sd->inAddr, &addrSize, sd->nd->timeout, &err);
                    if (rlen < 1) dataError = true; // didn't get all data
                    if (err != errNone) goto Error; // line or other error
                    if (c == 0) break;
                    sb.add(c);
                    if (dataError) break;
                }
                pci->val_release(val); // release the old value
                if (!pci->val_copyString(val, sb.c_str())) goto Error;
            }
            break;
    }

    return true;
Error:
    sd->err = err;
    return false;
MemError:
    sd->err = netErrOutOfMemory;
    return false;
DataError:
    sd->err = netErrSocketClosedByRemote;
    return false;
}

// to send, we create a temp buffer and pack all the data into it
// then send the whole buffer in one shot
void Socket_send(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout, bool sendTo) {
    int count = pci->vm_pop()->iVal;
    Value* vtype = pci->vm_pop();
    char* strFormat = pci->val_lock(vtype);
    long addrData = pci->vm_pop()->iVal;
    Value* stAddr = NULL;
    if (sendTo) {
        stAddr = pci->vm_pop();
    }
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    
    NetSocketAddrINType inAddr = {0}; 
    if (!sendTo || StringToAddr(pci->val_lock(stAddr), &inAddr)) {
        pci->vm_retVal->type = vtInt;
        pci->vm_retVal->iVal = netErrSocketNotOpen;
        
        if (sd->socketRef) {
            pci->vm_retVal->iVal = 2; // netErrOther
            // calculate the total size for the buffer
            long size = pci->ts_data_size(addrData, strFormat, count);
            if (size) {
                sd->bytes = new char[size];
                if (sd->bytes) {
                    sd->offset = 0;
                    pci->ts_iterate(addrData, strFormat, count, pack_buffer, sd);
                    if (sd->offset == size) {
                        // we packed everything, now send
                        Err err = 0;
                        UInt32 sent = 0;
                        while (err == 0 && sent < size) {
                            if (sendTo)
                                sent += NetLibSend(libRef, sd->socketRef, sd->bytes + sent, size - sent, 0, &inAddr, sizeof(inAddr), timeout, &err);
                            else
                                sent += NetLibSend(libRef, sd->socketRef, sd->bytes + sent, size - sent, 0, 0, 0, timeout, &err);
                        }
                        pci->vm_retVal->iVal = err;
                    }
                    delete sd->bytes;
                }
            }
        }
    } else {
        pci->vm_retVal->iVal = 1;
    }
    
    pci->val_unlockRelease(vtype);
    if (sendTo) pci->val_unlockRelease(stAddr);
}

void Socket_recv(PocketCInterface* pci, NetworkData* networkData, bool recvFrom) {
    int count = pci->vm_pop()->iVal;
    Value* vtype = pci->vm_pop();
    char* strFormat = pci->val_lock(vtype);
    long addrData = pci->vm_pop()->iVal;
    Value* rfAddr = NULL;
    if (recvFrom) {
        rfAddr = pci->vm_deref(pci->vm_pop()->iVal);
    }
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    
    NetSocketAddrType inAddr = {0};
    if (recvFrom) sd->inAddr = (NetSocketAddrINType*)&inAddr;
    else sd->inAddr = NULL;
    //sd->inAddr = NULL;
    
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = netErrSocketNotOpen;
    
    if (sd->socketRef) {
        sd->err = 0;
        sd->nd = networkData;
        pci->ts_iterate(addrData, strFormat, count, recv_value, sd);
        pci->vm_retVal->iVal = sd->err;
        if (recvFrom) {
            pci->val_release(rfAddr);
            char* strRfAddr = pci->val_newString(rfAddr, 21);
            AddrToString((NetSocketAddrINType*)&inAddr, strRfAddr);
            pci->val_unlock(rfAddr);
        }
    }
    
    pci->val_unlockRelease(vtype);
}

void Socket_shutdown(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    int dir = pci->vm_pop()->iVal;
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    Err err = 0;
    
    NetLibSocketShutdown(libRef, sd->socketRef, dir, timeout, &err);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Socket_get_localAddr(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    Err err = 0;
    
    NetSocketAddrINType localAddr;
    NetSocketAddrINType remoteAddr;
    Int16 localAddrSize = sizeof(localAddr);
    Int16 remoteAddrSize = sizeof(remoteAddr);
    
    NetLibSocketAddr(libRef, sd->socketRef, (NetSocketAddrType*)&localAddr, &localAddrSize,
        (NetSocketAddrType*)&remoteAddr, &remoteAddrSize, timeout, &err);
        
    if (err == errNone) {
        char* str = pci->val_newString(pci->vm_retVal, 21);
        if (str) {
            AddrToString(&localAddr, str);
            pci->val_unlock(pci->vm_retVal);
        } else {
            pci->val_newConstString(pci->vm_retVal, "");
        }
    } else {
        //VMError(pci, "NetLibSocketAddr", err);
        pci->val_newConstString(pci->vm_retVal, "");
    }
}

void Socket_get_remoteAddr(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    int id = pci->vm_pop()->iVal;
    SocketData* sd = (SocketData*)networkData->ht.find(pci, id);
    Err err = 0;
    
    NetSocketAddrINType localAddr;
    NetSocketAddrINType remoteAddr;
    Int16 localAddrSize = sizeof(localAddr);
    Int16 remoteAddrSize = sizeof(remoteAddr);
    
    NetLibSocketAddr(libRef, sd->socketRef, (NetSocketAddrType*)&localAddr, &localAddrSize,
        (NetSocketAddrType*)&remoteAddr, &remoteAddrSize, timeout, &err);
        
    if (err == errNone) {
        char* str = pci->val_newString(pci->vm_retVal, 21);
        if (str) {
            AddrToString(&remoteAddr, str);
            pci->val_unlock(pci->vm_retVal);
        } else {
            pci->val_newConstString(pci->vm_retVal, "");
        }
    } else {
        //VMError(pci, "NetLibSocketAddr", err);
        pci->val_newConstString(pci->vm_retVal, "");
    }
}

void Network_get_timeout(PocketCInterface* pci, NetworkData* networkData) {
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = networkData->timeout;
}

void Network_set_timeout(PocketCInterface* pci, NetworkData* networkData) {
    UInt32 timeout = pci->vm_pop()->iVal;
    networkData->timeout = timeout;
    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = timeout;
}

void Network_getHostByName(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    long addrResult = pci->vm_pop()->iVal;
    Value* result = pci->vm_deref(addrResult);
    Value* vName = pci->vm_pop();
    char* name = pci->val_lock(vName);
    Err err = 0;
    
    NetHostInfoBufType buff = {0};
    NetLibGetHostByName(libRef, name, &buff, -1, &err);
    pci->val_unlockRelease(vName);
    
    if (err == errNone) {
        if (buff.addressList[0]) {
            pci->val_release(result);
            char* resStr = pci->val_newString(result, 21);
            if (!resStr) pci->vm_error("out of memory");
            NetSocketAddrINType addr = {0};
            addr.family = netSocketAddrINET;
            addr.addr = *buff.addressList[0];
            AddrToString(&addr, resStr);
            pci->val_unlock(result);
        } else {
            err = netErrTimeout;
        }
    }

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Network_getHostByAddr(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    long addrResult = pci->vm_pop()->iVal;
    Value* result = pci->vm_deref(addrResult);
    Value* vName = pci->vm_pop();
    char* name = pci->val_lock(vName);
    Err err = 0;
    
    NetSocketAddrINType addr = {0};
    if (StringToAddr(name, &addr)) {
        NetHostInfoBufType buff = {0};
        NetLibGetHostByAddr(libRef, (UInt8*)&addr.addr, sizeof(addr.addr), addr.family, &buff, -1, &err);
        
        if (err == errNone) {
            pci->val_release(result);
            pci->val_copyString(result, buff.name);
        }
    } else {
        err = 1;
    }
    pci->val_unlockRelease(vName);

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

// these two methods return "setting not found" on my Tungsten|C, so
// it is unlikely that many devices will be configured with anything
void Network_get_hostName(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    Err err = 0;

    char name[64];
    UInt16 size = sizeof(name);
    
    pci->val_newConstString(pci->vm_retVal, "");

    err = NetLibSettingGet(libRef, netSettingHostName, name, &size);
    if (err == errNone) {
        pci->val_copyString(pci->vm_retVal, name);
    } else {
        //VMError(pci, "NetLibSettingGet", err);
    }

}

void Network_get_domainName(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    Err err = 0;

    char name[256];
    UInt16 size = sizeof(name);
    
    pci->val_newConstString(pci->vm_retVal, "");

    err = NetLibSettingGet(libRef, netSettingDomainName, name, &size);
    if (err == errNone) {
        pci->val_copyString(pci->vm_retVal, name);
    } else {
        //VMError(pci, "NetLibSettingGet", err);
    }
}


void Network_get_localAddr(PocketCInterface* pci, NetworkData* networkData, UInt16 libRef, UInt32 timeout) {
    Err err = 0;
    
    UInt32 creator;
    UInt16 instance;
    UInt16 index = 0;
    
    pci->val_newConstString(pci->vm_retVal, "");
    
    while (NetLibIFGet(libRef, index, &creator, &instance) == 0) {
        UInt8 up;
        UInt16 size = sizeof(up);
        if (NetLibIFSettingGet(libRef, creator, instance, netIFSettingUp, &up, &size) != 0)
            break;
        if (up) {
            UInt32 addr;
            size = sizeof(addr);
            if (NetLibIFSettingGet(libRef, creator, instance, netIFSettingActualIPAddr, &addr, &size) != 0)
                break;
            char* str = pci->val_newString(pci->vm_retVal, 21);
            if (str) {
                NetSocketAddrINType sockAddr = {0};
                sockAddr.family = netSocketAddrINET;
                sockAddr.addr = addr;
                AddrToString(&sockAddr, str);
                pci->val_unlock(pci->vm_retVal);
            }
            break;
        } else {
            index++;
        }
    }
}

void Network_open(PocketCInterface* pci, NetworkData* networkData) {
    Err err = 0;
    UInt16 ifErr;
    
    err = NetLibOpen(networkData->libRefNum, &ifErr);
    if (err == netErrAlreadyOpen) err = errNone;
    
    if (err == errNone) {
        networkData->openCount++;
    }

    pci->vm_retVal->type = vtInt;
    pci->vm_retVal->iVal = err;
}

void Network_close(PocketCInterface* pci, NetworkData* networkData) {
    if (networkData->openCount > 0) {
        NetLibClose(networkData->libRefNum, false);
        networkData->openCount--;
    }
}

void* __Startup__(PocketCInterface* pci, void* data, UInt16 iFunc)
{
    if (iFunc == NATIVE_STARTUP) {
        NetworkData* networkData = new NetworkData;
        networkData->libRefNum = 0;
        networkData->openCount = 0;
        networkData->timeout = SysTicksPerSecond() * 10;

        (void)SysLibFind("Net.lib", &networkData->libRefNum);
        return networkData;
    } else if (iFunc == NATIVE_SHUTDOWN) {
        NetworkData* networkData = (NetworkData*)data;
        SocketData* sd;
        while (NULL != (sd = (SocketData*)networkData->ht.findNext())) {
            SocketClose(sd, networkData->libRefNum, networkData->timeout);
            delete sd;
        }
        while (networkData->openCount--)
            NetLibClose(networkData->libRefNum, false);
        delete networkData;
        return NULL;
    }

    NetworkData* networkData = (NetworkData*)data;
    
    switch (iFunc) {
        case 0: Socket_open(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 1: Socket_close(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 2: Socket_accept(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 3: Socket_bind(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 4: Socket_connect(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 5: Socket_listen(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 6: Socket_send(pci, networkData, networkData->libRefNum, networkData->timeout, false); break;
        //case 7: Socket_send(pci, networkData, networkData->libRefNum, networkData->timeout, true); break;
        case 7: Socket_recv(pci, networkData, false); break;
        //case 9: Socket_recv(pci, networkData, true); break;
        case 8: Socket_shutdown(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 9: Socket_get_localAddr(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 10: Socket_get_remoteAddr(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 11: Network_get_timeout(pci, networkData); break;
        case 12: Network_set_timeout(pci, networkData); break;
        case 13: Network_getHostByName(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 14: Network_getHostByAddr(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        //case 15:Network_get_hostName(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        //case 16:Network_get_domainName(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 15:Network_get_localAddr(pci, networkData, networkData->libRefNum, networkData->timeout); break;
        case 16:Network_open(pci, networkData); break;
        case 17:Network_close(pci, networkData); break;
    }
    return NULL;
}
