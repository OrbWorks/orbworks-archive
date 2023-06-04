#define assert(x) { ErrFatalDisplayIf(!(x), #x); }
//#define assert(x)
#define byref

char lower(char);
#define tolower(a) lower(a)
bool isdigit(char);
bool isspace(char);
bool isalpha(char);
bool isalnum(char);

// PalmOS doesn't support 32-short's in their StrAToI function
long atoi(const char*);
float atof(char*);
void ftoa(float, char*);
char* itoa(long n, char* a, short r);
#define _itoa itoa

int strcmp(const char* a, const char* b);
char* strdup(char*);
#define _strdup strdup
#define strlen(a) StrLen(a)
#define strcat(a,b) StrCat(a,b)
#define strncpy(a, b, c) StrNCopy(a, b, c)
#define strcpy(a,b) StrCopy(a,b)
#define memcpy(a,b,c) MemMove(a,b,c)
#define memset(a,b,c) MemSet(a,c,b)
#define malloc(a) MemPtrNew(a)
#define free(a) MemPtrFree(a)
#define sprintf StrPrintF

inline void* operator new(unsigned long, void* addr) { return addr; }

