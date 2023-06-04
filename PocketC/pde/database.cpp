#include "stdafx.h"
#include "pde.h"

// Database class
Database::Database() {
   nrecs = 0;
   memset(recs, 0, sizeof(char*)*10);
   memset(rechs, 0, sizeof(RecordHdr)*10);
   memset(&hdr, 0, sizeof(hdr));
}

bool Database::Read(char* file) {
   FILE* f = fopen(file, "rb");
   if (!f) return false;

   int size = filelength(f->_file);
   fread(&hdr, sizeof(hdr), 1, f);
   nrecs = nativeShort(hdr.nrecs);
   //int off = nativeShort(hdr.first);
   //if (off) fseek(f, off, SEEK_CUR);

   int i;
   for (i=0;i<nrecs;i++) {
      int nRead = fread(&rechs[i], sizeof(RecordHdr), 1, f);
      ASSERT(nRead);
      offset[i] = nativeLong(rechs[i].off);
   }
   for (i=0;i<nrecs-1;i++)
      len[i] = offset[i+1] - offset[i];
   len[nrecs-1] = size - offset[nrecs-1];

   for (i=0;i<nrecs;i++) {
      recs[i] = new char[len[i]];
      int nRead = fread(recs[i], 1, len[i], f);
      ASSERT(nRead == len[i]);
   }
   fclose(f);
   return true;
}

bool Database::Write(char* file, char* name) {
   FILE* f = fopen(file, "wb");
   if (!f) return false;

   hdr.nrecs = nativeShort(nrecs);
    
    time_t tt;
    time(&tt);

    hdr.crdate = nativeLong(tt);
    //hdr.moddate = 0x8608ee36;
   strcpy(hdr.name, name);

   fwrite(&hdr, sizeof(hdr), 1, f);
   int off = sizeof(DatabaseHdr) + nrecs * sizeof(RecordHdr);
   int i;
   for (i=0;i<nrecs;i++) {
      rechs[i].off = nativeLong(off);
        off+=len[i];
      int nWrite = fwrite(&rechs[i], sizeof(RecordHdr), 1, f);
      //ASSERT(nWrite==sizeof(RecordHdr));
   }
   for (i=0;i<nrecs;i++) {
      int nWrite = fwrite(recs[i], 1, len[i], f);
      //ASSERT(nWrite==sizeof(RecordHdr));
   }
   fclose(f);
   return true;
}

void Database::SetType(long type, long creator) {
   hdr.type = nativeLong(type);
   hdr.creator = nativeLong(creator);
}

int Database::GetNumRecs() {
   return nrecs;
}

//#define max(a,b) ((a)>(b) ? (a) : (b))
void Database::SetRec(int id, void* data, int _len) {
   nrecs = max(nrecs, id+1);
   recs[id] = (char*)data;
   //rechs[id].tag = 0x40;
    //rechs[id].z0 = 0xa9;
    //rechs[id].attr = 0xf0;
    rechs[id].uid[0] = id + 1;
    //rechs[id].uid[1] = id;
    //rechs[id].uid[2] = id;
   len[id] = _len;
}

char* Database::GetRec(int id) {
   return recs[id];
}

short Database::nativeShort(short s) {
   return ((s&0x00ff)<<8) | ((s&0xff00)>>8);
}

long Database::nativeLong(long l) {
    long res;
    char* src = (char*)&l;
    char* dest = (char*)&res;
    dest[0] = src[3];
    dest[1] = src[2];
    dest[2] = src[1];
    dest[3] = src[0];
    return res;
}
