#define RUNTIME_CHECK(x) x

void CIDitos(UInt32, Value*);
UInt32 CIDstoi(char*);

typedef bool (*IterFunc)(Value*, VarType, int, void*); // return true to continue iterating
long IterateValues(long addr, char* strFormat, int count, IterFunc func, void* pContext);

word TypeDataSize(long addr, char* strFormat, int count);
word TypeSize(char* str, bool bMin);

class NativeObject {
public:
    NativeObject();
    virtual ~NativeObject();
    int AddRef(bool ex);
    int Release(bool ex);
    
    int nRefs;
    int nXRefs; // explicit references
    int id;
};

NativeObject* PopNO(VM* vm);
void lib_nobject_destroy(VM* vm, int);
void lib_nobject_copy(VM* vm, int);
void nobject_init(VM* vm, NativeObject* pno);

#define NATIVE_INIT(name, type) \
void name(VM* vm, int) { type* pno = new type(); nobject_init(vm, pno); }

// pile of objects that maps ids to object pointers
class NativePile {
public:
    NativePile();
    ~NativePile();
    short Add(NativeObject*);
    void Remove(short id);
    NativeObject* Find(short id);
    
    vector<NativeObject*> nops;
    short nEmpty;
};

class NativeDBRecord;

class NativeDatabase : public NativeObject {
public:
    NativeDatabase();
    ~NativeDatabase();

    void EnsureOpen(VM*);
    bool Open(char* name, bool ro);
    bool OpenTC(UInt32 type, UInt32 cid, bool ro);
    bool Create(char* name, UInt32 type, UInt32 cid, bool res);
    void Close();
    void Delete();
    void SetCatName(int iCat, char* name);
    void GetCatName(int iCat, Value*);
    void MoveRec(int iFrom, int iTo);
    bool GetRec(UInt16 iRec, NativeDBRecord* pRec, bool ro);
    bool GetRes(UInt32 type, UInt32 id, NativeDBRecord* pRec);
    bool NewRec(UInt16* piRec, NativeDBRecord* pRec);
    bool NewRes(UInt32 type, UInt32 id, NativeDBRecord* pRec);
    bool GetResInfo(UInt16 index, UInt32* id, UInt16* type);
    long FindRec(UInt32 uniqueId);
    
    void RemoveRec(int iRec);
    void RemoveRes(UInt32 type, UInt32 id);
    void DelRec(int iRec);
    void ArchiveRec(int iRec);
    
    bool HasAppInfo();
    long GetAppInfo();
    bool SetAppInfo(UInt16);
    bool CreateAppInfo();
    dword GetDate(int which);
    
    long get_nrecs();
    bool get_bit(word bit);
    void set_bit(word bit, bool set);
    long get_version();
    void set_version(long);
    long get_size();
    void get_type(Value*);
    void get_cid(Value*);	
    void get_name(Value*);
    void set_name(char*);
    bool get_res();
    bool get_inrom();
    
    DmOpenRef dbRef;
    bool bRes;
    LocalID lid;
    UInt16 card;
};

class NativeDBRecord : public NativeObject {
public:
    NativeDBRecord();
    ~NativeDBRecord();
    
    bool Open(UInt16, NativeDatabase*, bool ro);
    bool Create(UInt16*, NativeDatabase*);
    bool OpenRes(UInt32, UInt32, NativeDatabase*);
    bool CreateRes(UInt32, UInt32, NativeDatabase*);
    long OpenFromFile(FileRef fileRef, UInt16 index);
    long OpenResFromFile(FileRef fileRef, UInt32 type, UInt32 id);
    long Read(long addr, char*, int);
    long Write(long addr, char*, int);
    void Erase();
    void Close();
    void EnsureOpen(VM* vm);
    void Opened(NativeDatabase*);
    void Created(NativeDatabase*);
    
    long get_offset();
    long set_offset(long);
    long get_size();
    long get_id();
    long get_uniqueid();
    int  get_cat();
    int  set_cat(int);
    
    bool ReadVal(Value*, VarType type, int len);
    bool WriteVal(Value*, VarType type, int len, byte* pData, word& doffset);
    
    MemHandle hRec;
    byte* pRec;
    bool isDirty;
    bool isEmpty;
    bool bRes;
    long offset;
    long size;
    word iRec;
    dword uid;
    bool readOnly;
    FileRef fileRef;
    NativeDatabase* pDB;
};

class NativeMemo : public NativeObject {
public:
    NativeMemo();
    ~NativeMemo();
    
    void EnsureOpen(VM* vm);

    bool Find(const char* name);
    bool Open(UInt16 id);
    bool Create();
    void Close();
    void Erase();
    void Rewind();
    void Puts(const char*);
    void SetText(const char*);
    
    int offset;
    UInt16 recid;
    MemHandle hRec;
    bool dirty;
    bool end;
    
    static DmOpenRef dbMemo;
    static int nMemos;
};

class NativeDraw : public NativeObject {
public:
    NativeDraw();
    ~NativeDraw();
    
    //
    long handle() { return (long)hWnd; }

    // direct drawing
    void attachGadget(word id);
    void attachForm(word id);

    // offscreen
    bool create(int w, int h);
    bool copyForm(word id);
    bool copyGadget(word id);
    void release();

    void begin(bool bNative, bool freeDraw = false);
    void end();

    // state
    int fg(int);
    void fgRGB(int r, int g, int b);
    int bg(int);
    void bgRGB(int r, int g, int b);
    int textColor(int);
    void textRGB(int r, int g, int b);
    int font(int);
    int underline(int);
    int textAlign(int);
    
    // utilities
    int textWidth(char*);
    int textHeight();
    int indexFromColor(int, int, int);
    int nw() { return w; }
    int nh() { return h; }
    int uiColorIndex(int type);

    // drawing
    void line(int c, int x1, int y1, int x2, int y2);
    void pixel(int c, int x, int y);
    void rect(int c, int x1, int y1, int x2, int y2, int rad);
    void frame(int c, int x1, int y1, int x2, int y2, int rad, int thick);
    void bitmap(int id, int x, int y, int mode);
    void text(int c, int x, int y, char* s, bool bPocketC);
    void textTrunc(int x, int y, int w, char* s);
    void draw(NativeDraw*, int x, int y, int mode);

    // database
    bool write(NativeDBRecord*, int iRec);
    bool read(NativeDBRecord*, int iRec);
    
protected:
    bool createOffscreen(int w, int h);
    int nBegin;
    int iGadget;
    int x, y, w, h;
    int alignMode, ulMode;
    WinHandle hWnd, hPrevWnd;
    bool bOffscreen;
    bool bNative;
    RectangleType old_clip;
    
    RGBColorType old_rgb_fg, old_rgb_bg, rgb_text;
};

class NativeDict : public NativeObject {
public:
    NativeDict();
    ~NativeDict();
    
    long count();
    void clear();
    long add(char*, char*);
    long del(char*);
    char* find(char*);
    char* key(int);
    char* value(int);
    
    // TODO: optimize the dictionary
private:
    long findIndex(char*);
    vector<char*> keys, vals;
};

class NativeStringList : public NativeObject {
public:
    NativeStringList();
    ~NativeStringList();
    
    long count();
    void clear();
    long insert(long, char*);
    long del(long);
    long find(char*);
    char* item(long);
    long tokens(char*, char*);
    void sort(bool caseSensitive);
    
//private:
    vector<char*> strs;
};
