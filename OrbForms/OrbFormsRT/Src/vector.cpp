#include "OrbFormsRT.h"

base_vector::base_vector(long delta) {
    nDelta = delta;
    nAlloc = nItems = 0;
    hMem = NULL;
    pt = NULL;
}

base_vector::~base_vector() {
    if (hMem) {
        MemHandleUnlock(hMem);
        MemHandleFree(hMem);
    }
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
    MemMove((char*)pt+(index+1)*type_size, (char*)pt+(index*type_size), type_size*(nItems-index-1));
    MemMove((char*)pt+(index*type_size), pItem, type_size);
    return nItems;
}

long base_vector::erase(unsigned short index, long type_size) {
    if (index >= nItems)
        return nItems;
    if (index == nItems-1)
        return remove(type_size);
    MemMove((char*)pt+(index*type_size), (char*)pt+(index+1)*type_size, type_size*(nItems-index-1));
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
