#pragma once

// forward declaration
class FixedSizeAlloc;

class ChunkAllocator {
public:
    FixedSizeAlloc *mAllocs[32];

    ChunkAllocator(int, int, int);
    void *Alloc(int);
    void Free(void *, int);
    static void UploadDebugStats();

    // *might* be wrong
    operator bool() { return mAllocs != 0; }
};

class FixedSizeAlloc {
public:
    FixedSizeAlloc(int, ChunkAllocator *, int);
    virtual ~FixedSizeAlloc();
    virtual void *RawAlloc(int);

    int mAllocSizeWords;
    int mNumAllocs;
    int mMaxAllocs;
    int mNumChunks;
    int *mFreeList;
    int mNodesPerChunk;
    ChunkAllocator *mAlloc;

    void *Alloc();
    void Free(void *);
    void Refill();
};

void *PoolAlloc(int classSize, int reqSize, const char *file, int line, const char *name);
void PoolFree(int, void *mem, const char *file, int line, const char *name);

#define POOL_OVERLOAD(class_name, line_num)                                              \
    static void *operator new(unsigned int s) {                                          \
        return PoolAlloc(s, s, __FILE__, line_num, #class_name);                         \
    }                                                                                    \
    static void *operator new(unsigned int s, void *place) { return place; }             \
    static void operator delete(void *v) {                                               \
        PoolFree(sizeof(class_name), v, __FILE__, line_num, #class_name);                \
    }
