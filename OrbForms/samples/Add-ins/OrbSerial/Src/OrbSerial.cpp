#include <PalmOS.h>
#include "OrbNative.h"

extern "C" {
void* __Startup__(OrbFormsInterface*, void*, UInt16);
}

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

// called by compiler
extern "C" 
void *__copy(char *to,char *from,unsigned long size) {
    char *f,*t;

    if(to) for(f=(char *)from,t=(char *)to; size>0; size--) *t++=*f++;
    return to;
}

/*
const int serCradlePort = 0x8000;
const int serIRPort = 0x8001;

const int serStatusCTS = 1;
const int serStatusRTS = 2;
const int serStatusDSR = 4;
const int serStatusBreak = 8;

const int serErrorParity = 0x01;
const int serErrorHWOverrun = 0x02;
const int serErrorFraming = 0x04;
const int serErrorBreak = 0x08;
const int serErrorHShake = 0x10;
const int serErrorSWOverrun = 0x20;
const int serErrorCarrierLost = 0x40;


struct Serial {
    int id;
    void _init() @ library("OrbSerial", 0);
    void _destroy() @ 80;
    void _copy(Serial) @ 89;

    int error @ library("OrbSerial", 6);
    int status @ library("OrbSerial", 7);

    bool open(int port, int baud) @ library("OrbSerial", 1);
    void close() @ library("OrbSerial", 2);
    void settings(int charbits, int stopbits, int parity, char flowctl, int ctstimeout) @ library("OrbSerial", 3);
    bool setbuffer(int size) @ library("OrbSerial", 4);
    void clearerr() @ library("OrbSerial", 5);
    
    bool send(void* data, string type, int count) @ library("OrbSerial", 8);
    bool recv(void* data, string type, int count, int timeout) @ library("OrbSerial", 9);
    bool sendb(char byte) @ library("OrbSerial", 10);
    bool recvb(char* pbyte) @ library("OrbSerial", 11);
    int  sendcheck() @ library("OrbSerial", 12);
    int  recvcheck() @ library("OrbSerial", 13);
    bool sendwait() @ library("OrbSerial", 14);
    bool recvwait(int bytes, int timeout) @ library("OrbSerial", 15);
    void sendflush() @ library("OrbSerial", 16);
    void recvflush(int timeout) @ library("OrbSerial", 17);
};}*/

struct SerialData {
    bool bOpen;
    UInt16 portId;
    void* buffer;
    bool bIr;
    
    // fields for send/recv iteration
    char* bytes;
    UInt16 offset;
    UInt32 endtime;
    bool bError;
};

void Serial_destroy(OrbFormsInterface* ofi, void* data, void* obj) {
    SerialData* sd = (SerialData*)obj;
    // make sure it is already closed
    if (sd->bOpen) {
        if (sd->buffer) {
            // must delete the buffer before close
            SrmSetReceiveBuffer(sd->portId, NULL, 0);
            delete sd->buffer;
            sd->buffer = NULL;
        }
        SrmClose(sd->portId);
    }
    delete sd;
}

void Serial_init(OrbFormsInterface* ofi) {
    SerialData* sd = new SerialData;
    sd->bOpen = false;
    sd->portId = 0;
    sd->buffer = NULL;
    sd->bIr = false;
    ofi->nobj_create(sd, Serial_destroy);
}

void Serial_open(OrbFormsInterface* ofi) {
    UInt32 baud = ofi->vm_pop()->iVal;
    UInt32 port = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;

    // check that the new serial manager is present
    UInt32 value = 0;
    Err err = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &value);
    if (err != errNone || value == 0)
        return; // not supported
        
    if (!sd->bOpen) {
        Err err = SrmOpen(port, baud, &sd->portId);
        if (err == errNone) {
            if (port == serPortIrPort) {
                sd->bIr = true;
                // turn on IR
                SrmControl(sd->portId, srmCtlIrDAEnable, NULL, NULL);
                SrmControl(sd->portId, srmCtlRxEnable, NULL, NULL);
                // flush the 0xff
                SrmReceiveFlush(sd->portId, 1);
            }
            sd->bOpen = true;
            ofi->vm_retVal->iVal = 1;
        }
    }
}

void Serial_close(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();

    if (sd->bOpen) {
        if (sd->buffer) {
            // must delete the buffer before close
            SrmSetReceiveBuffer(sd->portId, NULL, 0);
            delete sd->buffer;
            sd->buffer = NULL;
        }
        SrmClose(sd->portId);
        sd->bOpen = false;
    }
}

void Serial_error_get(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        UInt32 status;
        UInt16 error;
        SrmGetStatus(sd->portId, &status, &error);
        ofi->vm_retVal->iVal = error;
    }
}

void Serial_status_get(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        UInt32 status;
        UInt16 error;
        SrmGetStatus(sd->portId, &status, &error);
        ofi->vm_retVal->iVal = status;
    }
}

void Serial_settings(OrbFormsInterface* ofi) {
    UInt32 ctsTimeout = ofi->vm_pop()->iVal;
    char flowCtl = ofi->vm_pop()->cVal;
    int parity = ofi->vm_pop()->iVal;
    int stopBits = ofi->vm_pop()->iVal;
    int charBits = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    
    UInt32 f = 0;
    // bits per char
    if (charBits == 6) f |= srmSettingsFlagBitsPerChar6;
    else if (charBits == 7) f |= srmSettingsFlagBitsPerChar7;
    else f |= srmSettingsFlagBitsPerChar8;
    // parity
    if (parity == 2) f |= (srmSettingsFlagParityOnM | srmSettingsFlagParityEvenM);
    else if (parity == 1) f |= srmSettingsFlagParityOnM;
    // stop bits
    if (stopBits == 2) f |= srmSettingsFlagStopBits2;
    // handshaking
    if (flowCtl=='x') f |= srmSettingsFlagXonXoffM;
    else if (flowCtl=='r') f |= srmSettingsFlagRTSAutoM;
    else if (flowCtl=='c') f |= srmSettingsFlagCTSAutoM;
    else if (flowCtl=='h') f |= srmSettingsFlagRTSAutoM | srmSettingsFlagCTSAutoM;

    if (sd->bOpen) {
        UInt16 size = sizeof(UInt32);
        SrmControl(sd->portId, srmCtlSetFlags, &f, &size);
        SrmControl(sd->portId, srmCtlSetCtsTimeout, &ctsTimeout, &size);
    }
}

void Serial_setbuffer(OrbFormsInterface* ofi) {
    int size = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        void* buffer = (void*)new char[size];
        if (buffer) {
            SrmSetReceiveBuffer(sd->portId, buffer, size);
            if (sd->buffer) {
                delete sd->buffer;
            }
            sd->buffer = buffer;
            ofi->vm_retVal->iVal = 1;
        }
    }
}

void Serial_clearerr(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    if (sd->bOpen) {
        SrmClearErr(sd->portId);
    }
}

void Serial_sendb(OrbFormsInterface* ofi) {
    char byte = ofi->vm_pop()->cVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        Err err;
        if (sd->bIr)
            SrmControl(sd->portId, srmCtlRxDisable, NULL, NULL);
        SrmSend(sd->portId, &byte, 1, &err);
        if (err == errNone && sd->bIr) {
            err = SrmSendWait(sd->portId);
            SrmControl(sd->portId, srmCtlRxEnable, NULL, NULL);
        }
        if (err == errNone) {
            ofi->vm_retVal->iVal = 1;
        }
    }
}

void Serial_recvb(OrbFormsInterface* ofi) {
    long addr = ofi->vm_pop()->iVal;
    Value* val = ofi->vm_deref(addr);
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;

    if (sd->bOpen) {
        Err err;
        char byte = 0;
        SrmReceive(sd->portId, &byte, 1, 100, &err);
        if (err == errNone) {
            ofi->vm_retVal->iVal = 1;
            val->cVal = byte;
        }
    }
}

// send helper
bool pack_buffer(OrbFormsInterface* ofi, Value* val, VarType type, int len, void* context) {
    SerialData* sd = (SerialData*)context;
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

// to send, we create a temp buffer and pack all the data into it
// then send the whole buffer in one shot
void Serial_send(OrbFormsInterface* ofi) {
    int count = ofi->vm_pop()->iVal;
    Value* vtype = ofi->vm_pop();
    char* strFormat = ofi->val_lock(vtype);
    long addr = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    
    if (sd->bOpen) {
        // calculate the total size for the buffer
        long size = ofi->ts_data_size(addr, strFormat, count);
        if (size) {
            sd->bytes = new char[size];
            if (sd->bytes) {
                sd->offset = 0;
                ofi->ts_iterate(addr, strFormat, count, pack_buffer, sd);
                if (sd->offset == size) {
                    // we packed everything, now send
                    Err err;
                    if (sd->bIr)
                        SrmControl(sd->portId, srmCtlRxDisable, NULL, NULL);
                    SrmSend(sd->portId, sd->bytes, size, &err);
                    if (err == errNone && sd->bIr) {
                        err = SrmSendWait(sd->portId);
                        SrmControl(sd->portId, srmCtlRxEnable, NULL, NULL);
                    }
                    if (err == errNone)
                        ofi->vm_retVal->iVal = 1;
                }
                delete sd->bytes;
            }
        }
    }
    
    ofi->val_unlockRelease(vtype);
}

bool recv_value(OrbFormsInterface* ofi, Value* val, VarType type, int len, void* context) {
    SerialData* sd = (SerialData*)context;
    UInt32 timeout = TimGetTicks();
    if (timeout >= sd->endtime) return false; // timed out
    timeout = sd->endtime - timeout;
    
    Err err;
    int rlen;
    char buffer[512] = {0};
    if (len > 511) ofi->vm_error("Serial.recv() string too long");
    
    switch (type) {
        case vtInt:
        case vtFloat:
        case vtChar:
            val->iVal = 0;
                rlen = SrmReceive(sd->portId, (char*)(&val->iVal)+4-len, len, timeout, &err);
            if (rlen < len) goto Error; // didn't get all data
            if (err != errNone) goto Error; // line or other error
            break;
        case vtString:
            if (len) {
                rlen = SrmReceive(sd->portId, buffer, len, timeout, &err);
                if (rlen < len) goto Error; // didn't get all data
                if (err != errNone) goto Error; // line or other error
                buffer[rlen] = 0;
                ofi->val_release(val); // release the old value
                if (!ofi->val_copyString(val, buffer)) goto Error;
            } else {
                int i = 0;
                // receive one byte at a time until we get a NULL
                while (i < 511) {
                    // simplify by keeping the same timout
                    rlen = SrmReceive(sd->portId, &buffer[i], 1, timeout, &err);
                    if (rlen < 1) goto Error; // didn't get all data
                    if (err != errNone) goto Error; // line or other error
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
    sd->bError = true;
    return false;
}

// to receive, we read one value from the port at a time
void Serial_recv(OrbFormsInterface* ofi) {
    UInt32 timeout = ofi->vm_pop()->iVal;
    int count = ofi->vm_pop()->iVal;
    Value* vtype = ofi->vm_pop();
    char* strFormat = ofi->val_lock(vtype);
    long addr = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    
    if (sd->bOpen) {
        sd->endtime = TimGetTicks() + timeout;
        sd->bError = false;
        ofi->ts_iterate(addr, strFormat, count, recv_value, sd);
        if (!sd->bError)
            ofi->vm_retVal->iVal = 1;
    }
    
    ofi->val_unlockRelease(vtype);
}

void Serial_sendcheck(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        UInt32 size = 0; // 0 returned on error
        Err err = SrmSendCheck(sd->portId, &size);
        ofi->vm_retVal->iVal = size;
    }
}

void Serial_recvcheck(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        UInt32 size = 0; // 0 returned on error
        Err err = SrmReceiveCheck(sd->portId, &size);
        ofi->vm_retVal->iVal = size;
    }
}

void Serial_sendwait(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        Err err = SrmSendWait(sd->portId);
        if (err == errNone)
            ofi->vm_retVal->iVal = 1;
    }
}

void Serial_recvwait(OrbFormsInterface* ofi) {
    UInt32 timeout = ofi->vm_pop()->iVal;
    UInt32 bytes = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    ofi->vm_retVal->type = vtInt;
    ofi->vm_retVal->iVal = 0;
    if (sd->bOpen) {
        Err err = SrmReceiveWait(sd->portId, bytes, timeout);
        if (err == errNone)
            ofi->vm_retVal->iVal = 1;
    }
}

void Serial_sendflush(OrbFormsInterface* ofi) {
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    if (sd->bOpen) {
        SrmSendFlush(sd->portId);
    }
}

void Serial_recvflush(OrbFormsInterface* ofi) {
    UInt32 timeout = ofi->vm_pop()->iVal;
    SerialData* sd = (SerialData*)ofi->nobj_pop();
    if (sd->bOpen) {
        SrmReceiveFlush(sd->portId, timeout);
    }
}


void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
{
    // we don't need to setup any global state
    if (iFunc == NATIVE_STARTUP) return NULL;
    if (iFunc == NATIVE_SHUTDOWN) return NULL;

    switch (iFunc) {
        case 0: Serial_init(ofi); break;
        case 1: Serial_open(ofi); break;
        case 2: Serial_close(ofi); break;
        case 3: Serial_settings(ofi); break;
        case 4: Serial_setbuffer(ofi); break;
        case 5: Serial_clearerr(ofi); break;
        case 6: Serial_error_get(ofi); break;
        case 7: Serial_status_get(ofi); break;
        case 8: Serial_send(ofi); break;
        case 9: Serial_recv(ofi); break;
        case 10: Serial_sendb(ofi); break;
        case 11: Serial_recvb(ofi); break;
        case 12: Serial_sendcheck(ofi); break;
        case 13: Serial_recvcheck(ofi); break;
        case 14: Serial_sendwait(ofi); break;
        case 15: Serial_recvwait(ofi); break;
        case 16: Serial_sendflush(ofi); break;
        case 17: Serial_recvflush(ofi); break;
    }

    return NULL;
}
