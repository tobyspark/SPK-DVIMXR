// *spark audio-visual
// OLED display using SSD1305 driver
// Copyright *spark audio-visual 2012

// TODO
// sendBufferIfNeeded() -- library caches whether buffer has been updated, and this method sends buffer if so

#ifndef SPK_OLED_SSD1305_h
#define SPK_OLED_SSD1305_h

#include "mbed.h"
#include <string> 

#define bufferCount 1056
#define bufferWidth 132
#define pixelWidth 128
#define pixelHeight 64
#define pixInPage 8
#define pageCount 8

class SPKDisplay
{
  public:
    SPKDisplay(PinName mosi, PinName clk, PinName cs, PinName dc, PinName res, Serial *debugSerial = NULL);

    void clearBuffer();
    void clearBufferRow(int row);
    void imageToBuffer();
    void horizLineToBuffer(int y);
    void textToBuffer(std::string message, int row);
    
    void sendBuffer();
    
  private:
    SPI *spi;
    DigitalOut *cs;
    DigitalOut *dc;
    DigitalOut *res;
    
    Serial *debug;
    uint8_t buffer[bufferCount];
    
    void setup(); 
};

#endif
