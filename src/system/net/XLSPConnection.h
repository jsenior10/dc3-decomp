#pragma once
#include "os/ThreadCall.h"
#include "os/Timer.h"
#include "utl/Str.h"

class XLSPConnection : public ThreadCallback {
public:
    enum State {
    };
    XLSPConnection();
    virtual ~XLSPConnection();
    virtual int ThreadStart();
    virtual void ThreadDone(int);

    State GetState() { return unk4; }
    void Poll();
    unsigned int GetServiceIP();
    void Connect(const char *, unsigned int);
    void Disconnect();

private:
    State unk4;
    int unk8;
    String unkc;
    unsigned int unk14;
    int unk18;
    int unk1c;
    int unk20;
    int unk24;
    XOVERLAPPED mXOverlapped; // 0x28
    int unk44;
    Timer unk48;
};
