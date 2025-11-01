#pragma once
#include "../win_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard,
    GetFileExMaxInfoLevel
} GET_FILEEX_INFO_LEVELS;

BOOL GetFileAttributesExA(
    LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation
);

#ifdef __cplusplus
}
#endif
