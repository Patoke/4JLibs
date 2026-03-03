/*
MIT License

Copyright (c) 2026 Patoke

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "STO_SaveGame.h"

CSaveGame::CSaveGame()
{
    m_pSaveData = nullptr;
    m_uiSaveSize = 0;
    m_bIsSafeDisabled = false;

    ZeroMemory(m_szSaveUniqueName, sizeof(m_szSaveUniqueName));
    ZeroMemory(m_szSaveTitle, sizeof(m_szSaveTitle));

    m_pSaveDetails = nullptr;
    m_bHasSaveDetails = false;

    GetCurrentDirectoryA(sizeof(m_szSaveUniqueName), m_szSaveUniqueName);

    char dirName[256];
    char curDir[256];
    GetCurrentDirectoryA(sizeof(dirName), dirName);
    sprintf(curDir, "%s/Windows64/GameHDD/", dirName);
    CreateDirectoryA(curDir, 0);
}

void CSaveGame::SetSaveDisabled(bool bDisable)
{
    m_bIsSafeDisabled = bDisable;
}

bool CSaveGame::GetSaveDisabled(void)
{
    return m_bIsSafeDisabled;
}

void CSaveGame::ResetSaveData()
{
    free(m_pSaveData);
    m_pSaveData = nullptr;
    m_uiSaveSize = 0;
}

C4JStorage::ESaveGameState CSaveGame::GetSavesInfo(int iPad, int (*Func)(LPVOID lpParam, SAVE_DETAILS *pSaveDetails, const bool), LPVOID lpParam,
                                                   char *pszSavePackName)
{
    WIN32_FIND_DATAA findFileData;
    WIN32_FILE_ATTRIBUTE_DATA fileInfoBuffer;

    if (!m_pSaveDetails)
    {
        m_pSaveDetails = new SAVE_DETAILS();
        memset(m_pSaveDetails, 0, sizeof(SAVE_DETAILS));
    }

    delete[] m_pSaveDetails->SaveInfoA;
    m_pSaveDetails->SaveInfoA = nullptr;
    m_pSaveDetails->iSaveC = 0;

    char dirName[256];
    char curDir[256];
    GetCurrentDirectoryA(sizeof(dirName), dirName);
    sprintf(curDir, "%s\\Windows64\\GameHDD\\*", dirName);

    int resultCount = 0;
    HANDLE h = FindFirstFileExA(curDir, FindExInfoStandard, &findFileData, FindExSearchLimitToDirectories, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        printf("Error finding save dirs: 0x%08lx\n", error);
    }
    else
    {
        do
        {
            if ((findFileData.dwFileAttributes & 0x10) != 0)
            {
                resultCount++;
            }
        } while (FindNextFileA(h, &findFileData));
        FindClose(h);
    }

    if (resultCount > 0)
    {
        m_pSaveDetails->SaveInfoA = new SAVE_INFO[resultCount];
        memset(m_pSaveDetails->SaveInfoA, 0, 184LL * resultCount);

        m_pSaveDetails->iSaveC = 0;
        int i = 0;
        HANDLE fi = FindFirstFileExA(curDir, FindExInfoStandard, &findFileData, FindExSearchLimitToDirectories, 0, 0);
        if (fi != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 && strcmp(findFileData.cFileName, ".") &&
                    strcmp(findFileData.cFileName, ".."))
                {
                    strcpy_s(m_pSaveDetails->SaveInfoA[i].UTF8SaveFilename, findFileData.cFileName);
                    strcpy_s(m_pSaveDetails->SaveInfoA[i].UTF8SaveTitle, findFileData.cFileName);

                    char fileName[280];
                    sprintf(fileName, "%s\\Windows64\\GameHDD\\%s\\saveData.ms", dirName, findFileData.cFileName);

                    GetFileAttributesExA(fileName, GetFileExInfoStandard, &fileInfoBuffer);
                    m_pSaveDetails->SaveInfoA[i++].metaData.dataSize = fileInfoBuffer.nFileSizeLow;
                    m_pSaveDetails->iSaveC++;
                }
            } while (FindNextFileA(fi, &findFileData));
            FindClose(fi);
        }
    }

    m_bHasSaveDetails = true;
    if (Func)
    {
        Func(lpParam, m_pSaveDetails, true);
    }

    return C4JStorage::ESaveGame_Idle;
}

PSAVE_DETAILS CSaveGame::ReturnSavesInfo()
{
    if (m_bHasSaveDetails)
        return m_pSaveDetails;
    else
        return nullptr;
}

void CSaveGame::ClearSavesInfo()
{
    m_bHasSaveDetails = false;
    if (m_pSaveDetails)
    {
        if (m_pSaveDetails->SaveInfoA)
        {
            delete[] m_pSaveDetails->SaveInfoA;
            m_pSaveDetails->SaveInfoA = nullptr;
            m_pSaveDetails->iSaveC = 0;
        }
        delete m_pSaveDetails;
        m_pSaveDetails = 0;
    }
}

C4JStorage::ESaveGameState CSaveGame::LoadSaveDataThumbnail(PSAVE_INFO pSaveInfo,
                                                            int (*Func)(LPVOID lpParam, PBYTE pbThumbnail, DWORD dwThumbnailBytes), LPVOID lpParam)
{
    Func(lpParam, pSaveInfo->thumbnailData, pSaveInfo->metaData.thumbnailSize);
    return C4JStorage::ESaveGame_GetSaveThumbnail;
}

C4JStorage::ESaveGameState CSaveGame::LoadSaveData(PSAVE_INFO pSaveInfo, int (*Func)(LPVOID lpParam, const bool, const bool), LPVOID lpParam)
{
    SetSaveUniqueFilename(pSaveInfo->UTF8SaveFilename);

    if (m_pSaveData)
    {
        free(m_pSaveData);
    }

    m_pSaveData = malloc(pSaveInfo->metaData.dataSize);
    m_uiSaveSize = pSaveInfo->metaData.dataSize;

    char dirName[256];
    char curDir[256];
    char fileName[280];
    GetCurrentDirectoryA(sizeof(curDir), curDir);
    sprintf(dirName, "%s/Windows64/GameHDD/%s", curDir, m_szSaveUniqueName);
    CreateDirectoryA(dirName, 0);
    sprintf(fileName, "%s/saveData.ms", dirName);

    HANDLE h = CreateFileA(fileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    bool success = false;
    if (h != INVALID_HANDLE_VALUE)
    {
        DWORD bytesRead = 0;
        BOOL res = ReadFile(h, m_pSaveData, m_uiSaveSize, &bytesRead, 0);
        _ASSERT(res && bytesRead == m_uiSaveSize);
        CloseHandle(h);
        success = true;
    }

    if (Func)
    {
        Func(lpParam, 0, success);
    }

    return C4JStorage::ESaveGame_Idle;
}

unsigned int CSaveGame::GetSaveSize()
{
    return m_uiSaveSize;
}

void CSaveGame::GetSaveData(void *pvData, unsigned int *puiBytes)
{
    if (pvData)
    {
        memmove(pvData, m_pSaveData, m_uiSaveSize);
        *puiBytes = m_uiSaveSize;
    }
    else
    {
        *puiBytes = 0;
    }
}

bool CSaveGame::GetSaveUniqueNumber(INT *piVal)
{
    return false;
}

bool CSaveGame::GetSaveUniqueFilename(char *pszName)
{
    return false;
}

void CSaveGame::SetSaveTitle(LPCWSTR pwchDefaultSaveName)
{
    CreateSaveUniqueName();
    memmove(m_szSaveTitle, pwchDefaultSaveName, sizeof(m_szSaveTitle));
}

PVOID CSaveGame::AllocateSaveData(unsigned int uiBytes)
{
    free(m_pSaveData);

    m_pSaveData = malloc(uiBytes);
    if (m_pSaveData)
    {
        m_uiSaveSize = uiBytes;
    }

    return m_pSaveData;
}

void CSaveGame::SetSaveImages(PBYTE pbThumbnail, DWORD dwThumbnailBytes, PBYTE pbImage, DWORD dwImageBytes, PBYTE pbTextData, DWORD dwTextDataBytes)
{
    ;
}

C4JStorage::ESaveGameState CSaveGame::SaveSaveData(int (*Func)(LPVOID, const bool), LPVOID lpParam)
{
    char dirName[256];
    char curDir[256];
    char fileName[280];
    GetCurrentDirectoryA(sizeof(curDir), curDir);
    sprintf(dirName, "%s/Windows64/GameHDD/%s", curDir, m_szSaveUniqueName);
    CreateDirectoryA(dirName, 0);
    sprintf(fileName, "%s/saveData.ms", dirName);

    HANDLE h = CreateFileA(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    DWORD bytesWritten = 0;
    BOOL res = WriteFile(h, m_pSaveData, m_uiSaveSize, &bytesWritten, 0);
    _ASSERT(res && bytesWritten == m_uiSaveSize);

    CloseHandle(h);

    Func(lpParam, true);

    return C4JStorage::ESaveGame_Idle;
}

C4JStorage::ESaveGameState CSaveGame::DeleteSaveData(PSAVE_INFO pSaveInfo, int (*Func)(LPVOID lpParam, const bool), LPVOID lpParam)
{
    return C4JStorage::ESaveGame_Idle;
}

void CSaveGame::SetSaveUniqueFilename(char *szFilename)
{
    strcpy_s(m_szSaveUniqueName, szFilename);
}

void CSaveGame::CreateSaveUniqueName(void)
{
    _SYSTEMTIME UTCSysTime;
    GetSystemTime(&UTCSysTime);

    sprintf_s(m_szSaveUniqueName, sizeof(m_szSaveUniqueName), "%4d%02d%02d%02d%02d%02d", UTCSysTime.wYear, UTCSysTime.wMonth, UTCSysTime.wDay,
              UTCSysTime.wHour, UTCSysTime.wMinute, UTCSysTime.wSecond);
}
