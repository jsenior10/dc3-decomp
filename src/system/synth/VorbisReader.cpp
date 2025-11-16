#include "synth/VorbisReader.h"
#include "KeyChain.h"
#include "VorbisReader.h"
#include "codec.h"
#include "obj/DataFile.h"
#include "ogg.h"
#include "os/CritSec.h"
#include "os/Debug.h"
#include "synth/Synth.h"
#include "xdk/win_types.h"
#include "xdk/xapilibi/processthreadsapi.h"
#include "xdk/xapilibi/synchapi.h"
#include "xdk/xapilibi/xbox.h"

namespace {
    unsigned char gKey[256];
    std::list<VorbisReader *> gReaders;
    std::list<VorbisReader *> gNewReaders;
    CriticalSection gLock;

    int gCipher = -1;
    int gKeySize = 16;
    unsigned char gRB1Key[16] = { 0x37, 0xB2, 0xE2, 0xB9, 0x1C, 0x74, 0xFA, 0x9E,
                                  0x38, 0x81, 0x08, 0xEA, 0x36, 0x23, 0xDB, 0xE4 };
    HANDLE gEvent = INVALID_HANDLE_VALUE;

    DWORD DecodeThreadEntry(HANDLE);
}

#define VORBIS_FAIL(name, err)                                                           \
    MILO_NOTIFY("Ogg Vorbis failure: %s, error code %i", name, err);

VorbisReader::VorbisReader(File *file, bool expectMap, StandardStream *stream, bool b2)
    : unk28(-1), unk2c(-1), mFile(file), mHeadersRead(0), mReadBuffer(0),
      mEnableReads(true), unk40(0), unk44(0), mDone(0), mStream(stream), mOggSync(0),
      mOggStream(0), mVorbisInfo(0), mVorbisComment(0), mVorbisDsp(0), mVorbisBlock(0),
      unka0(0), mSeekTarget(-1), mSamplesToSkip(0), unkc0(0), mHdrBuf(0), mCtrState(0),
      unkec(b2), unked(0), unkee(0), mFail(0), unk100(-1), unk108(0) {
    MILO_ASSERT(mFile, 0xEC);
    if (expectMap) {
        mHdrBuf = new char[60000];
        mFile->ReadAsync(mHdrBuf, 60000);
        mFail = mFile->Fail();
    }
    mOggSync = new ogg_sync_state();
    ogg_sync_init(mOggSync);
    unk24 = false;
    if (gEvent == INVALID_HANDLE_VALUE) {
        gEvent = CreateEventA(nullptr, false, false, nullptr);
        MILO_ASSERT(gEvent, 0xFE);
        HANDLE t = CreateThread(nullptr, 0x10000, DecodeThreadEntry, nullptr, 4, nullptr);
        MILO_ASSERT(t, 0x103);
        DWORD ret = XSetThreadProcessor(t, 2);
        MILO_ASSERT(ret != -1, 0x107);
        ret = ResumeThread(t);
        MILO_ASSERT(ret != -1, 0x109);
    }
    CritSecTracker tracker(&gLock);
    gNewReaders.push_front(this);
}

VorbisReader::~VorbisReader() {
    unk24 = true;
    unked = false;
    while (unk24) {
        SetEvent(gEvent);
    }
    delete[] mHdrBuf;
    mHdrBuf = nullptr;
    if (mOggStream) {
        ogg_stream_clear(mOggStream);
    }
    if (mVorbisBlock) {
        vorbis_block_clear(mVorbisBlock);
    }
    if (mVorbisDsp) {
        vorbis_dsp_clear(mVorbisDsp);
    }
    if (mVorbisComment) {
        vorbis_comment_clear(mVorbisComment);
    }
    if (mVorbisInfo) {
        vorbis_info_clear(mVorbisInfo);
    }
    ogg_sync_clear(mOggSync);
    RELEASE(mVorbisBlock);
    RELEASE(mVorbisDsp);
    RELEASE(mVorbisComment);
    RELEASE(mVorbisInfo);
    RELEASE(mOggStream);
    RELEASE(mOggSync);
    RELEASE(mCtrState);
}

void VorbisReader::Seek(int sample) {
    CritSecTracker tracker(this);
    MILO_ASSERT(mHeadersRead == 3, 0x1BD);
    MILO_ASSERT(mEnableReads, 0x1BE);
    MILO_ASSERT(sample >= 0, 0x1BF);
    mSeekTarget = sample;
    mDone = false;
    unked = false;
}

void VorbisReader::Init() {
    MILO_ASSERT(mStream, 0x41F);
    mStream->InitInfo(unk28, unk2c, false, mOggMap.GetSongLengthSamples());
}

int VorbisReader::ConsumeData(void **v, int i1, int i2) {
    MILO_ASSERT(mSeekTarget == -1, 0x436);
    if (mSamplesToSkip > 0) {
        int ret = Min(i1, mSamplesToSkip);
        mSamplesToSkip -= ret;
        return ret;
    } else {
        MILO_ASSERT(mStream, 0x43F);
        return mStream->ConsumeData(v, i1, i2);
    }
}

void VorbisReader::setupCypher(int moggVersion) {
    DataArray *arr = DataReadString("{Na 42 'O32'}");
    unsigned int iEval = arr->Evaluate(0).Int();
    arr->Release();

    char i6 = (iEval % 13);
    i6 = i6 + 'A';
    char script[256];
    unsigned char masterKey[256];
    sprintf(script, "{%c %d %c}", i6, (int)masterKey ^ iEval, i6);
    DataArray *buf118Arr = DataReadString(script);
    buf118Arr->Evaluate(0);
    buf118Arr->Release();
    KeyChain::getKey(mKeyIndex, gKey, masterKey);
    TheSynth->Grinder().GrindArray(mMagicA, mMagicB, gKey, 0x10, moggVersion);
    for (int i = 0; i < 16; i++) {
        gKey[i] ^= mKeyMask[i];
    }
    int ret = ctr_start(gCipher, mNonce, gKey, gKeySize, 0, mCtrState);
    memset(gKey, 0, gKeySize);
    MILO_ASSERT(ret == 0, 0xB0);

    sprintf(script, "{ha %d 1}", mMagicA);
    DataArray *magicGenA = DataReadString(script);
    mMagicHashA = magicGenA->Evaluate(0).Int();
    magicGenA->Release();

    sprintf(script, "{ha %d 2}", mMagicB);
    DataArray *magicGenB = DataReadString(script);
    mMagicHashB = magicGenB->Evaluate(0).Int();
    magicGenB->Release();
}

bool VorbisReader::TryReadHeader() {
    if (!mOggStream) {
        ogg_page page;
        int pageOut = ogg_sync_pageout(mOggSync, &page);
        if (pageOut < 0)
            VORBIS_FAIL("StreamInit", pageOut);
        if (pageOut > 0) {
            mOggStream = new ogg_stream_state();
            ogg_stream_init(mOggStream, ogg_page_serialno(&page));
            ogg_stream_pagein(mOggStream, &page);
            mVorbisInfo = new vorbis_info();
            vorbis_info_init(mVorbisInfo);
            mVorbisComment = new vorbis_comment();
            vorbis_comment_init(mVorbisComment);
        } else
            return false;
    }
    if (mHeadersRead == 3)
        return false;
    else {
        ogg_packet packet;
        if (TryReadPacket(packet)) {
            int vorbisErr =
                vorbis_synthesis_headerin(mVorbisInfo, mVorbisComment, &packet);
            if (vorbisErr < 0)
                VORBIS_FAIL("HeaderIn", vorbisErr);
            mHeadersRead++;
            return true;
        } else
            return false;
    }
}

void VorbisReader::InitDecoder() {
    MILO_ASSERT(!mVorbisDsp && !mVorbisBlock, 0x2C0);
    MILO_ASSERT(mHeadersRead == 3, 0x2C1);
    mVorbisDsp = new vorbis_dsp_state();
    vorbis_synthesis_init(mVorbisDsp, mVorbisInfo);
    mVorbisBlock = new vorbis_block();
    vorbis_block_init(mVorbisDsp, mVorbisBlock);
}

bool VorbisReader::DoSeek() {
    mEnableReads = false;
    DoFileRead();
    if (!mFail && !mReadBuffer) {
        int i1, i2;
        mOggMap.GetSeekPos(mSeekTarget, i1, i2);
        DoRawSeek(i1);
        mSamplesToSkip = mSeekTarget - i2;
        MILO_ASSERT(mSamplesToSkip >= 0, 0x3C2);
        mSeekTarget = -1;
        mEnableReads = true;
        return true;
    }
    return false;
}
