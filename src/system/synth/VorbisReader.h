#pragma once
#include "oggvorbis/codec.h"
#include "oggvorbis/ogg.h"
#include "os/CritSec.h"
#include "os/File.h"
#include "synth/OggMap.h"
#include "synth/StandardStream.h"
#include "synth/StreamReader.h"
#include "synth/tomcrypt/mycrypt.h"

class VorbisReader : public StreamReader, public CriticalSection {
public:
    VorbisReader(File *, bool, StandardStream *, bool);
    virtual ~VorbisReader();
    virtual void Poll(float);
    virtual void Seek(int);
    virtual void EnableReads(bool enable) { mEnableReads = enable; }
    virtual bool Done() { return mDone; }
    virtual bool Fail() { return mFail; }

protected:
    virtual void Init();
    virtual int ConsumeData(void **, int, int);
    virtual void EndData() {}

    bool unk24;
    int unk28;
    int unk2c;
    File *mFile; // 0x30
    int mHeadersRead; // 0x34
    int unk38;
    bool mEnableReads; // 0x3c
    int unk40;
    bool unk44;
    bool mDone; // 0x45
    int unk48;
    StandardStream *mStream; // 0x4c
    ogg_sync_state *mOggSync; // 0x50
    ogg_stream_state *mOggStream; // 0x54
    vorbis_info *mVorbisInfo; // 0x58
    vorbis_comment *mVorbisComment; // 0x5c
    vorbis_dsp_state *mVorbisDsp; // 0x60
    vorbis_block *mVorbisBlock; // 0x64
    int unk68;
    int unk6c;
    int unk70;
    int unk74;
    int unk78;
    int unk7c;
    ogg_packet mPendingPacket; // 0x80
    bool unka0;
    int mSeekTarget; // 0xa4
    int mSamplesToSkip; // 0xa8
    OggMap mOggMap; // 0xac
    int unkc0;
    char *mHdrBuf; // 0xc4
    symmetric_CTR *mCtrState; // 0xc8
    unsigned char mNonce[16]; // 0xcc
    unsigned char mKeyMask[16]; // 0xdc
    bool unkec;
    bool unked;
    bool unkee;
    bool mFail; // 0xef
    int unkf0;
    std::vector<short> unkf4; // 0xf4
    u64 unk100;
    int unk108;
};
