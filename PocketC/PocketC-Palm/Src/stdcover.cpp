#include "PocketC.h"

bool isdigit(char c) {
    if (c >='0' && c <='9') return true;
    return false;
}

char lower(char c) {
    if (c >= 'A' && c <= 'Z') return c + 32;
    else return c;
}

bool isspace(char c) {
    if (c==' ' || c=='\n' || c=='\t' || c=='\r') return true;
    return false;
}

bool isalpha(char c) {
    if ((c>='A' && c<='Z') || (c>='a' && c<='z')) return true;
    return false;
}

bool isalnum(char c) {
    if (isdigit(c) || isalpha(c)) return true;
    return false;
}

char* itoa(long n, char* a, short) {
    if (n != 0x80000000)
        return StrIToA(a, n);
    else
        strcpy(a, "-2147483648"); // this is fixed in OS 3.5
    return a;
}
        

long atoi(char* src) {
    char neg=0;
    long res = 0;
    while (isspace(*src)) src++;
    if (*src=='-') { neg=1; src++; }
    else if (*src=='+') src++;
    
    res=0;
    if (*src == '0')
        if (*++src == 'x' || *src=='X') { // Hex number
            src++;
            while (isdigit(*src) || (lower(*src) >= 'a' && lower(*src) <= 'f')) {
                res*=16;
                if (isdigit(*src)) res+=(*src-'0');
                else res+=10+(lower(*src)-'a');
                src++;
            }
            return res;
        }
                 
    while (isdigit(*src)) {
        res*=10;
        res+=(*src-'0');
        src++;
    }
    return (neg ? -res : res);
}

long h_atoi(Handle h_src) {
    long res;
    char* src = (char*)MemHandleLock(h_src);
    res = atoi(src);
    MemHandleUnlock(h_src);
    return res;
}
    
void ftoa(float f, char* s) {
    FlpCompDouble d;
    d.d = f;
    FlpFToA(d.fd, s);
}
    

float atof(char* src) {
    float res;
    FlpCompDouble d;
    if (*src == '+')
        src++; // skip leading '+'
    d.fd = FlpAToF(src);
    res = d.d;
    return res;
}

float h_atof(Handle h_src) {
    float res;
    char* src = (char*)MemHandleLock(h_src);
    if (*src=='+') // skip leading +
        src++;
    res = atof(src);
    MemHandleUnlock(h_src);
    return res;
}

short h_strcmp(Value* a, Value* b) {
    short res;
    char* aa = LockString(a); //(char*)MemHandleLock(a);
    char* bb = LockString(b); //(char*)MemHandleLock(b);
    res = strcmp(aa,bb);
    UnlockString(a); //MemHandleUnlock(a);
    UnlockString(b); //MemHandleUnlock(b);
    return res;
}

char* strdup(char* src) {
    short len = strlen(src) + 1;
    char* dest = (char*)malloc(len);
    if (!dest) return NULL;
    strcpy(dest, src);
    return dest;
}

Handle h_strdup(Handle h_src) {
    char* src = (char*)MemHandleLock(h_src);
    short len = strlen(src) + 1;
    Handle h_dst = (Handle)MemHandleNew(len);
    if (!h_dst)
        return NULL;
    char* dest = (char*)MemHandleLock(h_dst);
    strcpy(dest, src);
    MemHandleUnlock(h_src);
    MemHandleUnlock(h_dst);
    return h_dst;
}

Handle h_strdup3(char* str) {
    Handle res = (Handle)h_malloc(strlen(str)+1);
    if (!res)
        return NULL;
    char* dest = (char*)MemHandleLock(res);
    strcpy(dest, str);
    MemHandleUnlock(res);
    return res;
}

/*
Handle h_strdup2(char* str) {
    Handle res;
    char* dst;
    NewString(strlen(str)+1, &res, &dst);
    if (!res)
        return NULL;
    strcpy(dst, str);
    UnlockString(res);
    return res;
}
*/

short h_strlen(Handle h_str) {
    char* str = (char*)MemHandleLock(h_str);
    short res = strlen(str);
    MemHandleUnlock(h_str);
    return res;
}

void h_strcat(Handle a, Handle b) {
    char* aa = (char*)MemHandleLock(a);
    char* bb = (char*)MemHandleLock(b);
    strcat(aa,bb);
    MemHandleUnlock(a);
    MemHandleUnlock(b);
}

short h_strcpy(Handle a, Handle b) {
    char* aa = (char*)MemHandleLock(a);
    char* bb = (char*)MemHandleLock(b);
    strcpy(aa,bb);
    short ret = strlen(aa);
    MemHandleUnlock(a);
    MemHandleUnlock(b);
    return ret;
}

int strcmp(const char * src, const char * dst) {
    int ret = 0;
    while (!(ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst)
            ++src, ++dst;
    return ret;
}
