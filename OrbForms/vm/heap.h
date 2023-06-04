#define MAX_CHUNKS 32
#define CHUNK_SIZE 2048
#define CHUNK_BITS 11
#define CHUNK_MASK 0x07ff

struct HeapBlock {
    word size;
    word addr;
};

class Heap {
public:
    Heap();
    ~Heap();
    
    bool init(word size);
    word alloc(word count, char* type);
    void release(word index);
    word getsize(word index);
    void clear();
    
    inline word size() {
        return nItems;
    }
    inline Value& operator[] (word index) {
        assert(index < nItems);
        assert(pChunk[index >> CHUNK_BITS]);
        return pChunk[index >> CHUNK_BITS][index & CHUNK_MASK];
    }


private:
    bool reserve(word size, word& addr); // returns rounded size
    bool blockReserve(word size);
    word nAlloc, nItems, nDelta;
    Value* pChunk[MAX_CHUNKS];
    vector<HeapBlock> elements;
};

