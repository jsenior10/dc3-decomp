#pragma once
#include "SongCollision.h"
#include "char/Character.h"
#include "char/FileMerger.h"
#include "hamobj/Difficulty.h"
#include "hamobj/HamCamShot.h"
#include "hamobj/HamCharacter.h"
#include "hamobj/HamVisDir.h"
#include "obj/Dir.h"
#include "obj/Object.h"
#include "rndobj/Dir.h"
#include "rndobj/Draw.h"
#include "rndobj/Poll.h"
#include "rndobj/PostProc.h"
#include "rndobj/PropAnim.h"
#include "rndobj/TexRenderer.h"
#include "utl/MemMgr.h"
#include "world/Dir.h"

class AnimPtr;

class HamDirector : public RndPollable, public RndDrawable {
public:
    struct DircutEntry {};
    // Hmx::Object
    virtual ~HamDirector();
    OBJ_CLASSNAME(HamDirector);
    OBJ_SET_TYPE(HamDirector);
    virtual DataNode Handle(DataArray *, bool);
    virtual bool SyncProperty(DataNode &, DataArray *, int, PropOp);
    virtual void Save(BinStream &);
    virtual void Copy(const Hmx::Object *, Hmx::Object::CopyType);
    virtual void Load(BinStream &);
    // RndPollable
    virtual void Poll();
    virtual void Enter();
    virtual void Exit();
    virtual void ListPollChildren(std::list<RndPollable *> &) const;
    // RndDrawable
    virtual void DrawShowing();
    virtual void ListDrawChildren(std::list<RndDrawable *> &);
    virtual void CollideList(const Segment &, std::list<Collision> &);

    OBJ_MEM_OVERLOAD(0x6D)
    NEW_OBJ(HamDirector)

protected:
    HamDirector();

    ObjDirPtr<RndDir> unk48; // 0x48
    std::map<Difficulty, AnimPtr> unk5c; // 0x5c
    std::map<Difficulty, AnimPtr> unk74; // 0x74
    ObjPtr<RndPropAnim> unk8c; // 0x8c
    ObjPtr<RndPropAnim> unka0; // 0xa0
    ObjPtr<RndPropAnim> unkb4; // 0xb4
    float unkc8; // 0xc8
    Symbol unkcc; // 0xcc
    float unkd0; // 0xd0
    ObjPtr<FileMerger> unkd4; // 0xd4
    ObjPtr<FileMerger> unke8; // 0xe8
    ObjPtr<FileMerger> unkfc; // 0xfc
    ObjPtr<WorldDir> unk110; // 0x110
    ObjPtr<SongCollision> unk124; // 0x124
    Symbol unk138; // 0x138
    Symbol unk13c; // 0x13c
    bool unk140; // 0x140
    int unk144; // 0x144
    int unk148; // 0x148
    bool unk14c; // 0x14c
    ObjPtr<RndPostProc> unk150; // 0x150
    ObjPtr<RndPostProc> unk164; // 0x164
    ObjPtr<RndPostProc> unk178; // 0x178
    ObjPtr<RndPostProc> unk18c; // 0x18c
    float unk1a0; // 0x1a0
    float unk1a4; // 0x1a4
    ObjPtr<RndPostProc> unk1a8; // 0x1a8
    ObjPtr<RndPostProc> unk1bc; // 0x1bc
    float unk1d0; // 0x1d0
    float unk1d4; // 0x1d4
    ObjPtr<RndPostProc> unk1d8; // 0x1d8
    ObjPtr<RndPostProc> unk1ec; // 0x1ec
    bool unk200; // 0x200
    ObjPtr<HamCharacter> unk204; // 0x204
    ObjPtr<HamCharacter> unk218; // 0x218
    ObjPtr<HamCharacter> unk22c; // 0x22c
    ObjPtr<HamCharacter> unk240; // 0x240
    bool unk254; // 0x254
    bool unk255;
    bool unk256;
    bool unk257;
    bool unk258;
    bool unk259;
    bool unk25a;
    ObjPtr<HamCamShot> unk25c; // 0x25c
    ObjPtr<HamCamShot> unk270; // 0x270
    ObjPtr<HamCamShot> unk284; // 0x284
    Symbol unk298; // 0x298
    float unk29c; // 0x29c
    bool unk2a0; // 0x2a0
    bool unk2a1; // 0x2a1
    int unk2a4; // 0x2a4
    float unk2a8; // 0x2a8
    bool unk2ac; // 0x2ac
    Keys<DircutEntry, DircutEntry> unk2b0; // 0x2b0
    bool unk2bc; // 0x2bc
    bool unk2bd; // 0x2bd
    ObjPtr<HamVisDir> unk2c0; // 0x2c0
    Symbol unk2d4; // 0x2d4
    Symbol unk2d8; // 0x2d8
    int unk2dc; // 0x2dc
    int unk2e0; // 0x2e0
    int unk2e4; // 0x2e4
    int unk2e8; // 0x2e8
    int unk2ec; // 0x2ec
    Symbol unk2f0; // 0x2f0
    Symbol unk2f4[2]; // 0x2f4
    Symbol unk2fc[2]; // 0x2fc
    int unk304; // 0x304
    ObjPtr<ObjectDir> unk308; // 0x308
    ObjPtr<ObjectDir> unk31c; // 0x31c
    Symbol unk330; // 0x330
    bool unk334; // 0x334
    bool unk335; // 0x335
    bool unk336; // 0x336
    int unk338; // 0x338
    bool unk33c; // 0x33c
    bool unk33d; // 0x33d
    ObjPtr<Character> unk340; // 0x340
    ObjPtr<RndTexRenderer> unk354; // 0x354
    bool unk368; // 0x368
    bool unk369; // 0x369
    int unk36c; // 0x36c
    std::set<Hmx::Object *> unk370; // 0x370
};

extern HamDirector *TheHamDirector;

class AnimPtr : public ObjPtr<RndPropAnim> {
public:
    AnimPtr(RndPropAnim *anim) : ObjPtr<RndPropAnim>(TheHamDirector, anim) {}
};
