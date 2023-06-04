#include "OrbFormsRT.h"

Heap::Heap() {
    nAlloc = 0;
    nItems = 0;
    MemSet(&pChunk[0], sizeof(Value*)*MAX_CHUNKS, 0);
}

Heap::~Heap() {
    clear();
}

void Heap::clear() {
    if (nAlloc) {
        while (nItems) {
            Value& val = (*this)[nItems-1];
            if (val.type == vtString)
                val.release();
            nItems--;
        }
        nItems = 0;
        nAlloc = 0;
        
        for (int i=0;i<MAX_CHUNKS;i++) {
            if (pChunk[i]) {
                MemPtrFree(pChunk[i]);
                pChunk[i] = NULL;
            }
        }
    }
}

bool Heap::init(word size) {
    word addr;
    clear();
    if (reserve(size, addr)) {
        assert(addr == 0);
        for (word i=0;i<size;i++) {
            Value& val = (*this)[i];
            val.type = vtInt;
            val.iVal = 0;
        }
        return true;
    }
    return false;
}

word Heap::alloc(word count, char* type) {
    word addr;
    int len = strlen(type);
    if (!reserve(len * count, addr))
        return 0;
    word i = 0;
    char* pt = type;
    while (i < len * count) {
        if (*pt == '\0')
            pt = type;
            
        Value& val = (*this)[i+addr];
        val.iVal = 0;
        switch (*pt) {
            case 'p':
            case 'i':
            case 'o':
                val.type = vtInt;
                break;
            case 'c':
                val.type = vtChar;
                break;
            case 'f':
                val.type = vtFloat;
                break;
            case 's':
                val.newConstString("");
                break;
            default:
                assert(!"unsupported type in Heap::alloc");
        }
        i++;
        pt++;
    }
    return addr;
}

bool Heap::reserve(word size, word& addr) {
    // if it fits in the current block, just put it there
    if (size + nItems <= nAlloc) {
        goto putitattheend;
    }
    int i=0;
    for (;i<elements.size();i++) {
        HeapBlock& hb = elements[i];
        if ((hb.size & 0x8000) && ((hb.size & 0x7fff) >= size)) {
            // put it here
            if ((hb.size & 0x7fff) > size) {
                HeapBlock nhb;
                nhb.size = hb.size - size; // still marked free
                nhb.addr = hb.addr + size;
                // unable to access hb after this
                elements.insert(i+1, nhb);
            }
            elements[i].size = size;
            addr = elements[i].addr;
            return true;
        }
    }

    // doesn't fit, allocate more space
    if (!blockReserve(size)) {
        addr = 0;
        return false;
    }
putitattheend:
    HeapBlock hb = { size, nItems };
    elements.add(hb);
    addr = nItems;
    nItems += size;
    return true;
}

bool Heap::blockReserve(word size) {
    word dsize = size - (nAlloc - nItems);
    bool good = true;
    int nBlocks = (dsize + CHUNK_SIZE - 1) / CHUNK_SIZE, i = 0;
    int lastBlock = nAlloc >> CHUNK_BITS;
    while (i < nBlocks) {
        pChunk[lastBlock + i] = (Value*)MemPtrNew(CHUNK_SIZE*sizeof(Value));
        if (pChunk[lastBlock + i] == NULL) {
            good = false;
            break;
        }
        i++;
    }
    if (!good) {
        while (i) {
            i--;
            MemPtrFree(pChunk[lastBlock + i]);
            pChunk[lastBlock + i] = NULL;
        }
        return false;
    }
    nAlloc += nBlocks * CHUNK_SIZE;
    return true;
}

word Heap::getsize(word addr) {
    int i=0;
    elements.lock();
    for (;i<elements.size();i++)
        if (addr == elements[i].addr) break;
    if (i >= elements.size() || (elements[i].size & 0x8000)) {
        elements.unlock();
        return -1; // TODO: error?
    }
    
    word size = elements[i].size;
    elements.unlock();
    return size;
}

void Heap::release(word addr) {
    int i=0;
    elements.lock();
    for (;i<elements.size();i++)
        if (addr == elements[i].addr) break;
    if (i >= elements.size()) {
        elements.unlock();
        vm->vm_error("attempt to delete invalid memory");
        return;
    }
    
    if (elements[i].size & 0x8000) {
        elements.unlock();
        vm->vm_error("attempt to delete already deleted memory");
        return;
    }
    
    word size = elements[i].size;
    for (word j=0;j<size;j++) {
        Value& val = (*this)[j+addr];
        if (val.type == vtString) {
            val.release();
            val.type = vtInt;
        }
    }
    if (i > 0 && (elements[i-1].size & 0x8000)) {
        elements[i-1].size += elements[i].size;
        elements.erase(i);
        i--;
    }
    if (i+1 < elements.size() && (elements[i+1].size & 0x8000)) {
        elements[i+1].size += (elements[i].size & 0x7fff);
        elements[i+1].addr = elements[i].addr;
        elements.erase(i);
    }
    elements[i].size |= 0x8000;
    elements.unlock();
}
