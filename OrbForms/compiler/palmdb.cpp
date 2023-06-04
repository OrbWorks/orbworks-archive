// PalmDB read/write module
// (c) 1998-2006 OrbWorks

#ifdef WIN32
#include <stdio.h>
#include <io.h>
#include <assert.h>
#include <time.h>

#include <windows.h>
#pragma warning(disable:4018) // signed/unsigned mismatch
#else
#include "stdcover.h"
#include "string.h"
#include "vector.h"
#endif
#include "palmdb.h"

#ifdef WIN32
PalmDB::PalmDB() : isRes(true), isLocked(false), isHidden(false) {
    memset(&dh, 0, sizeof(dh));
}

PalmDB::~PalmDB() { Clear(); }

bool PalmDB::Read(const char* fileName) {
    Clear();

    DatabaseHdr hdr;
    int size=0;
    memset(&hdr, 0, sizeof(hdr));

    FILE* f = fopen(fileName, "rb");
    if (!f) return false;

    size = _filelength(f->_file);
    fread(&hdr, sizeof(hdr), 1, f);
    FlipDatabaseHdr(hdr);

    isRes = (hdr.attr & 0x1);
    isLocked = (hdr.attr & 0x40) != 0;
    isHidden = (hdr.attr & 0x100) != 0;

    int i;
    if (!isRes) {
        RecordHdr* ph = new RecordHdr[hdr.nrecs];
        for (i=0;i<hdr.nrecs;i++) {
            size_t nRead = fread(&ph[i], sizeof(RecordHdr), 1, f);
            assert(nRead);
            FlipRecordHdr(ph[i]);
        }
        fseek(f, ph[0].offset, SEEK_SET);
        for (i=0;i<hdr.nrecs;i++) {
            PalmRecord rec;
            if (i == hdr.nrecs-1)
                rec.len = size - ph[i].offset;
            else
                rec.len = ph[i+1].offset - ph[i].offset;
            rec.data = new byte[rec.len];
            fread(rec.data, 1, rec.len, f);
            recs.push_back(rec);
        }
        delete[] ph;
    } else {
        ResHdr* ph = new ResHdr[hdr.nrecs];
        for (i=0;i<hdr.nrecs;i++) {
            size_t nRead = fread(&ph[i], sizeof(ResHdr), 1, f);
            assert(nRead);
            FlipResHdr(ph[i]);
        }
        fseek(f, ph[0].offset, SEEK_SET);
        for (i=0;i<hdr.nrecs;i++) {
            PalmResRecord rec;
            if (i == hdr.nrecs-1)
                rec.len = size - ph[i].offset;
            else
                rec.len = ph[i+1].offset - ph[i].offset;
            rec.data = new byte[rec.len];
            fread(rec.data, 1, rec.len, f);
            rec.id = ph[i].id;
            char type[5] = {0};
            strncpy(type, ph[i].type, 4);
            rec.type = type;
            rrecs.push_back(rec);
        }
        delete[] ph;
    }
    
    fclose(f);
    dh = hdr;
    return true;
}

bool PalmDB::Write(const char* fileName) {
    DatabaseHdr hdr = dh;

    FILE* f = fopen(fileName, "wb");
    if (!f) return false;

    time_t tt;
    time(&tt);
    hdr.crdate = tt + 2082844800;
    hdr.moddate = hdr.crdate;
    if (isRes)
        hdr.attr = 0x1;
    if (isLocked)
        hdr.attr |= 0x40;
    if (isHidden)
        hdr.attr |= 0x100;

    FlipDatabaseHdr(hdr);
    fwrite(&hdr, sizeof(hdr), 1, f);

    int off, i;
    if (isRes) {
        off = sizeof(DatabaseHdr) + dh.nrecs * sizeof(ResHdr);
        for (i=0;i<dh.nrecs;i++) {
            ResHdr rh;
            rh.id = rrecs[i].id;
            memset(rh.type, 0, 4);
            strncpy(rh.type, rrecs[i].type.c_str(), 4);
            rh.offset = off;
            FlipResHdr(rh);
            off += rrecs[i].len;
            fwrite(&rh, sizeof(rh), 1, f);
        }
        for (i=0;i<dh.nrecs;i++) {
            fwrite(rrecs[i].data, 1, rrecs[i].len, f);
        }

    } else {
        off = sizeof(DatabaseHdr) + dh.nrecs * sizeof(RecordHdr);
        for (i=0;i<dh.nrecs;i++) {
            RecordHdr rh;
            rh.offset = off;
            rh.attr = 0;
            rh.uid[0] = rh.uid[1] = rh.uid[2] = 0;
            FlipRecordHdr(rh);
            off += recs[i].len;
            fwrite(&rh, sizeof(rh), 1, f);
        }
        for (i=0;i<dh.nrecs;i++) {
            fwrite(recs[i].data, 1, recs[i].len, f);
        }

    }
    fclose(f);
    return true;
}

bool PalmDB::AddRec(PalmRecord& rec) {
    if (!isRes) {
        byte* old_data = rec.data, *data = new byte[rec.len];
        memcpy(data, rec.data, rec.len);
        rec.data = data;
        recs.push_back(rec);
        rec.data = old_data;
        dh.nrecs = (short)recs.size();
        return true;
    }
    return false;
}

bool PalmDB::SetRec(int id, PalmRecord& rec) {
    if (!isRes && recs.size() > id) {
        delete [] recs[id].data;
        recs[id].data = new byte[rec.len];
        recs[id].len = rec.len;
        memcpy(recs[id].data, rec.data, rec.len);
        return true;
    }
    return false;
}

bool PalmDB::AddResRec(PalmResRecord& rec) {
    if (isRes) {
        byte* old_data = rec.data, *data = new byte[rec.len];
        memcpy(data, rec.data, rec.len);
        rec.data = data;
        rrecs.push_back(rec);
        rec.data = old_data;
        dh.nrecs = (short)rrecs.size();
        return true;
    }
    return false;
}

PalmRecord* PalmDB::GetRec(int id) {
    if (isRes)
        return NULL;

    if (id < recs.size())
        return &recs[id];
    return NULL;
}

PalmResRecord* PalmDB::GetResRec(int id) {
    if (!isRes)
        return NULL;

    if (id < rrecs.size())
        return &rrecs[id];
    return NULL;
}

PalmResRecord* PalmDB::GetResRec(const char* type, int id) {
    if (!isRes)
        return NULL;

    for (int i=0;i<rrecs.size();i++) {
        if (rrecs[i].id == id && strncmp(rrecs[i].type.c_str(), type, 4)==0)
            return &rrecs[i];
    }
    return NULL;
}

string PalmDB::GetName() {
    return dh.name;
}

void PalmDB::SetName(const char* name) {
    memset(dh.name, 0, 32);
    strncpy(dh.name, name, 31);
}

void PalmDB::SetCreatorType(const char* creator, const char* type) {
    memset(dh.creator, 0, 4);
    memset(dh.type, 0, 4);
    strncpy(dh.creator, creator, 4);
    strncpy(dh.type, type, 4);
}

void PalmDB::SetRes(bool _isRes) {
    isRes = _isRes;
}

void PalmDB::SetLocked(bool _isLocked) {
    isLocked = _isLocked;
}

void PalmDB::SetHidden(bool _isHidden) {
    isHidden = _isHidden;
}

bool PalmDB::GetRes() {
    return isRes;
}

bool PalmDB::GetLocked() {
    return isLocked;
}

bool PalmDB::GetHidden() {
    return isHidden;
}

int PalmDB::GetCount() {
    if (isRes)
        return (int)rrecs.size();
    else
        return (int)recs.size();
}

short PalmDB::nativeShort(short s) {
   return ((s&0x00ff)<<8) | ((s&0xff00)>>8);
}

long PalmDB::nativeLong(long l) {
    long res;
    char* src = (char*)&l;
    char* dest = (char*)&res;
    dest[0] = src[3];
    dest[1] = src[2];
    dest[2] = src[1];
    dest[3] = src[0];
    return res;
}

void PalmDB::FlipDatabaseHdr(DatabaseHdr& hdr) {
    hdr.attr = nativeShort(hdr.attr);
    hdr.ver = nativeShort(hdr.ver);
    hdr.crdate = nativeLong(hdr.crdate);
    hdr.moddate = nativeLong(hdr.moddate);
    hdr.bkdate = nativeLong(hdr.bkdate);
    hdr.modnum = nativeLong(hdr.modnum);
    hdr.app_info = nativeLong(hdr.app_info);
    hdr.sort_info = nativeLong(hdr.sort_info);
    hdr.u_seed = nativeLong(hdr.u_seed);
    hdr.next = nativeLong(hdr.next);
    hdr.nrecs = nativeShort(hdr.nrecs);
}

void PalmDB::FlipRecordHdr(RecordHdr& hdr) {
    hdr.attr = nativeShort(hdr.attr);
    hdr.offset = nativeLong(hdr.offset);
}

void PalmDB::FlipResHdr(ResHdr& hdr) {
    hdr.id = nativeShort(hdr.id);
    hdr.offset = nativeLong(hdr.offset);
}

void PalmDB::swapBytes(const byte* src, byte* dest, const char* format, int count) {
    // b - byte, w - word, l - long
    // "b4w2" 4 bytes, 2 words
    const char* o_format = format;

    while (count--) {
        format = o_format;
        if (!dest)
            dest = (byte*)src;
        int count;

        while (*format) {
            switch (*format++) {
                case 'b':
                    assert(isdigit(*format));
                    count = *format++ - '0';
                    if (isdigit(*format)) {
                        count*=10;
                        count += *format++ - '0';
                    }
                    if (src != dest) {
                        while (count--) {
                            *dest++ = *src++;
                        }
                    } else {
                        src += count;
                        dest += count;
                    }
                    break;
                case 'w':
                    assert(isdigit(*format));
                    count = *format++ - '0';
                    if (isdigit(*format)) {
                        count*=10;
                        count += *format++ - '0';
                    }
                    while (count--) {
                        byte b = src[1];
                        dest[1] = src[0];
                        dest[0] = b;
                        src+=2;
                        dest+=2;
                    }
                    break;
                case 'l':
                    assert(isdigit(*format));
                    count = *format++ - '0';
                    if (isdigit(*format)) {
                        count*=10;
                        count += *format++ - '0';
                    }
                    while (count--) {
                        byte b = src[3];
                        dest[3] = src[0];
                        dest[0] = b;
                        b = src[2];
                        dest[2] = src[1];
                        dest[1] = b;
                        src+=4;
                        dest+=4;
                    }
                    break;
                default:
                    assert(!"confused in swap info!");
                    return;
            }
        }
    }
}

void PalmDB::Clear() {
    int i;
    for (i=0;i<rrecs.size();i++)
        delete rrecs[i].data;
    rrecs.clear();

    for (i=0;i<recs.size();i++)
        delete recs[i].data;
    recs.clear();
}

struct Color {
    byte r,g,b,a;
};

struct Bitmap {
    int w, h;
    //Color[1];
};

struct PalmImage {
    word x, y;
    word cbRow, ff;
    byte pixelsize;
    byte version;

    word next;
    byte transparentIndex;
    byte comp;
    word reserved;
    // data
};

struct PalmImageV3 {
    word x, y;
    word cbRow, ff;
    byte pixelsize;
    byte version;

    byte size;
    byte pixelformat;
    byte unused;
    byte compression;
    word density;
    dword transparentVal;
    dword next;
};

bool PalmDB::CreateBitmapFamily(int id, bool isIcon, const char* fn1,
    const char* fn2, const char* fn3, const char* fn4)
{
    string err;
    PalmBitmapFamily family = { NULL };
    family.images[0] = (char*)fn1;
    family.images[1] = (char*)fn2;
    family.images[2] = (char*)fn3;
    family.images[3] = (char*)fn4;
    return CreateBitmapFamily(id, isIcon, &family, err);
}

bool PalmDB::CreateBitmapFamily(int id, bool isIcon, PalmBitmapFamily* pFamily, string& err)
{
    int i;
    bool hasNext = false;
    bool hasHigh = false;

    // NULL out empty strings
    for (i=0;i<5;i++) if (pFamily->images[i] && !*pFamily->images[i]) pFamily->images[i] = NULL;
    for (i=0;i<5;i++) if (pFamily->imagesHigh[i] && !*pFamily->imagesHigh[i]) pFamily->imagesHigh[i] = NULL;

    // Load bitmaps
    Bitmap* aBitmaps[10] = {0, };
    PalmImage* aPImages[10] = {0, };
    int sizes[10] = {0, };

    if (isIcon && !pFamily->images[0]) {
        err = "1-bit image must be specified";
        return false; // 1-bit must be specified
    }
    
    for (i=0;i<5;i++) {
        if (pFamily->images[i]) {
            aBitmaps[i] = (Bitmap*)LoadBitmap(pFamily->images[i]);
            if (!aBitmaps[i]) {
                char buff[16];
                _itoa(1 << i, buff, 10);
                err = "failed to load ";
                err += buff;
                err += "-bit image";
                return false;
            }
        }
    }
    for (i=0;i<5;i++) {
        if (pFamily->imagesHigh[i]) {
            aBitmaps[5+i] = (Bitmap*)LoadBitmap(pFamily->imagesHigh[i]);
            hasHigh = true;
            if (!aBitmaps[5+i]) {
                char buff[16];
                _itoa(1 << (i), buff, 10);
                err = "failed to load ";
                err += buff;
                err += "-bit high-density image";
                return false;
            }
        }
    }

    // check sizes
    int w, h;
    if (isIcon && id == 1001) {
        w = aBitmaps[0]->w;
        h = aBitmaps[0]->h;
        if (!(w == 15 && h == 9)) {
            err = "small icon must be 15x9";
            goto error;
        }
    } else if (isIcon && id == 1000) {
        w = aBitmaps[0]->w;
        h = aBitmaps[0]->h;
        if (!(w == 32 && h == 32 ||
            w == 32 && h == 22 ||
            w == 22 && h == 22))
        {
            err = "large icon must be 22x22, 32x22, or 32x32";
            goto error;
        }
    } else {
        for (i=0;i<10;i++) {
            if (aBitmaps[i]) {
                if (i < 5) {
                    w = aBitmaps[i]->w;
                    h = aBitmaps[i]->h;
                } else {
                    w = aBitmaps[i]->w / 2;
                    h = aBitmaps[i]->h / 2;
                }
            }
        }
    }

    for (i=0;i<5;i++) {
        if (aBitmaps[i] && (w != aBitmaps[i]->w || h != aBitmaps[i]->h)) {
            err = "all images must be the same size";
            goto error;
        }
    }
    for (i=0;i<5;i++) {
        if (aBitmaps[5+i] && (w*2 != aBitmaps[5+i]->w || h*2 != aBitmaps[5+i]->h)) {
            err = "all high-density images must be double normal size";
            goto error;
        }
    }

    for (i=9; i>=0; i--) {
        if (aBitmaps[i]) {
            aPImages[i] = (PalmImage*)ConvertBitmap(aBitmaps[i], 1 << (i%5), hasNext, &sizes[i], i > 4, !isIcon);
            hasNext = true;
            if (!aPImages[i]) {
                char buff[16];
                _itoa(1 << (i%5), buff, 10);
                err = "failed to load ";
                err += buff;
                if (i > 4)
                    err += "-bit high-density image";
                else
                    err += "-bit image";
                goto error;
            }
        }
    }

    // create the db item
    {
        int size = 0;
        for (i=0;i<10;i++) size += sizes[i];
        if (hasHigh) size += sizeof(PalmImage);
        if (size > 0xff00) {
            err = "the total size of all images in one bitmap must be < 64k";
            goto error;
        }
        byte* data = new byte[size];
        byte* iter = data;

        for (i=0;i<10;i++) {
            if (i == 5 && hasHigh) {
                // insert a fake bitmap header
                PalmImage fake;
                memset(&fake, 0, sizeof(fake));
                fake.version = 1;
                fake.pixelsize = 255;
                memcpy(iter, &fake, sizeof(fake));
                iter += sizeof(fake);
            }
            if (aPImages[i]) {
                memcpy(iter, aPImages[i], sizes[i]);
                iter += sizes[i];
            }
        }
        
        PalmResRecord rec;
        rec.data = data;
        rec.id = id;
        rec.len = size;
        rec.type = isIcon ? "tAIB" : "Tbmp";
        AddResRec(rec);
        delete data;

        for (i=0;i<10;i++) {
            if (aBitmaps[i]) free(aBitmaps[i]);
            if (aPImages[i]) free(aPImages[i]);
        }
    }

    return true;

error:
    // different sizes
    for (i=0;i<10;i++) {
        if (aBitmaps[i]) free(aBitmaps[i]);
        if (aPImages[i]) free(aPImages[i]);
    }
    return false;
}


void* PalmDB::LoadBitmap(const char* fn) {
    HBITMAP hBmp = (HBITMAP)LoadImage(NULL, fn, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!hBmp)
        return NULL;

    BITMAPINFO bi = {0, };
    HDC hDC = GetDC(NULL);
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    if (!GetDIBits(hDC, hBmp, 0, 0, NULL, &bi, DIB_RGB_COLORS)) {
        int e = GetLastError();
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    // ensure that the loaded bitmap is 32-bit, uncompressed (and no bitfields)
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    int w, h;
    w = bi.bmiHeader.biWidth;
    h = bi.bmiHeader.biHeight;
    if (h < 0) h = -h;
    bi.bmiHeader.biHeight = -h;

    Bitmap* pB = (Bitmap*)malloc(sizeof(Bitmap) + w * h * sizeof(Color));
    if (!pB) {
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    pB->w = w;
    pB->h = h;

    if (!GetDIBits(hDC, hBmp, 0, h, pB + 1, &bi, DIB_RGB_COLORS)) {
        int e = GetLastError();
        free(pB);
        ReleaseDC(NULL, hDC);
        return NULL;
    }

    Color* col = (Color*)(pB + 1);
    for (int i=0;i<w*h;i++) {
        byte t = col[i].r;
        col[i].r = col[i].b;
        col[i].b = t;
    }

    DeleteObject(hBmp);
    ReleaseDC(NULL, hDC);
    return pB;
}

static Color pal1[2] = 
{
  { 255, 255, 255}, {   0,   0,   0 }
};

static Color pal2[4] = 
{
  //{ 255, 255, 255}, { 170, 170, 170}, { 85, 85, 85 }, {   0,   0,   0 }
  { 255, 255, 255}, { 192, 192, 192}, { 128, 128, 128 }, {   0,   0,   0 }
};

static Color pal4[16] = 
{
  { 255, 255, 255}, { 238, 238, 238 }, { 221, 221, 221 }, { 204, 204, 204 },
  { 187, 187, 187}, { 170, 170, 170 }, { 153, 153, 153 }, { 136, 136, 136 },
  { 119, 119, 119}, { 102, 102, 102 }, {  85,  85,  85 }, {  68,  68,  68 },
  {  51,  51,  51}, {  34,  34,  34 }, {  17,  17,  17 }, {   0,   0,   0 }
};

static Color pal8[256] = 
{
  { 255, 255, 255 }, { 255, 204, 255 }, { 255, 153, 255 }, { 255, 102, 255 }, 
  { 255,  51, 255 }, { 255,   0, 255 }, { 255, 255, 204 }, { 255, 204, 204 }, 
  { 255, 153, 204 }, { 255, 102, 204 }, { 255,  51, 204 }, { 255,   0, 204 }, 
  { 255, 255, 153 }, { 255, 204, 153 }, { 255, 153, 153 }, { 255, 102, 153 }, 
  { 255,  51, 153 }, { 255,   0, 153 }, { 204, 255, 255 }, { 204, 204, 255 },
  { 204, 153, 255 }, { 204, 102, 255 }, { 204,  51, 255 }, { 204,   0, 255 },
  { 204, 255, 204 }, { 204, 204, 204 }, { 204, 153, 204 }, { 204, 102, 204 },
  { 204,  51, 204 }, { 204,   0, 204 }, { 204, 255, 153 }, { 204, 204, 153 },
  { 204, 153, 153 }, { 204, 102, 153 }, { 204,  51, 153 }, { 204,   0, 153 },
  { 153, 255, 255 }, { 153, 204, 255 }, { 153, 153, 255 }, { 153, 102, 255 },
  { 153,  51, 255 }, { 153,   0, 255 }, { 153, 255, 204 }, { 153, 204, 204 },
  { 153, 153, 204 }, { 153, 102, 204 }, { 153,  51, 204 }, { 153,   0, 204 },
  { 153, 255, 153 }, { 153, 204, 153 }, { 153, 153, 153 }, { 153, 102, 153 },
  { 153,  51, 153 }, { 153,   0, 153 }, { 102, 255, 255 }, { 102, 204, 255 },
  { 102, 153, 255 }, { 102, 102, 255 }, { 102,  51, 255 }, { 102,   0, 255 },
  { 102, 255, 204 }, { 102, 204, 204 }, { 102, 153, 204 }, { 102, 102, 204 },
  { 102,  51, 204 }, { 102,   0, 204 }, { 102, 255, 153 }, { 102, 204, 153 },
  { 102, 153, 153 }, { 102, 102, 153 }, { 102,  51, 153 }, { 102,   0, 153 },
  {  51, 255, 255 }, {  51, 204, 255 }, {  51, 153, 255 }, {  51, 102, 255 },
  {  51,  51, 255 }, {  51,   0, 255 }, {  51, 255, 204 }, {  51, 204, 204 },
  {  51, 153, 204 }, {  51, 102, 204 }, {  51,  51, 204 }, {  51,   0, 204 },
  {  51, 255, 153 }, {  51, 204, 153 }, {  51, 153, 153 }, {  51, 102, 153 },
  {  51,  51, 153 }, {  51,   0, 153 }, {   0, 255, 255 }, {   0, 204, 255 },
  {   0, 153, 255 }, {   0, 102, 255 }, {   0,  51, 255 }, {   0,   0, 255 },
  {   0, 255, 204 }, {   0, 204, 204 }, {   0, 153, 204 }, {   0, 102, 204 },
  {   0,  51, 204 }, {   0,   0, 204 }, {   0, 255, 153 }, {   0, 204, 153 },
  {   0, 153, 153 }, {   0, 102, 153 }, {   0,  51, 153 }, {   0,   0, 153 },
  { 255, 255, 102 }, { 255, 204, 102 }, { 255, 153, 102 }, { 255, 102, 102 },
  { 255,  51, 102 }, { 255,   0, 102 }, { 255, 255,  51 }, { 255, 204,  51 },
  { 255, 153,  51 }, { 255, 102,  51 }, { 255,  51,  51 }, { 255,   0,  51 },
  { 255, 255,   0 }, { 255, 204,   0 }, { 255, 153,   0 }, { 255, 102,   0 },
  { 255,  51,   0 }, { 255,   0,   0 }, { 204, 255, 102 }, { 204, 204, 102 },
  { 204, 153, 102 }, { 204, 102, 102 }, { 204,  51, 102 }, { 204,   0, 102 },
  { 204, 255,  51 }, { 204, 204,  51 }, { 204, 153,  51 }, { 204, 102,  51 },
  { 204,  51,  51 }, { 204,   0,  51 }, { 204, 255,   0 }, { 204, 204,   0 },
  { 204, 153,   0 }, { 204, 102,   0 }, { 204,  51,   0 }, { 204,   0,   0 },
  { 153, 255, 102 }, { 153, 204, 102 }, { 153, 153, 102 }, { 153, 102, 102 },
  { 153,  51, 102 }, { 153,   0, 102 }, { 153, 255,  51 }, { 153, 204,  51 },
  { 153, 153,  51 }, { 153, 102,  51 }, { 153,  51,  51 }, { 153,   0,  51 },
  { 153, 255,   0 }, { 153, 204,   0 }, { 153, 153,   0 }, { 153, 102,   0 },
  { 153,  51,   0 }, { 153,   0,   0 }, { 102, 255, 102 }, { 102, 204, 102 },
  { 102, 153, 102 }, { 102, 102, 102 }, { 102,  51, 102 }, { 102,   0, 102 },
  { 102, 255,  51 }, { 102, 204,  51 }, { 102, 153,  51 }, { 102, 102,  51 },
  { 102,  51,  51 }, { 102,   0,  51 }, { 102, 255,   0 }, { 102, 204,   0 },
  { 102, 153,   0 }, { 102, 102,   0 }, { 102,  51,   0 }, { 102,   0,   0 },
  {  51, 255, 102 }, {  51, 204, 102 }, {  51, 153, 102 }, {  51, 102, 102 },
  {  51,  51, 102 }, {  51,   0, 102 }, {  51, 255,  51 }, {  51, 204,  51 },
  {  51, 153,  51 }, {  51, 102,  51 }, {  51,  51,  51 }, {  51,   0,  51 },
  {  51, 255,   0 }, {  51, 204,   0 }, {  51, 153,   0 }, {  51, 102,   0 },
  {  51,  51,   0 }, {  51,   0,   0 }, {   0, 255, 102 }, {   0, 204, 102 },
  {   0, 153, 102 }, {   0, 102, 102 }, {   0,  51, 102 }, {   0,   0, 102 },
  {   0, 255,  51 }, {   0, 204,  51 }, {   0, 153,  51 }, {   0, 102,  51 },
  {   0,  51,  51 }, {   0,   0,  51 }, {   0, 255,   0 }, {   0, 204,   0 },
  {   0, 153,   0 }, {   0, 102,   0 }, {   0,  51,   0 }, {  17,  17,  17 },
  {  34,  34,  34 }, {  68,  68,  68 }, {  85,  85,  85 }, { 119, 119, 119 },
  { 136, 136, 136 }, { 170, 170, 170 }, { 187, 187, 187 }, { 221, 221, 221 },
  { 238, 238, 238 }, { 192, 192, 192 }, { 128,   0,   0 }, { 128,   0, 128 },
  {   0, 128,   0 }, {   0, 128, 128 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 },
  {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }, {   0,   0,   0 }
};

static int colorTo16(Color c) {
    return ((((int)c.r & 0xF8) << 8) |       // 1111100000000000 
        (((int)c.g & 0xFC) << 3) |       // 0000011111100000
        (((int)c.b & 0xF8) >> 3));       // 0000000000011111
}

static int findNearest(Color c, int depth) {
    int nc = 1 << depth;
    Color* pal;
    switch (depth) {
        case 1: pal = pal1; break;
        case 2: pal = pal2; break;
        case 4: pal = pal4; break;
        case 8: pal = pal8; break;
        default:
            return 0;
    }

    int minDiff = 255 * 255 * 3;
    int minIndex = 0;
    for (int i=0;i<nc;i++) {
        int diff = 
            (c.r - pal[i].r)*(c.r - pal[i].r) +
            (c.b - pal[i].b)*(c.b - pal[i].b) +
            (c.g - pal[i].g)*(c.g - pal[i].g);
        if (diff < minDiff) {
            minDiff = diff;
            minIndex = i;
        }
        if (diff == 0)
            break;
    }

    return minIndex;
}

static void setPix1(byte* data, int x, int y, int c, int cbLine) {
    int i = y * cbLine + (x >> 3);
    data[i] &= ~(1 << (7 - (x & 7)));
    data[i] |= (c & 1) << (7 - (x & 7));
}

static void setPix2(byte* data, int x, int y, int c, int cbLine) {
    int i = y * cbLine + (x >> 2);
    data[i] &= ~(3 << ((3 - (x & 3))*2));
    data[i] |= (c & 3) << ((3 - (x & 3))*2);
}

static void setPix4(byte* data, int x, int y, int c, int cbLine) {
    int i = y * cbLine + (x >> 1);
    data[i] &= ~((x&1) ? 0x0f : 0xf0);
    c &= 0x0f;
    data[i] |= (x&1) ? c : (c<<4);
}

static void setPix8(byte* data, int x, int y, int c, int cbLine) {
    int i = y * cbLine + x;
    data[i] = c;
}

static void setPix16(byte* data, int x, int y, int c, int cbLine) {
    int i = y * cbLine + x * 2;
    data[i] = (c >> 8) & 0xff;
    data[i+1] = c & 0xff;
}

bool PalmDB::CompressBitmap(byte* bitmap, int cbLine, int h, bool v3, byte *& data, int& size) {
    data = new byte[cbLine * h * 2 + 32];
    memset(data, 0, cbLine * h * 2 + 32);
    size = v3 ? 4 : 2;
    int flag;

    for (int y = 0; y < h; y++) {
        flag = 0;
        for (int b = 0; b < cbLine; b++) {
            if (y == 0 || bitmap[cbLine * y + b] != bitmap[cbLine * (y - 1) + b])
                flag |= (0x80 >> (b & 7));
            if ((b & 7) == 7 || b == cbLine - 1) {
                data[size++] = (byte)flag;
                for (int i = (b & ~7); i <= b; i++) {
                    if (((flag <<= 1) & 0x100) != 0)
                        data[size++] = bitmap[cbLine * y + i];
                }
                flag = 0;
            }
        }
    }

    if (size < cbLine * h) {
        // the bitmap was compressed
        int i = 0;
        if (v3) {
            data[i++] = (byte)((size & 0xff000000) >> 24);
            data[i++] = (byte)((size & 0x00ff0000) >> 16);
        }
        data[i++] = (byte)((size & 0x0000ff00) >> 8);
        data[i++] = (byte)(size & 0x000000ff);
        return true;
    } else {
        delete [] data;
        data = NULL;
        return false;
    }
}

void* PalmDB::ConvertBitmap(void* pvBitmap, int bpp, bool hasNext, int* pSize, bool bHighDensity, bool bCompress) {
    Bitmap* pBmp = (Bitmap*)pvBitmap;
    int x, y, w = pBmp->w, h = pBmp->h;
    int cbLine = ((w * bpp + 15) / 16) * 2; // word aligned scan lines
    bool version3 = bHighDensity;
    int headerSize = version3 ? sizeof(PalmImageV3) : sizeof(PalmImage);
    if (bpp == 16 && !version3) headerSize += 8; // Direct color info
    int size = (headerSize + cbLine * h + 3) & ~0x3;
    int iTrans = 0, iNonTrans, nColors = 1 << bpp;
    bool bTrans = false;

    void  (*setPixFunc)(byte*, int, int, int, int);
    switch (bpp) {
        case 1: setPixFunc = setPix1; break;
        case 2: setPixFunc = setPix2; break;
        case 4: setPixFunc = setPix4; break;
        case 8: setPixFunc = setPix8; break;
        case 16: setPixFunc = setPix16; break;
        default:
            return NULL;
    }

    PalmImage* pImage = (PalmImage*)malloc(size);
    PalmImageV3* pImageV3 = (PalmImageV3*)pImage;
    if (!pImage)
        return NULL;
    memset(pImage, 0, size);

    pImage->cbRow = cbLine;
    pImage->pixelsize = bpp;
    pImage->version = (bpp >= 4) ? 2 : 1; // reset if bTrans
    pImage->x = w;
    pImage->y = h;
    if (version3) {
        pImageV3->size = 0x18; // sizeof struct
        pImageV3->pixelformat = bpp == 16 ? 1 : 0; // 0 == indexed, 1 = 565
        pImageV3->unused = 0;
        pImageV3->compression = 0;
        pImageV3->density = bHighDensity ? 144 : 72;
        Color c = { 255, 128, 255, 0 };
        pImageV3->transparentVal = colorTo16(c);
        pImageV3->ff = 0;
        pImageV3->version = 3;
    } else {
        pImage->comp = 0;
        pImage->reserved = 0;
    }

    Color* src = (Color*)(pBmp + 1);
    byte * dest = (byte*)pImage;
    if (version3) dest += sizeof (PalmImageV3);
    else dest += sizeof(PalmImage);
    if (!version3 && bpp == 16) dest += 8;
    byte* compStart = dest;

    if (bpp == 4 || bpp == 8) {
        // count colors
        byte nPixPerColor[256] = { 0, };
        for (y=0;y<h;y++) {
            for (x=0;x<w;x++) {
                Color& c = src[y*w+x];
                if (c.r == 255 && c.g == 128 && c.b == 255)
                    bTrans = true;
                else
                    nPixPerColor[findNearest(c, bpp)]++;
            }
        }

        if (bTrans) {
            for (int i=0;i<nColors;i++) {
                if (nPixPerColor[i] < nPixPerColor[iTrans]) {
                    iTrans = i;
                }
            }
            if (bpp != 8) {
                iNonTrans = (iTrans == nColors - 1) ? iTrans - 1 : iTrans + 1;
            }
        }
    }

    // write the bitmap data
    for (y=0;y<h;y++) {
        for (x=0;x<w;x++) {
            int index = iTrans;
            Color& c = src[y*w+x];
            if (bpp == 16) {
                if (!bTrans && c.r == 255 && c.g == 128 && c.b == 255)
                    bTrans = true;
                index = colorTo16(c);
            } else {
                if (!bTrans || c.r != 255 || c.g != 128 || c.b != 255) {
                    index = findNearest(c, bpp);
                    if (index == iTrans && bTrans)
                        index = iNonTrans;
                }
            }
            (*setPixFunc)(dest, x, y, index, cbLine);
        }
    }

    if (version3) {
        if (bTrans) {
            if (bpp == 16) {
                pImageV3->ff = 0x2400; // should this have the directColor flag?
            } else {
                pImageV3->transparentVal = iTrans;
                pImageV3->ff = 0x2000;
            }
        } else if (bpp == 16) {
            pImageV3->ff = 0x0400;
        }
    } else {
        if (bTrans) {
            pImage->version = 2;
            pImage->ff = 0x2000; // properties
            pImage->transparentIndex = iTrans;
        }
        if (bpp == 16) {
            pImage->version = 2;
            if (bTrans)
                pImage->ff = 0x2400; // properties
            else
                pImage->ff = 0x0400;

            // add Direct info
            byte* pData = (byte*)(pImage + 1);
            *pData++ = 0;
            *pData++ = 5;
            *pData++ = 6;
            *pData++ = 5;
            *pData++ = 0;
            *pData++ = 255 & 0xF8;
            *pData++ = 128 & 0xFC;
            *pData++ = 255 & 0xF8;
        }
    }

    // attempt compression
    int newSize = 0;
    byte* newData = NULL;

    if (bCompress && bpp <= 8 && CompressBitmap(compStart, cbLine, h, version3, newData, newSize)) {
        memcpy(compStart, newData, newSize);
        size = (headerSize + newSize + 3) & ~3;
        pImage->ff |= 0x8000;
    }

    if (version3) {
        //pImageV3->next = hasNext ? (size >> 2) : 0;
        pImageV3->next = hasNext ? size : 0;
        swapBytes((byte*)pImage, (byte*)pImage, "w4b6w1l2");
    } else {
        pImage->next = hasNext ? (size >> 2) : 0;
        swapBytes((byte*)pImage, (byte*)pImage, "w4b2w1b2w1");
    }

    *pSize = size;
    return pImage;
}
#else
PalmDB::PalmDB() : isRes(true), isLocked(false), isHidden(false), dmRef(NULL), wasWritten(false) {
}

PalmDB::~PalmDB() {
    if (dmRef) {
        DmCloseDatabase(dmRef);
        if (!wasWritten) {
            LocalID lid = DmFindDatabase(0, name.c_str());
            if (lid) DmDeleteDatabase(0, lid);
        }
    }
}

bool PalmDB::Create() {
    LocalID lid = DmFindDatabase(0, name.c_str());
    if (lid) DmDeleteDatabase(0, lid);
    char atype[4];
    char acid[4];
    for (int i=0;i<4 && i<type.length();i++)
        atype[i] = type[i];
    for (int i=0;i<4 && i<creator.length();i++)
        acid[i] = creator[i];
    Err err = DmCreateDatabase(0, name.c_str(), *((dword*)&acid[0]), *((dword*)&type[0]), isRes);
    if (err != errNone) return false;
    lid = DmFindDatabase(0, name.c_str());
    dmRef = DmOpenDatabase(0, lid, dmModeReadWrite | dmModeExclusive);
    //dmRef = DmOpenDatabaseByTypeCreator(*((dword*)&type[0]), *((dword*)&acid[0]),
    //	dmModeReadWrite | dmModeExclusive);
    return dmRef != NULL;
}

bool PalmDB::Write(const char* fileName) {
    wasWritten = true;
    return true;
}

bool PalmDB::AddRec(PalmRecord& rec) {
    if (!isRes) {
        if (!dmRef) {
            if (!Create())
                return false;
        }
        word newid = -1;
        MemHandle hMem = DmNewRecord(dmRef, &newid, rec.len);
        if (!hMem) return false;
        byte* pRec = (byte*)MemHandleLock(hMem);
        DmWrite(pRec, 0, rec.data, rec.len);
        MemHandleUnlock(hMem);
        return true;
    }
    return false;
}

bool PalmDB::AddResRec(PalmResRecord& rec) {
    if (isRes) {
        if (!dmRef) {
            if (!Create())
                return false;
        }
        char atype[4] = {0};
        for (int i=0;i<4 && i<rec.type.length();i++)
            atype[i] = rec.type[i];
        MemHandle hMem = DmNewResource(dmRef, *((dword*)&atype[0]), rec.id, rec.len);
        if (!hMem) return false;
        byte* pRec = (byte*)MemHandleLock(hMem);
        DmWrite(pRec, 0, rec.data, rec.len);
        MemHandleUnlock(hMem);
        return true;
    }
    return false;
}

void PalmDB::SetName(const char* _name) {
    name = _name;
}

void PalmDB::SetCreatorType(const char* _creator, const char* _type) {
    creator = _creator;
    type = _type;
}

void PalmDB::SetRes(bool _isRes) {
    isRes = _isRes;
}

void PalmDB::SetLocked(bool _isLocked) {
    isLocked = _isLocked;
}

void PalmDB::SetHidden(bool _isHidden) {
    isHidden = _isHidden;
}

short PalmDB::nativeShort(short s) {
   return ((s&0x00ff)<<8) | ((s&0xff00)>>8);
}

long PalmDB::nativeLong(long l) {
    long res;
    char* src = (char*)&l;
    char* dest = (char*)&res;
    dest[0] = src[3];
    dest[1] = src[2];
    dest[2] = src[1];
    dest[3] = src[0];
    return res;
}
#endif
