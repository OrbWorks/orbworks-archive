#include "..\compiler\compiler.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("deprc file.prc\n");
        return -1;
    }

    PalmDB db;
    if (db.Read(argv[1])) {
        int nRecs = db.GetCount();
        for (int i=0;i<nRecs;i++) {
            PalmResRecord* prec = db.GetResRec(i);
            char buff[32];
            wsprintf(buff, "%s%0.4d.bin", prec->type.c_str(), prec->id);
            FILE* f = fopen(buff, "wb");
            if (!f)
                continue;
            fwrite(prec->data, 1, prec->len, f);
            fclose(f);
        }
    }

    return 0;
}
