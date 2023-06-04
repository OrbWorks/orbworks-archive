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
    inline long add(long type_size) {
        if (nItems >= nAlloc) {
            MemHandleUnlock(hMem);
            if (MemHandleResize(hMem, type_size * (nAlloc + nDelta)))
                oom();
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
    vector(bool ptr = false, long delta = 10) : base_vector(ptr, delta) { }

    inline void init(long size = 10) {
        base_vector::init(size, sizeof(T));
    }
    
    inline void clear() {
        base_vector::clear();
    }
    
    inline long add(T& item) {
        return base_vector::add(&item, sizeof(T));
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
