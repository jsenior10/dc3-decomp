#include "hamobj/HamGameData.h"
#include "HamGameData.h"
#include "gesture/GestureMgr.h"
#include "hamobj/HamPlayerData.h"
#include "obj/Data.h"
#include "obj/DataFunc.h"
#include "obj/Dir.h"
#include "obj/Msg.h"
#include "obj/Object.h"
#include "os/DateTime.h"
#include "os/Debug.h"
#include "os/PlatformMgr.h"
#include "utl/Symbol.h"

HamGameData *TheGameData;

HamGameData::HamGameData() : mPlayerSidesLocked(0), unk35(0), mPlayers(this), unk50(0) {
    GetDateAndTime(unk48);
    for (int i = 0; i < kMaxPlayers; i++) {
        mPlayers.push_back(new HamPlayerData(i));
    }
}

HamGameData::~HamGameData() { DeleteAll(mPlayers); }

BEGIN_HANDLERS(HamGameData)
    HANDLE_EXPR(getp, *PlayerProp(_msg->Int(2), _msg->Sym(3)))
    HANDLE_ACTION(setp, SetPlayerProp(_msg->Int(2), _msg->Sym(3), _msg->Node(4)))
    HANDLE_EXPR(max_players, kMaxPlayers)
    HANDLE_EXPR(player, mPlayers[_msg->Int(2)])
    HANDLE_SUPERCLASS(Hmx::Object)
END_HANDLERS

BEGIN_PROPSYNCS(HamGameData)
    SYNC_PROP_SET(venue, Venue(), mVenue = _val.Sym())
    SYNC_PROP(song, mSong)
    SYNC_PROP_SET(
        player_sides_locked, mPlayerSidesLocked, SetPlayerSidesLocked(_val.Int())
    )
    SYNC_SUPERCLASS(Hmx::Object)
END_PROPSYNCS

DataNode OnGetCrewForCharacter(DataArray *);
DataNode OnGetOutfitGender(DataArray *);
DataNode OnGetNumCharacterOutfits(DataArray *);
DataNode OnGetCharacterOutfit(DataArray *);
DataNode GetPlayableOutfits(DataArray *);

void HamGameData::Init() {
    REGISTER_OBJ_FACTORY(HamGameData);
    RELEASE(TheGameData);
    TheGameData = new HamGameData();
    TheGameData->SetName("gamedata", ObjectDir::Main());
    DataRegisterFunc("get_crew_for_character", OnGetCrewForCharacter);
    DataRegisterFunc("outfit_gender", OnGetOutfitGender);
    DataRegisterFunc("num_character_outfits", OnGetNumCharacterOutfits);
    DataRegisterFunc("character_outfit", OnGetCharacterOutfit);
    DataRegisterFunc("playable_outfits", GetPlayableOutfits);
}

void HamGameData::Clear() {
    mSong = gNullStr;
    mVenue = gNullStr;
}

Symbol HamGameData::Venue() const {
    static Symbol bid("bid");
    return unk35 ? bid : mVenue;
}

const DataNode *HamGameData::PlayerProp(int index, Symbol s2) const {
    MILO_ASSERT((0) <= (index) && (index) < (2), 0x224);
    return mPlayers[index]->Property(s2, true);
}

void HamGameData::SetPlayerProp(int index, Symbol s2, const DataNode &n) {
    MILO_ASSERT((0) <= (index) && (index) < (2), 0x22A);
    mPlayers[index]->SetProperty(s2, n);
}

HamPlayerData *HamGameData::Player(int index) const {
    MILO_ASSERT((0) <= (index) && (index) < (2), 0x230);
    return mPlayers[index];
}

String HamGameData::GetPlayerName(int player) {
    MILO_ASSERT((0) <= (player) && (player) < (2), 0x2B7);
    HamPlayerData *pPlayer = mPlayers[player];
    MILO_ASSERT(pPlayer, 0x2BA);
    return pPlayer->GetPlayerName();
}

void HamGameData::AssignSkeleton(int player, int id) {
    MILO_ASSERT((0) <= (player) && (player) < (2), 0x247);
    if (id != -1) {
        for (int i = 0; i < kMaxPlayers; i++) {
            if (i != player && id == mPlayers[i]->mSkeletonTrackingID) {
                MILO_NOTIFY(
                    "AssignSkeleton called for player %i with tracking_id %i, butplayer % i is already using it",
                    player,
                    id,
                    i
                );
            }
        }
    }
    mPlayers[player]->SetSkeletonTrackingID(id);
}

void HamGameData::UnassignSkeletons() {
    mPlayers[0]->SetSkeletonTrackingID(-1);
    mPlayers[1]->SetSkeletonTrackingID(-1);
}

void HamGameData::SwapPlayerSidesByIDOnly() {
    HamPlayerData *pPlayer0 = mPlayers[0];
    MILO_ASSERT(pPlayer0, 0x27D);
    HamPlayerData *pPlayer1 = mPlayers[1];
    MILO_ASSERT(pPlayer1, 0x27F);
    int id0 = pPlayer0->mSkeletonTrackingID;
    pPlayer0->SetSkeletonTrackingID(pPlayer1->mSkeletonTrackingID);
    pPlayer1->SetSkeletonTrackingID(id0);
}

bool HamGameData::SidesSwapped() {
    static Symbol side("side");
    int side0 = mPlayers[0]->mProvider->Property(side, true)->Int();
    return side0 != 1;
}

bool HamGameData::IsSkeletonPresent(int index) const {
    return TheGestureMgr->InControllerMode() || Player(index)->mSkeletonTrackingID > 0;
}

int HamGameData::GetPlayerFromSkeleton(const Skeleton &skeleton) const {
    for (int i = 0; i < kMaxPlayers; i++) {
        HamPlayerData *player = TheGameData->Player(i);
        if (player->mSkeletonTrackingID == skeleton.TrackingID())
            return i;
    }
    return -1;
}

bool HamGameData::SetAssociatedPadNum(int player, int padnum) {
    MILO_ASSERT((0) <= (player) && (player) < (2), 0x2CD);
    HamPlayerData *pPlayer = mPlayers[player];
    MILO_ASSERT(pPlayer, 0x2D0);
    if (padnum >= 0 && ThePlatformMgr.IsSignedIn(padnum)) {
        HamPlayerData *playerData = mPlayers[player];
        if (playerData->mPadNum == padnum) {
            playerData->SetAssociatedPadNum(-1, gNullStr);
        }
        return playerData->SetAssociatedPadNum(padnum, ThePlatformMgr.GetName(padnum));
    } else {
        return pPlayer->SetAssociatedPadNum(-1, gNullStr);
    }
}

void HamGameData::SwapPlayerSides() {
    static Symbol side("side");
    int side0 = mPlayers[0]->mProvider->Property(side, true)->Int();
    int side1 = mPlayers[1]->mProvider->Property(side, true)->Int();
    mPlayers[0]->mProvider->SetProperty(side, side1);
    mPlayers[1]->mProvider->SetProperty(side, side0);
    for (int i = 0; i < kMaxPlayers; i++) {
        if (0.5f < Player(i)->TrackingAgeSeconds()) {
            static Message side_moved("side_moved");
            Player(i)->mProvider->Export(side_moved, true);
        }
    }
}

void HamGameData::HandlePoseFound(int i1) {
    static Message poseFound("pose_found", 0);
    poseFound[0] = i1;
    Export(poseFound, true);
}

void HamGameData::UpdateAssociatedPads() {
    for (int i = 0; i < kMaxPlayers; i++) {
        HamPlayerData *pPlayer = mPlayers[i];
        MILO_ASSERT(pPlayer, 0x2C4);
        SetAssociatedPadNum(i, pPlayer->mPadNum);
    }
}
