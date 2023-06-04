class Source {
public:
    virtual ~Source() { }
    virtual char* GetSrc() { return NULL; }
    virtual SourceType GetType() { return srcNone; }
};

class BufferSource : public Source {
public:
    BufferSource(const char* buffer, bool bFree);
    virtual ~BufferSource();
    virtual char* GetSrc();
    virtual SourceType GetType();

private:
    char* buffer;
    bool bFree;
};

class MacroSource : public BufferSource {
public:
    MacroSource(char* src);
    virtual SourceType GetType();
};

#ifndef WIN32
class MemoSource : public Source {
public:
    MemoSource(short id);
    virtual ~MemoSource();
    virtual char* GetSrc();
    virtual SourceType GetType();

private:
    MemHandle hMem;
};

class DocSource : public Source {
public:
    DocSource(short id);
    virtual ~DocSource();
    virtual char* GetSrc();
    virtual SourceType GetType();

private:
    MemHandle hMem;
};

class HandleSource : public Source {
public:
    HandleSource(MemHandle hMem);
    virtual ~HandleSource();
    virtual char* GetSrc();
    virtual SourceType GetType();

private:
    MemHandle hMem;
};

class FileSource : public Source {
    // TODO: implement FileSource
};
#endif

