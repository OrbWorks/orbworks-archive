char lower(char);
bool isdigit(char);
bool isspace(char);
bool isalpha(char);
bool isalnum(char);

// PalmOS doesn't support hex
long atoi(char*);
float atof(char*);
void ftoa(float, char*);
char* itoa(long n, char* a, int r);
//#define itoa(a,b,c) StrIToA(b,a)

int strcmp(const char* a, const char* b);
#define stricmp(a,b) StrCaselessCompare(a,b)
#define strncmp(a, b, n) StrNCompare(a,b,n)
char* strdup(char*);

#define strlen(a) StrLen(a)
#define strcat(a,b) StrCat(a,b)
#define strncpy(a, b, c) StrNCopy(a, b, c)
#define strcpy(a,b) StrCopy(a,b)
#define sprintf StrPrintF

#define free(a) MemPtrFree((void*)a)
#define h_free(a) MemHandleFree((VoidHand)a)

#define malloc(a) MemPtrNew(a)
#define h_malloc(a) MemHandleNew(a)

#define memcpy(a,b,c) MemMove(a,b,c)
#define memmove(a,b,c) MemMove(a,b,c)
#define memset(dest,len,val) MemSet(dest,len,val)