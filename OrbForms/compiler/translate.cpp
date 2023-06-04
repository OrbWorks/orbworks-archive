#ifdef WIN32
#include "compiler.h"

string EscapeString(string str);
string UnescapeString(string str);

const char* MyPathFindFileName(const char* filename) {
    const char* filepart = filename + strlen(filename);
    while (filepart > filename) {
        if (*filepart == '/' || *filepart == '\\')
            return ++filepart;
        filepart--;
    }
    return filename;
}

char* GetNextLine(char* file, string& line) {
    line = "";
    while (*file && *file != '\r' && *file != '\n') line += *file++;
    if (*file == '\r') file++;
    if (*file == '\n') file++;
    return file;
}

void Compiler::loadTranslation(const char* filename) {
    translationFile = filename;
    nUsedTranslationEntries = 0;
    Source* fileSource = GetFile(filename, false);
    if (!fileSource) {
        return;
    }

    char* file = fileSource->GetSrc();
    TranslationEntry entry;
    int state = 0;
    int iLine = 0;
    
    try {
        while (*file) {
            string line;
            file = GetNextLine(file, line);
            iLine++;
            if (state != 3 && line.empty()) continue; // skip blank lines

            switch (state) {
                case 0: { // compiler comment
                    if (line.find("//") == string::npos)
                        c_error("invalid translation file", filename, iLine);
                    entry.compilerComment = ""; // remove compiler comment
                    break;
                }
                case 1: {
                    if (line.find("//") == string::npos)
                        c_error("invalid translation file", filename, iLine);
                    if (line.length() > 2 && line[2] == ' ')
                        line = line.substr(3, line.length() - 1);
                    else
                        line = line.substr(2, line.length() - 1);
                    entry.userComment = line;
                    break;
                }
                case 2: {
                    if (line.length() < 2 || line[0] != '"' || line[line.length()-1] != '"')
                        c_error("invalid translation file", filename, iLine);
                    line = line.substr(1, line.length() - 2);
                    entry.base = UnescapeString(line);
                    break;
                }
                case 3: {
                    if (line.length() < 2 || line[0] != '"' || line[line.length()-1] != '"')
                        c_error("invalid translation file", filename, iLine);
                    line = line.substr(1, line.length() - 2);
                    entry.translation = UnescapeString(line);
                    entry.order = -1;
                    translationTable[entry.base] = entry;
                    break;
                }
            }
            state = (state + 1) % 4;
        }
    } catch (...) {
        delete fileSource;
        throw;
    }
    delete fileSource;
}

void Compiler::writeTranslation() {
    if (translationFile.empty()) return;

    FILE* file = fopen(translationFile.c_str(), "wt");
    if (file) {
        vector<string> order;
        order.resize(translationTable.size(), string(""));
        int iUnused = nUsedTranslationEntries;
        map<string, TranslationEntry>::iterator iter = translationTable.begin();
        while (iter != translationTable.end()) {
            TranslationEntry& entry = iter->second;
            if (entry.order == -1) {
                entry.order = iUnused++;
            }
            order[entry.order] = entry.base;
            iter++;
        }

        for (int i = 0; i < order.size(); i++) {
            TranslationEntry& entry = translationTable[order[i]];
            fprintf(file, "// %s\n", entry.compilerComment.empty() ? "unused" : entry.compilerComment.c_str());
            fprintf(file, "// %s\n", entry.userComment.c_str());
            fprintf(file, "\"%s\"\n", EscapeString(entry.base).c_str());
            fprintf(file, "\"%s\"\n\n", EscapeString(entry.translation).c_str());
        }
        fclose(file);
    }
}

string Compiler::translateString(const char* str) {
    string comment;
    comment = MyPathFindFileName(lex.incStack[lex.incLevel].name);
    comment += " @ ";
    char buff[10];
    _itoa(lex.currLine, buff, 10);
    comment += buff;
    return translateString(str, comment.c_str());
}

string Compiler::translateString(const char* str, const char* name, const char* comment) {
    string fullComment = name;
    fullComment += ": ";
    fullComment += comment;
    return translateString(str, fullComment.c_str());
}

bool IsStringTranslatable(const char* str) {
    while (*str) {
        if (isalnum(*str) || *str > 127) return true;
        str++;
    }
    return false;
}

string Compiler::translateString(const char* str, const char* comment) {
    if (translationFile.empty()) return string(str);
    if (!IsStringTranslatable(str)) return string(str);

    if (translationTable.find(str) != translationTable.end()) {
        if (translationTable[str].compilerComment.empty()) {
            translationTable[str].compilerComment = comment;
            translationTable[str].order = nUsedTranslationEntries++;
        }
        if (translationTable[str].translation.empty())
            return string(str);
        else
            return translationTable[str].translation;
    }
    TranslationEntry entry;
    entry.base = str;
    entry.compilerComment = comment;
    entry.order = nUsedTranslationEntries++;
    translationTable[str] = entry;
    return string(str);
}
#endif
