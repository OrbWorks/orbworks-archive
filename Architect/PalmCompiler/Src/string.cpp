#include "string.h"
#include "stdcover.h"

string::string() {
    len = 0;
    str = 0;
    buf[0] = '\0';
}

string::string(const string& s) {
    len = 0;
    str = 0;
    buf[0] = '\0';
    assign(s);
}

string::string(const char* s) {
    len = 0;
    str = 0;
    buf[0] = '\0';
    assign(s);
}

string::~string() {
    dealloc();
}

string& string::operator=(const string& s) {
    assign(s);
    return *this;
}

string& string::operator+=(const string& s) {
    append(s.c_str());
    return *this;
}

string& string::operator+=(const char c) {
    char ca[2] = { c, 0 };
    append(ca);
    return *this;
}

string& string::operator+=(const char* s) {
    append(s);
    return *this;
}

void string::assign(const char* src, int slen) {
    dealloc();
    if (slen < bufSize - 1) {
        memcpy(buf, src, slen);
        buf[slen] = '\0';
    } else {
        str = new char[slen + 1];
        memcpy(str, src, slen);
        str[slen] = '\0';
    }
    len = slen;
}

void string::assign(const char* src) {
    dealloc();
    int slen = strlen(src);
    if (slen < bufSize - 1) {
        memcpy(buf, src, slen + 1);
    } else {
        str = new char[slen + 1];
        memcpy(str, src, slen + 1);
    }
    len = slen;
}

void string::assign(const string& s) {
    if (this == &s) return;
    if (s.str) {
        assign(s.str);
    } else {
        dealloc();
        len = s.len;
        memcpy(buf, s.buf, len + 1);
    }
}

bool string::equals(const char* s) const {
    return strcmp(c_str(), s) == 0;
}

bool string::equals(const string& s) const {
    if (s.len == len) return strcmp(c_str(), s.c_str()) == 0;
    return false;
}

void string::dealloc() {
    if (str)
        delete[] str;
    str = NULL;
}

void string::append(const char* s) {
    int slen = strlen(s);
    append(s, slen);
}

void string::append(const char* s, int slen) {
    if (len + slen < bufSize - 1) {
        memcpy(&buf[len], s, slen);
        buf[len + slen] = '\0';
    } else {
        char* newstr = new char[len + slen + 1];
        strcpy(newstr, c_str());
        memcpy(newstr + len, s, slen);
        newstr[len + slen] = '\0';
        dealloc();
        str = newstr;		
    }
    len += slen;
}

int string::find_first_of(const char c) {
    const char* p = c_str();
    for (int i=0;i<len;i++) {
        if (p[i] == c) return i;
    }
    return npos;
}

string string::substr(int start, int count) {
    string str;
    if (count == npos) {
        str.assign(c_str() + start);
    } else {
        str.append(c_str() + start, count);		
    }
    return str;
}

bool operator==(const string& a, const string& b) { return a.equals(b); }
bool operator==(const string& a, const char* b) { return a.equals(b); }
bool operator==(const char* a, const string& b) { return b.equals(a); }

bool operator!=(const string& a, const string& b) { return !a.equals(b); }
bool operator!=(const string& a, const char* b) { return !a.equals(b); }
bool operator!=(const char* a, const string& b) { return !b.equals(a); }

string operator+(const char* a, const string& b) { string res(a); res.append(b.c_str()); return res; }
string operator+(const string& a, const string& b) { string res(a); res.append(b.c_str()); return res; }
string operator+(const string& a, const char* b) { string res(a); res.append(b); return res; }
