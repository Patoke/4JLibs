#pragma once
#include "4J_Input.h"

void ClearGlobalText();
uint16_t *GetGlobalText();
void SeedEditBox();

class CKeyboard
{
public:
    enum EKeyboardProcessState
    {
    };

    struct SKeyboardInfo
    {
    };

    void Tick(void);

    EKeyboardResult RequestKeyboard(LPCWSTR Title, LPCWSTR Text, DWORD dwPad, UINT uiMaxChars, int (*Func)(LPVOID, const bool), LPVOID lpParam,
                                    C_4JInput::EKeyboardMode eMode);
    void GetText(uint16_t *UTF16String);

    BYTE gap0[0x68];
};