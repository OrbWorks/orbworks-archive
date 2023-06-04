#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <malloc.h>
#include "..\..\OrbForms\compiler\palmdb.h"

HWND hPalm=0;

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM) {
   char buff[256];
   char geo[] = "Artillery";
   //char geo[] = "Palm OS� Emulator";

   GetWindowText(hWnd, buff, 256);
   if (!strcmp(buff, geo)) {
      hPalm = hWnd;
      return FALSE;
   }
   return TRUE;
}

char* GetFile(char* file) {
   FILE* f = fopen(file, "rt");
   if (!f) return NULL;

   int len = filelength(fileno(f));
   char* data = (char*)malloc(len+1);
   int alen = fread(data, 1, len, f);
   data[alen] = 0;
   fclose(f);
   return data;
}

void genStroke(char c, bool up) {
   SHORT s = VkKeyScan(c);
   SetForegroundWindow(hPalm);
   if (s & 0x0100 && !up)
      keybd_event(VK_LSHIFT, 0, 0, 0);
   keybd_event(VkKeyScan(c), 0, (up ? KEYEVENTF_KEYUP : 0), 0);
   if (s & 0x0100 && up)
      keybd_event(VK_LSHIFT, 0, KEYEVENTF_KEYUP, 0);
   Sleep(20);
}

void memoload(char* name) {
   HANDLE hProc = GetCurrentProcess();
   SetProcessAffinityMask(hProc, 1);

   EnumWindows((WNDENUMPROC)EnumProc, 0);
   if (!hPalm) {
      printf("Unable to find Palm Emulator window\n");
      return;
   }
   char* data = GetFile(name);
   if (!data) {
      printf("Error reading file\n");
      return;
   }
   while (*data) {
      genStroke(*data, false);
      genStroke(*data, true);
      data++;
   }
}
struct DROPFILES
{
DWORD pFiles; // offset of file list
POINT pt; // drop point (client coords)
BOOL fNC; // is it on NonClient area and pt is in screen coords
BOOL fWide; // wide character flag
};

void memodrop(char* name) {
   EnumWindows((WNDENUMPROC)EnumProc, 0);
   if (!hPalm) {
      printf("Unable to find Palm Emulator window\n");
      return;
   }
   DWORD dwSize = sizeof(DROPFILES) + strlen(name) + 2;
   HGLOBAL hMem = GlobalAlloc( GMEM_DDESHARE | GMEM_SHARE | GHND, dwSize );

   DROPFILES* pdf = (DROPFILES*)GlobalLock(hMem);
   pdf->pFiles = sizeof(DROPFILES);
   pdf->pt.x = 0;
   pdf->pt.y = 0;
   pdf->fNC = FALSE;
   pdf->fWide = FALSE;
   memcpy(pdf + 1, name, strlen(name) + 1);
   *((char*)(pdf + 1) + strlen(name) + 1) = 0;
   GlobalUnlock(hMem);

   SendMessage(hPalm, WM_DROPFILES, (WPARAM)hMem, 0);
}

void memoadd(char* dbname, char* mname) {
    PalmDB db;
    if (db.Read(dbname)) {
        PalmRecord rec;
        rec.data = (byte*)GetFile(mname);
        if (rec.data) {
            rec.len = strlen((char*)rec.data) + 1;
            db.AddRec(rec);
        } else {
            printf("unable to open memo '%s'\n", mname);
        }
        free(rec.data);
        db.Write(dbname);
    } else {
        printf("unable to open database '%s'\n", dbname);
    }
}

void memoextract(char* dbname, char* name, char* title) {
    PalmDB db;
    if (db.Read(dbname)) {
        int id = 0;
        if (isdigit(title[0])) {
            id = atoi(title);
        } else {
            // search for memo
            PalmRecord* pRec = NULL;
            bool found = false;
            for (id=0; pRec=db.GetRec(id); id++) {
                if (strncmp((char*)pRec->data, title, strlen(title)) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                id = -1;
                printf("Unable to find memo '%s'\n", title);
            }
        }
        
        if (id != -1) {
            PalmRecord* pRec = db.GetRec(id);
            if (pRec) {
                FILE* file = fopen(name, "w");
                if (file) {
                    fwrite(pRec->data, 1, pRec->len, file);
                    fclose(file);
                } else {
                    printf("Unable to open '%s'", name);
                }
            } else {
                printf("Unable to read memo %d\n", id);
            }
        }
    } else {
        printf("unable to open database '%s'\n", dbname);
    }
}

void memolist(char* dbname) {
    PalmDB db;
    if (db.Read(dbname)) {
        PalmRecord* pRec = NULL;
        int id;
        for (id=0;pRec=db.GetRec(id);id++) {
            char buff[1024];
            int i;
            for (i=0;i<1024 && i<pRec->len && pRec->data[i] != '\n';i++)
                buff[i] = pRec->data[i];
            buff[i] = '\0';
            printf("%3d: %s\n", id, buff);
        }
    } else {
        printf("unable to open database '%s'\n", dbname);
    }
}

void usage() {
    printf(
        "usage: memo -load filename\n"
        "       memo -add filename [dbname=MemoDB.pdb]\n"
        "       memo -extract filename index [dbname=MemoDB.pdb]\n"
        "       memo -extract filename \"first line\" [dbname=MemoDB.pdb\n"
        "       memo -list [dbname=MemoDB.pdb]\n"
        "       memo -drop filename\n"
    );
}

enum Verb { vLoad, vAdd, vExtract, vList, vDrop, vLast };
char* verbText[] = { "load", "add", "extract", "list", "drop" };

bool argCheck(Verb v, int argc, int req) {
    if (argc != req + 2) {
        printf("%s requires %d args\n", verbText[v], req);
        usage();
        return false;
    }
    return true;
}

void main(int argc, char* argv[]) {
    if (argc < 2 || argv[1][0] !='-') {
        usage();
        return;
    }

    Verb v;
    int iv;
    for (iv=vLoad;iv<vLast;iv++) {
       if (0 == stricmp(verbText[iv], &argv[1][1]))
           break;
    }
    v = (Verb)iv;
    if (v == vLast) {
        puts("unknown verb\n");
        usage();
        return;
    }

    switch (v) {
        case vLoad:
            if (argCheck(vLoad, argc, 1)) {
                memoload(argv[2]);
            }
            break;

        case vDrop:
            if (argCheck(vDrop, argc, 1)) {
                memodrop(argv[2]);
            }
            break;

        case vAdd:
            if (argc == 3) {
                memoadd("MemoDB.pdb", argv[2]);
            } else if (argc == 4) {
                memoadd(argv[3], argv[2]);
            } else {
                puts("add requires 1 or 2 args");
                usage();
            }
            break;

        case vExtract:
            if (argc == 4) {
                memoextract("MemoDB.pdb", argv[2], argv[3]);
            } else if (argc == 5) {
                memoextract(argv[4], argv[2], argv[3]);
            } else {
                puts("extract requires 2 or 3 args");
                usage();
            }
            break;

        case vList:
            if (argc == 2) {
                memolist("MemoDB.pdb");
            } else if (argc == 3) {
                memolist(argv[2]);
            } else {
                puts("list requires 0 or 1 args");
                usage();
            }
            break;
    }
}
