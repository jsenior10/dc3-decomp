#include "hamobj/MoveDir.h"
#include "HamMaster.h"
#include "MoveDir.h"
#include "ScoreUtl.h"
#include "char/Character.h"
#include "flow/PropertyEventProvider.h"
#include "gesture/GestureMgr.h"
#include "gesture/SkeletonClip.h"
#include "gesture/SkeletonDir.h"
#include "gesture/SkeletonUpdate.h"
#include "gesture/SkeletonViz.h"
#include "hamobj/CharFeedback.h"
#include "hamobj/DancerSequence.h"
#include "hamobj/DetectFrame.h"
#include "hamobj/Difficulty.h"
#include "hamobj/FilterVersion.h"
#include "hamobj/HamDirector.h"
#include "hamobj/HamGameData.h"
#include "hamobj/HamMove.h"
#include "hamobj/HamPlayerData.h"
#include "hamobj/MoveDetector.h"
#include "hamobj/ScoreUtl.h"
#include "meta/SongMetadata.h"
#include "meta/SongMgr.h"
#include "obj/Data.h"
#include "obj/DataFile.h"
#include "obj/DataUtl.h"
#include "obj/Dir.h"
#include "obj/DirLoader.h"
#include "obj/Object.h"
#include "obj/Task.h"
#include "obj/Utl.h"
#include "os/Debug.h"
#include "os/File.h"
#include "os/System.h"
#include "rndobj/Dir.h"
#include "rndobj/Font.h"
#include "rndobj/FontBase.h"
#include "rndobj/Overlay.h"
#include "ui/ResourceDirPtr.h"
#include "ui/UILabelDir.h"
#include "utl/BinStream.h"
#include "utl/FilePath.h"
#include "utl/Loader.h"
#include "utl/SongInfoCopy.h"
#include "utl/Std.h"
#include "utl/Symbol.h"
#include "world/Dir.h"

std::vector<FilterVersion *> MoveDir::sFilterVersions;

MoveDir::MoveDir()
    : mShowMoveOverlay(0), mErrorNodeInfo(0), mPlayClip(this), mRecordClip(this),
      unk2bc(this), unk2d0(this), unk2e4(0), mReportMove(this), mFiltersEnabled(0),
      unk308(0), unk30c(0), mFilterQueue(0), mAsyncDetector(0), unk394(0),
      mFinishingMoveMeasure(10000), mMoveOverlay(RndOverlay::Find("ham_move", true)),
      mDancerSeq(this), unk414(0), mSkeletonViz(Hmx::Object::New<SkeletonViz>()),
      unk41c(0), mDebugLatencyOffset(0), unkef8(0), mLastPollMs(0), mDebugCollision(0),
      unkf84(-1) {
    for (int i = 0; i < 2; i++) {
        mMovePlayerData[i].Reset();
        unkf04[i].Reset();
    }
    SetFilterVersion("ham2");
}

MoveDir::~MoveDir() {
    RELEASE(mFilterQueue);
    RELEASE(mAsyncDetector);
    mMoveOverlay = RndOverlay::Find("ham_move", false);
    if (mMoveOverlay && mMoveOverlay->GetCallback() == this) {
        mMoveOverlay->SetCallback(nullptr);
        if (TheLoadMgr.EditMode()) {
            mMoveOverlay->SetShowing(false);
        }
    }
    delete mSkeletonViz;
    if (SkeletonUpdate::HasInstance()) {
        SkeletonUpdateHandle handle = SkeletonUpdate::InstanceHandle();
        if (handle.HasCallback(this)) {
            handle.RemoveCallback(this);
        }
    }
}

BEGIN_HANDLERS(MoveDir)
    HANDLE_ACTION(start_song_record, 0)
    HANDLE_ACTION(stop_song_record, StopSongRecord())
    HANDLE_ACTION(
        simulate_song,
        SimulateSong(
            _msg->Size() > 2 ? _msg->Int(2) : 0, _msg->Size() > 3 ? _msg->Int(3) : 0
        )
    )
    HANDLE_ACTION(reload_scoring, ReloadScoring())
    HANDLE_ACTION(reset_detection, ResetDetection())
    HANDLE(stream_jump, OnStreamJump)
    HANDLE_EXPR(import_clip, ImportClip(_msg->Int(2)))
    HANDLE_ACTION(debug_rotate, mSkeletonViz->Rotate(_msg->Float(2)))
    // these don't appear to be inlined methods
    {
        static Symbol _s("disable_all_detectors");
        if (sym == _s) {
            MILO_ASSERT(mAsyncDetector, 0x136F);
            mAsyncDetector->DisableAllDetectors();
            return 0;
        }
    }
    {
        static Symbol _s("enable_detector");
        if (sym == _s) {
            MILO_ASSERT(mAsyncDetector, 0x1371);
            mAsyncDetector->EnableDetector(_msg->Obj<HamMove>(2));
            return 0;
        }
    }
    {
        static Symbol _s("disable_detector");
        if (sym == _s) {
            MILO_ASSERT(mAsyncDetector, 0x1373);
            mAsyncDetector->DisableDetector(_msg->Obj<HamMove>(2));
            return 0;
        }
    }
    HANDLE_EXPR(
        active_detector_result,
        mAsyncDetector->MoveRatingFrac(
            _msg->Int(2), (MoveAsyncDetector::RatingBar)0, _msg->Obj<HamMove>(3)
        )
    )
    HANDLE_EXPR(
        last_detector_result,
        mAsyncDetector->MoveRatingFrac(
            _msg->Int(2), (MoveAsyncDetector::RatingBar)1, _msg->Obj<HamMove>(3)
        )
    )
    HANDLE_EXPR(cur_move_normalized_result, mCurMoveNormalizedResult[_msg->Int(2)])
    HANDLE_EXPR(
        active_detector_looped_result,
        mAsyncDetector->MoveRatingFrac(
            _msg->Int(2), (MoveAsyncDetector::RatingBar)2, _msg->Obj<HamMove>(3)
        )
    )
    HANDLE_EXPR(
        cur_move_normalized_result_smoothed, mCurMoveSmoothers[_msg->Int(2)].Level()
    )
    HANDLE_ACTION(
        detector_clear_looped_result,
        mAsyncDetector->ClearLoopedRatingFrac(_msg->Obj<HamMove>(2))
    )
    HANDLE_EXPR(get_cur_move, mCurMove[_msg->Int(2)])
    HANDLE_EXPR(get_cur_measure, MoveIdx())
    HANDLE_EXPR(get_cur_beat, TheTaskMgr.TotalBeat())
    HANDLE_EXPR(get_finishing_move_measure, mFinishingMoveMeasure)
    HANDLE_ACTION(clear_limb_feedback, ClearLimbFeedback(_msg->Int(2)))
    HANDLE_ACTION(beat, OnBeat())
    HANDLE_SUPERCLASS(SkeletonDir)
END_HANDLERS

BEGIN_PROPSYNCS(MoveDir)
    SYNC_PROP_SET(current_move, mMovePlayerData[0].mCurMove.Ptr(), )
    SYNC_PROP_SET(filters_enabled, mFiltersEnabled, SetFiltersEnabled(_val.Int()))
    SYNC_PROP_SET(move_overlay, mShowMoveOverlay, SetMoveOverlay(_val.Int()))
    SYNC_PROP(debug_latency_offset, mDebugLatencyOffset)
    SYNC_PROP_SET(
        debug_skeleton_rotation,
        mSkeletonViz->PhysicalCamRotation(),
        mSkeletonViz->SetPhysicalCamRotation(_val.Float())
    )
    SYNC_PROP(debug_collision, mDebugCollision)
    SYNC_PROP(debug_node_types, mErrorNodeInfo)
    SYNC_PROP(debug_node_joints, mErrorNodeInfo)
    SYNC_PROP_SET(play_clip, mPlayClip.Ptr(), SetSongPlayClip(_val.Obj<SkeletonClip>()))
    SYNC_PROP(report_move, mReportMove)
    SYNC_PROP(record_clip, mRecordClip)
    SYNC_PROP(import_clip_path, mImportClipPath)
    SYNC_SUPERCLASS(SkeletonDir)
END_PROPSYNCS

BEGIN_SAVES(MoveDir)
    SAVE_REVS(35, 0)
    SAVE_SUPERCLASS(SkeletonDir)
    if (IsProxy()) {
        bs << mFiltersEnabled;
    }
    bs << mShowMoveOverlay;
    bs << mErrorNodeInfo;
    if (!bs.Cached()) {
        bs << mImportClipPath;
    } else {
        bs << 0;
    }
    MILO_ASSERT(mFilterVer, 0x922);
    bs << mFilterVer->mVersionSym;
END_SAVES

BEGIN_COPYS(MoveDir)
    COPY_SUPERCLASS(SkeletonDir)
    CREATE_COPY(MoveDir)
    BEGIN_COPYING_MEMBERS
        COPY_MEMBER(mShowMoveOverlay)
        COPY_MEMBER(mErrorNodeInfo)
        COPY_MEMBER(mImportClipPath)
        COPY_MEMBER(mFiltersEnabled)
        COPY_MEMBER(mPlayClip)
        COPY_MEMBER(mRecordClip)
        COPY_MEMBER(unk2bc)
        COPY_MEMBER(unk2d0)
        COPY_MEMBER(mReportMove)
    END_COPYING_MEMBERS
END_COPYS

BEGIN_LOADS(MoveDir)
    PreLoad(bs);
    PostLoad(bs);
END_LOADS

void MoveDir::PreLoad(BinStream &bs) {
    LOAD_REVS(bs)
    ASSERT_REVS(0x23, 0)
    if (d.rev < 9) {
        RndDir::PreLoad(bs);
    } else {
        SkeletonDir::PreLoad(bs);
    }
    Symbol song = TheGameData->GetSong();
    if (!IsProxy() && gLoadingProxyFromDisk && !song.Null()) {
        SongMgr *songMgr = ObjectDir::Main()->Find<SongMgr>("song_mgr", false);
        if (songMgr) {
            const SongMetadata *songData =
                songMgr->Data(songMgr->GetSongIDFromShortName(song, true));
            if (songData->Version() < 11) {
                unk394 = dynamic_cast<DirLoader *>(TheLoadMgr.AddLoader(
                    FilePath(FileRoot(), songMgr->SongFilePath(song, "_update.milo", 11)),
                    kLoadFront
                ));
            }
        }
    }
    d.PushRev(this);
}

void MoveDir::PostLoad(BinStream &bs) {
    BinStreamRev d(bs, bs.PopRev(this));
    if (d.rev < 9) {
        RndDir::PostLoad(bs);
    } else {
        SkeletonDir::PostLoad(bs);
    }
    if (d.rev < 5) {
        bool b;
        d >> b;
    }
    if (d.rev > 0 && d.rev < 2) {
        String str;
        d >> str;
    }
    if (!IsProxy() || d.rev < 8) {
        if (d.rev > 3 && d.rev < 9) {
            String str;
            d >> str;
        }
        if (d.rev > 5 && d.rev < 32) {
            if (d.rev > 0x1A) {
                ObjPtrVec<HamMove> moves(this, (EraseMode)0, kObjListAllowNull);
                d >> moves;
            } else {
                ObjPtr<HamMove> move(this);
                d >> move;
            }
        }
    }
    if (IsProxy() && d.rev > 10 && d.rev < 0xD) {
        ObjPtr<Character> character(this);
        WorldDir *wDir = TheHamDirector ? TheHamDirector->GetVenueWorld() : nullptr;
        character.Load(d.stream, true, wDir);
    }
    if (IsProxy() && d.rev > 0xC) {
        d >> mFiltersEnabled;
    }
    char buf[0x80];
    if (d.rev < 0x23) {
        if (IsProxy() && d.rev > 0xE) {
            d.stream.ReadString(buf, 0x80);
        }
        if (IsProxy() && d.rev > 0xD) {
            d.stream.ReadString(buf, 0x80);
        }
        if (IsProxy() && d.rev > 0x17) {
            d.stream.ReadString(buf, 0x80);
        }
    }
    if (d.rev > 6) {
        if (d.rev > 9 && d.rev < 18) {
            int x;
            d >> x;
            mShowMoveOverlay = x;
        } else {
            d >> mShowMoveOverlay;
        }
    }
    if (d.rev > 0xF && d.rev < 0x1F) {
        bool b;
        d >> b;
    }
    if (d.rev > 0x16 && d.rev < 0x22) {
        bool b;
        d >> b;
    }
    if (d.rev > 0x14) {
        if (d.rev > 0x1B) {
            d >> mErrorNodeInfo;
        } else {
            Symbol s;
            d >> s;
        }
    } else if (d.rev > 0x11) {
        int x;
        d >> x;
    }
    if (d.rev > 0x15 && d.rev < 0x1D) {
        bool b;
        d >> b;
    }
    if (d.rev > 0x19 && d.rev < 0x21) {
        int x;
        d >> x;
        bool b;
        d >> b;
        int y, z;
        d >> y >> z;
        for (int i = 0; i < 3; i++) {
            d >> b >> b;
        }
    }
    if (d.rev > 0x13) {
        d >> mImportClipPath;
    }
    if (d.rev < 0x19) {
        if (d.rev > 0x14) {
            int x;
            d >> x;
            Symbol s;
            for (int i = 0; i < x; i++) {
                int n;
                d >> s >> n;
            }
        } else if (d.rev > 0xB) {
            int max = 5;
            if (d.rev < 0x11) {
                max = 4;
            }
            for (int i = 0; i < max; i++) {
                int x;
                d >> x;
            }
        }
    }
    Symbol filterVersion;
    static Symbol ham1("ham1");
    static Symbol ham2("ham2");
    if (d.rev < 0x1A) {
        filterVersion = ham1;
    } else if (d.rev < 0x1E) {
        filterVersion = ham2;
    } else {
        d >> filterVersion;
    }
    SetFilterVersion(filterVersion);
    if (unk394) {
        ObjectDir *loaderDir = unk394->GetDir();
        RELEASE(unk394);
        if (loaderDir) {
            for (ObjDirItr<Hmx::Object> it(loaderDir, true); it != nullptr; ++it) {
                Hmx::Object *cur = it;
                if (cur != loaderDir) {
                    const char *curName = cur->Name();
                    HamMove *move = dynamic_cast<HamMove *>(cur);
                    if (move) {
                        HamMove *find = Find<HamMove>(curName, false);
                        if (find) {
                            find->Update(move);
                        }
                    } else {
                        ObjectDir *dir = dynamic_cast<ObjectDir *>(cur);
                        if (dir && !*dir->GetPathName()) {
                            continue;
                        } else {
                            Hmx::Object *find = Find<Hmx::Object>(curName, false);
                            if (find) {
                                delete find;
                            }
                            it->SetName(curName, this);
                        }
                    }
                }
            }
            delete loaderDir;
        } else {
            MILO_NOTIFY("%s has no associated update file for song", PathName(this));
        }
    }
    static Symbol DLC_UPDATE_FONTS("DLC_UPDATE_FONTS");
    DataArray *updateArray = DataGetMacro(DLC_UPDATE_FONTS);
    for (int i = 0; i < updateArray->Size(); i++) {
        char buffer[256];
        String curStr(updateArray->Str(i));
        strcpy(buffer, MakeString("%s_%s", curStr, SystemLanguage()));
        AddClassExt(buffer, RndFont::StaticClassName());
        RndFont *updateFont = Find<RndFont>(buffer, false);
        if (updateFont) {
            FilePath path;
            if (ResourceDirBase::MakeResourcePath(
                    path, "HamLabel", "UILabelDir", curStr.c_str()
                )) {
                ObjDirPtr<UILabelDir> labelDirPtr;
                labelDirPtr.LoadFile(path, false, true, kLoadFront, false);
                if (labelDirPtr.IsLoaded()) {
                    RndFontBase *font = labelDirPtr->FontObj(gNullStr);
                    MILO_ASSERT(font, 0xA52);
                    ReplaceObject(font, updateFont, false, false, true);
                    unk398.push_back(labelDirPtr);
                }
            }
        }
    }
    if (d.rev < 3 && !IsProxy()) {
        MILO_NOTIFY(
            "%s MoveDir older than version 3, need to resave this file", PathName(this)
        );
    }
    if (TheLoadMgr.EditMode()) {
        if (mFiltersEnabled) {
            MiloInit();
        }
    } else {
        mRecordClip = nullptr;
    }
}

void MoveDir::Poll() {
    SkeletonDir::Poll();
    mSkeletonViz->Poll();
    if (TheHamDirector) {
        int curMeasure = TheTaskMgr.CurrentMeasure();
        for (int i = 0; i < 2; i++) {
            HamMove *oldMove = mCurMove[i];
            mCurMove[i] = nullptr;
            filler[i] = oldMove;
            MovePlayerData &curPlayerData = mMovePlayerData[i];
            if (curMeasure >= 0 && curMeasure < curPlayerData.unk20.size()) {
                mCurMove[i] = curPlayerData.unk20[curMeasure].move;
            }
            MoveRating oldRating = mCurMoveRating[i];
            mCurMoveRating[i] = kMoveRatingOk;
            unk3f0[i] = oldRating;
            float oldRes = mCurMoveNormalizedResult[i];
            mCurMoveNormalizedResult[i] = 0;
            unk3e8[i] = oldRes;

            if (mCurMove[i]) {
                std::pair<DetectFrame *, DetectFrame *> frames;
                DetectRange(curPlayerData.unk14, frames, curMeasure, curMeasure);
                float frac = DetectFrac(i, mCurMove[i], frames);
                mCurMoveRating[i] =
                    DetectFracToMoveRating(frac, mCurMove[i]->RatingOverride());
                mCurMoveNormalizedResult[i] =
                    DetectFracToRatingFrac(frac, mCurMove[i]->RatingOverride());
            }
            mCurMoveSmoothers[i].Smooth(
                mCurMoveNormalizedResult[i],
                TheMaster && TheMaster->Unk70() == 3 ? TheTaskMgr.DeltaUISeconds() * 4.0f
                                                     : TheTaskMgr.DeltaUISeconds()
            );
            if (mCurMoveRating[i] <= kMoveRatingPerfect && unk3f0[i] > 1) {
                static Symbol passed_move_p1("passed_move_p1");
                static Symbol passed_move_p2("passed_move_p2");
                TheHamProvider->Export(
                    Message(i == 0 ? passed_move_p1 : passed_move_p2, mCurMove[i]->Name()),
                    true
                );
            }
        }
        if ((mCurMoveRating[0] <= 1 || mCurMoveRating[1] <= 1) && unk3f0[0] > 1
            && unk3f0[1] > 1) {
            static Message msg_passed_move("passed_move");
            TheHamProvider->Export(msg_passed_move, true);
        }
    }
}

void MoveDir::ClearLimbFeedback(int player) {
    MILO_LOG("MoveDir::ClearLimbFeedback(int player = %d)\n", player);
    CharFeedback *feedback = mMovePlayerData[player].mFeedback;
    HamPlayerData *hpd = TheGameData->Player(player);
    if (feedback && hpd) {
        feedback->ResetErrors();
        for (int i = 0; i < 4; i++) {
            feedback->UpdateLimb(i, false);
        }
    }
}

void MoveDir::SetFiltersEnabled(bool enabled) {
    mFiltersEnabled = enabled;
    if (mFiltersEnabled && TheLoadMgr.EditMode()) {
        MiloInit();
    }
}

void MoveDir::SetFilterVersion(Symbol version) {
    for (int i = 0; i < sFilterVersions.size(); i++) {
        if (sFilterVersions[i]->mVersionSym == version) {
            mFilterVer = sFilterVersions[i];
            return;
        }
    }
    MILO_FAIL("Could not find filter version %s", version);
}

const FilterVersion *MoveDir::FindFilterVersion(FilterVersionType t) {
    for (std::vector<FilterVersion *>::iterator it = sFilterVersions.begin();
         it != sFilterVersions.end();
         ++it) {
        if ((*it)->mType == t)
            return *it;
    }
    return nullptr;
}

HamMove *MoveDir::CurrentMove(int player) const {
    MILO_ASSERT((0) <= (player) && (player) < (2), 0x164);
    return mMovePlayerData[player].mCurMove;
}

int MoveDir::MoveIdx() const { return TheTaskMgr.CurrentMeasure(); }
int MoveDir::MoveBeat() const { return TheTaskMgr.CurrentBeat(); }

void MoveDir::SetMoveOverlay(bool overlay) {
    if (!mFiltersEnabled && TheLoadMgr.EditMode()) {
        mFiltersEnabled = true;
        if (TheLoadMgr.EditMode()) {
            MiloInit();
        }
    }
    mShowMoveOverlay = overlay;
    mMoveOverlay->SetShowing(overlay);
}

SkeletonClip *MoveDir::ImportClip(bool b1) {
    if (mImportClipPath.empty()) {
        MILO_NOTIFY("Set import_clip_path first");
        return nullptr;
    } else {
        const char *filename = FileGetName(mImportClipPath.c_str());
        SkeletonClip *clip = Find<SkeletonClip>(filename, false);
        if (clip) {
            MILO_LOG("%s already exists, not importing\n", filename);
        } else {
            clip = Hmx::Object::New<SkeletonClip>();
            clip->SetName(filename, this);
            clip->SetPath(mImportClipPath.c_str());
        }
        return clip;
    }
}

void MoveDir::StopSongRecord() {
    if (mRecordClip && mRecordClip->IsRecording()) {
        mRecordClip->StopRecording();
        if (unk2bc)
            unk2bc->StopRecording();
    } else {
        MILO_NOTIFY("Start recording first");
    }
}

String RecordClipName(const char *, int);

void MoveDir::FlushMoveRecord() {
    SkeletonClip *clip = unk2d0;
    if (clip) {
        clip->FlushMoveRecord(RecordClipName("ktb", -1).c_str());
    } else {
        MILO_NOTIFY("skeleton recording not yet active");
    }
}

void MoveDir::SwapMoveRecord() {
    if (unk2d0) {
        unk2d0->SwapMoveRecord();
    } else {
        MILO_NOTIFY("skeleton recording not yet active");
    }
}

HamMove *MoveDir::GetMoveAtMeasure(int player, int i2) {
    static Symbol move("move");
    HamPlayerData *hpd = TheGameData->Player(player);
    Keys<Symbol, Symbol> *keys =
        TheHamDirector->GetPropKeys(hpd->GetDifficulty(), move)->AsSymbolKeys();
    return Find<HamMove>((*keys)[i2].value.Str(), false);
    return nullptr;
}

DancerSequence *MoveDir::PerformanceSequence(Difficulty diff) {
    MILO_ASSERT((0) <= (diff) && (diff) < (kNumDifficulties), 0x207);
    Symbol diffSym = DifficultyToSym(diff);
    const char *seqName = MakeString("performance_%s.seq", diffSym);
    return Find<DancerSequence>(seqName, false);
}

void SetupRecordClip(
    ObjPtr<SkeletonClip> &clip, int i1, int i2, const char *cc, ObjectDir *dir
) {
    clip = Hmx::Object::New<SkeletonClip>();
    clip->EnableAlternateRecord(i1);
    String clipName = RecordClipName(cc, i1);
    clipName += ".clp";
    clip->SetName(clipName.c_str(), dir);
    const char *path = MakeString("devkit:\\%s", clip->Name());
    MILO_LOG("Starting song recording: %s\n", path);
    clip->StartXboxRecording(path);
}

void MoveDir::FinishGameRecord() {
    MILO_ASSERT(!TheLoadMgr.EditMode(), 0x604);
    if (mRecordClip) {
        MILO_LOG("Finishing song recording: %s\n", mRecordClip->Path());
        mRecordClip->StopRecording();
        RELEASE(mRecordClip);
    }
    if (unk2bc) {
        MILO_LOG("Finishing song recording: %s\n", unk2bc->Path());
        unk2bc->StopRecording();
        RELEASE(unk2bc);
    }
    RELEASE(unk2d0);
}

void MoveDir::SetupSongRecordClip() {
    static Symbol rhythm_battle("rhythm_battle");
    bool b1 = unk308 && unk308->Type() == rhythm_battle;
    bool b7 = false;
    if (unk308) {
        static Message msg("is_game_over");
        b7 = unk308->Handle(msg, true).Int();
    }
    if (!b7) {
        const char *modeStr;
        if (b1) {
            modeStr = "ktb";
        } else if (TheHamDirector->InPracticeMode()) {
            modeStr = "bid";
        } else
            modeStr = "pi";
        if (sGameRecord && !mRecordClip) {
            unsigned int x = sGameRecord2Player;
            if (x) {
                SetupRecordClip(mRecordClip, 0, 0, modeStr, this);
                SetupRecordClip(unk2bc, 1, 1, modeStr, this);
            } else {
                SetupRecordClip(mRecordClip, x, -1, modeStr, this);
            }
        }
        if (b1 && !unk2d0) {
            SetupRecordClip(unk2d0, 2, 0, modeStr, this);
        }
    }
}

void MoveDir::SetDancerSequence(DancerSequence *seq) { mDancerSeq = seq; }

void MoveDir::LoadScoring(const DataArray *cfg) {
    static Symbol min_frame_dist_beats("min_frame_dist_beats");
    cfg->FindData(min_frame_dist_beats, HamMove::sMinFrameDistBeats);
    static Symbol latency_offset("latency_offset");
    cfg->FindData(latency_offset, sLatencySeconds);
    sLatencySeconds /= 1000;
    static Symbol plf_min_time_error("plf_min_time_error");
    sPLFMinTimeError = cfg->FindFloat(plf_min_time_error);
    ScoreUtlInit(cfg);
    DeleteAll(sFilterVersions);
    DataArray *versionsArr = cfg->FindArray("versions");
    for (int i = 1; i < versionsArr->Size(); i++) {
        sFilterVersions.push_back(FilterVersion::Create(versionsArr->Array(i)));
    }
    MILO_ASSERT(!sFilterVersions.empty(), 0x2E2);
}

void MoveDir::ReloadScoring() {
    MILO_ASSERT(TheLoadMgr.EditMode(), 0x1268);
    DataArray *cfg = SystemConfig("scoring");
    DataArray *file = DataReadFile(cfg->Array(1)->File(), true);
    LoadScoring(file);
    ScoreUtlInit(file);
    Enter();
    file->Release();
}

void MoveDir::ResetDetection() {
    if (TheHamDirector) {
        if (SkeletonUpdate::HasInstance()) {
            SkeletonUpdateHandle handle = SkeletonUpdate::InstanceHandle();
            if (!handle.HasCallback(this)) {
                handle.AddCallback(this);
            }
        }
        MILO_ASSERT(TheGameData, 0x642);
        for (int i = 0; i < 2; i++) {
            HamPlayerData *player_data = TheGameData->Player(i);
            MILO_ASSERT(player_data, 0x646);
            if (player_data->IsPlaying()) {
                ResetDetectFrames(i, player_data->GetDifficulty());
            }
        }
        SetupSongRecordClip();
    }
}

void MoveDir::SetSongPlayClip(SkeletonClip *clip) {
    if (!mFiltersEnabled && clip) {
        mFiltersEnabled = true;
        if (TheLoadMgr.EditMode()) {
            MiloInit();
        }
    }
    if (mRecordClip && mRecordClip->IsRecording()) {
        MILO_NOTIFY("Can't set play clip while recording");
    } else {
        mPlayClip = clip;
        SetSkeletonClip(clip);
        ResetDetection();
        TheGameData->UnassignSkeletons();
    }
}

void MoveDir::MiloUpdate() {
    SkeletonDir::MiloUpdate();
    MILO_ASSERT(TheGestureMgr, 0xB14);
    SetCurrentMove(0, mMovePlayerData[0].mCurMove);
    SetMoveOverlay(mShowMoveOverlay);
    SetSongPlayClip(mPlayClip);
}

DataNode MoveDir::OnStreamJump(const DataArray *) {
    if (unkef8) {
        ResetDetection();
        unk310 = -1;
    }
    return 0;
}

// TODO: once this is fully implemented, remove the noinline part
__declspec(noinline) void MoveDir::OnBeat() {
    if (TheMaster) {
        for (int i = 0; i < 2; i++) {
            mCurMoveSmoothers[i].Reset();
        }
    }
}
