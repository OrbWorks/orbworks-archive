#include "PocketC.h"

base_vector::base_vector(bool ptr, long delta) {
    nDelta = delta;
    nAlloc = nItems = 0;
    hMem = NULL;
    pt = NULL;
    isPtr = ptr;
}

base_vector::~base_vector() {
    clear();
}

void base_vector::init(long size, long type_size) {
    assert(hMem==NULL);
    if (size == 0) size = 1;
    hMem = (MemHandle)MemHandleNew(size * type_size);
    if (!hMem) oom();
    pt = (void*)MemHandleLock(hMem);
    nAlloc = size;
}

void base_vector::clear() {
    if (hMem) {
        if (isPtr) {
            for (long i=0;i<nItems;i++)
                MemPtrFree(((void**)pt)[i]);
        }
        MemHandleUnlock(hMem);
        MemHandleFree(hMem);
    }
    nAlloc = nItems = 0;
    hMem = NULL;
    pt = NULL;
}

long base_vector::add(void* pItem, long type_size) {
    if (!hMem)
        init(10, type_size);

    if (nItems >= nAlloc) {
        MemHandleUnlock(hMem);
        if (MemHandleResize(hMem, type_size * (nItems + nDelta)))
            oom();
        nAlloc += nDelta;
        pt = (void*) MemHandleLock(hMem);
    }
        
    MemMove((char*)pt + (type_size * nItems), pItem, type_size);
    return ++nItems;
}

long base_vector::insert(unsigned short index, void* pItem, long type_size) {
    if (index == -1 || index >= nItems)
        return add(pItem, type_size);
    add(type_size);
    MemMove((char*)pt+(index+1)*type_size, (char*)pt+(index*type_size), type_size);
    MemMove((char*)pt+(index*type_size), pItem, type_size);
    return nItems;
}

long base_vector::erase(unsigned short index, long type_size) {
    if (index >= nItems)
        return nItems;
    if (index == nItems-1)
        return remove(type_size);
    MemMove((char*)pt+(index*type_size), (char*)pt+(index+1)*type_size, type_size);
    remove(type_size);
    return nItems;
}

void base_vector::reserve(long size, long type_size) {
    if (nItems + size > nAlloc) {
        long thisDelta = nItems + size - nAlloc;
        if (thisDelta % nDelta)
            thisDelta += nDelta - (thisDelta % nDelta);
        MemHandleUnlock(hMem);
        if (MemHandleResize(hMem, type_size * (nAlloc + thisDelta)))
            oom();
        nAlloc += thisDelta;
        pt = (void*)MemHandleLock(hMem);
    }
}

void base_vector::fill(long size, long type_size) {
    reserve(size, type_size);
    nItems += size;
}

// hvector

base_hvector::base_hvector( long delta) {
    nDelta = delta;
    nAlloc = nItems = 0;
    hMem = NULL;
    locked = false;
}

base_hvector::~base_hvector() {
    clear();
}

void base_hvector::init(long size, long type_size) {
    assert(hMem==NULL);
    if (size == 0) size = 1;
    hMem = (MemHandle)MemHandleNew(size * type_size);
    if (!hMem) oom();
    nAlloc = size;
}

void base_hvector::clear() {
    assert(!locked);
    if (hMem)
        MemHandleFree(hMem);
    nAlloc = nItems = 0;
    hMem = NULL;
}

long base_hvector::add(void* pItem, long type_size) {
    bool wasLocked = locked;
    if (!hMem)
        init(10, type_size);

    if (nItems >= nAlloc) {
        if (locked) {
            MemHandleUnlock(hMem);
            locked = false;
        }
        if (MemHandleResize(hMem, type_size * (nItems + nDelta)))
            oom();
        nAlloc += nDelta;
    }
    if (!locked) {
        pt = MemHandleLock(hMem);
        locked = true;
    }
        
    MemMove((char*)pt + (type_size * nItems), pItem, type_size);
    if (!wasLocked)
        unlock();
    return ++nItems;
}

long base_hvector::insert(unsigned short index, void* pItem, long type_size) {
    if (index == -1 || index >= nItems)
        return add(pItem, type_size);
    add(type_size);
    bool wasLocked = locked;
    if (!locked)
        lock();
    MemMove((char*)pt+(index+1)*type_size, (char*)pt+(index*type_size), type_size);
    MemMove((char*)pt+(index*type_size), pItem, type_size);
    if (!wasLocked)
        unlock();
    return nItems;
}

long base_hvector::erase(unsigned short index, long type_size) {
    if (index >= nItems)
        return nItems;
    if (index == nItems-1)
        return remove(type_size);
    bool wasLocked = locked;
    if (!locked)
        lock();
    MemMove((char*)pt+(index*type_size), (char*)pt+(index+1)*type_size, type_size);
    remove(type_size);
    if (!wasLocked)
        unlock();
    return nItems;
}

inline void* base_hvector::lock() {
    assert(hMem);
    assert(!locked);
    locked = true;
    return (pt = MemHandleLock(hMem));
}

inline void base_hvector::unlock() {
    assert(hMem);
    assert(locked);
    if (locked) {
        locked = false;
        pt = NULL;
        MemHandleUnlock(hMem);
    }
}

void base_hvector::reserve(long size, long type_size) {
    bool wasLocked = locked;
    if (nItems + size > nAlloc) {
        long thisDelta = nItems + size - nAlloc;
        if (thisDelta % nDelta)
            thisDelta += nDelta - (thisDelta % nDelta);
        if (locked) {
            MemHandleUnlock(hMem);
            locked = false;
        }
        if (MemHandleResize(hMem, type_size * (nAlloc + thisDelta)))
            oom();
        nAlloc += thisDelta;
        if (wasLocked)
            lock();
    }
}

void base_hvector::fill(long size, long type_size) {
    reserve(size, type_size);
    nItems += size;
}
