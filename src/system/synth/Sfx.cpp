#include "synth/Sfx.h"
#include "SampleInst.h"
#include "math/Utl.h"
#include "synth/MoggClip.h"
#include "synth/Synth.h"
#include "synth/SynthSample.h"
#include "synth/Utl.h"

#pragma region SfxInst

SfxInst::SfxInst(Sfx *sfx) : SeqInst(sfx), mSfx(sfx), mStartProgress(0) {
    FOREACH (it, sfx->SfxMaps()) {
        SampleInst *inst = nullptr;
        if (it->Sample()) {
            inst = it->Sample()->NewInst(false, 0, -1);
        }
        if (inst) {
            inst->SetBankVolume(it->Volume() + mRandVol);
            inst->SetBankPan(it->Pan() + mRandPan);
            inst->SetBankSpeed(CalcSpeedFromTranspose(it->Transpose() + mRandTp));
            inst->SetFXCore(it->GetFXCore());
            inst->SetADSR(it->ADSR());
            inst->SetSend(sfx->GetSend());
            inst->SetReverbMixDb(sfx->GetReverbMixDb());
            inst->SetReverbEnable(sfx->GetReverbEnable());
            mSamples.push_back(inst);
        }
    }
    FOREACH (it, mSfx->MoggClipMaps()) {
        if (it->GetMoggClip()) {
            it->GetMoggClip()->SetSend(sfx->GetSend());
        }
    }
}

SfxInst::~SfxInst() { DeleteAll(mSamples); }

#pragma endregion
#pragma region Sfx

Sfx::Sfx()
    : mMaps(this), mMoggClipMaps(this), mSend(this), mReverbMixDb(kDbSilence),
      mReverbEnable(false), mSfxInsts(this) {
    mFaders.Add(TheSynth->MasterFader());
    mFaders.Add(TheSynth->SfxFader());
}
