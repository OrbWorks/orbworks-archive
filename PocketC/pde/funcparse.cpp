#include "stdafx.h"
#include "pde.h"

void _nextToken();

char info[0x10000];
int iInfo;
static void InsertFunc(char* name, int line) {
    char buff[256];
    wsprintf(buff, "%s()\t%d\n", name, line);
    int len = strlen(buff);
    if (iInfo + len >= 0x10000) return; // don't overrun the buffer
    strcpy(&info[iInfo], buff);
    iInfo += len;
}

char* GetFuncs(char* src) {
    info[0] = 0;
    iInfo = 0;
    tok.id = tiIf;
    currLine = 1;
    nMacros = 0;
    char name[33];
    int line;
    enum Mode { modeTop, modeFoundName, modeParamList, modeBody } mode = modeTop;
    int braceLevel = 0;
    source = src;
    bNoErrorPopup = true;
    
    try {
        while (tok.id != tiEnd) {
            _nextToken();
            switch (mode) {
            case modeTop:
                if (tok.id == tiIdent) {
                    strcpy(name, tok.value);
                    mode = modeFoundName;
                    line = tok.line;
                }
                break;

            case modeFoundName:
                if (tok.id == tiLParen) {
                    mode = modeParamList;
                } else
                    mode = modeTop;
                break;

            case modeParamList:
                if (tok.id == tiRParen) {
                    _nextToken();
                    if (tok.id == tiSemiColon)
                        mode = modeTop;
                    else if (tok.id == tiLBrace) {
                        mode = modeBody;
                        braceLevel = 1;
                        // Add Function
                        InsertFunc(name, line);
                    } else mode = modeTop; // ???
                }
                break;

            case modeBody:
                if (tok.id == tiLBrace) braceLevel++;
                else if (tok.id == tiRBrace) braceLevel--;
                if (braceLevel == 0) mode = modeTop;
                break;
            }
        }
    }
    catch (...) {
        //MessageBox(NULL, "Exception thrown", "GetFuncs()", MB_OK);
    }
    bNoErrorPopup = false;
    return info;
    //MessageBox(NULL, info, "User Functions", MB_OK);
}
