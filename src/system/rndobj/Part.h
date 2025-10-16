#pragma once
#include "obj/Object.h"
#include "rndobj/Anim.h"
#include "rndobj/Draw.h"
#include "rndobj/Poll.h"
#include "rndobj/Trans.h"
#include "utl/MemMgr.h"

class RndParticle {};

class RndParticleSys : public RndAnimatable,
                       public RndPollable,
                       public RndTransformable,
                       public RndDrawable {
public:
    enum Type {
        kBasic = 0,
        kFancy = 1
    };
    // Hmx::Object
    virtual ~RndParticleSys();
    virtual bool Replace(ObjRef *, Hmx::Object *);
    OBJ_CLASSNAME(ParticleSys);
    OBJ_SET_TYPE(ParticleSys);
    virtual DataNode Handle(DataArray *, bool);
    virtual bool SyncProperty(DataNode &, DataArray *, int, PropOp);
    virtual void Save(BinStream &);
    virtual void Copy(const Hmx::Object *, Hmx::Object::CopyType);
    virtual void Load(BinStream &);
    // RndAnimatable
    virtual void SetFrame(float, float);
    virtual float EndFrame();
    // RndPollable
    virtual void Poll();
    virtual void Enter();
    // RndDrawable
    virtual void UpdateSphere();
    virtual bool MakeWorldSphere(Sphere &, bool);
    virtual void Mats(std::list<class RndMat *> &, bool);
    virtual void DrawShowing();
    // RndHighlightable
    virtual void Highlight() { RndDrawable::Highlight(); }

    virtual void SetPreserveParticles(bool b);
    virtual void SetPool(int, Type);
    virtual void SetPersistentPool(int, Type);

    OBJ_MEM_OVERLOAD(0x90)
    NEW_OBJ(RndParticleSys)
    static void Init() { REGISTER_OBJ_FACTORY(RndParticleSys) }

protected:
    RndParticleSys();
};

void InitParticleSystem();
