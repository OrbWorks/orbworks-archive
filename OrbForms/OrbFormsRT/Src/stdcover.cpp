#include "OrbFormsRT.h"

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

char* itoa(long n, char* a, int) {
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

char* strdup(char* src) {
    int len = strlen(src) + 1;
    char* dest = (char*)malloc(len);
    if (!dest) return NULL;
    strcpy(dest, src);
    return dest;
}

int strcmp(const char * src, const char * dst) {
    int ret = 0;
    while (!(ret = *(unsigned char *)src - *(unsigned char *)dst) && *dst)
            ++src, ++dst;
    return ret;
}
