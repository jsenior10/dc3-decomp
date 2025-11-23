#pragma once
#include "hamobj/HamMove.h"
#include "obj/Data.h"
#include "obj/Dir.h"
#include "obj/Object.h"
#include "rndobj/Poll.h"
#include "synth/Sound.h"
#include "utl/MemMgr.h"

class HollaBackMinigame : public RndPollable {
public:
    enum State {
    };
    // Hmx::Object
    virtual ~HollaBackMinigame();
    OBJ_CLASSNAME(HollaBackMinigame);
    OBJ_SET_TYPE(HollaBackMinigame);
    virtual DataNode Handle(DataArray *, bool);
    virtual bool SyncProperty(DataNode &, DataArray *, int, PropOp);
    virtual void Save(BinStream &);
    virtual void Copy(const Hmx::Object *, Hmx::Object::CopyType);
    virtual void Load(BinStream &);
    // RndPollable
    virtual void Poll();
    virtual void Enter() { RndPollable::Enter(); }
    virtual void Exit() { RndPollable::Exit(); }

    void BeginMinigame(DataArray *);
    void EndMinigame(bool);
    void OnBeat();
    Symbol GetMoveState(int) const;
    void SetMoveState(int, Symbol);
    void SetDefaultShot();
    void EndShoutOut();
    void SetNumMoves(int);
    void SetState(State);

    OBJ_MEM_OVERLOAD(0x21)
    NEW_OBJ(HollaBackMinigame)

protected:
    HollaBackMinigame();

    int unk4;
    int unk8;
    Symbol unkc[0x100]; // 0xc
    bool unk410; // 0x410
    int unk414;
    int unk418;
    int unk41c;
    int unk420;
    int unk424;
    Symbol unk428;
    std::vector<Symbol> unk42c;
    std::vector<Symbol> unk438;
    bool unk444;
    int unk448;
    int unk44c;
    float unk450;
    ObjectDir *unk454;
    ObjectDir *unk458;
    int unk45c;
    int unk460;
    int unk464;
    int unk468;
    bool unk46c;
    float unk470;
    bool unk474;
    Sound *unk478;
    int unk47c;
    bool unk480;
    bool unk481;
    int unk484;
    std::vector<HamMove *> unk488;
    int unk494;
};
