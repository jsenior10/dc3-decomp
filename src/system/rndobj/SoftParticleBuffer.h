#pragma once
#include "obj/Object.h"
#include "rndobj/Draw.h"
#include "rndobj/PostProc.h"

class RndSoftParticleBuffer : public Hmx::Object, public PostProcessor {
public:
    RndSoftParticleBuffer();
    virtual ~RndSoftParticleBuffer() { FreeData(); }
    // Hmx::Object
    OBJ_CLASSNAME(SoftParticleBuffer);
    OBJ_SET_TYPE(SoftParticleBuffer);
    // PostProcessor
    virtual void DoPost();
    virtual const char *GetProcType() { return "SoftParticleBuffer"; }

    NEW_OBJ(RndSoftParticleBuffer)

private:
    void FreeData();

    int unk30; // 0x30
    int unk34; // 0x34
    int unk38; // 0x38
    ObjPtrList<RndDrawable> unk3c; // 0x3c
};
