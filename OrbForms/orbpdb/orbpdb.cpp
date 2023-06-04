// OrbPDB - PDB/CSV conversion tool for OrbForms/PocketC
// (c) 1998-2004 OrbWorks

#include "stdafx.h"

// 4i2.12sz(i4sz)
class OrbError {
public:
    OrbError(string e) { err = e; }
    string err;
};

enum VarType { vtInt, vtChar, vtFloat, vtString };

class Format {
public:
    Format(string str);
    bool Next(VarType& type, long& len);
private:
    string format, rformat;
    char* iter;
    int len, count;
    VarType type;
};

Format::Format(string str) {
    len = count = 0;
    iter = NULL;
    if (str.length() == 0) throw new OrbError("missing format specifier");
    size_t i = str.find('(');
    if (i != string::npos) {
        size_t j = str.find(')', i);
        if (j == string::npos) throw new OrbError("invalid format string");
        format = str.substr(0, i);
        rformat = str.substr(i+1, j-i-1);
    } else {
        format = str;
    }
    if (format.empty())
        iter = (char*)rformat.c_str();
    else
        iter = (char*)format.c_str();
}

bool Format::Next(VarType& _type, long& _len) {
    if (count) {
        // same as last time
        count--;
        _type = type;
        _len = len;
        return true;
    }
    
    if (*iter == '\0' && rformat.empty())
        return false;
    
    if (*iter == '\0') {
        iter = (char*)rformat.c_str();
    }
    
tryagain:
    switch (*iter) {
        case 'i':
            type = vtInt;
            len = 4;
            iter++;
            if (*iter == '4') {
                iter++;
            } else if (*iter == '2') {
                len = 2;
                iter++;
            } else if (*iter == '1') {
                len = 1;
                iter++;
            }
            if (*iter == '.')
                iter++;
            break;
        case 'w':
            type = vtInt;
            len = 2;
            iter++;
            break;
        case 'b':
            type = vtInt;
            len = 1;
            iter++;
            break;
        case 'c':
            type = vtChar;
            len = 1;
            iter++;
            break;
        case 'f':
            type = vtFloat;
            len = 4;
            iter++;
            break;
        case 's':
        case 'l':
            type = vtString;
            len = 0;
            iter++;
            if (*iter == 'z') {
                iter++;
            } else {
                len = 0;
                while (isdigit(*iter)) {
                    len*=10;
                    len+=(*iter++) -'0';
                }
                if (*iter == '.')
                    iter++;
            }
            break;
        default:
            if (!isdigit(*iter)) {
                throw new OrbError("invalid format string");
            }
            count = 0;
            while (isdigit(*iter)) {
                count *= 10;
                count += (*iter++) - '0';
            }
            count--; // we are about to grab the first one
            goto tryagain;
    }
    _type = type;
    _len = len;
    return true;
}

class ByteVector : public vector<byte> {
public:
    void pushByte(byte b) {
        push_back(b);
    }
    void pushWord(word w) {
        push_back((w >> 8) & 0xff);
        push_back(w & 0xff);
    }
    void pushLong(dword d) {
        push_back((d >> 24) & 0xff);
        push_back((d >> 16) & 0xff);
        push_back((d >> 8) & 0xff);
        push_back(d & 0xff);
    }
    void pushString(string s, int len = 0) {
        if (len == 0) {
            for (size_t i=0;i<s.length();i++)
                push_back(s[i]);
            push_back(0);
        } else {
            int slen = (int)min(s.length(), (size_t)len);
            for (int i=0;i<slen;i++)
                push_back(s[i]);
            for (int j=0;j<len-slen;j++)
                push_back(0);
        }
    }
};

bool RecordToCSV(PalmRecord* prec, CSVFile& csv, string& format) {
    Format f(format);
    VarType type;
    long len;
    
    PalmDBStream dbstr(prec->data);
    while (f.Next(type, len)) {
        if (dbstr.getLoc() + len > prec->len) break;
        if (len == 0 && dbstr.getLoc() + 1 > prec->len) break;
        switch (type) {
            case vtInt: {
                int i;
                if (len == 1) {
                    i = dbstr.getByte();
                } else if (len == 2) {
                    i = dbstr.getWord();
                } else {
                    i = dbstr.getLong();
                }
                csv.SetInt(i);
                break;
            }
            case vtChar: {
                char c = dbstr.getByte();
                string s;
                s += c;
                csv.SetValue(s);
                break;
            }
            case vtFloat: {
                long l = dbstr.getLong();
                float f;
                //l = PalmDB::nativeLong(l);
                f = *((float*)&l);
                char buff[24];
                _gcvt(f, 10, buff);
                csv.SetValue(buff);
                break;
            }
            case vtString: {
                string s;
                char c;
                if (len) {
                    while (len) {
                        len--;
                        c = dbstr.getByte();
                        s += c;
                    }
                } else {
                    while (dbstr.getLoc() < prec->len) {
                        c = dbstr.getByte();
                        if (c == 0) break;
                        s += c;
                    }
                }
                csv.SetValue(s);
                break;
            }
        }
    }
    csv.WriteLine();
    return true;
}

bool CSVToRecord(CSVFile& csv, PalmDB& db, string& format) {
    Format f(format);
    VarType type;
    long len;
    ByteVector bv;
    
    while (f.Next(type, len) && !csv.eol()) {
        switch (type) {
            case vtInt: {
                long l = csv.GetInt();
                if (len == 1)
                    bv.pushByte(l & 0xff);
                else if (len == 2)
                    bv.pushWord(l & 0xffff);
                else
                    bv.pushLong(l);
                break;
            }
            case vtChar: {
                string s = csv.GetValue();
                bv.pushByte(s[0]);
                break;
            }
            case vtFloat: {
                string s = csv.GetValue();
                float f = atof(s.c_str());
                //long l = PalmDB::nativeLong(*((long*)&f));
                long l = *((long*)&f);
                bv.pushLong(l);
                break;
            }
            case vtString: {
                string s = csv.GetValue();
                bv.pushString(s, len);
                break;
            }
        }
    }
    PalmRecord rec;
    rec.len = (int)bv.size();
    rec.data = &bv[0];
    db.AddRec(rec);
    return true;
}

void usage() {
    printf(
        "orbpdb - PDB <-> CSV converter for OrbForms Designer and PocketC\n"
        "Copyright (c) 2002 OrbWorks, Inc.\n"
        "\n"
        "usage: orbpdb -[csv|pdb] [-n dbname] [-t type] [-c creator_id] [-a] [-rA[-[B]]] format srcfile destfile\n"
        "    -csv    source file is CSV (output to PDB)\n"
        "    -pdb    source file is PDB (output to CSV)\n"
        "    -n      set the output pdb name (invalid if -pdb or -a is specified)\n"
        "    -t      set the output pdb type (invalid if -pdb or -a is specified)\n"
        "    -c      set the output pdb creator (invalid if -pdb or -a is specified)\n"
        "    -a      append to output file\n"
        "    -r      specify records/rows to convert (use one of the 3 forms below)\n"
        "            -rA    convert only row A\n"
        "            -rA-   convert from row A to the end\n"
        "            -rA-B  convert from row A to row B inclusive\n"
        "            (row numbers begin with 0)\n"
        "\n"
        "    format: a format string as used for dwreadx() or DBRecord.read()\n"
        "            see orbpdb.txt for full specification\n"
        );
    exit(-1);
}

int main(int argc, char* argv[]) {
    int first = 0, last = -1;
    bool append = false;
    bool tocsv = false;
    bool error = false;
    string srcfile, destfile;
    string dbname, dbtype, dbcreator;
    string format;

    if (argc < 5)
        usage();

    int iarg = 1;
    // determine output format
    if (_stricmp(argv[iarg], "-csv") == 0) {
        tocsv = false;
    } else if (_stricmp(argv[iarg], "-pdb") == 0) {
        tocsv = true;
    } else {
        usage();
    }
    iarg++;

    if (_stricmp(argv[iarg], "-n") == 0) {
        iarg++;
        if (iarg >= argc) {
            printf("-n requires a name\n");
            usage();
        }
        dbname = argv[iarg];
        if (dbname.length() > 32) {
            printf("database name must be <= 32 characters\n");
            usage();
        }
        iarg++;
    }
    
    if (_stricmp(argv[iarg], "-t") == 0) {
        iarg++;
        if (iarg >= argc) {
            printf("-t requires a type\n");
            usage();
        }
        dbtype = argv[iarg];
        if (dbtype.length() != 4) {
            printf("database type must be 4 characters\n");
            usage();
        }
        iarg++;
    } else {
        dbtype = "data";
    }
    
    if (_stricmp(argv[iarg], "-c") == 0) {
        iarg++;
        if (iarg >= argc) {
            printf("-c requires a creator id\n");
            usage();
        }
        dbcreator = argv[iarg];
        if (dbcreator.length() != 4) {
            printf("database creator id must be 4 characters\n");
            usage();
        }
        iarg++;
    }

    if (_stricmp(argv[iarg], "-a") == 0) {
        append = true;
        iarg++;
    }

    if (_strnicmp(argv[iarg], "-r", 2) == 0) {
        if (strlen(argv[iarg]) < 3) {
            printf("-r requires arguments\n");
            usage();
        }
        char* pc = argv[iarg] + 2;
        while (isdigit(*pc)) {
            first *= 10;
            first += *pc - '0';
            pc++;
        }
        if (*pc == '-') {
            pc++;
            if (isdigit(*pc)) {
                last = 0;
                while (isdigit(*pc)) {
                    last *= 10;
                    last += *pc - '0';
                    pc++;
                }
            }
        } else {
            last = first;
        }
        if (last != -1 && first > last) {
            printf("last row must be greater than first row\n");
            usage();
        }
        iarg++;
    }
    if (argc - iarg < 3)
        usage();

    format = argv[iarg];
    srcfile = argv[iarg+1];
    destfile = argv[iarg+2];

    PalmDB db;
    CSVFile csv;

    try {
        if (tocsv) {
            if (!db.Read(srcfile.c_str()))
                throw new OrbError("unable to read source pdb");
            if (db.GetRes())
                throw new OrbError("source pdb cannot be a resource database");
            char* mode = append ? "a" : "w";
            if (!csv.Open(destfile.c_str(), mode))
                throw new OrbError("unable to open dest csv");
            if (db.GetCount() < first) {
                char buff[256];
                wsprintf(buff, "source pdb does not have first requested record (only %d records)\n", db.GetCount());
                throw new OrbError(buff);
            }

            int row = first;
            while (true) {
                if (row >= db.GetCount() || last != -1 && row > last)
                    break;
                // convert the record
                PalmRecord* prec = db.GetRec(row);
                bool res = RecordToCSV(prec, csv, format);
                error = error || !res;
                row++;
            }
        } else {
            if (!csv.Open(srcfile.c_str(), "r"))
                throw new OrbError("unable to read source csv");
            if (append) {
                // read in the current file
                if (!db.Read(destfile.c_str()))
                    throw new OrbError("unable to read dest pdb");
                if (db.GetRes())
                    throw new OrbError("dest pdb cannot be a resource database");
            } else {
                if (dbname.empty()) {
                    dbname = destfile;
                }
                if (dbcreator.empty()) {
                    throw new OrbError("creator id must be specified when creating a new database");
                }
                db.SetRes(false);
                db.SetName(dbname.c_str());
                db.SetCreatorType(dbcreator.c_str(), dbtype.c_str());
            }

            int row = 0;
            while (row < first && csv.ReadLine())
                row++;
            if (row != first) {
                char buff[256];
                wsprintf(buff, "source csv does not have %d rows, cannot start\n", first);
                throw new OrbError(buff);
            }

            while (true) {
                if (last != -1 && row > last) break;
                if (!csv.ReadLine()) break;
                bool res = CSVToRecord(csv, db, format);
                error = error || !res;
                row++;
            }

            if (!db.Write(destfile.c_str())) {
                throw new OrbError("unable to write pdb");
            }
        }
    } catch (OrbError* pe) {
        printf("OrbError: %s\n", pe->err.c_str());
        exit(-1);
    } catch (...) {
        printf("Internal error\n");
        exit(-1);
    }

    if (error) {
        printf("There were errors during the conversion\n");
    }
    return 0;
}

