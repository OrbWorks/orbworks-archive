#include "stdafx.h"

CSVFile::CSVFile() {
    file = NULL;
    line[0] = '\0';
}

CSVFile::~CSVFile() {
    if (file) fclose(file);
}

bool CSVFile::Open(const char* name, const char* mode) {
    file = fopen(name, mode);
    return (file != NULL);
}
    
bool CSVFile::ReadLine() {
    if (fgets(line, 2048, file)) {
        curr = line;
        return true;
    } else
        return false; // end of file
}

/*
static void unescape(char* str) {
   char temp[1024];
   int i=0,j=0;
   while (str[i]) {
      if (str[i]=='\\') {
         i++;
         if (str[i]=='\\') temp[j++]='\\';
         else {
            int val = 64*(str[i++]-'0') + 8*(str[i++]-'0') + (str[i++]-'0');
            temp[j++] = (char)val;
         }
      } else temp[j++] = str[i++];
   }
   temp[j] = '\0';
   strcpy (str, temp);
}
*/
string CSVFile::GetValue() {
    char val[1024];
    int j=0;

    if (!*curr) return ""; // sorry, already done

    // @todo correctly handle ""
    // get next value
    if (*curr=='"') {
        curr++;
        while (true) {
            if (*curr == '"' && *(curr+1) == '"')
                curr++; // escaped ", skip first one
            else if (*curr == '"' || *curr == 0 || *curr == '\n')
                break;
            val[j++] = *curr++;
        }
        /*
        while (*curr!='"' && *curr && *curr!='\n') {
            if (*curr=='\\' && *(curr+1) == '"') // embedded "
                curr++;
            val[j++] = *curr++;
        }
        */

        if (*curr)
            curr++;
    }
    while (*curr!=',' && *curr && *curr!='\n')
        val[j++] = *curr++;
    if (*curr)
        curr++;
    val[j] = '\0';

    // unescape
    //unescape(val);

    return val; // convert to string
}

int CSVFile::GetInt() {
    string s = GetValue();
    return atoi(s.c_str());
}

bool CSVFile::eol() {
    return (*curr == '\0');
}

static void escape(char* str) {
    char temp[1024];
    char* pt = temp;
    char* ps = str;

    while (*str) {
        /*
        if (*str == '"' || *str == '\\')
            *pt++ = '\\';
        */
        if (*str == '"') *pt++ = '"'; // " becomes ""
        *pt++ = *str++;
    }

    *pt = '\0';
    strcpy(ps, temp);
}

bool CSVFile::SetValue(string value) {
    char val[1024];
    val[0] = ',';
    val[1] = '"';
    strcpy(&val[2], value.c_str());
    escape(&val[2]);
    strcat(val, "\"");
    
    if (line[0])
        strcat(line, val);
    else
        strcat(line, &val[1]);
    return true;
}

bool CSVFile::SetInt(int value) {
    char buff[32];
    wsprintf(buff, ",\"%d\"", value);
    if (line[0])
        strcat(line, buff);
    else
        strcat(line, &buff[1]);
    return true;
}

bool CSVFile::WriteLine() {
    fputs(line, file);
    fputc('\n', file);
    line[0] = '\0';
    return true;
}
