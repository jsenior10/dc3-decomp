#include "hamobj/HollaBackMinigame.h"
#include "MoveMgr.h"
#include "flow/Flow.h"
#include "flow/PropertyEventProvider.h"
#include "gesture/BaseSkeleton.h"
#include "hamobj/HamDirector.h"
#include "hamobj/HamGameData.h"
#include "hamobj/HamMaster.h"
#include "hamobj/HamPlayerData.h"
#include "math/Easing.h"
#include "obj/Dir.h"
#include "obj/Msg.h"
#include "obj/Object.h"
#include "obj/Task.h"
#include "rndobj/Poll.h"
#include "rndobj/PropAnim.h"
#include "synth/SynthSample.h"
#include "utl/Loader.h"
#include "utl/TimeConversion.h"

void JumpToMeasure(float beat) {
    float ms = BeatToMs(beat * 4.0f);
    Hmx::Object *game = ObjectDir::Main()->Find<Hmx::Object>("game", true);
    if (game) {
        game->Handle(Message("jump", ms), true);
    }
}

HollaBackMinigame::HollaBackMinigame()
    : unk410(0), unk424(-1), unk448(2), unk450(0), unk474(0), unk478(0) {}

HollaBackMinigame::~HollaBackMinigame() { EndMinigame(true); }

BEGIN_HANDLERS(HollaBackMinigame)
    HANDLE_ACTION(begin_minigame, BeginMinigame(_msg->Array(2)))
    HANDLE_ACTION(end_minigame, EndMinigame(false))
    HANDLE_ACTION(beat, OnBeat())
    HANDLE_EXPR(get_move_state, GetMoveState(_msg->Int(2)))
    HANDLE_ACTION(set_move_state, SetMoveState(_msg->Int(2), _msg->Sym(3)))
    HANDLE_ACTION(set_default_shot, SetDefaultShot())
    HANDLE_EXPR(get_first_move_idx, unk4 + 5)
    HANDLE_SUPERCLASS(RndPollable)
    HANDLE_SUPERCLASS(Hmx::Object)
END_HANDLERS

BEGIN_PROPSYNCS(HollaBackMinigame)
    SYNC_SUPERCLASS(RndPollable)
    SYNC_SUPERCLASS(Hmx::Object)
END_PROPSYNCS

BEGIN_SAVES(HollaBackMinigame)
    SAVE_REVS(5, 0)
    SAVE_SUPERCLASS(RndPollable)
END_SAVES

BEGIN_COPYS(HollaBackMinigame)
    COPY_SUPERCLASS(RndPollable)
    CREATE_COPY(HollaBackMinigame)
END_COPYS

BEGIN_LOADS(HollaBackMinigame)
    LOAD_REVS(bs)
    ASSERT_REVS(5, 0)
    LOAD_SUPERCLASS(RndPollable)
END_LOADS

void HollaBackMinigame::Poll() {
    if (!TheLoadMgr.EditMode() && unk410) {
        RndPropAnim *anim =
            TheHamDirector->GetVenueWorld()->Find<RndPropAnim>("set_bid.anim", true);
        if (anim) {
            anim->Animate(0, false, 0, nullptr, kEaseLinear, 0, false);
        }

        if (unk478 && !unk478->IsPlaying()) {
            EndShoutOut();
        }
        if (unk474) {
            Hmx::Object *game = ObjectDir::Main()->Find<Hmx::Object>("game", true);
            bool b20 = !game ? false : !game->Handle(Message("is_waiting"), true).Int();
            if (b20 && unk474) {
                JumpToMeasure(unk470);
                TheMaster->Audio()->SetPaused(true);
                unk474 = false;
            }
        }
        if (unk414 == 0) {
            static Symbol holla_back_stage("holla_back_stage");
            static Symbol exit_title("exit_title");
            static Symbol start_score_move_index("start_score_move_index");
            static Symbol hide_hud("hide_hud");
            for (int i = 0; i < 2; i++) {
                HamPlayerData *hpd = TheGameData->Player(i);
                if (!hpd->IsPlaying()) {
                    hpd->Provider()->SetProperty(start_score_move_index, 1000);
                    hpd->Provider()->Export(Message(hide_hud, 0), true);
                    unk454
                        ->Find<RndPropAnim>(
                            hpd->Side() == kSkeletonRight ? "player_1_hud.anim"
                                                          : "player_2_hud.anim",
                            true
                        )
                        ->SetFrame(0, 1);
                }
            }
            if (!unk481 && TheMaster->Audio()->IsReady() && unk494-- <= 0) {
                unk481 = true;
                TheHamProvider->SetProperty(holla_back_stage, Symbol("title"));
                Hmx::Object *game = ObjectDir::Main()->Find<Hmx::Object>("game", true);
                if (game) {
                    game->Handle(Message("set_realtime", 1), true);
                }
                unk478 = unk458->Find<Sound>(MakeString("%s.snd", unk428.Str()), false);
                if (unk478) {
                    unk478->Play(0, 0, 0, nullptr, 0);
                }
                TheHamProvider->SetProperty("game_stage", Symbol("title"));
                TheHamDirector->GetVenueWorld()
                    ->Find<Flow>("animate_timeywimey.flow", true)
                    ->Activate();
                SetDefaultShot();
            } else if (unk481 && TheMaster->Audio()->IsReady()) {
                if (unk478) {
                    SynthSample *sample = unk478->Sample();
                    float f23 = unk478->ElapsedTime() - 1.0f;
                    if (f23 >= sample->LengthMs() / 1000.0f)
                        goto next;
                }
                TheMaster->Audio()->SetPaused(false);
                Hmx::Object *game = ObjectDir::Main()->Find<Hmx::Object>("game", true);
                if (game) {
                    game->Handle(Message("set_realtime", 0), true);
                }
                SetNumMoves(unk448);
                TheHamProvider->SetProperty(holla_back_stage, exit_title);
                unk480 = true;
                TheHamProvider->SetProperty("game_stage", Symbol("playing"));
                TheHamProvider->SetProperty("hide_venue", 0);
                SetState((State)1);
                SetDefaultShot();
            }
        }
    next:
        float curBeat = TheTaskMgr.Beat();
        HamMove *move =
            TheMoveMgr->FindHamMoveFromName(TheHamDirector->MoveNameFromBeat(curBeat, 0));
        if (unk414 == 0) {
            unk454->Find<UILabel>("song_name.lbl", true)
                ->SetPrelocalizedString(String("???"));
            unk454->Find<UILabel>("song_artist.lbl", true)
                ->SetPrelocalizedString(String("???"));
        }
    }
}
