#pragma once

#include "os/Timer.h"

class Profiler {
public:
    Profiler(char const *, int);
    void Start();
    void Stop();

private:
    char const *mName; // 0x0
    Timer mTimer; // 0x8
    float mMin; // 0x38
    float mMax; // 0x3c
    float mSum; // 0x40
    int mCount; // 0x44
    int mCountMax; // 0x48
};
