#include "utl/CacheMgr_Xbox.h"
#include "Cache.h"
#include "CacheMgr.h"
#include "obj/Object.h"
#include "os/Debug.h"
#include "os/PlatformMgr.h"
#include "utl/Cache.h"
#include "utl/CacheMgr.h"
#include "utl/Cache_Xbox.h"
#include "xdk/XAPILIB.h"
#include "xdk/win_types.h"
#include "xdk/xapilibi/xbase.h"
#include "xdk/xapilibi/xbox.h"

bool IsDeviceConnected(DWORD deviceID) {
    return XContentGetDeviceState(deviceID, nullptr) == 0;
}

CacheMgrXbox::CacheMgrXbox()
    : unk18(INVALID_HANDLE_VALUE), mppCacheID(nullptr), unk3c(0), unk40(0) {
    memset(&mContentData, 0, sizeof(XCONTENT_DATA));
    mContentData.DeviceID = 0;
}

CacheMgrXbox::~CacheMgrXbox() {}

void CacheMgrXbox::Poll() {
    switch (GetOp()) {
    case kOpNone:
        break;
    case kOpSearch:
        PollSearch();
        break;
    case kOpChoose:
        PollChoose();
        break;
    case kOpMount:
        PollMount();
        break;
    case kOpUnmount:
        PollUnmount();
        break;
    case kOpDelete:
        PollDelete();
        break;
    default:
        MILO_FAIL("Unknown OpType encountered in CacheMgr::Poll()\n");
        break;
    }
}

bool CacheMgrXbox::SearchAsync(const char *cc, CacheID **ppCacheID) {
    if (!IsDone()) {
        SetLastResult(kCache_ErrorBusy);
        return false;
    } else if (ppCacheID && !*ppCacheID) {
        mStrCacheName = cc;
        if (mStrCacheName.empty()) {
            MILO_LOG("SearchAsync BAD PARAM: mStrCacheName is empty\n");
            SetLastResult(kCache_ErrorBadParam);
            return false;
        } else {
            DWORD bufferSize = 0;
            DWORD res = XContentCreateEnumerator(0xFE, 0, 1, 0, 1, &bufferSize, &unk18);
            if (res == 0x12) {
                unk18 = INVALID_HANDLE_VALUE;
                SetLastResult(kCache_ErrorNoStorageDevice);
                return false;
            } else {
                if (res == 0 && unk18 != INVALID_HANDLE_VALUE) {
                    MILO_ASSERT(bufferSize == sizeof(XCONTENT_DATA), 0x88);
                    memset(&mContentData, 0, sizeof(XCONTENT_DATA));
                    mContentData.DeviceID = 0;
                    memset(&mOverlapped, 0, sizeof(XOVERLAPPED));
                    DWORD enumRes =
                        XEnumerate(unk18, &mContentData, 0x134, nullptr, &mOverlapped);
                    if (enumRes != 0x3E5) {
                        MILO_NOTIFY(
                            "CacheMgrXbox::SearchAsync(): Unhandled error %u returned from XEnumerate().\n",
                            enumRes
                        );
                        EndSearch(kCache_ErrorUnknown);
                        return false;
                    } else {
                        mppCacheID = ppCacheID;
                        SetOp(kOpSearch);
                        SetLastResult(kCache_NoError);
                        return true;
                    }
                } else {
                    MILO_NOTIFY(
                        "CacheMgrXbox::SearchAsync(): Unhandled error %u returned from XContentCreateEnumerator().\n",
                        res
                    );
                    unk18 = INVALID_HANDLE_VALUE;
                    SetLastResult(kCache_ErrorUnknown);
                    return false;
                }
            }
        }
    } else {
        MILO_LOG("SearchAsync BAD PARAM: ppCacheID = 0x%X", ppCacheID);
        if (ppCacheID) {
            MILO_LOG(", *ppCacheID = 0x%X", *ppCacheID);
        }
        MILO_LOG("\n");
        SetLastResult(kCache_ErrorBadParam);
        return false;
    }
}

bool CacheMgrXbox::ShowUserSelectUIAsync(
    LocalUser *user, u64 u, const char *cc1, const char *cc2, CacheID **ppCacheID
) {
    if (!IsDone()) {
        SetLastResult(kCache_ErrorBusy);
        return false;
    } else if (ppCacheID && !*ppCacheID) {
        mStrCacheName = cc1;
        unk184 = cc2;
        if (!mStrCacheName.empty() && !unk184.empty()) {
            DWORD padnum = !user ? 0xFF : user->GetPadNum();
            memset(&mContentData, 0, sizeof(XCONTENT_DATA));
            mContentData.DeviceID = 0;
            memset(&mOverlapped, 0, sizeof(XOVERLAPPED));
            ULARGE_INTEGER ul;
            ul.QuadPart = u;
            if (ul.QuadPart != 0) {
                ul.QuadPart = XContentCalculateSize(ul.QuadPart, 1);
            }
            DWORD res = ThePlatformMgr.ShowDeviceSelectorUI(
                padnum, 1, 0, ul, &mContentData.DeviceID, &mOverlapped
            );
            switch (res) {
            case 5:
                SetLastResult(kCache_Error360GuideAlreadyOut);
                return false;
            case 0x3E5:
                mppCacheID = ppCacheID;
                SetLastResult(kCache_NoError);
                SetOp(kOpChoose);
                return true;
            default:
                MILO_NOTIFY(
                    "CacheMgrXbox::ShowUserSelectUIAsync(): Unhandled error %u returned from XShowDeviceSelectorUI().\n",
                    res
                );
                SetLastResult(kCache_ErrorUnknown);
                return false;
                break;
            }
        } else {
            SetLastResult(kCache_ErrorBadParam);
            return false;
        }
    } else {
        SetLastResult(kCache_ErrorBadParam);
        return false;
    }
}

bool CacheMgrXbox::CreateCacheIDFromDeviceID(
    unsigned int ui, const char *cc1, const char *cc2, CacheID **ppCacheID
) {
    if (!IsDone()) {
        SetLastResult(kCache_ErrorBusy);
        return false;
    } else if (ppCacheID && !*ppCacheID) {
        mStrCacheName = cc1;
        unk184 = cc2;
        if (!mStrCacheName.empty() && !unk184.empty()) {
            memset(&mContentData, 0, sizeof(XCONTENT_DATA));
            mContentData.DeviceID = ui;
            mppCacheID = ppCacheID;
            CreateCacheIDForChosenDevice();
            SetLastResult(kCache_NoError);
            SetOp(kOpNone);
            return true;
        } else {
            SetLastResult(kCache_ErrorBadParam);
            return false;
        }
    } else {
        SetLastResult(kCache_ErrorBadParam);
        return false;
    }
}

bool CacheMgrXbox::MountAsync(CacheID *pCacheIDXbox, Cache **ppCache, Hmx::Object *o) {
    if (!IsDone()) {
        MILO_NOTIFY("MountAsync: !IsDone() current op is %i", GetOp());
        SetLastResult(kCache_ErrorBusy);
        return false;
    } else if (ppCache && !*ppCache) {
        CacheIDXbox *myCacheXbox = dynamic_cast<CacheIDXbox *>(pCacheIDXbox);
        if (!myCacheXbox) {
            MILO_NOTIFY("pCacheIDXbox == NULL");
            SetLastResult(kCache_ErrorBadParam);
            return false;
        } else {
            ULARGE_INTEGER u;
            u.HighPart = 0;
            u.LowPart = 0;
            memset(&mOverlapped, 0, sizeof(XOVERLAPPED));
            DWORD res = XContentCreateEx(
                0xFF,
                myCacheXbox->Name(),
                myCacheXbox->ContentData(),
                4,
                nullptr,
                nullptr,
                0,
                u,
                &mOverlapped
            );
            if (res != 0x3E5) {
                if (XContentGetDeviceState(myCacheXbox->DeviceID(), nullptr) != 0) {
                    SetLastResult(kCache_ErrorStorageDeviceMissing);
                    return false;
                } else {
                    MILO_NOTIFY(
                        "CacheMgrXbox::MountAsync(): Unhandled error %u returned from XContentCreateEx().\n",
                        res
                    );
                    SetLastResult(kCache_ErrorUnknown);
                    return false;
                }
            } else {
                unk44 = myCacheXbox;
                unk3c = ppCache;
                unk40 = o;
                SetLastResult(kCache_NoError);
                SetOp(kOpMount);
                return true;
            }
        }
    } else if (!ppCache) {
        MILO_NOTIFY("ppCache == NULL");
        SetLastResult(kCache_ErrorBadParam);
        return false;
    } else {
        MILO_NOTIFY("*ppCache != NULL");
        SetLastResult(kCache_ErrorBadParam);
        return false;
    }
}

bool CacheMgrXbox::UnmountAsync(Cache **ppCache, Hmx::Object *o) {
    if (!IsDone()) {
        MILO_NOTIFY("MountAsync: !IsDone() current op is %i", GetOp());
        SetLastResult(kCache_ErrorBusy);
        return false;
    } else if (ppCache && *ppCache) {
        memset(&mOverlapped, 0, sizeof(XOVERLAPPED));
        const char *name = (*ppCache)->GetCacheName();
        DWORD res = XContentClose(name, &mOverlapped);
        if (res != 0x3E5) {
            if (XContentGetDeviceState(unk44->DeviceID(), nullptr) != 0) {
                MILO_NOTIFY("UnmountAsync: device is not connected");
                SetLastResult(kCache_ErrorStorageDeviceMissing);
                return false;
            } else {
                MILO_NOTIFY(
                    "CacheMgrXbox::UnmountAsync(): Unhandled error %u returned from XContentClose().\n",
                    res
                );
                SetLastResult(kCache_ErrorUnknown);
                return false;
            }
        } else {
            unk3c = ppCache;
            unk40 = o;
            SetLastResult(kCache_NoError);
            SetOp(kOpUnmount);
            return true;
        }
    } else {
        MILO_LOG("UnmountAsync: ppCache is NULL (MU pull?)");
        SetLastResult(kCache_ErrorBadParam);
        return false;
    }
}

bool CacheMgrXbox::DeleteAsync(CacheID *id) {
    if (!IsDone()) {
        SetLastResult(kCache_ErrorBusy);
        return false;
    } else {
        CacheIDXbox *cacheXbox = dynamic_cast<CacheIDXbox *>(id);
        if (!cacheXbox) {
            SetLastResult(kCache_ErrorBadParam);
            return false;
        } else {
            memset(&mOverlapped, 0, sizeof(XOVERLAPPED));
            DWORD res = XContentDelete(0xFF, cacheXbox->ContentData(), &mOverlapped);
            if (res != 0x3E5) {
                if (XContentGetDeviceState(unk44->DeviceID(), nullptr) != 0) {
                    SetLastResult(kCache_ErrorStorageDeviceMissing);
                    return false;
                } else {
                    MILO_NOTIFY(
                        "CacheMgrXbox::DeleteAsync(): Unhandled error %u returned from XContentClose().\n",
                        res
                    );
                    SetLastResult(kCache_ErrorUnknown);
                    return false;
                }
            } else {
                unk44 = cacheXbox;
                SetLastResult(kCache_NoError);
                SetOp(kOpDelete);
                return true;
            }
        }
    }
}
