#include "rndobj/Mesh.h"
#include "obj/Data.h"
#include "obj/Object.h"
#include "obj/PropSync.h"
#include "os/Debug.h"
#include "rndobj/Mat.h"
#include "rndobj/MultiMesh.h"
#include "utl/BinStream.h"
#include "utl/Std.h"

RndMesh::RndMesh()
    : mMat(this), mGeomOwner(this, this), mBones(this), mMutable(0),
      mVolume(kVolumeTriangles), mBSPTree(nullptr), mMultiMesh(nullptr), unk170(0),
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
    SYNC_PROP(has_ao_calculation, unk170)
    SYNC_PROP_SET(keep_mesh_data, mKeepMeshData, SetKeepMeshData(_val.Int() > 0))
    SYNC_PROP(verts, Verts())
    SYNC_SUPERCLASS(RndTransformable)
    SYNC_SUPERCLASS(RndDrawable)
    SYNC_SUPERCLASS(Hmx::Object)
END_PROPSYNCS

int RndMesh::EstimatedSizeKb() const {
    // sizeof(Vert) is 0x50 here
    // but the actual struct is size 0x60
    return (NumVerts() * 0x50 + NumFaces() * sizeof(Face)) / 1024;
}

void RndMesh::ClearCompressedVerts() {
    RELEASE(mCompressedVerts);
    mNumCompressedVerts = 0;
}

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
