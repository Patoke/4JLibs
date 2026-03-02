#pragma once
#include "LinkedList.h"

#define MAX_EFFECTS 10

class CForceFeedback
{
public:
    enum FF_PARAMS
    {
        FF_PARAMS_RAMP,
        FF_PARAMS_PERIODIC
    };

    struct RUMBLE_EFFECT
    {
        DWORD m_Pad;
        uint64_t m_TimeLeft;
        XINPUT_VIBRATION m_RumbleData;
        WORD unk;
    };

    struct FF_EFFECT
    {
        unsigned int m_uiEffectType;
        unsigned int m_uiDurationMs;
        BYTE gap8[4];
        unsigned int m_uiMagnitude;
        BYTE gap10[8];
        DWORD dword18;
        DWORD dword1C;
    };

    struct FF_EFFECT_INSTANCE
    {
        bool m_bIsActive;
        unsigned int m_uiInst;
        CForceFeedback::FF_EFFECT *m_Effect;
    };

    void Initialise(int iInputStateC, unsigned char ucMapC, unsigned char ucActionC, unsigned char ucMenuActionC);
    void RumbleEnable(int iQuadrant, bool bRumbleEnabled);
    void CreateEffect(void);
    void AddEffectInstance(unsigned int uiEffect);
    void SetEffectInstanceState(unsigned int uiEffectInstance, unsigned int uiState);

    void ProcessEventInstance(FF_EFFECT_INSTANCE *pEffectInst);
    void PauseEventInstance(FF_EFFECT_INSTANCE *pEffectInst);

    void AddRumble(unsigned int uiPad, WORD leftMotorSpeed, WORD rightMotorSpeed, float fSeconds);

    void Tick(void);

    LinkedList m_EffectList;
    FF_EFFECT *m_Effects;
    int m_EffectC;
    LinkedList m_RumbleList;
    float m_TicksPerSecond;
    bool m_unkBool;
};