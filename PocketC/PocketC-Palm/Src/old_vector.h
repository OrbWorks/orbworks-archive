void oom();

template <class T>
class vector {
public:
    vector(unsigned short delta=10) {
        nDelta = delta;
        nAlloc = nItems = 0;
        hMem = NULL;
        locked = false;
    }

    ~vector() {
        assert(!locked);
        if (hMem)
            MemHandleFree(hMem);
    }
    
    void init(unsigned short size = 10) {
        assert(hMem==NULL);
        hMem = (Handle)MemHandleNew(size * sizeof(T));
        if (!hMem) oom();
        nAlloc = size;
    }
    
    void clear(bool bFree = false) {
        assert(!locked);
        if (bFree) {
            pt = MemHandleLock(hMem);
            for (unsigned short i=0;i<nItems;i++)
                MemPtrFree(pt[i]);
            MemHandleUnlock(hMem);
        }
        if (hMem)
            MemHandleFree(hMem);
        nAlloc = nItems = 0;
        hMem = NULL;
    }
    
    int add(T& item) {
        if (!hMem)
            init();

        assert(!locked);
        if (nItems >= nAlloc) {
            if (MemHandleResize(hMem, sizeof(T) * (nItems + nDelta)))
                oom();
            nAlloc += nDelta;
        }
        T* pt = (T*) MemHandleLock(hMem);
        memcpy(&pt[nItems], &item, sizeof(T));
        MemHandleUnlock(hMem);
        return ++nItems;
    }
    
    inline unsigned short size() {
        return nItems;
    }
    
    inline T& operator[] (unsigned short index) {
        assert(locked);
        assert(index < nItems);
        return pt[index];
    }
    
    T* lock() {
        assert(hMem);
        assert(!locked);
        locked = true;
        return (pt = (T*)MemHandleLock(hMem));
    }

    void unlock() {
        assert(hMem);
        assert(locked);
        locked = false;
        MemHandleUnlock(hMem);
    }

private:
    Handle hMem;
    unsigned short nAlloc, nItems, nDelta;
    bool locked;
    T* pt;
};
