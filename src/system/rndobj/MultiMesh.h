#pragma once
#include "obj/Object.h"
#include "rndobj/Draw.h"
#include "rndobj/Mesh.h"
#include "utl/MemMgr.h"

/**
 * @brief An object that instances \ref RndMesh "RndMeshes".
 * Original _objects description:
 * "A MultiMesh object draws a simple Mesh in many places
 * quickly. Create a multimesh by instancing the base mesh lots of
 * times in Max, then using the multiobject wizard."
 */
class RndMultiMesh : public RndDrawable {
public:
    struct Instance {
        Instance();

        bool unk0; // 0x0
        Transform mXfm; // 0x4
    };

    // Hmx::Object
    OBJ_CLASSNAME(MultiMesh);
    OBJ_SET_TYPE(MultiMesh);
    virtual DataNode Handle(DataArray *, bool);
    virtual bool SyncProperty(DataNode &, DataArray *, int, PropOp);
    virtual void Save(BinStream &);
    virtual void Copy(const Hmx::Object *, Hmx::Object::CopyType);
    virtual void Load(BinStream &);
    virtual void Print();
    // RndDrawable
    virtual float GetDistanceToPlane(const Plane &, Vector3 &);
    virtual bool MakeWorldSphere(Sphere &, bool);
    virtual void Mats(std::list<class RndMat *> &, bool);
    virtual void DrawShowing();
    virtual void ListDrawChildren(std::list<RndDrawable *> &);
    virtual void CollideList(const Segment &, std::list<Collision> &);

    OBJ_MEM_OVERLOAD(0x1C);
    NEW_OBJ(RndMultiMesh)
    static void Init() { REGISTER_OBJ_FACTORY(RndMultiMesh) }
    static void Terminate();

    RndMesh *Mesh() const { return mMesh; }
    void SetMesh(RndMesh *);
    std::list<RndMultiMesh::Instance> &Instances() { return mInstances; }

protected:
    RndMultiMesh();

    /** The simple mesh to draw. */
    ObjPtr<RndMesh> mMesh; // 0x20
    /** The locations at which the mesh should be drawn. */
    std::list<RndMultiMesh::Instance> mInstances; // 0x2C
};
