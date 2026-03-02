#include "INP_Keyboard.h"

void CKeyboard::Tick(void) {}

EKeyboardResult CKeyboard::RequestKeyboard(LPCWSTR Title, LPCWSTR Text, DWORD dwPad, UINT uiMaxChars, int (*Func)(LPVOID, const bool), LPVOID lpParam,
                                           C_4JInput::EKeyboardMode eMode)
{
    ClearGlobalText();
    SeedEditBox();
    Func(lpParam, C_4JInput::EKeyboardMode_Numeric);

    return EKeyboard_ResultAccept;
}

void CKeyboard::GetText(uint16_t *UTF16String)
{
    uint16_t *keyString = GetGlobalText();
    for (unsigned int index = 0; keyString[index] && index < 0x200; index++)
    {
        UTF16String[index] = keyString[index];
    }
}