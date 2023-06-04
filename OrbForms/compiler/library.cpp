#include "compiler.h"

int Compiler::addLibrary(string name, bool bPocketC) {
    for (int i=0;i<libraries.size();i++) {
        if (libraries[i].name == name)
            return i;
    }
    Library lib;
    lib.name = name;
    lib.marked = false;
    lib.bPocketC = bPocketC;
    libraries.push_back(lib);
    return libraries.size() - 1;
}

#ifdef WIN32
void Compiler::addLibraryContents(string name, PalmDB& db, int id) {
    // open the library
    PalmDB libDb;
    string fileName = name + ".prc";
    string filePath = lex.FindFile(fileName.c_str(), true);
    if (!libDb.Read(filePath.c_str())) {
        string err = "unable to open library '" + fileName + "'";
        c_error(err.c_str(), -1);
    }

    for (int i=0;i<libDb.GetCount();i++) {
        PalmResRecord* pRec = libDb.GetResRec(i);
        if (pRec->id == 0 && pRec->type == "ofNA") {
            // this is the main code resource, so add it as 'id'
            pRec->id = id;
            if (db.GetResRec(pRec->type.c_str(), pRec->id)) {
                char buff[16];
                _itoa(pRec->id, buff, 10);
                string err = "resource conflict adding library '" + name +
                    "' (" + pRec->type + "," + string(buff) + ")";
                c_error(err.c_str(), -1);
            }
            db.AddResRec(*pRec);
            pRec->id = 0;
        } else {
            if (db.GetResRec(pRec->type.c_str(), pRec->id)) {
                char buff[16];
                _itoa(pRec->id, buff, 10);
                string err = "resource conflict adding library '" + name +
                    "' (" + pRec->type + "," + string(buff) + ")";
                c_error(err.c_str(), -1);
            }
            db.AddResRec(*pRec);
        }
    }
}
#endif
