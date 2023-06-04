class string {
public:
    static const int npos = -1;
    
    string();
    string(const string& s);
    string(const char* s);
    ~string();
    
    string& operator=(const string& s);
    string& operator+=(const string& s);
    string& operator+=(const char c);
    string& operator+=(const char* s);
    void append(const char* s, int len);
    
    char& operator[](int i) { return ((char*)c_str())[i]; }
    
    int length() { return len; }
    bool empty() { return len == 0; }
    const char* c_str() const { return str ? str : buf; }
    
    int find_first_of(const char c);
    string substr(int start, int count = npos);
    void assign(const char* s, int count);
    
private:
    void assign(const char* s);
    void assign(const string& s);
    bool equals(const char* s) const;
    bool equals(const string& s) const;
    void append(const char* s);
    void dealloc();
    
    static const int bufSize = 16;
    
    int len;
    char* str;
    char buf[bufSize];

    friend bool operator==(const string& a, const string& b);
    friend bool operator==(const string& a, const char* b);
    friend bool operator==(const char* a, const string& b);

    friend bool operator!=(const string& a, const string& b);
    friend bool operator!=(const string& a, const char* b);
    friend bool operator!=(const char* a, const string& b);

    friend string operator+(const char* a, const string& b);
    friend string operator+(const string& a, const string& b);
    friend string operator+(const string& a, const char* b);
};

bool operator==(const string& a, const string& b);
bool operator==(const string& a, const char* b);
bool operator==(const char* a, const string& b);

bool operator!=(const string& a, const string& b);
bool operator!=(const string& a, const char* b);
bool operator!=(const char* a, const string& b);

string operator+(const char* a, const string& b);
string operator+(const string& a, const string& b);
string operator+(const string& a, const char* b);
