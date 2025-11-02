#pragma once
#include "xdk/XAPILIB.h"
#include "os/ThreadCall.h"

bool MainThread();

inline bool MainThread() {
    if (gMainThreadID == -1)
        return true;
    if (GetCurrentThreadId() == gMainThreadID)
        return true;
    return false;
}

inline DWORD CurrentThreadId() { return GetCurrentThreadId(); }

bool ValidateThreadId(unsigned long);
