#include "synth/SampleInst.h"
#include "math/Decibels.h"
#include "obj/Object.h"
#include "synth/SynthSample.h"

SampleInst::SampleInst(SynthSample *sample)
    : unk38(this, sample), mVolume(1), mBankVolume(1), mPan(0), mBankPan(0), mSpeed(1),
      mBankSpeed(1), mSend(this), mEventReceiver(this), unk98(-1), unka0(0), unka1(0) {
    if (unk38) {
        unk38->RegisterChild(this);
    }
}

SampleInst::~SampleInst() {
    if (unk38) {
        unk38->UnregisterChild(this);
    }
}

void SampleInst::Play(float f1) {
    SetVolume(f1);
    Stop(false);
    StartImpl();
    StartPolling();
    unka1 = true;
    unka0 = true;
}

void SampleInst::Stop(bool b1) {
    StopImpl(b1);
    CancelPolling();
}

bool SampleInst::DonePlaying() {
    bool ret = unka1 && !IsPlaying();
    if (ret) {
        delete this;
    }
    return ret;
}

void SampleInst::SetVolume(float vol) {
    mVolume = DbToRatio(vol);
    UpdateVolume();
}

void SampleInst::SetPan(float pan) {
    mPan = pan;
    SetPanImpl(mPan + mBankPan);
}

void SampleInst::SetSpeed(float spd) {
    mSpeed = spd;
    SetSpeedImpl(mSpeed * mBankSpeed);
}

void SampleInst::SetReverbMixDb(float db) {
    mReverbMixDb = db;
    SetReverbMixDbImpl(mReverbMixDb);
}

void SampleInst::SetReverbEnable(bool b) {
    mReverbEnabled = b;
    SetReverbEnableImpl(mReverbEnabled);
}

void SampleInst::SetSend(FxSend *send) {
    mSend = send;
    SetSendImpl(send);
}

void SampleInst::SetEventReceiver(Hmx::Object *rcvr) { mEventReceiver = rcvr; }

void SampleInst::EndLoop() {
    mEventReceiver = nullptr;
    EndLoopImpl();
}

void SampleInst::UpdateVolume() { SetVolumeImpl(mVolume * mBankVolume); }

void SampleInst::SetBankVolume(float vol) {
    mBankVolume = DbToRatio(vol);
    UpdateVolume();
}

void SampleInst::SetBankPan(float bpan) {
    mBankPan = bpan;
    SetPanImpl(mPan + mBankPan);
}

void SampleInst::SetBankSpeed(float bspd) {
    mBankSpeed = bspd;
    SetSpeedImpl(mSpeed * mBankSpeed);
}
