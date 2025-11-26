#include "char/ClipDistMap.h"
#include "char/CharClip.h"
#include "char/CharUtl.h"
#include "macros.h"
#include "math/Utl.h"

void FindWeights(
    std::vector<RndTransformable *> &transes,
    std::vector<float> &floats,
    const DataArray *arr
) {
    floats.resize(transes.size());
    float f1 = 0;
    for (int i = 0; i < transes.size(); i++) {
        float len = Length(transes[i]->LocalXfm().v);
        if (arr) {
            float f84 = 1;
            arr->FindData(transes[i]->Name(), f84, false);
            len *= f84;
        }
        floats[i] = len;
        f1 += floats[i];
    }
    for (int i = 0; i < floats.size(); i++) {
        floats[i] *= floats.size() / f1;
    }
}

ClipDistMap::ClipDistMap(
    CharClip *clip1, CharClip *clip2, float f1, float f2, int i, const DataArray *a
)
    : mClipA(clip1), mClipB(clip2), mWeightData(a), mSamplesPerBeat(8),
      mLastMinErr(kHugeFloat), mBeatAlign(f1), mBeatAlignOffset(0), mBlendWidth(f2),
      mNumSamples(i) {
    int h = CalcHeight();
    int w = CalcWidth();
    mDists.Resize(w, h);

    mBeatAlignPeriod = mBeatAlign * mSamplesPerBeat + 0.5;

    int temp;
    if (mBeatAlignPeriod != 0) {
        temp = mAStart * mSamplesPerBeat - mBStart * mSamplesPerBeat;
        mBeatAlignOffset = temp - (temp / mBeatAlignPeriod) * mBeatAlignPeriod;

        if (mBeatAlignOffset < 0) {
            mBeatAlignOffset += mBeatAlignPeriod;
        }
    }
}

bool ClipDistMap::BeatAligned(int i1, int i2) {
    int l1;
    int l2;

    if (mBeatAlignPeriod == 0) {
        l1 = 0;
    } else {
        l2 = i1 - i2;
        l1 = l2 - (l2 / mBeatAlignPeriod) * mBeatAlignPeriod;
        if (l1 < 0) {
            mBeatAlignPeriod += l1;
        }
    }

    return mBeatAlignOffset - l1 == 0;
}

void ClipDistMap::FindNodes(float f1, float f2, float f3) {
    std::vector<Node> nodes = mNodes;
    mLastMinErr = f1;
    float f5 = f2;
    float f6;
    float f4 = f2 * 0.44999998807907104;
    if (f5 == 0.0) {
        f4 = kHugeFloat;
        f6 = f4;
    } else if (f6 == 0.0) {
        f6 = f4;
    }

    FindBestNodeRecurse(f1, f4, (f4 * 2.0 - f5), mAStart, mAEnd);
}
