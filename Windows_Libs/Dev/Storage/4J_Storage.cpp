#pragma once
#include "4J_Storage.h"
#include "STO_Main.h"

C4JStorage StorageManager;
XMARKETPLACE_CONTENTOFFER_INFO InternalContentOfferInfo;

C4JStorage::C4JStorage() {}

void C4JStorage::Tick(void)
{
    InternalStorageManager.Tick();
}

C4JStorage::EMessageResult C4JStorage::RequestMessageBox(UINT uiTitle, UINT uiText, UINT *uiOptionA, UINT uiOptionC, DWORD dwPad,
                                                         int (*Func)(LPVOID, int, const C4JStorage::EMessageResult), LPVOID lpParam,
                                                         C4JStringTable *pStringTable, WCHAR *pwchFormatString, DWORD dwFocusButton)
{
    return EMessage_Undefined;
}

C4JStorage::EMessageResult C4JStorage::GetMessageBoxResult()
{
    return EMessage_Undefined;
}

bool C4JStorage::SetSaveDevice(int (*Func)(LPVOID, const bool), LPVOID lpParam, bool bForceResetOfSaveDevice)
{
    return true;
}

void C4JStorage::Init(unsigned int uiSaveVersion, LPCWSTR pwchDefaultSaveName, char *pszSavePackName, int iMinimumSaveSize,
                      int (*Func)(LPVOID, const ESavingMessage, int), LPVOID lpParam, LPCSTR szGroupID)
{
    InternalStorageManager.Init(Func, lpParam, szGroupID);
}

void C4JStorage::ResetSaveData()
{
    InternalStorageManager.m_SaveGame.ResetSaveData();
}

void C4JStorage::SetDefaultSaveNameForKeyboardDisplay(LPCWSTR pwchDefaultSaveName)
{
    ;
}

void C4JStorage::SetSaveTitle(LPCWSTR pwchDefaultSaveName)
{
    InternalStorageManager.m_SaveGame.SetSaveTitle(pwchDefaultSaveName);
}

bool C4JStorage::GetSaveUniqueNumber(INT *piVal)
{
    return InternalStorageManager.m_SaveGame.GetSaveUniqueNumber(piVal);
}

bool C4JStorage::GetSaveUniqueFilename(char *pszName)
{
    return InternalStorageManager.m_SaveGame.GetSaveUniqueFilename(pszName);
}

void C4JStorage::SetSaveUniqueFilename(char *szFilename)
{
    InternalStorageManager.m_SaveGame.SetSaveUniqueFilename(szFilename);
}

void C4JStorage::SetState(ESaveGameControlState eControlState, int (*Func)(LPVOID, const bool), LPVOID lpParam)
{
    ;
}

void C4JStorage::SetSaveDisabled(bool bDisable)
{
    InternalStorageManager.m_SaveGame.SetSaveDisabled(bDisable);
}

bool C4JStorage::GetSaveDisabled(void)
{
    return InternalStorageManager.m_SaveGame.GetSaveDisabled();
}

unsigned int C4JStorage::GetSaveSize()
{
    return InternalStorageManager.m_SaveGame.GetSaveSize();
}

void C4JStorage::GetSaveData(void *pvData, unsigned int *puiBytes)
{
    InternalStorageManager.m_SaveGame.GetSaveData(pvData, puiBytes);
}

PVOID C4JStorage::AllocateSaveData(unsigned int uiBytes)
{
    return InternalStorageManager.m_SaveGame.AllocateSaveData(uiBytes);
}

void C4JStorage::SetSaveImages(PBYTE pbThumbnail, DWORD dwThumbnailBytes, PBYTE pbImage, DWORD dwImageBytes, PBYTE pbTextData, DWORD dwTextDataBytes)
{
    InternalStorageManager.m_SaveGame.SetSaveImages(pbThumbnail, dwThumbnailBytes, pbImage, dwImageBytes, pbTextData, dwTextDataBytes);
}

C4JStorage::ESaveGameState C4JStorage::SaveSaveData(int (*Func)(LPVOID, const bool), LPVOID lpParam)
{
    return InternalStorageManager.m_SaveGame.SaveSaveData(Func, lpParam);
}

void C4JStorage::CopySaveDataToNewSave(PBYTE pbThumbnail, DWORD cbThumbnail, WCHAR *wchNewName, int (*Func)(LPVOID lpParam, bool), LPVOID lpParam)
{
    ;
}

void C4JStorage::SetSaveDeviceSelected(unsigned int uiPad, bool bSelected)
{
    ;
}

bool C4JStorage::GetSaveDeviceSelected(unsigned int iPad)
{
    return true;
}

C4JStorage::ESaveGameState C4JStorage::DoesSaveExist(bool *pbExists)
{
    *pbExists = true;
    return ESaveGame_Idle;
}

bool C4JStorage::EnoughSpaceForAMinSaveGame()
{
    return true;
}

void C4JStorage::SetSaveMessageVPosition(float fY)
{
    ;
}

C4JStorage::ESaveGameState C4JStorage::GetSavesInfo(int iPad, int (*Func)(LPVOID lpParam, SAVE_DETAILS *pSaveDetails, const bool), LPVOID lpParam,
                                                    char *pszSavePackName)
{
    return InternalStorageManager.m_SaveGame.GetSavesInfo(iPad, Func, lpParam, pszSavePackName);
}

PSAVE_DETAILS C4JStorage::ReturnSavesInfo()
{
    return InternalStorageManager.m_SaveGame.ReturnSavesInfo();
}

void C4JStorage::ClearSavesInfo()
{
    InternalStorageManager.m_SaveGame.ClearSavesInfo();
}

C4JStorage::ESaveGameState C4JStorage::LoadSaveDataThumbnail(PSAVE_INFO pSaveInfo,
                                                             int (*Func)(LPVOID lpParam, PBYTE pbThumbnail, DWORD dwThumbnailBytes), LPVOID lpParam)
{
    return InternalStorageManager.m_SaveGame.LoadSaveDataThumbnail(pSaveInfo, Func, lpParam);
}

void C4JStorage::GetSaveCacheFileInfo(DWORD dwFile, XCONTENT_DATA &xContentData)
{
    ;
}

void C4JStorage::GetSaveCacheFileInfo(DWORD dwFile, PBYTE *ppbImageData, DWORD *pdwImageBytes)
{
    ;
}

C4JStorage::ESaveGameState C4JStorage::LoadSaveData(PSAVE_INFO pSaveInfo, int (*Func)(LPVOID lpParam, const bool, const bool), LPVOID lpParam)
{
    return InternalStorageManager.m_SaveGame.LoadSaveData(pSaveInfo, Func, lpParam);
}

C4JStorage::ESaveGameState C4JStorage::DeleteSaveData(PSAVE_INFO pSaveInfo, int (*Func)(LPVOID lpParam, const bool), LPVOID lpParam)
{
    return InternalStorageManager.m_SaveGame.DeleteSaveData(pSaveInfo, Func, lpParam);
}

void C4JStorage::RegisterMarketplaceCountsCallback(int (*Func)(LPVOID lpParam, C4JStorage::DLC_TMS_DETAILS *, int), LPVOID lpParam)
{
    ;
}

void C4JStorage::SetDLCPackageRoot(char *pszDLCRoot)
{
    InternalStorageManager.m_DLC.SetPackageRoot(pszDLCRoot);
}

C4JStorage::EDLCStatus C4JStorage::GetDLCOffers(int iPad, int (*Func)(LPVOID, int, DWORD, int), LPVOID lpParam, DWORD dwOfferTypesBitmask)
{
    return EDLC_Idle;
}

DWORD C4JStorage::CancelGetDLCOffers()
{
    return 0;
}

void C4JStorage::ClearDLCOffers()
{
    ;
}

XMARKETPLACE_CONTENTOFFER_INFO &C4JStorage::GetOffer(DWORD dw)
{
    return InternalContentOfferInfo;
}

int C4JStorage::GetOfferCount()
{
    return 0;
}

DWORD C4JStorage::InstallOffer(int iOfferIDC, __uint64 *ullOfferIDA, int (*Func)(LPVOID, int, int), LPVOID lpParam, bool bTrial)
{
    return 0;
}

DWORD C4JStorage::GetAvailableDLCCount(int iPad)
{
    return InternalStorageManager.m_DLC.GetAvailableDLCCount(iPad);
}

C4JStorage::EDLCStatus C4JStorage::GetInstalledDLC(int iPad, int (*Func)(LPVOID, int, int), LPVOID lpParam)
{
    return InternalStorageManager.m_DLC.GetInstalledDLC(iPad, Func, lpParam);
}

XCONTENT_DATA &C4JStorage::GetDLC(DWORD dw)
{
    return InternalStorageManager.m_DLC.GetDLC(dw);
}

DWORD C4JStorage::MountInstalledDLC(int iPad, DWORD dwDLC, int (*Func)(LPVOID, int, DWORD, DWORD), LPVOID lpParam, LPCSTR szMountDrive)
{
    return InternalStorageManager.m_DLC.MountInstalledDLC(iPad, dwDLC, Func, lpParam, szMountDrive);
}

DWORD C4JStorage::UnmountInstalledDLC(LPCSTR szMountDrive)
{
    return InternalStorageManager.m_DLC.UnmountInstalledDLC(szMountDrive);
}

void C4JStorage::GetMountedDLCFileList(const char *szMountDrive, std::vector<std::string> &fileList)
{
    InternalStorageManager.m_DLC.GetMountedDLCFileList(szMountDrive, fileList);
}

std::string C4JStorage::GetMountedPath(std::string szMount)
{
    return InternalStorageManager.m_DLC.GetMountedPath(szMount);
}

C4JStorage::ETMSStatus C4JStorage::ReadTMSFile(int iQuadrant, eGlobalStorage eStorageFacility, C4JStorage::eTMS_FileType eFileType,
                                               WCHAR *pwchFilename, BYTE **ppBuffer, DWORD *pdwBufferSize,
                                               int (*Func)(LPVOID, WCHAR *, int, bool, int), LPVOID lpParam, int iAction)
{
    return ETMSStatus_Idle;
}

bool C4JStorage::WriteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility, WCHAR *pwchFilename, BYTE *pBuffer, DWORD dwBufferSize)
{
    return true;
}

bool C4JStorage::DeleteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility, WCHAR *pwchFilename)
{
    return true;
}

void C4JStorage::StoreTMSPathName(WCHAR *pwchName)
{
    ;
}

C4JStorage::ETMSStatus C4JStorage::TMSPP_ReadFile(int iPad, C4JStorage::eGlobalStorage eStorageFacility, C4JStorage::eTMS_FILETYPEVAL eFileTypeVal,
                                                  LPCSTR szFilename, int (*Func)(LPVOID, int, int, PTMSPP_FILEDATA, LPCSTR), LPVOID lpParam,
                                                  int iUserData)
{
    return ETMSStatus_Idle;
}

unsigned int C4JStorage::CRC(unsigned char *buf, int len)
{
    return 0;
}
