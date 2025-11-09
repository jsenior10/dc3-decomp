#include "utl/MemStats.h"
#include "math/Utl.h"
#include "os/Debug.h"

int SizeLess(const void *v1, const void *v2) {
    const BlockStat *b1 = (const BlockStat *)v1;
    const BlockStat *b2 = (const BlockStat *)v2;
    if (b1->mSizeAct < b2->mSizeAct)
        return 1;
    // else...?
}

int NameLess(const void *v1, const void *v2) {
    const BlockStat *b1 = (const BlockStat *)v1;
    const BlockStat *b2 = (const BlockStat *)v2;
    return strcmp(b1->mName, b2->mName);
}

BlockStatTable::BlockStatTable(bool b)
    : mMaxStats(0x400), mNumStats(0), mSizeMatters(b) {}

void BlockStatTable::Clear() { mNumStats = 0; }

void BlockStatTable::SortBySize() {
    qsort(mStats, mNumStats, sizeof(BlockStat), SizeLess);
}

void BlockStatTable::SortByName() {
    qsort(mStats, mNumStats, sizeof(BlockStat), NameLess);
}

BlockStat &BlockStatTable::GetBlockStat(int iStat) {
    MILO_ASSERT((0) <= (iStat) && (iStat) < (mNumStats), 0x37);
    return mStats[iStat];
}

void BlockStatTable::Update(const char *cc, unsigned char uc, int i3, int i4) {
    int idx = 0;
    for (; idx < mNumStats; idx++) {
        if (mStats[idx].mHeap == uc && (!mSizeMatters || mStats[idx].mSizeReq == i3)) {
            if (strcmp(mStats[idx].mName, cc) == 0) {
                if (!mSizeMatters) {
                    mStats[idx].mSizeReq += i3;
                }
                mStats[idx].mSizeAct += i4;
                if (i3 >= mStats[idx].mMaxSize) {
                    mStats[idx].mMaxSize = i3;
                }
                mStats[idx].mNumAllocs++;
                return;
            }
        }
    }
    if (idx == mNumStats && mNumStats < mMaxStats) {
        mStats[mNumStats].mName = cc;
        mStats[mNumStats].mHeap = uc;
        mStats[mNumStats].mSizeReq = i3;
        mStats[mNumStats].mMaxSize = i3;
        mStats[mNumStats].mSizeAct = i4;
        mStats[mNumStats].mNumAllocs = 1;
        mNumStats++;
    } else {
        MILO_FAIL("Stack overflow in BlockStatTable!");
    }
}

void HeapStats::Alloc(int act, int req) {
    mTotalNumAllocs++;
    mTotalActSize += act;
    mTotalReqSize += req;
    mMaxNumAllocs = Max(mTotalNumAllocs, mMaxNumAllocs);
    mMaxActSize = Max(mTotalActSize, mMaxActSize);
}

void HeapStats::Free(int act, int req) {
    mTotalNumAllocs--;
    mTotalActSize -= act;
    mTotalReqSize -= req;
}
