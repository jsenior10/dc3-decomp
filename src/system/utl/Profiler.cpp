#include "utl/Profiler.h"
#include "math/Utl.h"
#include "os/Debug.h"
#include "os/Timer.h"

Profiler::Profiler(char const *c, int i)
    : mName(c), mMin(3.4028235e+38), mMax(0.0f), mSum(0.0f), mCount(0), mCountMax(i) {}

void Profiler::Start() { mTimer.Start(); }

void Profiler::Stop() {
    mTimer.Stop();
    mMin = Min(mMin, mTimer.Ms());
    mMax = Max(mMax, mTimer.Ms());
    mCount++;
    mSum += mTimer.Ms();
    if (mCount == mCountMax) {
        if (mCountMax == 1U) {
            MILO_LOG("%s: %s\n", FormatTime(mMin));
        } else {
            MILO_LOG(
                "%s: min %s max %s mean %s\n",
                mName,
                FormatTime(mMin),
                FormatTime(mMax),
                FormatTime(mSum / mCount)
            );
        }
        mCount = 0;
        mMin = 3.402823e+38;
        mMax = 0;
        mSum = 0;
    }
    mTimer.Reset();
}
