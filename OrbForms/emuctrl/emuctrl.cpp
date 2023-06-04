// emuctrl.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#define EMUCTRL_EXPORT __declspec(dllexport)
#include "emuctrl.h"

WORD crc_calc(WORD crc, char* buf, unsigned int nbytes);

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    return TRUE;
}

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

#pragma pack(push,2)

#define	sysPktRPCCmd					0x0A
#define	sysPktRPCRsp					0x8A

struct RPCParamInfo {
    byte byRef;							// true if param is by reference
    byte	size;								// # of Bytes of paramData	(must be even)			
    word	data[1];						// variable length array of paramData
};
    
struct RPCPacket {
    byte	command;					// Common Body header
    byte	_filler;
    word	trapWord;					// which trap to execute
    dword	resultD0;					// result from D0 placed here
    dword	resultA0;					// result from A0 placed here
    word	numParams;					// how many parameters follow
    // Following is a variable length array ofSlkRPCParamInfo's
    RPCParamInfo	param[1];
};

struct SLPHeader
{
    word	signature1;		// X  first 2 bytes of signature
    byte	signature2;		// X  3 and final byte of signature
    byte	dest;			// -> destination socket Id
    byte	src;			// -> src socket Id
    byte	type;			// -> packet type
    word	bodySize;		// X  size of body
    byte	transID;		// -> transaction Id
                            //    if 0 specified, it will be replaced 
    byte	checksum;		// X  check sum of header
};

#pragma pack(pop)

unsigned int CRC (unsigned char * Data,
                  unsigned int Length);
word swapWord(word val) {
    return (val >> 8) | ((val & 0xff) << 8);
}

dword swapDword(dword val) {
    dword res = val;
    char* c = (char*)&res;
    char t;
    t = c[0]; c[0] = c[3]; c[3] = t;
    t = c[1]; c[1] = c[2]; c[2] = t;
    return res;
}
struct PoserParam {
    char type;
    void* data;
    bool byref;
};

#define CREATORID 'OrbF'
#define DBTYPE    'appl'
#define	dmModeReadOnly				0x0001		// read  access

#define sysTrapDmDatabaseInfo	    0xA046
#define sysTrapSysUIAppSwitch		0xA0A7
#define sysTrapEvtEnqueueKey                0xA12D
#define sysTrapDmFindDatabase               0xA045
#define sysTrapDmOpenDatabaseByTypeCreator  0xA075

#define	launchChr	 0x0108
#define commandKeyMask     0x0008

class PoserRPC {
public:
    PoserRPC();
    ~PoserRPC();

    bool Connect();
    void Disconnect();
    bool DoRPC(word trapWord, int nArgs, PoserParam* pParams, dword* pD0, dword* pA0);
    bool SendPacket(byte src, byte dest, byte type, int size, void* body);
    byte* RecvPacket();
    int ParamSize(char type, void* data);
    void MakeParam(RPCParamInfo* param, char type, void* data, bool byref=false);

    SOCKET sock;
    byte transID;
    bool bWinsockInit;
    bool bSimulator;
};

PoserRPC::PoserRPC() : sock(INVALID_SOCKET), transID(0), bWinsockInit(false),
    bSimulator(false)
{
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD( 2, 2 );

    err = WSAStartup( wVersionRequested, &wsaData );
    if (err == 0)
        bWinsockInit = true;
}

PoserRPC::~PoserRPC() {
    if (bWinsockInit)
        WSACleanup();
}

bool PoserRPC::Connect() {
    bool bRet = false;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock != INVALID_SOCKET) {
        SOCKADDR_IN addr = {0, };
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_family = AF_INET;
        addr.sin_port = htons(2000);

        if (SOCKET_ERROR != connect(sock, (sockaddr*)&addr, sizeof(addr))) {
        //if (false) {
            bRet = true;
            bSimulator = true;
        } else {
            addr.sin_port = htons(6415);
            if (SOCKET_ERROR != connect(sock, (sockaddr*)&addr, sizeof(addr))) {
                bRet = true;
                bSimulator = false;
            } else {
                int err = WSAGetLastError();
                closesocket(sock);
                sock = INVALID_SOCKET;
            }
        }
    } else {
        int err = WSAGetLastError();
    }
    return bRet;
}

void PoserRPC::Disconnect() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

bool PoserRPC::DoRPC(word trapWord, int nArgs, PoserParam* params, dword* pD0, dword* pA0) {
    bool bRet = false;
    int i;

    size_t size = sizeof(RPCPacket) - sizeof(RPCParamInfo);
    for (i=0;i<nArgs;i++)
        size += ParamSize(params[i].type, params[i].data);

    RPCPacket* pPacket = (RPCPacket*)malloc(size);
    if (pPacket) {
        memset(pPacket, 0, size);

        pPacket->command = sysPktRPCCmd;
        pPacket->trapWord = swapWord(trapWord);
        pPacket->numParams = swapWord(nArgs);

        RPCParamInfo* curr = pPacket->param;
        for (i=nArgs-1;i>=0;i--) {
            MakeParam(curr, params[i].type, params[i].data, params[i].byref);
            curr = (RPCParamInfo*)(((byte*)curr) + ParamSize(params[i].type, params[i].data));
        }

        bRet = SendPacket(bSimulator ? 1 : 14, bSimulator ? 1 : 14, 0, size, pPacket);
        RPCPacket* resp = (RPCPacket*)RecvPacket();
        if (resp) {
            bRet = true;
            if (pD0) *pD0 = swapDword(resp->resultD0);
            if (pA0) *pA0 = swapDword(resp->resultA0);

            int nRetBytes = resp->numParams;
            RPCParamInfo * pRetParam = resp->param;
            
            int nRetArg = nArgs-1;
            
            do {
                // todo: ret by ref doesnot support string 
                if (params[nRetArg].byref && params[nRetArg].type!='s')	{
                    memcpy(params[nRetArg].data,pRetParam->data,pRetParam->size);
                    PoserParam * param = &params[nRetArg];
                    switch (params[nRetArg].type) {
                        case 'd': *((dword*)param->data) = swapDword(*((dword*)param->data)); break;
                        case 'w': *((word*)param->data) = swapWord(*((word*)param->data)); break;
                        case 'b': *((word*)param->data) = swapWord(*((word*)param->data)); break;
                    }
                
                } else {
                    while ((nRetArg>=0) && (params[nRetArg].byref==false)) {
                        nRetArg--;
                    }
                }

                if (nRetArg>=0)
                {
                    byte* tmpPtr = (byte*)pRetParam;
                    pRetParam = (RPCParamInfo*) (tmpPtr + pRetParam->size + 2);
                    nRetArg--;
                }
            }
            while ((nRetArg>=0) && (pRetParam!=NULL));
        
            
            delete (byte*)resp;
        }

        free(pPacket);
    }

    return bRet;
}

bool PoserRPC::SendPacket(byte src, byte dest, byte type, int bodySize, void* body) {
    bool bRet = false;
    transID++;
    int size = sizeof(SLPHeader) + bodySize + sizeof(word);
    SLPHeader* header = (SLPHeader*)malloc(size);
    memset(header, 0, size);
    if (header) {
        header->signature1 = swapWord(0xbeef);
        header->signature2 = 0xed;
        header->src = src;
        header->dest = dest;
        header->type = type;
        header->bodySize = swapWord(bodySize);
        header->transID = transID;
        byte* bh = (byte*)header;
        int chk = 0;
        for (int i=0;i<9;i++)
            chk += bh[i];
        header->checksum = chk % 256;
        memcpy(header + 1, body, bodySize);
        byte* bf = ((byte*)header) + sizeof(SLPHeader) + bodySize;
        *((word*)bf) = swapWord(crc_calc(0, (char*)header, sizeof(SLPHeader) + bodySize));

        if (size == send(sock, (char*)header, size, 0)) {
        } else {
            int err = WSAGetLastError();
        }
        free(header);
    }
    return bRet;
}

byte* PoserRPC::RecvPacket() {
    SLPHeader resp;
    bool bRet = false;
    dword dwNonBlock = 1;
    ioctlsocket(sock, FIONBIO, &dwNonBlock);
    dword ticks = GetTickCount();
    dword recvSize = 0;
    do {
        Sleep(50);
        recvSize = recv(sock, (char*)&resp, sizeof(resp), 0);
        if (recvSize == sizeof(resp))  {
            int respSize = swapWord(resp.bodySize);
            byte* buff = new byte[respSize + 2]; // includes footer
            if (buff) {
                if (respSize + 2 == recv(sock, (char*)buff, respSize + 2, 0)) {
                    bRet = true;
                    return buff;
                }
            }
        }
    } while (recvSize == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK && GetTickCount() - ticks < 5000);

    return NULL;
}

int PoserRPC::ParamSize(char type, void* data) {
    int size = sizeof(RPCParamInfo) - 2;
    switch (type) {
        case 'd': size += 4; break;
        case 'w': size += 2; break;
        case 'b': size += 1; break;
        case 's': size += strlen((char*)data) + 1; break;
    }
    size += size % 2;
    return size;
}

void PoserRPC::MakeParam(RPCParamInfo* param, char type, void* data, bool byref) {
    
    param->byRef = byref;
    if (type=='s')
        param->byRef = true;
    switch (type) {

        case 'd': param->size = 4; break;
        case 'w': param->size = 2; break;
        case 'b': param->size = 1; break;
        case 's': param->size = strlen((char*)data) + 1; break;
    }
    memcpy(param->data, data, param->size);
    switch (type) {
        case 'd': *((dword*)param->data) = swapDword(*((dword*)param->data)); break;
        case 'w': *((word*)param->data) = swapWord(*((word*)param->data)); break;
        case 'b': *((word*)param->data) = swapWord(*((word*)param->data)); break;
        case 's': param->size += param->size % 2; break;
    }
}

bool EMUCTRL_EXPORT IsEmulatorRunning() {
    PoserRPC poser;
    if (poser.Connect()) {
        poser.Disconnect();
        return true;
    }
    return false;
}

bool EMUCTRL_EXPORT InstallFile(char* fileName) {
    PoserRPC poser;
    if (poser.Connect()) {
        word selector = 0x0500;
        dword card = 0;
        PoserParam params[3] = { {'w', &selector,0}, {'s', fileName,1}, {'d', &card,0}};
        dword res = 0;
        bool bRet = poser.DoRPC(0xA344, 3, params, &res, NULL);
        if (bRet) {
            // returns 0 on success
            if (res != 0) bRet = false;
        }
        poser.Disconnect();
        return bRet;
    }
    return false;
}

bool EMUCTRL_EXPORT HasDatabase(char* fileName) {
    PoserRPC poser;
    if (poser.Connect()) {
        word card = 0;
        PoserParam params[2] = { {'w', &card, 0}, {'s', fileName, 1} };
        dword res = 0;
        bool bRet = poser.DoRPC(0xA045, 2, params, &res, NULL);
        if (bRet) {
            // returns a nonzero LocalID if found
            if (res == 0) bRet = false;
        }
        poser.Disconnect();
        return bRet;
    }
    return false;
}


short EMUCTRL_EXPORT OrbFormsRt_ver() {
    
    char fileName[]="OrbFormsRT";
    short wVer = 0;

    PoserRPC poser;
    if (poser.Connect()) {
        word card = 0;
        PoserParam params[2] = { {'w', &card, 0}, {'s', fileName, 1} };
        dword localid = 0;

        // returns a nonzero LocalID if found
        bool bRet = poser.DoRPC(sysTrapDmFindDatabase, 2, params, &localid, NULL);
        if (bRet && localid) {
            word dbattr = 0;
            word dbversion = 0;
            dword dbdate = 0;
            dword dbmoddate = 0;
            dword dbbckupdate = 0;
            dword dbmodnum = 0;
            dword dbappInfoIDP = 0;
            dword dbsortInfoIDP = 0;
            dword dbtype = 0;
            dword dbcreator = 0;

            PoserParam params[13] = { 
                {'w', &card, 0}, 
                {'d', &localid, 0}, 
                {'s', fileName, 1},
                {'w', &dbattr, 1},
                {'w', &dbversion,1},
                {'d', &dbdate,1},
                {'d', &dbmoddate,1},
                {'d', &dbbckupdate,1},
                {'d', &dbmodnum,1},
                {'d', &dbappInfoIDP,1},
                {'d', &dbsortInfoIDP,1},
                {'d', &dbtype,1},
                {'d', &dbcreator,1}
            };

            dword res = 0;
            bRet = poser.DoRPC(sysTrapDmDatabaseInfo,13, params,&res,NULL);
            if (bRet)
            {
                if (dbcreator==CREATORID)
                {
                    wVer = dbversion;
                }
                wVer = dbversion;
            }
        }

        poser.Disconnect();
    }

    return wVer;
}

void EMUCTRL_EXPORT LaunchApp(char* fileName) {
    
    PoserRPC poser;
    if (poser.Connect()) {
        word card = 0;
        dword res = 0;
        
        PoserParam params[2] = { {'w', &card, 0}, {'s', fileName, 1} };
        bool bRet = poser.DoRPC(0xA045, 2, params, &res, NULL);
        // returns a nonzero LocalID if found
        if (bRet && res) {
            dword app_ret = 0;
            word none = 0; dword dnone = 0;
            PoserParam params[4] = { 
                {'w', &card, 0}, {'d', &res, 0}, {'w', &none, 0},
                {'d', &dnone, 0}}; //, {'w', &none, 0}, {'d',&res, 0}};
            bRet = poser.DoRPC(sysTrapSysUIAppSwitch, 4, params, &app_ret, NULL);
        }
        poser.Disconnect();
    }
}

void EMUCTRL_EXPORT EmulatorReset() {
    PoserRPC poser;
    if (poser.Connect()) {
        bool bRet = poser.DoRPC(0xA08C, 0, NULL, NULL, NULL);
        poser.Disconnect();
    }
}


void EMUCTRL_EXPORT EmulatorStartAppLauncher() {
    PoserRPC poser;
    
    if (poser.Connect()) {
        word ascii = launchChr; 
        word keycode = launchChr;
        word modifiers = commandKeyMask;
        PoserParam params[3] = { {'w', &ascii, 0}, {'w', &keycode, 0}, {'w',&modifiers, 0} };
        bool bRet = poser.DoRPC(sysTrapEvtEnqueueKey, 3, params, NULL, NULL);
        poser.Disconnect();
    }
}


static WORD crc_table[256] = {
    0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
    0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
    0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
    0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
    0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
    0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
    0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
    0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
    0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
    0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
    0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
    0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
    0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
    0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
    0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
    0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
    0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
    0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
    0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
    0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
    0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
    0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
    0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
    0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
    0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
    0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
    0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
    0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
    0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
    0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
    0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
    0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0,
};


/* crc_calc() -- calculate cumulative crc-16 for buffer */

WORD crc_calc(WORD crc, char *buf, unsigned nbytes)
{
      unsigned char *p, *lim;

      p = (unsigned char *)buf;
      lim = p + nbytes;
      while (p < lim)
      {
          WORD a = (crc << 8) & 0xffff;
          WORD b = crc >> 8;
          crc = a ^ crc_table[(b ^ *p++) & 0xff];
      }
      return crc;
}
