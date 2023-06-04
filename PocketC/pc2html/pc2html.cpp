//-------------------------------------------------------
//
// pc2html.cpp - HTML generator for PocketC source code
//
// (c) 1998-2003 OrbWorks
//
//-------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <malloc.h>
#include <ctype.h>
#include <string.h>

struct FontInfo {
    int color;
    bool bold;
    bool italic;
};

enum TokenType { ttString, ttKeyword, ttFunc, ttIdent, ttConstant, ttComment, ttOper };
FontInfo fi[] = {
{ 0x000080, false, false }, // ttString
{ 0x0000ff, false, false }, // ttKeyword
{ 0x000000, true, false }, // ttFunc
{ 0x000000, false, false }, // ttIdent
{ 0x0000a0, false, false }, // ttConstant
{ 0x008000, false, false }, // ttComment
{ 0x000000, false, false }, // ttOperators
};

char* keywords[] = {"int", "char", "string", "pointer", "float", "for", "while",
    "do", "return", "break", "continue", "include", "library", "define", "ifdef",
    "ifndef", "else", "if", "endif", "#include", "#define", "#ifdef", "#ifndef",
    "#else", "#endif", "false", "true", "null" };
int nKeywords = sizeof(keywords)/sizeof(char*);

char* funcs[] = {
    "alert",
    "puts",
    "gets",
    "clear",
    "strlen",
    "substr",
    "strleft",
    "strright",
    "strupr",
    "strlwr",
    "cos",
    "sin",
    "tan",
    "acos",
    "asin",
    "atan",
    "cosh",
    "sinh",
    "tanh",
    "acosh",
    "asinh",
    "atanh",
    "exp",
    "log",
    "log10",
    "sqrt",
    "pow",
    "atan2",
    "rand",
    "random",
    "beep",
    "tone",
    "graph_on",
    "graph_off",
    "clearg",
    "text",
    "line",
    "rect",
    "title",
    "textattr",
    "wait",
    "waitp",
    "getc",
    "penx",
    "peny",
    "hex",
    "frame",
    "time",
    "date",
    "seconds",
    "ticks",
    "confirm",
    "mathlib",
    "frame2",
    "dbopen",
    "dbcreate",
    "dbclose",
    "dbwrite",
    "dbread",
    "dbpos",
    "dbseek",
    "dbbackup",
    "dbdelete",
    "event",
    "key",
    "pstate",
    "bstate",
    "mmnew",
    "mmfind",
    "mmopen",
    "mmclose",
    "mmputs",
    "mmgetl",
    "mmeof",
    "mmrewind",
    "mmdelete",
    "strstr",
    "bitmap",
    "sleep",
    "resetaot",
    "getsysval",
    "format",
    "seropen",
    "serclose",
    "sersend",
    "serrecv",
    "serdata",
    "textalign",
    "launch",
    "saveg",
    "restoreg",
    "serbuffsize",
    "malloc",
    "free",
    "settype",
    "typeof",
    "clipget",
    "clipset",
    "mmcount",
    "dbenum",
    "dbrec",
    "dbnrecs",
    "dbreadx",
    "dbwritex",
    "dbsize",
    "dbdelrec",
    "dbremrec",
    "dbarcrec",
    "dberase",
    "memcpy",
    "hookhard",
    "hookmenu",
    "exit",
    "strtoc",
    "ctostr",
    "getsd",
    "atexit",
    "textwidth",
    "version",
    "getsi",
    "sersenda",
    "serrecva",
    "unpack",
    "malloct",
    "getsm",
    "deepsleep",
    "dbgetcat",
    "dbsetcat",
    "dbcatname",
    "dbcreatex",
    "dbmoverec",
    "dbinfo",
    "dbtotalsize",
    "hooksilk",
    "hooksync",
    "serwait",
    "dbrename",
    "dbsetcatname",
    "dbwritexc",
    "dbgetappinfo",
    "dbsetappinfo",
    "dbreadxc",
    "dbmovecat",
};

int nFuncs = sizeof(funcs)/sizeof(char*);
int tabwidth = 0;

FILE* out;
char buff[8192];
char* source = NULL, *lToken;
char toLower(char c) {
    if (c>='A' && c<='Z') return c-'A'+'a';
    return c;
}

void outHTML(char* text) {
    while (*text) {
        if (*text=='<') fputs("&lt;", out);
        else if (*text=='>') fputs("&gt;", out);
        else fputc(*text, out);
        text++;
    }
}

void outItem(FontInfo fi) {
    static FontInfo lfi = { 0x12345678, false, false };
    static bool first = true;

    int len = source-lToken;
    if (!len) return;
    strncpy(buff, lToken, len);
    buff[len] = 0;

    bool changed = (0 != memcmp(&lfi, &fi, sizeof(FontInfo)));
    if (changed && !first) {
        if (lfi.italic) fprintf(out, "</i>");
        if (lfi.bold) fprintf(out, "</b>");
        fprintf(out, "</font>");
    }
    if (changed) {
        fprintf(out, "<font color=%06x>", fi.color);
        if (fi.bold) fprintf(out, "<b>");
        if (fi.italic) fprintf(out, "<i>");
        memcpy(&lfi, &fi, sizeof(FontInfo));
    }
    outHTML(buff);
    first = false;
    lToken = source;
}

void outWhite() {
    // Output whitespace
    int i=0;
    while (lToken < source) {
        if (*lToken == '\t' && tabwidth)
            for (int s=0;s<tabwidth;s++)
                buff[i++] = ' ';
        else
            buff[i++] = *lToken;
        lToken++;
    }

    buff[i] = 0;
    outHTML(buff);
    lToken = source;
}

char* GetFile(char* file) {
    FILE* f = fopen(file, "rb");
    if (!f) return NULL;

    int len = filelength(fileno(f));
    char* data = (char*)malloc(len+1);
    fread(data, 1, len, f);
    data[len] = 0;
    fclose(f);
    return data;
}

inline char ch() { return *source; }
inline char gch() { return *source++; }
inline char nch() { return *(source+1); }
inline void ugch() { source--; }

void process(char* file) {
    lToken = source;
    fprintf(out, "<html>\n<title>%s</title>\n<body>\n<b>%s</b> ("
        "<a href=\"%s\">plain text</a>)\n<hr>\n<pre>\n", file, file, file);
    while (*source) {
        while (isspace(*source))
            source++;
        if (source > lToken) {
            outWhite();
        }

        if (ch() == '/' && (nch() == '/' || nch() == '$')) {
            while (ch()!='\r') gch();
            outItem(fi[ttComment]);
            continue;
        }

        if (ch() == '/' && nch() == '*') {
            while (!(ch()=='*' && nch()=='/')) gch();
            gch();
            gch();
            outItem(fi[ttComment]);
            continue;
        }

        if (isdigit(ch())) {
            bool isHex = false;
            while (isdigit(ch())) {
                gch();
                char l = toLower(ch());
                if (l=='e' && nch()=='-') {
                    gch(); gch();
                } else if (l=='e' || ch()=='.')
                    gch();
                else if (l=='x') {
                    gch();
                    isHex = true;
                } else if (isHex && ('a' <= l && l <= 'f'))
                    gch();
            }
            outItem(fi[ttConstant]);
            continue;
        }

        if (isalpha(ch()) || ch()=='_' || ch()=='#') {
            gch();
            while (isalnum(ch()) || ch()=='_') gch();

            int i;
            for (i=0;i<nKeywords;i++) {
                if (!strncmp(lToken, keywords[i], source-lToken)
                        && (unsigned)(source-lToken) == strlen(keywords[i])) {
                    outItem(fi[ttKeyword]);
                    goto endOfLoop;
                }
            }
            for (i=0;i<nFuncs;i++) {
                if (!strncmp(lToken, funcs[i], source-lToken)
                        && (unsigned)(source-lToken) == strlen(funcs[i])) {
                    outItem(fi[ttFunc]);
                    goto endOfLoop;
                }
            }
            outItem(fi[ttIdent]);
            continue;
        }

        if (ch()=='\"') {
            gch();
            while (ch()) {
                if (ch()=='\\') {
                    gch();
                    gch();
                    continue;
                }
                if (ch()=='\"') {
                    gch();
                    break;
                }
                gch();
            }
            outItem(fi[ttString]);
            continue;
        }

        if (ch()=='\'') {
            gch();
            while (ch()) {
                if (ch()=='\\') {
                    gch();
                    gch();
                    continue;
                }
                if (ch()=='\'') {
                    gch();
                    break;
                }
                gch();
            }
            outItem(fi[ttString]);
            continue;
        }

        while (ch() && !isalnum(ch()) && !isspace(ch()) && ch() != '\"' &&
                ch() != '\'' && ch() != '_') gch();
        if (source != lToken)
            outItem(fi[ttOper]);
endOfLoop: ;
    }

    fprintf(out, "</pre>\n</body>\n</html>");
}

void usage() {
    fprintf(stderr, "PocketC source to HTML\nusage: pc2html [-tabsize] source [output]\n\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }
    
    int i = 1;
    out = stdout;
    char* srcName;

    while (i < argc) {
        if (argv[i][0] == '-') {
            tabwidth = argv[i][1] - '0';
            goto next_arg;
        }
        if (!source) {
            source = GetFile(argv[i]);
            srcName = argv[i];
            if (!source) {
                fprintf(stderr, "Unable to open '%s'\n", argv[i]);
                return 1;
            }
            goto next_arg;
        }
        out = fopen(argv[i], "wb");
        if (!out) {
            fprintf(stderr, "Unable to open '%s'\n", argv[i]);
            return 1;
        }
        break;
next_arg:
        i++;
    }

    process(srcName);
    if (out != stdout)
        fclose(out);
    
    return 0;
}
