class CSVFile {
public:
    CSVFile();
    ~CSVFile();
    bool Open(const char* name, const char* mode);
    
    // read file
    bool ReadLine();
    string GetValue();
    int GetInt();
    bool eol();

    // write file
    bool SetValue(string value);
    bool SetInt(int i);
    bool WriteLine();

private:

    char line[2048];
    char* curr;
    FILE* file;
};
