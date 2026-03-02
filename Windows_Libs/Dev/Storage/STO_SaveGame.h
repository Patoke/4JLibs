#pragma once
#include "4J_Storage.h"

class CSaveGame
{
public:
    CSaveGame();
    void SetSaveDisabled(bool bDisable);
    bool GetSaveDisabled(void);

    void ResetSaveData();
    C4JStorage::ESaveGameState GetSavesInfo(int iPad, int (*Func)(LPVOID lpParam, SAVE_DETAILS *pSaveDetails, const bool), LPVOID lpParam,
                                            char *pszSavePackName);
    PSAVE_DETAILS ReturnSavesInfo();
    void ClearSavesInfo();
    C4JStorage::ESaveGameState LoadSaveDataThumbnail(PSAVE_INFO pSaveInfo, int (*Func)(LPVOID lpParam, PBYTE pbThumbnail, DWORD dwThumbnailBytes),
                                                     LPVOID lpParam);
    C4JStorage::ESaveGameState LoadSaveData(PSAVE_INFO pSaveInfo, int (*Func)(LPVOID lpParam, const bool, const bool), LPVOID lpParam);
    unsigned int GetSaveSize();
    void GetSaveData(void *pvData, unsigned int *puiBytes);
    bool GetSaveUniqueNumber(INT *piVal);
    bool GetSaveUniqueFilename(char *pszName);
    void SetSaveTitle(LPCWSTR pwchDefaultSaveName);
    PVOID AllocateSaveData(unsigned int uiBytes);
    void SetSaveImages(PBYTE pbThumbnail, DWORD dwThumbnailBytes, PBYTE pbImage, DWORD dwImageBytes, PBYTE pbTextData, DWORD dwTextDataBytes);
    C4JStorage::ESaveGameState SaveSaveData(int (*Func)(LPVOID, const bool), LPVOID lpParam);
    C4JStorage::ESaveGameState DeleteSaveData(PSAVE_INFO pSaveInfo, int (*Func)(LPVOID lpParam, const bool), LPVOID lpParam);
    void SetSaveUniqueFilename(char *szFilename);

    void CreateSaveUniqueName(void);

    void *m_pSaveData;
    unsigned int m_uiSaveSize;
    char m_szSaveUniqueName[32];
    char m_szSaveTitle[256];
    bool m_bIsSafeDisabled;
    bool m_bHasSaveDetails;
    SAVE_DETAILS *m_pSaveDetails;
};