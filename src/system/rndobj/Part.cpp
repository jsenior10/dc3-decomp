#include "rndobj/Part.h"
#include "math/Geo.h"
#include "obj/Data.h"
#include "obj/DataFunc.h"
#include "obj/Object.h"
#include "os/System.h"
#include "rndobj/Anim.h"
#include "rndobj/Draw.h"
#include "rndobj/Mesh.h"
#include "rndobj/Poll.h"
#include "rndobj/Trans.h"
#include "utl/BinStream.h"

PartOverride gNoPartOverride;
ParticleCommonPool *gParticlePool;

namespace {
    int ParticlePoolSize() {
        return SystemConfig("rnd", "particlesys", "global_limit")->Int(1);
    }

    DataNode PrintParticlePoolSize(DataArray *) {
        MILO_LOG("Particle Pool Size:\n");
        if (gParticlePool) {
            int size = ParticlePoolSize();
            MILO_LOG(
                "   %d particles can be allocated, %.1f KB.\n",
                size,
                (float)(size * 200 * 0.0009765625f)
            );
            MILO_LOG(
                "   %d particles active, %d is the high water mark.\n",
                gParticlePool->NumActiveParticles(),
                gParticlePool->HighWaterMark()
            );
            MILO_LOG(
                "   Adding 30%%, suggesting a particle global limit of %d (set in default.dta).\n",
                (int)(gParticlePool->HighWaterMark() * 1.3f)
            );
        }
        return 0;
    }
}

BinStream &operator<<(BinStream &bs, const RndParticle &p) {
    bs << p.pos << p.col << p.size;
    return bs;
}

BinStream &operator>>(BinStream &bs, RndParticle &p) {
    bs >> p.pos >> p.col >> p.size;
    return bs;
}

PartOverride::PartOverride()
    : mask(0), life(0), speed(0), deltaSize(0), startColor(0), midColor(0), endColor(0),
      pitch(0, 0), yaw(0, 0), mesh(0), box(Vector3(0, 0, 0), Vector3(0, 0, 0)) {}

void InitParticleSystem() {
    if (!gParticlePool) {
        gParticlePool = new ParticleCommonPool();
    }
    if (gParticlePool) {
        gParticlePool->InitPool();
    }
    DataRegisterFunc("print_particle_pool_size", PrintParticlePoolSize);
}

void ParticleCommonPool::InitPool() {
    int size = ParticlePoolSize();
    mPoolParticles = new RndFancyParticle[size];
    for (int i = 0; i < size - 1; i++) {
        mPoolParticles[i].prev = nullptr;
        mPoolParticles[i].next = &mPoolParticles[i + 1];
    }
    mPoolParticles[size - 1].prev = nullptr;
    mPoolParticles[size - 1].next = nullptr;
    mPoolFreeParticles = mPoolParticles;
}

RndParticle *ParticleCommonPool::AllocateParticle() {
    RndParticle *cur = mPoolFreeParticles;
    RndParticle *ret = nullptr;
    if (cur) {
        mPoolFreeParticles = mPoolFreeParticles->next;
        cur->prev = cur;
        mNumActiveParticles++;
        ret = cur;
        if (mNumActiveParticles > mHighWaterMark) {
            mHighWaterMark = mNumActiveParticles;
        }
    }
    return ret;
}

BEGIN_CUSTOM_PROPSYNC(Attractor)
    SYNC_PROP(attractor, o.mAttractor)
    SYNC_PROP(strength, o.mStrength)
END_CUSTOM_PROPSYNC

BinStream &operator<<(BinStream &bs, const Attractor &a) {
    a.Save(bs);
    return bs;
}

void Attractor::Save(BinStream &bs) const {
    bs << mAttractor;
    bs << mStrength;
}

void Attractor::Load(BinStreamRev &d) {
    d >> mAttractor;
    d >> mStrength;
}

RndParticleSys::RndParticleSys()
    : mType(kBasic), mMaxParticles(0), mPersistentParticles(nullptr),
      mFreeParticles(nullptr), mActiveParticles(nullptr), mNumActive(0), mEmitCount(0),
      mFrameDrive(0), unk138(0), unk13c(0), mPauseOffscreen(0), unk144(0),
      mBubblePeriod(10, 10), mBubbleSize(1, 1), mLife(100, 100), mBoxExtent1(0, 0, 0),
      mBoxExtent2(0, 0, 0), mSpeed(1, 1), mPitch(0, 0), mYaw(0, 0), mEmitRate(1, 1),
      mStartSize(gUnitsPerMeter / 4, gUnitsPerMeter / 4), mDeltaSize(0, 0),
      mStartColorLow(1, 1, 1), mStartColorHigh(1, 1, 1), mEndColorLow(1, 1, 1),
      mEndColorHigh(1, 1, 1), mMeshEmitter(this), mMat(this), mPreserve(0),
      mMotionParent(this), mBounce(this), mForceDir(1, 1, 1), mDrag(1), mBubble(0),
      mPreSpawn(0), unk2ee(0), mSpin(0), mRPM(1, 1), mRPMDrag(1), mRandomDirection(1),
      mStartOffset(1, 1), mEndOffset(1, 1), mVelocityAlign(0), mStretchWithVelocity(0),
      mConstantArea(0), mPerspective(0), mStretchScale(1), mScreenAspect(1),
      mSubSamples(0), mGrowRatio(0), mShrinkRatio(1), mMidColorRatio(0.5),
      mMidColorLow(1, 1, 1), mMidColorHigh(1, 1, 1), mBirthMomentum(0),
      mBirthMomentumAmount(1), unk394(0), unk398(0), unk39c(0), mMaxBurst(0), unk3a4(1),
      mTimeBetween(15, 35), mPeakRate(4, 8), mDuration(20, 30), unk3c0(0), unk3c4(0),
      mAnimateUVs(0), mLoopUVAnim(1), mRandomAnimStart(0), mTileHoldTime(0),
      mNumTilesAcross(1), mNumTilesDown(1), mNumTilesTotal(1), mStartingTile(0),
      unk3e0(1), unk3e4(1), mAttractors(this) {
    SetRelativeMotion(0, this);
    SetSubSamples(0);
}

BEGIN_HANDLERS(RndParticleSys)
    HANDLE_EXPR(hi_emit_rate, Max(mEmitRate.x, mEmitRate.y))
    HANDLE(set_start_color, OnSetStartColor)
    HANDLE(set_end_color, OnSetEndColor)
    HANDLE(set_start_color_int, OnSetStartColorInt)
    HANDLE(set_end_color_int, OnSetEndColorInt)
    HANDLE(set_emit_rate, OnSetEmitRate)
    HANDLE(set_burst_interval, OnSetBurstInterval)
    HANDLE(set_burst_peak, OnSetBurstPeak)
    HANDLE(set_burst_length, OnSetBurstLength)
    HANDLE(add_emit_rate, OnAddEmitRate)
    HANDLE(launch_part, OnExplicitPart)
    HANDLE(launch_parts, OnExplicitParts)
    HANDLE(set_life, OnSetLife)
    HANDLE(set_speed, OnSetSpeed)
    HANDLE(set_rotate, OnSetRotate)
    HANDLE(set_swing_arm, OnSetSwingArm)
    HANDLE(set_drag, OnSetDrag)
    HANDLE(set_alignment, OnSetAlignment)
    HANDLE(set_start_size, OnSetStartSize)
    HANDLE(set_mat, OnSetMat)
    HANDLE(set_pos, OnSetPos)
    HANDLE_ACTION(set_mesh, SetMesh(_msg->Obj<RndMesh>(2)))
    HANDLE(active_particles, OnActiveParticles)
    HANDLE_EXPR(max_particles, mMaxParticles)
    HANDLE_ACTION(
        set_relative_parent,
        SetRelativeMotion(mRelativeMotion, _msg->Obj<RndTransformable>(2))
    )
    HANDLE_ACTION(clear_all_particles, FreeAllParticles())
    HANDLE_SUPERCLASS(RndDrawable)
    HANDLE_SUPERCLASS(RndAnimatable)
    HANDLE_SUPERCLASS(RndTransformable)
    HANDLE_SUPERCLASS(RndPollable)
    HANDLE_SUPERCLASS(Hmx::Object)
END_HANDLERS

bool AngleVectorSync(Vector2 &vec, DataNode &_val, DataArray *_prop, int _i, PropOp _op) {
    if (_i == _prop->Size())
        return true;
    else {
        Symbol sym = _prop->Sym(_i);
        static Symbol x("x");
        static Symbol y("y");
        float *coord = nullptr;
        if (sym == x) {
            coord = &vec.x;
            goto sync;
        } else if (sym == y) {
            coord = &vec.y;
            goto sync;
        } else
            return false;
    sync:
        if (_op == kPropSet)
            *coord = DegreesToRadians(_val.Float());
        else if (_op == kPropGet)
            _val = RadiansToDegrees(*coord);
        else
            return false;
    }
    return true;
}

BEGIN_PROPSYNCS(RndParticleSys)
    SYNC_PROP(mat, mMat)
    SYNC_PROP_SET(animate_uvs, mAnimateUVs, SetAnimatedUV(_val.Int()))
    SYNC_PROP(loop_uv_anim, mLoopUVAnim)
    SYNC_PROP(random_anim_start, mRandomAnimStart)
    SYNC_PROP_SET(tile_hold_time, mTileHoldTime, SetTileHoldTime(_val.Float()))
    SYNC_PROP_SET(num_tiles_across, mNumTilesAcross, mNumTilesAcross = Max(_val.Int(), 1))
    SYNC_PROP_SET(num_tiles_down, mNumTilesDown, mNumTilesDown = Max(_val.Int(), 1))
    SYNC_PROP_SET(num_tiles_total, mNumTilesTotal, SetNumTiles(_val.Int()))
    SYNC_PROP(starting_tile, mStartingTile)
    SYNC_PROP_SET(max_parts, mMaxParticles, SetPool(_val.Int(), mType))
    SYNC_PROP(emit_rate, mEmitRate)
    SYNC_PROP(screen_aspect, mScreenAspect)
    SYNC_PROP(life, mLife)
    SYNC_PROP(speed, mSpeed)
    SYNC_PROP(start_size, mStartSize)
    SYNC_PROP(delta_size, mDeltaSize)
    SYNC_PROP(force_dir, mForceDir)
    SYNC_PROP(bounce, mBounce)
    SYNC_PROP(start_color_low, mStartColorLow)
    SYNC_PROP(start_color_high, mStartColorHigh)
    SYNC_PROP(start_alpha_low, mStartColorLow.alpha)
    SYNC_PROP(start_alpha_high, mStartColorHigh.alpha)
    SYNC_PROP(end_color_low, mEndColorLow)
    SYNC_PROP(end_color_high, mEndColorHigh)
    SYNC_PROP(end_alpha_low, mEndColorLow.alpha)
    SYNC_PROP(end_alpha_high, mEndColorHigh.alpha)
    SYNC_PROP(preserve, mPreserve)
    SYNC_PROP_SET(fancy, mType, SetPool(mMaxParticles, (Type)_val.Int()))
    SYNC_PROP_SET(grow_ratio, mGrowRatio, SetGrowRatio(_val.Float()))
    SYNC_PROP_SET(shrink_ratio, mShrinkRatio, SetShrinkRatio(_val.Float()))
    SYNC_PROP(drag, mDrag)
    SYNC_PROP(mid_color_ratio, mMidColorRatio)
    SYNC_PROP(mid_color_low, mMidColorLow)
    SYNC_PROP(mid_color_high, mMidColorHigh)
    SYNC_PROP(mid_alpha_low, mMidColorLow.alpha)
    SYNC_PROP(mid_alpha_high, mMidColorHigh.alpha)
    SYNC_PROP(bubble, mBubble)
    SYNC_PROP(bubble_period, mBubblePeriod)
    SYNC_PROP(bubble_size, mBubbleSize)
    SYNC_PROP(max_burst, mMaxBurst)
    SYNC_PROP(time_between, mTimeBetween)
    SYNC_PROP(peak_rate, mPeakRate)
    SYNC_PROP(duration, mDuration)
    SYNC_PROP(spin, mSpin)
    SYNC_PROP(rpm, mRPM)
    SYNC_PROP(rpm_drag, mRPMDrag)
    SYNC_PROP(start_offset, mStartOffset)
    SYNC_PROP(end_offset, mEndOffset)
    SYNC_PROP(random_direction, mRandomDirection)
    SYNC_PROP(velocity_align, mVelocityAlign)
    SYNC_PROP(stretch_with_velocity, mStretchWithVelocity)
    SYNC_PROP(stretch_scale, mStretchScale)
    SYNC_PROP(constant_area, mConstantArea)
    SYNC_PROP(perspective, mPerspective)
    SYNC_PROP_SET(mesh_emitter, mMeshEmitter.Ptr(), SetMesh(_val.Obj<RndMesh>()))
    SYNC_PROP(box_extent_1, mBoxExtent1)
    SYNC_PROP(box_extent_2, mBoxExtent2) {
        static Symbol _s("pitch");
        if (sym == _s) {
            AngleVectorSync(mPitch, _val, _prop, _i + 1, _op);
            return true;
        }
    }
    {
        static Symbol _s("yaw");
        if (sym == _s) {
            AngleVectorSync(mYaw, _val, _prop, _i + 1, _op);
            return true;
        }
    }
    SYNC_PROP_SET(
        motion_parent,
        mMotionParent.Ptr(),
        SetRelativeMotion(mRelativeMotion, _val.Obj<RndTransformable>())
    )
    SYNC_PROP_SET(
        relative_motion, mRelativeMotion, SetRelativeMotion(_val.Float(), mMotionParent)
    )
    SYNC_PROP_SET(subsamples, mSubSamples, SetSubSamples(_val.Int()))
    SYNC_PROP_SET(frame_drive, mFrameDrive, SetFrameDrive(_val.Int()))
    SYNC_PROP(pre_spawn, mPreSpawn)
    SYNC_PROP_SET(pause_offscreen, mPauseOffscreen, SetPauseOffscreen(_val.Int()))
    SYNC_PROP(attractors, mAttractors)
    SYNC_PROP(birth_momentum, mBirthMomentum)
    SYNC_PROP(birth_momentum_amount, mBirthMomentumAmount)
    SYNC_SUPERCLASS(RndAnimatable)
    SYNC_SUPERCLASS(RndTransformable)
    SYNC_SUPERCLASS(RndDrawable)
    SYNC_SUPERCLASS(Hmx::Object)
END_PROPSYNCS

BEGIN_SAVES(RndParticleSys)
    SAVE_REVS(0x29, 0)
    SAVE_SUPERCLASS(Hmx::Object)
    SAVE_SUPERCLASS(RndPollable)
    SAVE_SUPERCLASS(RndAnimatable)
    SAVE_SUPERCLASS(RndTransformable)
    SAVE_SUPERCLASS(RndDrawable)
    bs << mLife;
    bs << mScreenAspect;
    bs << mBoxExtent1;
    bs << mBoxExtent2;
    bs << mSpeed;
    bs << mPitch;
    bs << mYaw;
    bs << mEmitRate;
    bs << mMaxBurst;
    bs << mTimeBetween;
    bs << mPeakRate;
    bs << mDuration;
    bs << mStartSize;
    bs << mDeltaSize;
    bs << mStartColorLow;
    bs << mStartColorHigh;
    bs << mEndColorLow;
    bs << mEndColorHigh;
    bs << mBounce;
    bs << mForceDir;
    bs << mMat;
    bs << mType;
    bs << mGrowRatio;
    bs << mShrinkRatio;
    bs << mMidColorRatio;
    bs << mMidColorLow;
    bs << mMidColorHigh;
    bs << mMaxParticles;
    bs << mBubblePeriod;
    bs << mBubbleSize;
    bs << mBubble;
    bs << mSpin;
    bs << mRPM;
    bs << mRPMDrag;
    bs << mRandomDirection;
    bs << mDrag;
    bs << mStartOffset;
    bs << mEndOffset;
    bs << mVelocityAlign;
    bs << mStretchWithVelocity;
    bs << mConstantArea;
    bs << mStretchScale;
    bs << mPerspective;
    bs << mRelativeMotion;
    bs << mMotionParent;
    bs << mMeshEmitter;
    bs << mSubSamples;
    bs << mFrameDrive;
    bs << mPauseOffscreen;
    bs << mPreSpawn;
    bs << mAnimateUVs;
    bs << mTileHoldTime;
    bs << mNumTilesAcross;
    bs << mNumTilesDown;
    bs << mNumTilesTotal;
    bs << mStartingTile;
    bs << mLoopUVAnim;
    bs << mRandomAnimStart;
    bs << mAttractors;
    bs << mBirthMomentum;
    bs << mBirthMomentumAmount;
    bs << mPreserve;
    unk2ee = mPreSpawn;
    if (mPreserve) {
        bs << mNumActive;
        for (RndParticle *p = mActiveParticles; p != nullptr; p = p->next) {
            bs << *p;
        }
    }
END_SAVES

void RndParticleSys::SetTileHoldTime(float f1) {
    mTileHoldTime = f1;
    unk3e0 = mNumTilesTotal * mTileHoldTime;
    unk3e0 = Max(unk3e0, 0.0001f);
    unk3e4 = 1.0f / unk3e0;
}

void RndParticleSys::SetGrowRatio(float f) {
    if (f >= 0 && f <= mGrowRatio)
        mGrowRatio = f;
}

void RndParticleSys::SetShrinkRatio(float f) {
    if (f >= mGrowRatio && f <= 1.0f)
        mShrinkRatio = f;
}

void RndParticleSys::SetFrameDrive(bool b) {
    mFrameDrive = b;
    if (mFrameDrive) {
        unk138 = GetFrame();
    } else
        unk13c = 0;
    unk144 = 0;
}

void RndParticleSys::SetPauseOffscreen(bool b) {
    mPauseOffscreen = b;
    unk144 = 0;
}
