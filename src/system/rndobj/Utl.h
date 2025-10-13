#pragma once
#include "obj/Dir.h"
#include "rndobj/Draw.h"
#include "rndobj/Mesh.h"
#include "rndobj/Poll.h"
#include "rndobj/Trans.h"
#include "math/Vec.h"
#include "math/Color.h"
#include <vector>

void SetLocalScale(RndTransformable *, const Vector3 &);

DataNode GetNormalMapTextures(ObjectDir *);
DataNode GetRenderTextures(ObjectDir *);
DataNode OnTestDrawGroups(DataArray *);

void ResetColors(std::vector<Hmx::Color> &colors, int newNumColors);
void RndScaleObject(Hmx::Object *, float, float);

bool SortDraws(RndDrawable *, RndDrawable *);
bool SortPolls(const RndPollable *, const RndPollable *);

void CreateAndSetMetaMat(RndMat *);
void FixVertOrder(const RndMesh *, RndMesh *);

void UtilDrawSphere(const Vector3 &, float, const Hmx::Color &, RndMat *);
void UtilDrawLine(const Vector2 &, const Vector2 &, const Hmx::Color &);
void UtilDrawString(const char *, const Vector3 &, const Hmx::Color &);

void TestTextureSize(ObjectDir *, int, int, int, int, int);
void TestTexturePaths(ObjectDir *);
void TestMaterialTextures(ObjectDir *);

void RndUtlPreInit();
void RndUtlInit();
void RndUtlTerminate();
void RndSplasherPoll();
void RndSplasherSuspend();
void RndSplasherResume();

void MakeTangentsLate(RndMesh *);
