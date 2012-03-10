// *spark audio-visual
// RS232 Control for TV-One products
// Good for 1T-C2-750, others will need some extra work
// Copyright *spark audio-visual 2009-2011

#ifndef SPKTVOne_mBed_h
#define SPKTVOne_mBed_h

#include "spk_tvone.h"
#include "mbed.h"

class SPKTVOne
{
  public:
    SPKTVOne(PinName txPin, PinName rxPin, PinName signWritePin = NC, PinName signErrorPin = NC, Serial *debugSerial = NULL);
    
    bool command(uint8_t channel, uint8_t window, int32_t func, int32_t payload);
    
    void setCustomResolutions();
    bool setHDCPOff();
     
  private:
    // Tx and Wait LED pins to go here
    void set1920x480(int resStoreNumber);
    void set1600x600(int resStoreNumber);
    
    Serial *serial;
    Serial *debug; 
    
    DigitalOut *writeDO;
    DigitalOut *errorDO;
    Timeout signErrorTimeout;
    void signErrorOff();
};

#endif
