#pragma once
#include "4J_Storage.h"

class CDLC
{
public:
    struct DriveMapping
    {
        DriveMapping(std::string szDirectoryPath, std::string szMountPath) : m_szDirectoryPath(szDirectoryPath), m_szMountPath(szMountPath)
        {
            ;
        }

        std::string m_szDirectoryPath;
        std::string m_szMountPath;
    };

    XCONTENT_DATA &GetDLC(DWORD dw);
    CDLC(void);

    C4JStorage::EDLCStatus GetOffers(int iPad, int (*Func)(LPVOID, int, DWORD, int), LPVOID lpParam,
                                     DWORD dwOfferTypesBitmask = XMARKETPLACE_OFFERING_TYPE_CONTENT);
    void ClearOffers();
    C4JStorage::EDLCStatus GetInstalledDLC(int iPad, int (*Func)(LPVOID, int, int), LPVOID lpParam);
    DWORD MountInstalledDLC(int iPad, DWORD dwDLC, int (*Func)(LPVOID, int, DWORD, DWORD), LPVOID lpParam, LPCSTR szMountDrive = NULL);
    DWORD UnmountInstalledDLC(LPCSTR szMountDrive = NULL);
    void GetMountedDLCFileList(const char *szMountDrive, std::vector<std::string> &fileList);
    std::string GetMountedPath(std::string szMount);

    void SetDLCProductCode(const char *szProductCode);
    void SetProductUpgradeKey(const char *szProductCode);
    int GetAvailableDLCCount(int iPad);
    void SetPackageRoot(char *pszDLCRoot);

    void Tick(void);
    void AddInstalled(XCONTENT_DATA *data);
    DWORD CancelOffers(void);

    DWORD dword0;
    int (*m_pInstalledDLCFunc)(LPVOID, int, int);
    LPVOID m_pInstalledDLCParam;
    BYTE gap18[16];
    int m_iHasNewInstalledDLCs;
    std::vector<XCONTENT_DATA> m_vInstalledDLCs;
    BYTE gap48[4];
    DWORD m_iHasNewMountedDLCs;
    int (*m_pMountedDLCFunc)(LPVOID, int, DWORD, DWORD);
    LPVOID m_pMountedDLCParam;
    std::string m_szMountPath;
    DWORD m_uiCurrentMappedDLC;
    DWORD dword94;
    char m_szPackageRoot[40];
    DWORD dwordC0;
    std::vector<DriveMapping> m_vDLCDriveMappings;
    char m_szDLCProductCode[16];
    char m_szProductUpgradeKey[60];
};