// pdoc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\..\OrbForms\compiler\palmdb.h"

char* GetFile(const char* file) {
    FILE* f = fopen(file, "rt");
    if (!f) {
        return NULL;
    }

    int len = _filelength(f->_file);
    char* data = new char[len+1];
    if (data) {
        int rlen = fread(data, 1, len, f);
        data[rlen] = 0;
    }
    fclose(f);
    return data;
}

class PDOCWriter : public PalmDB {
    struct DocHeader {
        byte crap;
        byte version;
        word spare;
        dword len;
        word nRecs;
        word maxRecSize;
        dword position;
    };
    static const int c_maxBlock = 4096;

public:
    PDOCWriter();
    bool ConvertFile(const char* filename, const char* outfilename, const char* docname, bool bCompress);
protected:
    bool ConvertBuffer(const char* buffer);
    bool AddBlock(const char* block, int len);
    void CompressBlock(byte* block, int olen, byte* cblock, int& clen);
    int FindToken(byte* block, int blen, int offset, int& tlen);

    DocHeader header;
    bool bCompress;
};

PDOCWriter::PDOCWriter() : PalmDB() {
    ZeroMemory(&header, sizeof(header));
}

bool PDOCWriter::ConvertFile(const char* filename, const char* outfilename, const char* docname, bool bCompress) {
    this->bCompress = bCompress;

    char* text = GetFile(filename);
    if (!text) {
        printf("unable to open file '%s'\n", filename);
        return false;
    }
    int len = strlen(text);

    if (len > 65000) {
        printf("file too large\n");
        return false;
    }

    char fullPath[MAX_PATH];
    char* pFileName = NULL;
    GetFullPathName(filename, MAX_PATH, fullPath, &pFileName);

    SetCreatorType("REAd", "TEXt");
    SetName(docname ? docname : pFileName);
    SetRes(false);

    // add the header first
    header.version = bCompress ? 2 : 1;
    header.len = len;
    PalmRecord rec;
    rec.data = (byte*)&header;
    rec.len = sizeof(header);
    AddRec(rec);

    if (!ConvertBuffer(text)) {
        puts("error in conversion");
        return false;
    }

    // update the header
    rec.data = (byte*)&header;
    rec.len = sizeof(header);
    swapBytes((byte*)&header, (byte*)&header, "b2w1l1w2l1");
    SetRec(0, rec);

    char pdb[MAX_PATH];
    strcpy(pdb, filename);
    strcat(pdb, ".pdb");
    if (!outfilename)
        outfilename = pdb;

    if (!Write(outfilename)) {
        printf("unable to write file '%s'\n", pdb);
        return false;
    }

    return true;
}

bool PDOCWriter::ConvertBuffer(const char* buffer) {
    int len = strlen(buffer);
    int start = 0;
    while (start < len) {
        int blen = min(strlen(buffer + start), c_maxBlock);
        if (!AddBlock(buffer + start, blen))
            return false;
        start += blen;
    }
    return true;
}

bool PDOCWriter::AddBlock(const char* block, int len) {
    PalmRecord rec;
    byte cblock[c_maxBlock * 2];
    int clen = 0;

    if (bCompress) {
        CompressBlock((byte*)block, len, cblock, clen);
        rec.data = (byte*)cblock;
        rec.len = clen;
    }
    
    if (!bCompress || clen > len) {
        rec.data = (byte*)block;
        rec.len = len;
    }
    AddRec(rec);

    header.maxRecSize = max(header.maxRecSize, len);
    header.nRecs++;
    
    return true;
}

const int c_minToken = 3;
const int c_maxToken = 10;
const int c_maxOffset = 2047;
int PDOCWriter::FindToken(byte* block, int blen, int offset, int& tlen) {
    tlen = 0;

    if (blen < c_minToken || blen - offset < c_minToken)
        return 0;

    for (int i = max(0, offset - c_maxOffset); i < offset - c_minToken; i++) {
        if (memcmp(block + offset, block + i, c_minToken) == 0) {
            while (i + tlen < offset && tlen < blen - offset && tlen < c_maxToken && block[tlen + offset] == block[tlen + i])
                tlen++;
            return offset - i;
        }
    }
    return 0;
}

void PDOCWriter::CompressBlock(byte* block, int blen, byte* cblock, int& clen) {
    bool bWasSpace = false;

    clen = 0;
    for (int i = 0; i < blen; i++) {
        int tlen = 0;
        int offset = FindToken(block, blen, i, tlen);
        if (offset) {
            if (bWasSpace) {
                cblock[clen++] = ' ';
            }
            bWasSpace = false;
            word w = 0x8000 | (offset << 3) | (tlen - 3);
            cblock[clen++] = w >> 8;
            cblock[clen++] = w & 0xff;
            i += tlen - 1;
        } else if (block[i] == ' ') {
            if (bWasSpace) {
                // two spaces in a row can't be compressed
                cblock[clen++] = ' ';
            }
            bWasSpace = true;
        } else if (block[i] < 0x80 && block[i] > 0x08) {
            if (block[i] < 0x40) {
                // not compatible with space compression
                if (bWasSpace)
                    cblock[clen++] = ' ';
                bWasSpace = false;
            }
            if (bWasSpace)
                cblock[clen++] = block[i] | 0x80;
            else
                cblock[clen++] = block[i];
            bWasSpace = false;
        } else {
            if (bWasSpace) {
                cblock[clen++] = ' ';
            }
            cblock[clen++] = 1; // one byte literal - this could be improved
            cblock[clen++] = block[i];
            bWasSpace = false;
        }
    }

    // trailing space
    if (bWasSpace) {
        cblock[clen++] = ' ';
    }
}

class PDOCReader: public PalmDB {
    struct DocHeader {
        byte crap;
        byte version;
        word spare;
        dword len;
        word nRecs;
        word maxRecSize;
        dword position;
    };
    static const int c_maxBlock = 4096;

public:
    PDOCReader();
    bool ConvertFile(const char* filename, const char* outfilename);
protected:
    char* ConvertRecord(char* buffer, PalmRecord* pRec);

    DocHeader header;
    bool bCompressed;
};

PDOCReader::PDOCReader() : PalmDB() {
    ZeroMemory(&header, sizeof(header));
}

bool PDOCReader::ConvertFile(const char* filename, const char* outfilename) {
    if (!Read(filename)) {
        printf("unable to read file '%s'\n", filename);
        return false;
    }

    // read in header
    PalmRecord* pRec = GetRec(0);
    if (!pRec || pRec->len < sizeof(DocHeader)) {
        puts("invalid database");
        return false;
    }
    swapBytes(pRec->data, (byte*)&header, "b2w1l1w2l1");

    if (GetCount() != header.nRecs + 1) {
        puts("invalid doc header");
        return false;
    }

    if (header.maxRecSize > c_maxBlock || header.len > 65000) {
        puts("doc header values out of bounds");
        return false;
    }

    if (header.version != 1 && header.version != 2) {
        puts("unsupported doc version");
        return false;
    }

    bCompressed = (header.version == 2);

    char* text = new char[header.len + 1];
    ZeroMemory(text, header.len + 1);

    char* iter = text;
    for (int i = 0; i < header.nRecs; i++) {
        iter = ConvertRecord(iter, GetRec(i + 1));
        if (!iter) {
            delete [] text;
            puts("Failed converting record");
            return false;
        }
    }

    if (!outfilename)
        outfilename = dh.name;

    FILE* pFile = fopen(outfilename, "wt");
    if (!pFile) {
        printf("unable to open output file '%s'\n", outfilename);
        delete [] text;
        return false;
    }

    fwrite(text, header.len, 1, pFile);
    fclose(pFile);

    delete [] text;
    return true;
}

char* PDOCReader::ConvertRecord(char* buffer, PalmRecord* pRec) {
    if (!bCompressed) {
        // uncompressed
        memcpy(buffer, pRec->data, pRec->len);
        return buffer + pRec->len;

    } else {
        int i = 0;
        while (i < pRec->len) {
            unsigned char c = pRec->data[i++];
            
            // space then char
            if (c >= 0xc0) {
                *buffer++ = ' ';
                c = c^0x80;
            }
            // pass-thru
            if (c == 0 || (c >= 0x09 && c <= 0x7f)) {
                *buffer++ = c;
            // literal
            } else if (c >= 0x01 && c <= 0x08) {
                memcpy(buffer, &pRec->data[i], c);
                buffer += c;
                i += c;
            // copy
            } else {
                word offset = (c << 8) | pRec->data[i++];
                offset &= 0x3fff;
                int clen = (offset & 0x07) + 3;
                offset >>= 3;
                while (clen--) {
                    *buffer = *(buffer - offset);
                    buffer++;
                }
            }
        }
        return buffer;
    }
}

void usage() {
    puts(
"OrbPDOC - a simple PDOC <-> text file converter\n"
"Copyright (c) 2006, OrbWorks\n"
"\n"
"usage: orbpdoc.exe [-d] infile [outfile [docname]]\n"
"  * If -d is NOT specified, the conversion is text -> PDOC. outfile is\n"
"    optional, and defaults to '<infile>.pdb'. docname is the document\n"
"    name as it will be seen by the Palm OS and PDOC reader, and defaults\n"
"    to infile.\n"
"  * If -d is specified, the conversion is PDOC -> text. outfile is optional,\n"
"    and defaults to the document name stored within the .pdb file. docname\n"
"    cannot be specified.\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        usage();
        return -1;
    }

    char* infile;
    char* outfile = NULL;
    char* docname = NULL;
    bool bDecompress = false;
    int iArg = 1;

    if (argc > 2 && argv[1][0] == '-' && argv[1][1] == 'd') {
        bDecompress = true;
        iArg++;
    }

    infile = argv[iArg++];
    if (argc > iArg)
        outfile = argv[iArg++];

    if (argc > iArg)
        docname = argv[iArg++];

    if (bDecompress) {
        PDOCReader db;
        if (!db.ConvertFile(infile, outfile)) {
            puts("conversion failed");
            return -1;
        }

    } else {
        PDOCWriter db;
        if (!db.ConvertFile(infile, outfile, docname, true)) {
            puts("conversion failed");
            return -1;
        }
    }
    
    return 0;
}
