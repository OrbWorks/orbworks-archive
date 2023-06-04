#ifndef byte
typedef unsigned char byte;
#endif
#ifndef word
typedef unsigned short word;
#endif
#ifndef dword
typedef unsigned long dword;
#endif

#ifdef WIN32
#include <string>
#include <vector>
#define ovector vector
using namespace std;
#endif

// public data structures

struct PalmRecord {
    PalmRecord() : len(0), data(NULL) { }
    ~PalmRecord() { } // memory deleted by PalmDB
    int len;
    byte* data;
};

struct PalmResRecord {
    PalmResRecord() : id(0), len(0), data(NULL) { }
    ~PalmResRecord() { }
    string type;
    int id;
    int len;
    byte* data;
};

#ifndef WIN32
class PalmDB {
public:
    PalmDB();
    ~PalmDB();

    bool Write(const char* fileName);
    void SetRes(bool isRes);
    void SetLocked(bool isLocked);
    void SetHidden(bool isLocked);
    void SetName(const char*);
    void SetCreatorType(const char* creator, const char* type);
    bool AddResRec(PalmResRecord& rec);
    bool AddRec(PalmRecord& rec);
    DmOpenRef GetDmRef() { return dmRef; }

    static long nativeLong(long);
    static short nativeShort(short);
    static void swapBytes(const byte* src, byte* dest, const char* format, int count = 1);

protected:
    bool Create();
    bool isRes;
    bool isLocked;
    bool isHidden;
    bool wasWritten;
    string name, creator, type;
    DmOpenRef dmRef;
};

#else
struct PalmBitmapFamily {
    char* images[5];
    char* imagesHigh[5];
};

class PalmDB {
public:
    PalmDB();
    ~PalmDB();

    bool Write(const char* fileName);
    void SetRes(bool isRes);
    void SetLocked(bool isLocked);
    void SetHidden(bool isLocked);
    void SetName(const char*);
    void SetCreatorType(const char* creator, const char* type);
    bool AddResRec(PalmResRecord& rec);
    bool AddRec(PalmRecord& rec);
    bool SetRec(int id, PalmRecord& rec);

    bool Read(const char* fileName);
    string GetName();
    bool GetRes();
    bool GetLocked();
    bool GetHidden();
    int GetCount();
    PalmRecord* GetRec(int id);
    PalmResRecord* GetResRec(const char* type, int id);
    PalmResRecord* GetResRec(int index);

    bool CreateBitmapFamily(int id, bool isIcon, const char* fn1,
        const char* fn2, const char* fn3, const char* fn4);
    bool CreateBitmapFamily(int id, bool isIcon, PalmBitmapFamily* pFamily, string& err);
    static long nativeLong(long);
    static short nativeShort(short);
    static void swapBytes(const byte* src, byte* dest, const char* format, int count = 1);

protected:
    void Clear();
    void* LoadBitmap(const char* fn); // Bitmap*
    void* ConvertBitmap(void*, int depth, bool hasNext, int* pSize, bool bHighDensity, bool bCompress); // PalmBitmap* (Bitmap*)
    bool CompressBitmap(byte*, int cbLine, int h, bool v3, byte *&data, int& size);

    ovector<PalmResRecord> rrecs;
    ovector<PalmRecord> recs;
    bool isRes;
    bool isLocked;
    bool isHidden;

    // private data structures
    #pragma pack(push, 2)
    struct DatabaseHdr {
        char name[32];
        short attr, ver;
        long crdate, moddate, bkdate;
        long modnum; // 0
        long app_info, sort_info; // 0

        char type[4], creator[4];
        long u_seed, next; // 0

        short nrecs;
        //short first;
    };

    struct RecordHdr {
        long offset;
        char attr;
        char uid[3];
    };

    struct ResHdr {
        char type[4];
        short id;
        long offset;
    };
    #pragma pack(pop)

    DatabaseHdr dh;
    static void FlipDatabaseHdr(DatabaseHdr&);
    static void FlipRecordHdr(RecordHdr&);
    static void FlipResHdr(ResHdr&);

};

#pragma warning(disable:4244)
#pragma warning(disable:4311)
#endif

class PalmDBStream {
public:
    PalmDBStream(int _size) : data(0), iter(0), ro(false) {
        size = _size;
        data = new byte[size];
        iter = data;
    }
    PalmDBStream(byte* _data) : data(_data), iter(_data), ro(true), size(0) { }	
    ~PalmDBStream() {
        if (!ro)
            delete data;
    }

    void addByte(byte b) {
        //assert(iter < data+size - 1);
        *iter++ = b;
    }
    void addWord(word s) {
        //assert(iter < data+size - 2);
        *iter++ = (s >> 8) & 0xff;
        *iter++ = s & 0xff;
    }
    void addLong(dword l) {
        //assert(iter < data+size - 4);
        *iter++ = (l >> 24) & 0xff;
        *iter++ = (l >> 16) & 0xff;
        *iter++ = (l >> 8) & 0xff;
        *iter++ = l & 0xff;
    }
    void addString(const char* str) {
        //assert(iter < data + strlen(str) + 1);
        strcpy((char*)iter, str);
        iter += strlen(str) + 1;
    }
    void addStringPad(const char* str) {
        strcpy((char*)iter, str);
        iter += strlen(str) + 1;
        if ((dword)iter & 0x1) *iter++ = 0;
    }
    void addData(const byte* d, int len) {
        //assert(iter < data + len);
        memcpy(iter, d, len);
        iter += len;
    }
#ifdef WIN32
    byte getByte() {
        return *iter++;
    }
    short getWord() {
        short s = (*iter++ << 8);
        s |= *iter++;
        return s;
    }
    long getLong() {
        long l = (*iter++ << 24);
        l |= (*iter++ << 16);
        l |= (*iter++ << 8);
        l |= *iter++;
        return l;
    }
#endif
    int getLoc() {
        return iter - data;
    }

    byte* data;
    byte* iter;
    int size;
    bool ro;
};
