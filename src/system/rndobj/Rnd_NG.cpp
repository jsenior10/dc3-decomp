#include "rndobj/Rnd_NG.h"
#include "math/Color.h"
#include "math/Vec.h"
#include "obj/Object.h"
#include "os/Debug.h"
#include "rndobj/Cam.h"
#include "rndobj/Flare.h"
#include "rndobj/Fur_NG.h"
#include "rndobj/Lit_NG.h"
#include "rndobj/Mat_NG.h"
#include "rndobj/ShaderMgr.h"
#include "rndobj/ShadowMap.h"
#include "rndobj/SoftParticleBuffer.h"
#include "rndobj/Stats_NG.h"
#include "rndobj/Tex.h"

NgStats gNgStats[3];
NgStats *TheNgStats = &gNgStats[0];

NgRnd::NgRnd()
    : unk1e0(), unk1f8(0), unk1fc(0), unk200(0), unk204(0), unk208(0), unk218(0) {}

NgRnd::~NgRnd() {}

void NgRnd::PreInit() {
    if (!unk218) {
        unk218 = true;
        Rnd::PreInit();
        // NgEnviron
        REGISTER_OBJ_FACTORY(NgMat)
        NgLight::Init();
        REGISTER_OBJ_FACTORY(NgFur)
        RndShadowMap::Init();
        REGISTER_OBJ_FACTORY(RndSoftParticleBuffer)
        CreateDefaults();
    }
}

void NgRnd::Init() {
    PreInit();
    TheShaderMgr.Init();
    unk20c.reserve(0x200);
    Rnd::Init();
    unk208 = Hmx::Object::New<RndSoftParticleBuffer>();
}

void NgRnd::ReInit() { TheShaderMgr.InitShaders(); }

void NgRnd::Terminate() {
    RELEASE(unk204);
    RELEASE(unk208);
    TheShaderMgr.Terminate();
    RndShadowMap::Terminate();
    Rnd::Terminate();
}

void NgRnd::SetShadowMap(RndTex *tex, RndCam *cam, const Hmx::Color *color) {
    unk1fc = tex;
    unk200 = cam;
    if (cam) {
        const Vector3 &v3 = cam->WorldXfm().m.y;
        Vector4 v4(v3.x, v3.y, v3.z, 1);
        TheShaderMgr.SetPConstant((PShaderConstant)0x6c, v4);
    }
    if (color) {
        Vector4 v4 = Vector4(color->red, color->green, color->blue, color->alpha);
        TheShaderMgr.SetPConstant((PShaderConstant)0x6B, v4);
    }
    if (tex) {
        TheShaderMgr.SetPConstant((PShaderConstant)5, tex);
    }
}

void NgRnd::RemovePointTest(RndFlare *flare) {
    Rnd::RemovePointTest(flare);
    FOREACH (it, unk20c) {
        if (it->unk0 == flare) {
            unk204->ReleaseQuery(it->unk8);
            unk204->ReleaseQuery(it->unk4);
            unk20c.erase(it);
            return;
        }
    }
}

void NgRnd::CreateLargeQuad(int, int, LargeQuadRenderData &) {
    MILO_FAIL("NgRnd::CreateLargeQuad not implemented!");
}

void NgRnd::DrawLargeQuad(
    const LargeQuadRenderData &, const Transform &, RndMat *, ShaderType
) {
    MILO_FAIL("NgRnd::DrawLargeQuad not implemented!");
}

void NgRnd::SetVertShaderTex(RndTex *, int) {
    MILO_FAIL("NgRnd::SetVertShaderTex not implemented!");
}

void NgRnd::ResetStats() {
    if (mProcCmds == kProcessWorld || mProcCmds == kProcessAll) {
        TheNgStats = &gNgStats[0];
    } else if (mProcCmds == kProcessPost) {
        TheNgStats = &gNgStats[1];
    } else {
        TheNgStats = &gNgStats[2];
    }
    memset(TheNgStats, 0, sizeof(NgStats));
    TheNgStats->mCams++;
}
