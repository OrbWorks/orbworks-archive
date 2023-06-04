char lower(char);
bool isdigit(char);
bool isspace(char);
bool isalpha(char);
bool isalnum(char);

// PalmOS doesn't support 32-short's in their StrAToI function
long atoi(char*);
long h_atoi(Handle);
float atof(char*);
float h_atof(Handle);
void ftoa(float, char*);
char* itoa(long n, char* a, short r);
//#define itoa(a,b,c) StrIToA(b,a)

//#define strcmp(a,b) StrCompareAscii(a,b)
//#define strcmp(a,b) StrCompare(a,b)
int strcmp(const char* a, const char* b);
short h_strcmp(Value*, Value*);

char* strdup(char*);
Handle h_strdup(Handle);
//Handle h_strdup2(char*);
Handle h_strdup3(char*);

#define strlen(a) StrLen(a)
short h_strlen(Handle);

#define strcat(a,b) StrCat(a,b)
void h_strcat(Handle, Handle);

#define strncpy(a, b, c) StrNCopy(a, b, c)
#define strcpy(a,b) StrCopy(a,b)
short h_strcpy(Handle, Handle);

#define free(a) MemPtrFree((VoidPtr)a)
#define h_free(a) MemHandleFree((VoidHand)a)

#define malloc(a) MemPtrNew(a)
#define h_malloc(a) MemHandleNew(a)

#define memcpy(a,b,c) MemMove(a,b,c)
