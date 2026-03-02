#pragma once
#include "STO_DLC.h"
#include "STO_SaveGame.h"

class CStorage
{
public:
    CStorage(void);

    void Init(int (*Func)(LPVOID, const C4JStorage::ESavingMessage, int), LPVOID lpParam, LPCSTR szGroupID);
    void Tick(void);
    unsigned int CRC(unsigned char *buf, int len);
    void MakeCRCTable(void);
    unsigned int UpdateCRC(unsigned int crc, unsigned __int8 *buf, int len);
    void DebugPrintf(const char *szFormat, ...);

    BYTE gap0[8];
    CSaveGame m_SaveGame;
    CDLC m_DLC;
    BYTE gap278[0x10];
    DWORD m_CRCTable[256];
    bool m_bHasCRCTable;
};

// Singleton
extern CStorage InternalStorageManager;