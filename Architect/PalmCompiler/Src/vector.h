void oom();

class base_vector {
public:
    base_vector(bool ptr, long delta);
    ~base_vector();
    void init(long size, long type_size);
    void clear();
    long add(void* pItem, long type_size);
    long insert(unsigned short index, void* pItem, long type_size);
    long erase(unsigned short index, long type_size);
    void reserve(long size, long type_size);
    void fill(long size, long type_size);

    // these forms are inline, since it will be called by very little code
    // but must be high speed (for stack control)
    long add(long type_size) {
        if (nItems >= nAlloc) {
            MemHandleUnlock(hMem);
            if (MemHandleResize(hMem, type_size * (nAlloc + nDelta))) {
                // return to consistent state
                pt = (void*)MemHandleLock(hMem);
                oom();
            }
            nAlloc += nDelta;
            pt = (void*)MemHandleLock(hMem);
        }
        return ++nItems;
    }
    inline long remove(long type_size) {
        nItems--;
        if (nAlloc - nItems > nDelta * 2) {
            MemHandleUnlock(hMem);
            MemHandleResize(hMem, type_size * (nAlloc - nDelta));
            nAlloc -= nDelta;
            pt = (void*)MemHandleLock(hMem);
        }
        return nItems;
    }
    
protected:
    MemHandle hMem;
    long nAlloc, nItems, nDelta;
    bool isPtr;
    void* pt;
};

class base_dbvector {
public:
    base_dbvector(long delta);
    ~base_dbvector();
    void init(DmOpenRef dbRef, long size, long type_size);
    void clear();
    long add(void* pItem, long type_size);
    long insert(unsigned short index, void* pItem, long type_size);
    void reserve(long size, long type_size);

    // these forms are inline, since it will be called by very little code
    // but must be high speed (for stack control)
    long add(long type_size) {
        if (nItems >= nAlloc) {
            MemHandleUnlock(hMem);
            MemHandle hNewMem = DmResizeRecord(dbRef, recId, type_size * (nAlloc + nDelta));
            if (!hNewMem) {			
                // return to consistent state
                pt = (void*)MemHandleLock(hMem);
                oom();
            }
            hMem = hNewMem;
            nAlloc += nDelta;
            pt = (void*)MemHandleLock(hMem);
        }
        return ++nItems;
    }
    
protected:
    MemHandle hMem;
    DmOpenRef dbRef;
    UInt16 recId;
    long nAlloc, nItems, nDelta;
    void* pt;
};

class base_hvector {
public:
    base_hvector(long delta);
    ~base_hvector();
    void init(long size, long type_size);
    void clear();
    long add(void* pItem, long type_size);
    long insert(unsigned short index, void* pItem, long type_size);
    long erase(unsigned short index, long type_size);
    void reserve(long size, long type_size);
    void fill(long size, long type_size);
    void* lock();
    void unlock();

    // these forms are inline, since it will be called by very little code
    // but must be high speed (for stack control)
    inline long add(long type_size) {
        bool wasLocked = locked;
        if (nItems >= nAlloc) {
            if (locked) {
                MemHandleUnlock(hMem);
                locked = false;
            }
            if (MemHandleResize(hMem, type_size * (nAlloc + nDelta)))
                oom();
            nAlloc += nDelta;
            if (wasLocked)
                lock();
            }
        return ++nItems;
    }
    inline long remove(long type_size) {
        bool wasLocked = locked;
        nItems--;
        if (nAlloc - nItems > nDelta * 2) {
            if (locked) {
                MemHandleUnlock(hMem);
                locked = false;
            }
            MemHandleResize(hMem, type_size * (nAlloc - nDelta));
            nAlloc -= nDelta;
            if (wasLocked)
                lock();
            }
        return nItems;
    }
    
protected:
    MemHandle hMem;
    long nAlloc, nItems, nDelta;
    bool locked;
    void* pt;
};

template <class T>
class vector : public base_vector {
public:
    vector(bool ptr = false, long delta = 10) : base_vector(ptr, delta) { /*init();*/ }

    vector(vector& v) : base_vector(v.isPtr, v.nDelta) {
        init(v.nItems);
        nItems = v.nItems;
        memcpy(pt, v.pt, v.nItems * sizeof(T));
    }

    inline void init(long size = 10) {
        base_vector::init(size, sizeof(T));
    }
    
    inline void clear() {
        base_vector::clear();
    }
    
    inline long add(T& item) {
        return base_vector::add(&item, sizeof(T));
    }
    
    inline void push_back(const T& item) {
        base_vector::add((void*)&item, sizeof(T));
    }
    
    inline void pop_back() {
        base_vector::remove(sizeof(T));
    }
    
    inline T& back() {
        return *((T*)pt + (nItems-1));
    }
    
    // this form is inline, since it will be called by very little code
    // but must be high speed (for stack control)
    inline long add() {
        return base_vector::add(sizeof(T));
    }
    
    inline long insert(unsigned short index, T& item) {
        return base_vector::insert(index, &item, sizeof(T));
    }
    
    inline long erase(unsigned short index) {
        return base_vector::erase(index, sizeof(T));
    }
    
    inline long remove() {
        return base_vector::remove(sizeof(T));
    }
    
    inline long size() {
        return nItems;
    }
    
    inline T& operator[] (unsigned short index) {
        assert(index < nItems);
        return *((T*)pt + index);
    }
    
    inline void reserve(long size) {
        base_vector::reserve(size, sizeof(T));
    }
    
    inline void fill(long size) {
        base_vector::fill(size, sizeof(T));
    }
    
    T* lock() {
        return (T*)pt;
    }
    
    void unlock() { }
};

template <class T>
class ovector : public base_vector {
public:
    ovector(long delta = 10) : base_vector(false, delta) { /*init();*/ }
    
    ovector(ovector& v) : base_vector(false, v.nDelta) {
        init(v.nItems);
        nItems = v.nItems;
        for (long i=0; i<nItems; i++) {
            T* pd = ((T*)pt) + i;
            T* ps = ((T*)v.pt) + i;
            new (pd) T(*ps);
        }
    }
    
    ~ovector() {
        for (long i=0; i<nItems; i++) {
            T* t = ((T*)pt)+i;
            t->~T();
        }
    }

    inline void init(long size = 10) {
        base_vector::init(size, sizeof(T));
    }
    
    inline void clear() {
        base_vector::clear();
    }
    
    void push_back(const T& item) {
        if (!hMem)
            init();
    
        base_vector::add(sizeof(T));
        T* t = ((T*)pt)+(nItems-1);
        new (t) T((T&)item);
    }
    
    inline long erase(unsigned short index) {
        T* t = ((T*)pt)+index;
        t->~T();
        return base_vector::erase(index, sizeof(T));
    }

    inline long size() {
        return nItems;
    }
    
    inline T& operator[] (unsigned short index) {
        assert(index < nItems);
        return *((T*)pt + index);
    }
    
    inline void reserve(long size) {
        base_vector::reserve(size, sizeof(T));
    }

    T* lock() {
        return (T*)pt;
    }
    
    void unlock() { }
};

template <class T>
class dbvector : public base_dbvector {
public:
    dbvector(long delta = 1024) : base_dbvector(delta) { }

    inline void init(DmOpenRef dbRef, long size = 1024) {
        base_dbvector::init(dbRef, size, sizeof(T));
    }
    
    inline long add(T& item) {
        return base_dbvector::add(&item, sizeof(T));
    }
    
    inline void push_back(const T& item) {
        base_dbvector::add((void*)&item, sizeof(T));
    }
        
    inline long size() {
        return nItems;
    }
    
    inline T& operator[] (unsigned short index) {
        assert(index < nItems);
        return *((T*)pt + index);
    }
    
    inline void reserve(long size) {
        base_dbvector::reserve(size, sizeof(T));
    }

private:
    dbvector(dbvector& v) { }
};

template <class T>
class hvector : public base_hvector {
public:
    hvector(long delta = 10) : base_hvector(delta) { }
    
    inline void init(long size = 10) {
        base_hvector::init(size, sizeof(T));
    }
    
    inline void clear() {
        base_hvector::clear();
    }
    
    inline long add(T& item) {
        return base_hvector::add(&item, sizeof(T));
    }
    
    // this form is inline, since it will be called by very little code
    // but must be high speed (for stack control)
    inline long add() {
        return base_hvector::add(sizeof(T));
    }
    
    inline long insert(unsigned short index, T& item) {
        return base_hvector::insert(index, &item, sizeof(T));
    }
    
    inline long erase(unsigned short index) {
        return base_hvector::erase(index, sizeof(T));
    }
    
    inline long remove() {
        return base_hvector::remove(sizeof(T));
    }
    
    inline long size() {
        return nItems;
    }
    
    inline T& operator[] (unsigned short index) {
        assert(index < nItems);
        assert(locked);
        return *((T*)pt + index);
    }
    
    inline void reserve(long size) {
        base_hvector::reserve(size, sizeof(T));
    }
    
    inline void fill(long size) {
        base_hvector::fill(size, sizeof(T));
    }
    
    inline T* lock() {
        return (T*)base_hvector::lock();
    }
    
    inline void unlock() {
        base_hvector::unlock();
    }
};
