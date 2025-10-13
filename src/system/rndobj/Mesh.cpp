#include "rndobj/Mesh.h"
#include "Utl.h"
#include "math/Vec.h"
#include "obj/Data.h"
#include "obj/Object.h"
#include "obj/PropSync.h"
#include "os/Debug.h"
#include "os/System.h"
#include "rndobj/BaseMaterial.h"
#include "rndobj/Draw.h"
#include "rndobj/Mat.h"
#include "rndobj/MultiMesh.h"
#include "rndobj/Trans.h"
#include "utl/BinStream.h"
#include "utl/Std.h"

int MESH_REV_SEP_COLOR = 0x25;

RndMesh::RndMesh()
    : mMat(this), mGeomOwner(this, this), mBones(this), mMutable(0),
      mVolume(kVolumeTriangles), mBSPTree(nullptr), mMultiMesh(nullptr), mHasAOCalc(0),
      mKeepMeshData(0), mCompressedVerts(nullptr), mNumCompressedVerts(0) {
    unk180 = 0x26;
}

RndMesh::~RndMesh() {
    RELEASE(mBSPTree);
    RELEASE(mMultiMesh);
    ClearCompressedVerts();
}

BEGIN_HANDLERS(RndMesh)
    HANDLE(compare_edge_verts, OnCompareEdgeVerts)
    HANDLE(attach_mesh, OnAttachMesh)
    HANDLE(get_face, OnGetFace)
    HANDLE(set_face, OnSetFace)
    HANDLE(get_vert_pos, OnGetVertXYZ)
    HANDLE(set_vert_pos, OnSetVertXYZ)
    HANDLE(get_vert_norm, OnGetVertNorm)
    HANDLE(set_vert_norm, OnSetVertNorm)
    HANDLE(get_vert_uv, OnGetVertUV)
    HANDLE(set_vert_uv, OnSetVertUV)
    HANDLE(unitize_normals, OnUnitizeNormals)
    HANDLE(build_from_bsp, OnBuildFromBSP)
    HANDLE(point_collide, OnPointCollide)
    HANDLE(configure_mesh, OnConfigureMesh)
    HANDLE_EXPR(estimated_size_kb, EstimatedSizeKb())
    HANDLE_ACTION(instance_bones, InstanceGeomOwnerBones())
    HANDLE_EXPR(has_instanced_bones, HasInstancedBones())
    HANDLE_EXPR(has_bones, !mBones.empty())
    HANDLE_ACTION(delete_bones, DeleteBones(_msg->Int(2)))
    HANDLE_ACTION(burn_xfm, BurnXfm())
    HANDLE_ACTION(reset_normals, ResetNormals())
    HANDLE_ACTION(tessellate, Tessellate())
    HANDLE_ACTION(clear_ao, ClearAO())
    HANDLE_ACTION(clear_bones, mBones.clear())
    HANDLE_ACTION(copy_geom_from_owner, CopyGeometryFromOwner())
    HANDLE_SUPERCLASS(RndDrawable)
    HANDLE_SUPERCLASS(RndTransformable)
    HANDLE_SUPERCLASS(Hmx::Object)
END_HANDLERS

bool RndMesh::HasInstancedBones() {
    return mGeomOwner && !mBones.empty() && mGeomOwner->mBones.Owner() == mBones.Owner();
}

BEGIN_CUSTOM_PROPSYNC(RndMesh::Vert)
    SYNC_PROP(pos, o.pos)
    SYNC_PROP(norm, o.norm)
    SYNC_PROP(color, o.color)
    SYNC_PROP(alpha, o.color.alpha)
    SYNC_PROP(tex, o.tex)
END_CUSTOM_PROPSYNC

BEGIN_CUSTOM_PROPSYNC(RndBone)
    SYNC_PROP(bone, o.mBone)
    SYNC_PROP(offset, o.mOffset)
END_CUSTOM_PROPSYNC

bool PropSync(
    RndMesh ::VertVector &vec, DataNode &node, DataArray *prop, int i, PropOp op
) {
    if (op == kPropUnknown0x40)
        return false;
    else if (i == prop->Size()) {
        MILO_ASSERT(op == kPropSize, 0xA7D);
        node = (int)vec.size();
        return true;
    } else {
        RndMesh::Vert &vert = vec[prop->Int(i++)];
        if (i < prop->Size() || op & (kPropGet | kPropSet | kPropSize)) {
            if (vec.size() > 0) {
                if (op & kPropSet) {
                    vec.unkc = true;
                }
                return PropSync(vert, node, prop, i, op);
            } else {
                MILO_NOTIFY_ONCE(
                    "Cannot modify verts (check if keep_mesh_data is set on the mesh)"
                );
            }
            return true;
        } else {
            MILO_NOTIFY("Cannot add or remove verts of a mesh via property system");
        }
        return true;
    }
}

BEGIN_PROPSYNCS(RndMesh)
    SYNC_PROP(mat, mMat)
    SYNC_PROP_MODIFY(geom_owner, mGeomOwner, if (!mGeomOwner) mGeomOwner = this)
    SYNC_PROP_BITFIELD(mutable, mGeomOwner->mMutable, 0xAA1)
    SYNC_PROP_SET(num_verts, Verts().size(), SetNumVerts(_val.Int()))
    SYNC_PROP_SET(num_faces, (int)Faces().size(), SetNumFaces(_val.Int()))
    SYNC_PROP_SET(volume, GetVolume(), SetVolume((Volume)_val.Int()))
    SYNC_PROP_SET(has_valid_bones, HasValidBones(nullptr), _val.Int())
    SYNC_PROP(bones, mBones)
    SYNC_PROP(has_ao_calculation, mHasAOCalc)
    SYNC_PROP_SET(keep_mesh_data, mKeepMeshData, SetKeepMeshData(_val.Int() > 0))
    SYNC_PROP(verts, Verts())
    SYNC_SUPERCLASS(RndTransformable)
    SYNC_SUPERCLASS(RndDrawable)
    SYNC_SUPERCLASS(Hmx::Object)
END_PROPSYNCS

BinStream &operator<<(BinStream &bs, const RndMesh::Face &face) {
    bs << face.v1 << face.v2 << face.v3;
    return bs;
}

BinStream &operator<<(BinStream &bs, const RndMesh::Vert &vert) {
    bs << vert.pos << vert.norm;
    bs << vert.color << vert.tex << vert.boneWeights << vert.boneIndices[0]
       << vert.boneIndices[1] << vert.boneIndices[2] << vert.boneIndices[3] << vert.unk50;
    return bs;
}

BinStream &operator<<(BinStream &bs, const RndBone &bone) {
    bs << bone.mBone << bone.mOffset;
    return bs;
}

BEGIN_SAVES(RndMesh)
    SAVE_REVS(0x26, 0)
    SAVE_SUPERCLASS(Hmx::Object)
    SAVE_SUPERCLASS(RndTransformable)
    SAVE_SUPERCLASS(RndDrawable)
    bs << mMat;
    bs << mGeomOwner << mMutable << mVolume << mBSPTree;
    SaveVertices(bs);
    bs << mFaces << mPatches << mBones;
    bs << mKeepMeshData;
    bs << mHasAOCalc;
END_SAVES

void RndMesh::CopyBones(const RndMesh *mesh) {
    if (mesh)
        mBones = mesh->mBones;
    else
        mBones.clear();
}

BEGIN_COPYS(RndMesh)
    COPY_SUPERCLASS(Hmx::Object)
    COPY_SUPERCLASS(RndTransformable)
    COPY_SUPERCLASS(RndDrawable)
    CREATE_COPY(RndMesh)
    BEGIN_COPYING_MEMBERS
        COPY_MEMBER(mMat)
        if (ty != kCopyFromMax)
            COPY_MEMBER(mKeepMeshData)
        if (ty == kCopyFromMax)
            mMutable |= c->mMutable;
        else
            COPY_MEMBER(mMutable)
        mHasAOCalc = false;
        if (ty == kCopyShallow || (ty == kCopyFromMax && c->mGeomOwner != c)) {
            mGeomOwner = c->mGeomOwner.Ptr();
            CopyBones(c);
        } else {
            CopyGeometry(c, ty != kCopyFromMax);
            if (ty != kCopyFromMax)
                COPY_MEMBER(mHasAOCalc);
        }
    END_COPYING_MEMBERS
    Sync(0xBF);
END_COPYS

BinStreamRev &operator>>(BinStreamRev &, RndMesh::Vert &);
BinStream &operator>>(BinStreamRev &, RndMesh::Face &);
BinStream &operator>>(BinStream &, RndBone &);

template <class T1, class T2>
BinStream &CachedRead(BinStream &, std::vector<T1, T2> &);

BEGIN_LOADS(RndMesh)
    LOAD_REVS(bs)
    ASSERT_REVS(0x26, 0)
    if (bsrev.rev > 0x19) {
        Hmx::Object::Load(bsrev.stream);
    }
    RndTransformable::Load(bsrev.stream);
    RndDrawable::Load(bsrev.stream);
    if (bsrev.rev < 15) {
        ObjPtrList<Hmx::Object> oList(this);
        int dummy;
        bsrev.stream >> dummy;
        bsrev.stream >> oList;
    }
    int i22 = 0;
    if (bsrev.rev < 0x14) {
        int ib8, ie8;
        bsrev.stream >> ib8;
        bsrev.stream >> ie8;
        if (ib8 == 0 || ie8 == 0) {
            i22 = 0;
        } else if (ib8 == 1) {
            i22 = 2;
        } else if (ie8 == 7) {
            i22 = 3;
        } else {
            i22 = 1;
        }
    }
    if (bsrev.rev < 3) {
        int dummy;
        bsrev.stream >> dummy;
    }
    bsrev.stream >> mMat;
    if (bsrev.rev > 0x1A && bsrev.rev < 0x1C) {
        char buf[0x80];
        bsrev.stream.ReadString(buf, 0x80);
        if (!mMat && buf[0] != '\0') {
            mMat = LookupOrCreateMat(buf, Dir());
        }
    }
    bsrev.stream >> mGeomOwner;
    if (!mGeomOwner) {
        mGeomOwner = this;
    }
    if (bsrev.rev < 0x14 && mMat && (i22 == 0 || mMat->GetZMode() != kZModeDisable)) {
        mMat->SetZMode((ZMode)i22);
    }
    if (bsrev.rev < 0xD) {
        ObjOwnerPtr<RndMesh> mesh(this);
        bsrev.stream >> mesh;
        if (mesh != mGeomOwner) {
            MILO_NOTIFY("Combining face and vert owner of %s", Name());
        }
    }
    if (bsrev.rev < 0xF) {
        ObjPtr<RndTransformable> trans(this);
        bsrev.stream >> trans;
        SetTransParent(trans, false);
        SetTransConstraint((Constraint)2, nullptr, false);
    }
    if (bsrev.rev < 0xE) {
        ObjPtr<RndTransformable> trans1(this);
        ObjPtr<RndTransformable> trans2(this);
        bsrev.stream >> trans1 >> trans2;
    }
    if (bsrev.rev < 3) {
        Vector3 v;
        bsrev.stream >> v;
    }
    if (bsrev.rev < 0xF) {
        Sphere s;
        bsrev.stream >> s;
        SetSphere(s);
    }
    if (bsrev.rev > 4 && bsrev.rev < 8) {
        bool b;
        bsrev >> b;
    }
    if (bsrev.rev > 5 && bsrev.rev < 0x15) {
        String str;
        int x;
        bsrev.stream >> str;
        bsrev.stream >> x;
    }
    if (bsrev.rev > 0xF) {
        bsrev.stream >> mMutable;
    } else if (bsrev.rev > 0xB) {
        bool b;
        bsrev >> b;
        mMutable = b ? 31 : 0;
    }
    if (bsrev.rev > 0x11) {
        bsrev.stream >> (int &)mVolume;
    }
    if (bsrev.rev > 0x12) {
        RELEASE(mBSPTree);
        bsrev.stream >> mBSPTree;
    }
    if (bsrev.rev > 6 && bsrev.rev < 8) {
        bool b;
        bsrev >> b;
    }
    if (bsrev.rev > 8 && bsrev.rev < 0xB) {
        int x;
        bsrev.stream >> x;
    }
    LoadVertices(bsrev);
    if (bsrev.stream.Cached()) {
        CachedRead(bsrev.stream, mFaces);
    } else {
        bsrev >> mFaces;
    }
    if (bsrev.rev > 4 && bsrev.rev < 0x18) {
        int count;
        unsigned short s1, s2;
        bsrev.stream >> count;
        for (; count != 0; count--) {
            bsrev.stream >> s1;
            bsrev.stream >> s2;
        }
    }
    if (bsrev.rev > 0x17) {
        if (bsrev.stream.Cached()) {
            CachedRead(bsrev.stream, mPatches);
        } else {
            bsrev >> mPatches;
        }
    } else if (bsrev.rev > 0x15) {
        mPatches.clear();
        int count;
        unsigned int ui;
        bs >> count;
        for (; count != 0; count--) {
            std::vector<unsigned short> usvec;
            std::vector<unsigned int> uivec;
            bsrev >> ui >> usvec >> uivec;
            mPatches.push_back(ui);
        }
    } else if (bsrev.rev > 0x10)
        bsrev >> mPatches;
    if (bsrev.rev > 0x1C) {
        bsrev >> mBones;
        int max = MaxBones();
        if (mBones.size() > max) {
            MILO_NOTIFY(
                "%s: exceeds bone limit (%d of %d)",
                PathName(this),
                mBones.size(),
                MaxBones()
            );
            mBones.resize(MaxBones());
        }
    } else if (bsrev.rev > 0xD) {
        ObjPtr<RndTransformable> trans(this);
        bsrev.stream >> trans;
        if (trans) {
            mBones.resize(4);
            if (bsrev.rev > 0x16) {
                mBones[0].mBone = trans;
                bs >> mBones[1].mBone >> mBones[2].mBone >> mBones[3].mBone;
                bs >> mBones[0].mOffset >> mBones[1].mOffset >> mBones[2].mOffset
                    >> mBones[3].mOffset;
                if (bsrev.rev < 0x19) {
                    for (Vert *it = mVerts.begin(); it != mVerts.end(); ++it) {
                        it->boneWeights.Set(
                            ((1.0f - it->boneWeights.x) - it->boneWeights.y)
                                - it->boneWeights.z,
                            it->boneWeights.x,
                            it->boneWeights.y,
                            it->boneWeights.z
                        );
                    }
                }
            } else {
                if (TransConstraint() == RndTransformable::kConstraintParentWorld) {
                    ObjPtr<RndTransformable> &bone = mBones[0].mBone;
                    bone = TransParent();
                } else {
                    mBones[0].mBone = this;
                }
                mBones[0].mOffset.Reset();
                mBones[1].mBone = trans;
                bs >> mBones[2].mBone >> mBones[1].mOffset >> mBones[2].mOffset;
                mBones[3].mBone = nullptr;
            }
            for (int i = 0; i < 4; i++) {
                if (!mBones[i].mBone) {
                    mBones.resize(i);
                    break;
                }
            }
        } else
            mBones.clear();
    }
    RemoveInvalidBones();
    if (bsrev.rev > 0 && bsrev.rev < 4) {
        std::vector<std::vector<unsigned short> > usvec;
        bsrev >> usvec;
    }
    if (bsrev.rev == 0) {
        bool bd4;
        int ic0, ic4, ic8, icc;
        bsrev >> bd4 >> ic0 >> ic4 >> ic8;
        bsrev >> icc;
    }
    if (bsrev.rev == 0x12) {
        if (mGeomOwner == this) {
            SetVolume(mVolume);
            goto yes;
        }
    } else {
    yes:
        if (bsrev.rev >= 0x1E)
            goto next;
    }
    if (mMat && mMat->NormalMap()) {
        MakeTangentsLate(this);
    }
next:
    if (bsrev.rev < 0x1F) {
        SetZeroWeightBones();
    }
    if (bsrev.rev > 0x23) {
        bsrev >> mKeepMeshData;
    }
    if (gRev < MESH_REV_SEP_COLOR && IsSkinned()) {
        for (Vert *it = mVerts.begin(); it != mVerts.end(); ++it) {
            it->boneWeights.Set(
                it->color.red, it->color.green, it->color.blue, it->color.alpha
            );
            it->color.Zero();
        }
    }
    if (bsrev.rev > 0x25) {
        bsrev >> mHasAOCalc;
    }
    Sync(0xBF);
END_LOADS

int RndMesh::EstimatedSizeKb() const {
    // sizeof(Vert) is 0x50 here
    // but the actual struct is size 0x60
    return (NumVerts() * 0x50 + NumFaces() * sizeof(Face)) / 1024;
}

void RndMesh::ClearCompressedVerts() {
    RELEASE(mCompressedVerts);
    mNumCompressedVerts = 0;
}

void RndMesh::SetMat(RndMat *mat) { mMat = mat; }

void RndMesh::SetGeomOwner(RndMesh *m) {
    MILO_ASSERT(m, 0x1D7);
    mGeomOwner = m;
}

void RndMesh::SetKeepMeshData(bool keep) {
    if (keep != mKeepMeshData) {
        mKeepMeshData = keep;
        if (!mKeepMeshData) {
            mVerts.resize(0);
            ClearAndShrink(mFaces);
            ClearAndShrink(mPatches);
        }
    }
}

void RndMesh::SetNumVerts(int num) {
    Verts().resize(num);
    Sync(0x3F);
}
