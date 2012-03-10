// *SPARK D-FUSER
// A project by *spark audio-visual
//
// spkRotaryEncoder extends mRotaryEncoder to return the change on pot state since last queried
// This allows the encoder to be polled when the host program is ready, and return info suitable for driving a TVOne style menu
// Importantly to driving such a menu, it will ignore any further rotation after the switch is pressed.

#include "mRotaryEncoder.h"

class SPKRotaryEncoder : public mRotaryEncoder {

public:
    bool    hasPressed();
    int     getChange();
    int     getPos(); // This would be a Get() override, but its not virtual. We use this instead to correct for positions-per-detent
    SPKRotaryEncoder(PinName pinA, PinName pinB, PinName pinSW, PinMode pullMode=PullUp, int debounceTime_us=1000);

private:
    void    onPress();
    bool    m_hasPressed;
    int     m_positionOld;
    int     m_positionOnPress;

};

SPKRotaryEncoder::SPKRotaryEncoder(PinName pinA, PinName pinB, PinName pinSW, PinMode pullMode, int debounceTime_us) : mRotaryEncoder(pinA, pinB, pinSW, pullMode, debounceTime_us)
{
    attachSW(this,&SPKRotaryEncoder::onPress);
}

bool SPKRotaryEncoder::hasPressed()
{
    bool hasPressed = m_hasPressed;
    m_hasPressed = false;
    
    return hasPressed;
}

int SPKRotaryEncoder::getChange()
{
    int positionEnc = this->getPos();
        
    int positionToUse = m_hasPressed ? m_positionOnPress : positionEnc;
    int change = positionToUse - m_positionOld;
    
    m_positionOld = positionEnc;

    return change;
}

int SPKRotaryEncoder::getPos()
{
    int positionEnc = this->Get();
    int positionsPerDetent = 2;
    
    return positionEnc / positionsPerDetent;
}

void SPKRotaryEncoder::onPress()
{
    m_positionOnPress = this->getPos();
    m_hasPressed = true;
}