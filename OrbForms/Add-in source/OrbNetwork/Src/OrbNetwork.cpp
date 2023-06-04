#include <PalmOS.h>
#include <Extensions\ExpansionMgr\VFSMgr.h>
#include "OrbNative.h"

typedef unsigned char byte;

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

//This will be a pain to use and implement
//object EndPoint {
//	int family;
//	int port;
//	int flowinfo;
//	int address[4];
//	int scopeid;
//	
//	string toString();
//	bool fromString(string addr);
//	void anyAddr();
//};


object Socket {
    int id;
    void _init() @ library("OrbNetwork", 0);
    void _destroy() @ 80;
    void _copy(Socket) @ 89;
    
    int open(int family, int type, int protocol) @ library("OrbNetwork", 1);
    int close() @ library("OrbNetwork", 2);
    int accept(Socket newSocket) @ library("OrbNetwork", 3);
    int bind(string localAddr) @ library("OrbNetwork", 4);
    int connect(string addr) @ library("OrbNetwork", 5);
    int listen(int backlog) @ library("OrbNetwork", 6);
    int sends(string data);
    int send(void* pdata, string format, int count) @ library("OrbNetwork", 7);
    //int sendto(string addr, void* pdata, string format, int count) @ library("OrbNetwork", 8);
    int recvs(string* pdata);
    int recv(void* pdata, string format, int count) @ library("OrbNetwork", 9);
    //int recvfrom(string* paddr, void* pdata, string format, int count) @ library("OrbNetwork", 10);
    int shutdown(int dir) @ library("OrbNetwork", 11);
    
    string localAddr @ library("OrbNetwork", 12);
    string remoteAddr @ library("OrbNetwork", 13);
};

int Socket.sends(string data) {
    return send(&data, "s", 1);
}

int Socket.recvs(string* pdata) {
    return recv(pdata, "s",  1);
}

object Network {
    int open() @ library("OrbNetwork", 21);
    void close() @ library("OrbNetwork", 22);
    int getHostByName(string name, string* paddr) @ library("OrbNetwork", 16);
    int getHostByAddr(string addr, string* pname) @ library("OrbNetwork", 17);
    
    //string hostName @ library("OrbNetwork", 18);
    //string domainName @ library("OrbNetwork", 19);
    string localAddr @ library("OrbNetwork", 20);
    int timeout @ library("OrbNetwork", 14:15); // timeout for all methods
};
*/

/*
void VMError(OrbFormsInterface* ofi, char* str, UInt32 err) {
    char buff[256];
    StrPrintF(buff, "%s: %ld", str, err);
    ofi->vm_error(buff);
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

struct NetworkData {
    UInt16 libRefNum;
    UInt32 timeout;
    int openCount;
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

Err SocketClose(SocketData* sd, UInt16 libRef, UInt32 timeout, OrbFormsInterface* ofi) {
    if (sd->socketRef) {
        Err err = 0;
        NetLibSocketClose(libRef, sd->socketRef, -1, &err);
        sd->socketRef = 0;
        return err;
    }
    return 0;
}

void Socket_destroy(OrbFormsInterface* ofi, void* data, void* obj) {
    SocketData* sd = (SocketData*)obj;
    SocketClose(sd, ((NetworkData*)data)->libRefNum, ((NetworkData*)data)->timeout, ofi);
    delete sd;
}

void Socket_init(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    SocketData* sd = new SocketData;
    sd->socketRef = 0;
    ofi->nobj_create(sd, Socket_destroy);
}

void Socket_open(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    long protocol = ofi->vm_pop()->iVal;
    long type = ofi->vm_pop()->iVal;
    long family = ofi->vm_pop()->iVal;
    SocketData* sd = (SocketData*)ofi->nobj_pop();

    Err err = 0;
    sd->socketRef = NetLibSocketOpen(libRef, (NetSocketAddrEnum)family, (NetSocketTypeEnum)type, protocol, timeout, &err);
    if (err != 0 || sd->socketRef == -1) {
        sd->socketRef = 0;
    } else {
        NetSocketLingerType linger;
        linger.onOff = true;
        linger.time = 0;
        NetLibSocketOptionSet(libRef, sd->socketRef,
            netSocketOptLevelSocket, netSocketOptSockLinger,
            &linger, sizeof(linger), -1, &err);
    }

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Socket_close(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    SocketData* sd = (SocketData*)ofi->nobj_pop();

    Err err = SocketClose(sd, libRef, timeout, ofi);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Socket_accept(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    SocketData* sdNew = (SocketData*)ofi->nobj_pop();
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    Err err = 0;
    NetSocketAddrType addr;
    Int16 addrSize = sizeof(addr);

    SocketClose(sdNew, libRef, timeout, ofi);
    sdNew->socketRef = NetLibSocketAccept(libRef, sd->socketRef, &addr, &addrSize, timeout, &err);
    if (err != 0 || sdNew->socketRef == -1) {
        sdNew->socketRef = 0;
    } else {
        NetSocketLingerType linger;
        linger.onOff = true;
        linger.time = 0;
        NetLibSocketOptionSet(libRef, sdNew->socketRef,
            netSocketOptLevelSocket, netSocketOptSockLinger,
            &linger, sizeof(linger), -1, &err);
    }

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Socket_bind(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    Value* vAddr = ofi->vm_pop();
    char* sAddr = ofi->val_lock(vAddr);
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    Err err = 0;
    NetSocketAddrType addr;
    Int16 addrSize = sizeof(addr);
    
    if (StringToAddr(sAddr, (NetSocketAddrINType*)&addr)) {
        NetLibSocketBind(libRef, sd->socketRef, &addr, addrSize, timeout, &err);
    } else {
        err = 1; // netErrInvalidAddrString
    }
    ofi->val_unlockRelease(vAddr);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Socket_connect(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    Value* vAddr = ofi->vm_pop();
    char* sAddr = ofi->val_lock(vAddr);
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    Err err = 0;
    NetSocketAddrType addr;
    Int16 addrSize = sizeof(addr);
    
    if (StringToAddr(sAddr, (NetSocketAddrINType*)&addr)) {
        //VMError(ofi, "addr", ((NetSocketAddrINType*)&addr)->port);
        NetLibSocketConnect(libRef, sd->socketRef, &addr, addrSize, timeout, &err);
    } else {
        err = 1; // netErrInvalidAddrString
    }
    ofi->val_unlockRelease(vAddr);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Socket_listen(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    int backlog = ofi->vm_pop()->iVal;
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    Err err = 0;
    
    NetLibSocketListen(libRef, sd->socketRef, backlog, timeout, &err);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

// send helper
bool pack_buffer(OrbFormsInterface* ofi, Value* val, VarType type, int len, void* context) {
    SocketData* sd = (SocketData*)context;
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

bool recv_value(OrbFormsInterface* ofi, Value* val, VarType type, int len, void* context) {
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
        case vtChar:
            val->iVal = 0;
            rlen = NetLibReceive(sd->nd->libRefNum, sd->socketRef, (char*)(&val->iVal)+4-len, len, 0, sd->inAddr, &addrSize, sd->nd->timeout, &err);
            if (rlen < len) goto DataError; // didn't get all data
            if (err != errNone) goto Error; // line or other error
            break;
        case vtString:
            if (len) {
                rlen = NetLibReceive(sd->nd->libRefNum, sd->socketRef, sb.c_str(), len, 0, sd->inAddr, &addrSize, sd->nd->timeout, &err);
                if (rlen < len) dataError = true; // didn't get all data
                if (err != errNone) goto Error; // line or other error
                ofi->val_release(val); // release the old value
                if (!ofi->val_copyString(val, sb.c_str())) goto Error;
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
                ofi->val_release(val); // release the old value
                if (!ofi->val_copyString(val, sb.c_str())) goto Error;
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
void Socket_send(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout, bool sendTo) {
    int count = ofi->vm_pop()->iVal;
    Value* vtype = ofi->vm_pop();
    char* strFormat = ofi->val_lock(vtype);
    long addrData = ofi->vm_pop()->iVal;
    Value* stAddr = NULL;
    if (sendTo) {
        stAddr = ofi->vm_pop();
    }
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    
    NetSocketAddrINType inAddr = {0}; 
    if (!sendTo || StringToAddr(ofi->val_lock(stAddr), &inAddr)) {
        ofi->vm_retVal->type = vtInt;
        ofi->vm_retVal->iVal = netErrSocketNotOpen;
        
        if (sd->socketRef) {
            ofi->vm_retVal->iVal = 2; // netErrOther
            // calculate the total size for the buffer
            long size = ofi->ts_data_size(addrData, strFormat, count);
            if (size) {
                sd->bytes = new char[size];
                if (sd->bytes) {
                    sd->offset = 0;
                    ofi->ts_iterate(addrData, strFormat, count, pack_buffer, sd);
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
                        ofi->vm_retVal->iVal = err;
                    }
                    delete sd->bytes;
                }
            }
        }
    } else {
        ofi->vm_retVal->iVal = 1;
    }
    
    ofi->val_unlockRelease(vtype);
    if (sendTo) ofi->val_unlockRelease(stAddr);
}

void Socket_recv(OrbFormsInterface* ofi, NetworkData* networkData, bool recvFrom) {
    int count = ofi->vm_pop()->iVal;
    Value* vtype = ofi->vm_pop();
    char* strFormat = ofi->val_lock(vtype);
    long addrData = ofi->vm_pop()->iVal;
    Value* rfAddr = NULL;
    if (recvFrom) {
        rfAddr = ofi->vm_deref(ofi->vm_pop()->iVal);
    }
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    
    NetSocketAddrType inAddr = {0};
    if (recvFrom) sd->inAddr = (NetSocketAddrINType*)&inAddr;
    else sd->inAddr = NULL;
    //sd->inAddr = NULL;
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = netErrSocketNotOpen;
    
    if (sd->socketRef) {
        sd->err = 0;
        sd->nd = networkData;
        ofi->ts_iterate(addrData, strFormat, count, recv_value, sd);
        ofi->vm_retVal->iVal = sd->err;
        if (recvFrom) {
            ofi->val_release(rfAddr);
            char* strRfAddr = ofi->val_newString(rfAddr, 21);
            AddrToString((NetSocketAddrINType*)&inAddr, strRfAddr);
            ofi->val_unlock(rfAddr);
        }
    }
    
    ofi->val_unlockRelease(vtype);
}

void Socket_shutdown(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    int dir = ofi->vm_pop()->iVal;
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    Err err = 0;
    
    NetLibSocketShutdown(libRef, sd->socketRef, dir, timeout, &err);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Socket_get_localAddr(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    Err err = 0;
    
    NetSocketAddrINType localAddr;
    NetSocketAddrINType remoteAddr;
    Int16 localAddrSize = sizeof(localAddr);
    Int16 remoteAddrSize = sizeof(remoteAddr);
    
    NetLibSocketAddr(libRef, sd->socketRef, (NetSocketAddrType*)&localAddr, &localAddrSize,
        (NetSocketAddrType*)&remoteAddr, &remoteAddrSize, timeout, &err);
        
    if (err == errNone) {
        char* str = ofi->val_newString(ofi->vm_retVal, 21);
        if (str) {
            AddrToString(&localAddr, str);
            ofi->val_unlock(ofi->vm_retVal);
        } else {
            ofi->val_newConstString(ofi->vm_retVal, "");
        }
    } else {
        //VMError(ofi, "NetLibSocketAddr", err);
        ofi->val_newConstString(ofi->vm_retVal, "");
    }
}

void Socket_get_remoteAddr(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    SocketData* sd = (SocketData*)ofi->nobj_pop();
    Err err = 0;
    
    NetSocketAddrINType localAddr;
    NetSocketAddrINType remoteAddr;
    Int16 localAddrSize = sizeof(localAddr);
    Int16 remoteAddrSize = sizeof(remoteAddr);
    
    NetLibSocketAddr(libRef, sd->socketRef, (NetSocketAddrType*)&localAddr, &localAddrSize,
        (NetSocketAddrType*)&remoteAddr, &remoteAddrSize, timeout, &err);
        
    if (err == errNone) {
        char* str = ofi->val_newString(ofi->vm_retVal, 21);
        if (str) {
            AddrToString(&remoteAddr, str);
            ofi->val_unlock(ofi->vm_retVal);
        } else {
            ofi->val_newConstString(ofi->vm_retVal, "");
        }
    } else {
        //VMError(ofi, "NetLibSocketAddr", err);
        ofi->val_newConstString(ofi->vm_retVal, "");
    }
}

void Network_get_timeout(OrbFormsInterface* ofi, NetworkData* networkData) {
    ofi->vm_pop(); // ignore Network address
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = networkData->timeout;
}

void Network_set_timeout(OrbFormsInterface* ofi, NetworkData* networkData) {
    UInt32 timeout = ofi->vm_pop()->iVal;
    ofi->vm_pop(); // ignore Network address
    networkData->timeout = timeout;
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = timeout;
}

void Network_getHostByName(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    long addrResult = ofi->vm_pop()->iVal;
    Value* result = ofi->vm_deref(addrResult);
    Value* vName = ofi->vm_pop();
    char* name = ofi->val_lock(vName);
    ofi->vm_pop(); // ignore address of Network
    Err err = 0;
    
    NetHostInfoBufType buff = {0};
    NetLibGetHostByName(libRef, name, &buff, -1, &err);
    ofi->val_unlockRelease(vName);
    
    if (err == errNone) {
        if (buff.addressList[0]) {
            ofi->val_release(result);
            char* resStr = ofi->val_newString(result, 21);
            if (!resStr) ofi->vm_error("out of memory");
            NetSocketAddrINType addr = {0};
            addr.family = netSocketAddrINET;
            addr.addr = *buff.addressList[0];
            AddrToString(&addr, resStr);
            ofi->val_unlock(result);
        } else {
            err = netErrTimeout;
        }
    }

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Network_getHostByAddr(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    long addrResult = ofi->vm_pop()->iVal;
    Value* result = ofi->vm_deref(addrResult);
    Value* vName = ofi->vm_pop();
    char* name = ofi->val_lock(vName);
    ofi->vm_pop(); // ignore address of Network
    Err err = 0;
    
    NetSocketAddrINType addr = {0};
    if (StringToAddr(name, &addr)) {
        NetHostInfoBufType buff = {0};
        NetLibGetHostByAddr(libRef, (UInt8*)&addr.addr, sizeof(addr.addr), addr.family, &buff, -1, &err);
        
        if (err == errNone) {
            ofi->val_release(result);
            ofi->val_copyString(result, buff.name);
        }
    } else {
        err = 1;
    }
    ofi->val_unlockRelease(vName);

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

// these two methods return "setting not found" on my Tungsten|C, so
// it is unlikely that many devices will be configured with anything
void Network_get_hostName(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    ofi->vm_pop(); // ignore address of Network
    Err err = 0;

    char name[64];
    UInt16 size = sizeof(name);
    
    ofi->val_newConstString(ofi->vm_retVal, "");

    err = NetLibSettingGet(libRef, netSettingHostName, name, &size);
    if (err == errNone) {
        ofi->val_copyString(ofi->vm_retVal, name);
    } else {
        //VMError(ofi, "NetLibSettingGet", err);
    }

}

void Network_get_domainName(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    ofi->vm_pop(); // ignore address of Network
    Err err = 0;

    char name[256];
    UInt16 size = sizeof(name);
    
    ofi->val_newConstString(ofi->vm_retVal, "");

    err = NetLibSettingGet(libRef, netSettingDomainName, name, &size);
    if (err == errNone) {
        ofi->val_copyString(ofi->vm_retVal, name);
    } else {
        //VMError(ofi, "NetLibSettingGet", err);
    }
}


void Network_get_localAddr(OrbFormsInterface* ofi, UInt16 libRef, UInt32 timeout) {
    ofi->vm_pop(); // ignore address of Network
    Err err = 0;
    
    UInt32 creator;
    UInt16 instance;
    UInt16 index = 0;
    
    ofi->val_newConstString(ofi->vm_retVal, "");
    
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
            char* str = ofi->val_newString(ofi->vm_retVal, 21);
            if (str) {
                NetSocketAddrINType sockAddr = {0};
                sockAddr.family = netSocketAddrINET;
                sockAddr.addr = addr;
                AddrToString(&sockAddr, str);
                ofi->val_unlock(ofi->vm_retVal);
            }
            break;
        } else {
            index++;
        }
    }
}

void Network_open(OrbFormsInterface* ofi, NetworkData* networkData) {
    ofi->vm_pop(); // ignore address of Network
    Err err = 0;
    UInt16 ifErr;
    
    err = NetLibOpen(networkData->libRefNum, &ifErr);
    if (err == netErrAlreadyOpen) err = errNone;
    
    if (err == errNone) {
        networkData->openCount++;
    }

    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = err;
}

void Network_close(OrbFormsInterface* ofi, NetworkData* networkData, bool immediate) {
    ofi->vm_pop(); // ignore address of Network
    if (networkData->openCount > 0) {
        NetLibClose(networkData->libRefNum, immediate);
        networkData->openCount--;
    }
}

void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
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
        while (networkData->openCount--)
            NetLibClose(networkData->libRefNum, false);
        delete networkData;
        return NULL;
    }

    NetworkData* networkData = (NetworkData*)data;
    
    switch (iFunc) {
        case 0: Socket_init(ofi, networkData->libRefNum, networkData->timeout); break;
        case 1: Socket_open(ofi, networkData->libRefNum, networkData->timeout); break;
        case 2: Socket_close(ofi, networkData->libRefNum, networkData->timeout); break;
        case 3: Socket_accept(ofi, networkData->libRefNum, networkData->timeout); break;
        case 4: Socket_bind(ofi, networkData->libRefNum, networkData->timeout); break;
        case 5: Socket_connect(ofi, networkData->libRefNum, networkData->timeout); break;
        case 6: Socket_listen(ofi, networkData->libRefNum, networkData->timeout); break;
        case 7: Socket_send(ofi, networkData->libRefNum, networkData->timeout, false); break;
        case 8: Socket_send(ofi, networkData->libRefNum, networkData->timeout, true); break;
        case 9: Socket_recv(ofi, networkData, false); break;
        case 10: Socket_recv(ofi, networkData, true); break;
        case 11: Socket_shutdown(ofi, networkData->libRefNum, networkData->timeout); break;
        case 12: Socket_get_localAddr(ofi, networkData->libRefNum, networkData->timeout); break;
        case 13: Socket_get_remoteAddr(ofi, networkData->libRefNum, networkData->timeout); break;
        case 14: Network_get_timeout(ofi, networkData); break;
        case 15: Network_set_timeout(ofi, networkData); break;
        case 16: Network_getHostByName(ofi, networkData->libRefNum, networkData->timeout); break;
        case 17: Network_getHostByAddr(ofi, networkData->libRefNum, networkData->timeout); break;
        case 18:Network_get_hostName(ofi, networkData->libRefNum, networkData->timeout); break;
        case 19:Network_get_domainName(ofi, networkData->libRefNum, networkData->timeout); break;
        case 20:Network_get_localAddr(ofi, networkData->libRefNum, networkData->timeout); break;
        case 21:Network_open(ofi, networkData); break;
        case 22:Network_close(ofi, networkData, false); break;
        case 23:Network_close(ofi, networkData, true); break;
    }
    return NULL;
}
