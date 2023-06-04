// Database Header

#pragma pack(push, 2)
struct DatabaseHdr {
   char name[32];
   short attr, ver;
   //char z0[24];
    long crdate, moddate, bkdate, modnum;
    long z0[2];

   long type, creator;
   long z1[2];

   short nrecs;
// short first;
};
#pragma pack(pop)

struct RecordHdr {
    long off;
    char attr;
   char uid[3];
};

class Database {
public:
   Database();
   bool Read(char* file);
   bool Write(char* file, char* name);
   void SetType(long type, long creator);
   int GetNumRecs();
   void SetRec(int id, void* data, int len);
   char* GetRec(int id);

   static short nativeShort(short);
   static long  nativeLong (long);

   DatabaseHdr hdr;
   char* recs[10];
   int len[10];
   int offset[10];
   RecordHdr rechs[10];
   int nrecs;
};
