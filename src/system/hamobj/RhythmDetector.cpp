#include "RhythmDetector.h"

#include "gesture/GestureMgr.h"

void CameraToScreenUnit(Vector3 &vec, const Skeleton &skel, SkeletonJoint skeljoint) {
    Vector2 vec2;
    skel.ScreenPos(skeljoint, vec2);
    vec.y = -skel.TrackedJoints()[skeljoint].unk60.z * 0.22977939;
    vec.x = (vec2.x - 0.5f) * 2;
    vec.z = (vec2.y - 0.5f) * 2;
}

DebugGraph::DebugGraph(
    float f1,
    float f2,
    float f3,
    float f4,
    Hmx::Color color1,
    Hmx::Color color2,
    int i1,
    float f5,
    float f6,
    String s
)
    : mRect(f1, f2, f3, f4), mColorA(color1), mColorB(color2), unk38(i1), unk3c(f5), unk40(f6),
      unk44(3.4028235e+38), unk48(s), unk50(1) {}

namespace {
    int kAnalyzeJoints[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
    int gDebugBone = -1;
    float gAdjust = 1;
    int gLog = -1;
    bool gClamp = true;
    const char **kConv;
    int kConvCount = 4;
    int kConvLen;

    DataNode TightenDebugBone(DataArray *da) {
        gAdjust *= 1.01f;
        MILO_LOG("scalar %f\n", gAdjust);
        return 0;
    }

    DataNode LoosenDebugBone(DataArray *da) {
        gAdjust *= 0.99009901f;
        MILO_LOG("scalar %f\n", gAdjust);
        return 0;
    }

    DataNode CycleDebugBone(DataArray *da) {
        gDebugBone += 1;
        gAdjust = 1.0;

        if (gDebugBone == 20) {
            gDebugBone = -1;
        }
        MILO_LOG("debug bone %d\n", gDebugBone);
        return 0;
    }

    DataNode DataSpaceCheat(DataArray *da) {
        gLog += 1;
        if (60 <= gLog) {
            gLog = -1;
        }
        return 0;
    }

    void AnalyzeData(std::vector<RhythmDetector::Frame> const &frames, float &f1, float &f2, float &f3, float f4, bool b1, Symbol sym, bool b2, DebugGraph *graph, int i1, TextStream *stream) {}

    std::vector<float> const &minJointSpeedVector() {
        return std::vector<float>();
    }

    float Mean(std::vector<float> const &vec, int i1, int i2) {
        float sum = 0;
        for (int i = 0; i < vec.size(); i++) {
            sum += vec[i];
        }
        if (i2 - vec.size() == 0) {
            return 0.0;
        }

        return sum / vec.size();
    }

}

DebugGraph::~DebugGraph() {}

BEGIN_HANDLERS(RhythmDetector)
HANDLE_ACTION(start_recording, StartRecording())
HANDLE_ACTION(stop_recording, StopRecording())
HANDLE_EXPR(is_recording, mRecording)
HANDLE_SUPERCLASS(RndPollable)
END_HANDLERS

RhythmDetector::RhythmDetector() : unkc(0), mRecording(0), unk10(-1), unk14(0), unk20(0), unk2c(0), unk38(0), unk44(8.0), unk48(0.0), unk4c(0.0), unk50(2), unk54(1.5), unk58(0.0), unk5c(0.0), unk60(0.0), unk68(0.0), unk80(), unka80(0), mRecordData()  {

}

RhythmDetector::~RhythmDetector(){}

float RhythmDetector::Groove() const {
    if (unkc) {
        return unk48;
    }
    return 0.0;
}

float RhythmDetector::Freshness() const {
    if (unkc) {
        return 1 - unk4c;
    }
    return 0.0;
}

Vector4 RhythmDetector::Data1(int i1) const {
    Vector4 vec;
    vec.x = unkaac[i1];
    vec.y = unkab0[i1];
    vec.w = 0.0;
    vec.z = unkab0[i1];
    return vec;
}

Vector4 RhythmDetector::Data2(int i1) const {
    Vector4 vec;
    vec.x = 0.0;
    vec.y = 1.0;
    vec.z = 1.0;
    vec.w = 1.0;
    return vec;
}

void RhythmDetector::AddDebugGraph(float f1, float f2, float f3, float f4, Hmx::Color color) {
    if (mDebugGraphE) {
        delete mDebugGraphE;
    }
    if (mDebugGraphE != 0) {
        String s = MakeString("beats %d fold %d dir %.1f %.1f %.1f", unk44, unk50, unk58, unk5c, unk60);
        mDebugGraphE = new DebugGraph(f1, f2, f3, f4, Hmx::Color(0x3ecccccd, 0x3ecccccd, 0x3ecccccd), Hmx::Color(0x3ecccccd, 0x3ecccccd, 0x3ecccccd), s.length(), 0, 2, s);
    }
}

void RhythmDetector::AddFullDebugGraphs() {
    if (gLog != -1) {
        if (mDebugGraphA) {
            delete mDebugGraphA;
        }
        mDebugGraphA = new DebugGraph(0.1,0.0,0.8,0.06, Hmx::Color(0.0,0.0,0.0,0.0), Hmx::Color(0.0,0.0,0.0,0.0), 0, -1.1, 1.1, "");
    }
    mDebugGraphA->SetUnk50(false);
}

void RhythmDetector::RemoveDebugGraphs() {
    if (mDebugGraphA) {
        delete mDebugGraphA;
    }
    if (mDebugGraphB) {
        delete mDebugGraphB;
    }
    if (mDebugGraphC) {
        delete mDebugGraphC;
    }
    if (mDebugGraphD) {
        delete mDebugGraphD;
    }
    if (mDebugGraphE) {
        delete mDebugGraphE;
    }
}

void RhythmDetector::ClearData() {
    unk68 = 0;
    if (!unk2c.empty()) {
        unk2c.erase(0);
    }
    if (!unk38.empty()) {
        unk38.erase(0);
    }
    if (!mRecordData.frames.empty()) {
        mRecordData.frames.erase(0);
    }
    unk48 = 0;
    unk4c = 0;
    mRecordData.unkc04 = true;
    mRecordData.unkbf4 = -1;
    mRecordData.unkbf8 = -1;
    mRecordData.unkbec = -1;
    mRecordData.unkbf0 = -1;
    mRecordData.unkbfc = -1;
    mRecordData.unkc00 = -1;
    unk14.clear();
    AddFullDebugGraphs();
}

RhythmDetector::RecordData const &RhythmDetector::GetRecord(float f1, float f2, bool b, Symbol sym, TextStream *stream) {
    if (mRecordData.unkbec == f1 && mRecordData.unkbf0 == f2) {
        if (stream) {
            //AnalyzeData(unk38, mRecordData.unkbfc, mRecordData.unkc00, unk4c, unk54, )
            mRecordData.unkc04 = true;
        }
    }
    else {
        if (!mRecordData.unkc04) {
            MILO_NOTIFY("new rhythm detector window w/o finalization [%.1f,%.1f] to [%.1f, %.1f]", mRecordData.unkbec, mRecordData.unkbf0);
        }
        ClearData();
        mRecordData.unkbec = f1;
        mRecordData.unkbf0 = f2;
        mRecordData.unkc04 = false;
        mRecordData.unkbf4 = -1;
        mRecordData.unkbf8 = -1;
        if (!mRecordData.frames.empty()) {
            mRecordData.frames.erase(0);
        }
    }
    return mRecordData;
}

void RhythmDetector::StartRecording() {
    if (mRecording + 1 == 1) {
        AddFullDebugGraphs();
        unkaa8 = TheTaskMgr.Beat();
        ClearData();
    }
    if (mRecording < 1) {
        MILO_ASSERT(mRecording >= 1, 0x3cc);
    }
    if (mRecording > 2) {
        MILO_ASSERT(mRecording <= 2, 0x3cd);
    }
}

void RhythmDetector::StopRecording() {
    if (mRecording - 1 == 0) {
        unkaa8 = TheTaskMgr.Beat();
        ClearData();
    }
    if (mRecording >= 127) {
        MILO_ASSERT(mRecording >= 0, 0x3da);
    }
    if (mRecording > 1) {
        MILO_ASSERT(mRecording <= 1, 0x3db);
    }
}

void SetupFrame(
    RhythmDetector::Frame &frame, float prev_beat, float delt_beat, Vector3 const *prev, Vector3 const *pos, float f3
) {
    if (prev_beat < 0) {
        MILO_ASSERT(prev_beat >= 0, 0x4d7);
    }
    if (delt_beat < 0) {
        MILO_ASSERT(delt_beat >= 0, 0x4d8);
    }
    if (!prev) {
        MILO_ASSERT(prev, 0x4d9);
    }
    if (!pos) {
        MILO_ASSERT(pos, 0x4da);
    }
}

RhythmDetector::Frame BlendFrameDataToBeat(RhythmDetector::Frame const &a, RhythmDetector::Frame const &b, float f1) {
    if (f1 < b.unk0 || a.unk0 < f1) {
        MILO_NOTIFY("bad rhythm detector floating point precision at %f %f %f\n", b.unk0, a.unk0);
    }
    int kSize = b.mJointVelocities.size();
    if (kSize != a.mJointVelocities.size()) {
        MILO_ASSERT(kSize == b.mJointVelocities.size(), 0x5a9);
    }
    return RhythmDetector::Frame();
}

void RhythmDetector::Poll() {
    if (mRecording != 0) {
        if (TheGestureMgr->GetSkeleton(unk10).IsTracked()) {
            if (mDebugGraphA) {
                mDebugGraphA->Draw();
            }
            if (mDebugGraphB) {
                mDebugGraphB->Draw();
            }
            if (mDebugGraphC) {
                mDebugGraphC->Draw();
            }
            if (mDebugGraphD) {
                mDebugGraphD->Draw();
            }
            if (mDebugGraphE) {
                mDebugGraphE->Draw();
            }
        }
    }
}

void EraseNewerData(std::vector<RhythmDetector::Frame> &frames, float f1) {
    int i = 0;
    FOREACH(it, frames) {
        if (it->unk0 >= f1) {
            frames.erase(it, frames.end());
        }
    }
}

void RhythmDetector::PostUpdate(const struct SkeletonUpdateData *sud) {

}