#include "hamobj/HamDirector.h"
#include "hamobj/HamGameData.h"
#include "math/Rand.h"
#include "math/Utl.h"
#include "obj/Data.h"
#include "obj/Object.h"
#include "os/Debug.h"

HamDirector *TheHamDirector;

HamDirector::HamDirector()
    : unk8c(this), unka0(this), unkb4(this), unkc8(0), unkcc(""), unkd0(1), unkd4(this),
      unke8(this), unkfc(this), unk110(this), unk124(this), unk140(0), unk14c(0),
      unk150(this), unk164(this), unk178(this), unk18c(this), unk1a0(0), unk1a4(1),
      unk1a8(this), unk1bc(this), unk1d0(0), unk1d4(0), unk1d8(this), unk1ec(this),
      unk200(1), unk204(this), unk218(this), unk22c(this), unk240(this), unk254(0),
      unk259(0), unk25a(0), unk25c(this), unk270(this), unk284(this), unk29c(-kHugeFloat),
      unk2a0(0), unk2a1(0), unk2a4(0), unk2a8(-kHugeFloat), unk2ac(1), unk2bc(0),
      unk2bd(0), unk2c0(this), unk2d4(0), unk2d8(0), unk2dc(1), unk2e0(1), unk2e8(0),
      unk304(0), unk308(this), unk31c(this), unk334(0), unk335(1), unk336(1), unk338(0),
      unk33c(RandomInt(0, 2)), unk33d(0), unk340(this), unk354(this), unk369(0),
      unk36c(0) {
    static DataNode &n = DataVariable("hamdirector");
    n = this;
    TheHamDirector = this;
    unk2b0.reserve(100);
}

HamDirector::~HamDirector() {
    MILO_ASSERT(TheGameData, 0xC5);
    TheGameData->Clear();
    if (TheHamDirector == this) {
        static DataNode &n = DataVariable("hamdirector");
        n = NULL_OBJ;
        TheHamDirector = nullptr;
    }
}
