void oom();
class base_vector {
public:
    base_vector(long delta = 10);
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
    void* pt;
};

template <class T>
class hvector {
public:
    hvector(long delta = 10) {
        nDelta = delta;
        nAlloc = nItems = 0;
        hMem = NULL;
        locked = false;
    }

    ~hvector() {
        assert(!locked);
        if (hMem)
            MemHandleFree(hMem);
    }
    
    void init(long size = 10) {
        assert(hMem==NULL);
        hMem = (MemHandle)MemHandleNew(size * sizeof(T));
        if (!hMem) oom();
        nAlloc = size;
    }
    
    void clear() {
        if (hMem)
            MemHandleFree(hMem);
        nAlloc = nItems = 0;
        hMem = NULL;
    }
    
    long add(T& item) {
        bool wasLocked = locked;
        if (!hMem)
            init();

        if (nItems >= nAlloc) {
            if (locked) {
                MemHandleUnlock(hMem);
                locked = false;
            }
            if (MemHandleResize(hMem, sizeof(T) * (nItems + nDelta)))
                oom();
            nAlloc += nDelta;
        }
        if (!locked) {
            pt = (T*) MemHandleLock(hMem);
            locked = true;
        }
            
        memcpy(&pt[nItems], &item, sizeof(T));
        if (!wasLocked)
            unlock();
        return ++nItems;
    }
    
    // this form is inline, since it will be called by very little code
    // but must be high speed (for stack control)
    inline long add() {
        bool wasLocked = locked;
        if (nItems >= nAlloc) {
            if (locked) {
                MemHandleUnlock(hMem);
                locked = false;
            }
            if (MemHandleResize(hMem, sizeof(T) * (nAlloc + nDelta)))
                oom();
            nAlloc += nDelta;
            if (wasLocked)
                lock();
        }
        return ++nItems;
    }
    
    long insert(unsigned short index, T& item) {
        if (index == -1 || index >= nItems)
            return add(item);
        add();
        bool wasLocked = locked;
        if (!locked)
            lock();
        memmove(&pt[index+1], &pt[index], sizeof(T) * (nItems-index-1));
        memcpy(&pt[index], &item, sizeof(T));
        if (!wasLocked)
            unlock();
        return nItems;
    }
    
    long erase(unsigned short index) {
        if (index >= nItems)
            return nItems;
        if (index == nItems-1)
            return remove();
        bool wasLocked = locked;
        if (!locked)
            lock();
        memmove(&pt[index], &pt[index+1], sizeof(T) * (nItems-index-1));
        remove();
        if (!wasLocked)
            unlock();
        return nItems;
    }
    
    inline long remove() {
        bool wasLocked = locked;
        nItems--;
        if (nAlloc - nItems > nDelta * 2) {
            if (locked) {
                MemHandleUnlock(hMem);
                locked = false;
            }
            MemHandleResize(hMem, sizeof(T) * (nAlloc - nDelta));
            nAlloc -= nDelta;
            if (wasLocked)
                lock();
        }
        return nItems;
    }
    
    inline long size() {
        return nItems;
    }
    
    inline T& operator[] (unsigned short index) {
        assert(locked);
        assert(index < nItems);
        return pt[index];
    }
    
    inline T* lock() {
        assert(hMem);
        assert(!locked);
        locked = true;
        return (pt = (T*)MemHandleLock(hMem));
    }

    inline void unlock() {
        assert(hMem);
        assert(locked);
        if (locked) {
            locked = false;
            pt = NULL;
            MemHandleUnlock(hMem);
        }
    }
    
    void reserve(long size) {
        bool wasLocked = locked;
        if (nItems + size > nAlloc) {
            long thisDelta = nItems + size - nAlloc;
            if (thisDelta % nDelta)
                thisDelta += nDelta - (thisDelta % nDelta);
            if (locked) {
                MemHandleUnlock(hMem);
                locked = false;
            }
            if (MemHandleResize(hMem, sizeof(T) * (nAlloc + thisDelta)))
                oom();
            nAlloc += thisDelta;
            if (wasLocked)
                lock();
        }
    }
    
    void fill(long size) {
        reserve(size);
        nItems += size;
    }

private:
    MemHandle hMem;
    long nAlloc, nItems, nDelta;
    bool locked;
    T* pt;
};

template <class T>
class vector : public base_vector {
public:
    vector(long delta = 10) : base_vector(delta) { }

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
    
    inline long addfast() {
        return ++nItems;
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
    
    inline long removefast() {
        return --nItems;
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
