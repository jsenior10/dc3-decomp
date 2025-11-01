#pragma once
#include "../win_types.h"
#include "findfirstfile.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);

#ifdef __cplusplus
}
#endif
