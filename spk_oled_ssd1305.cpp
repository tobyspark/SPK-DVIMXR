// *spark audio-visual
// OLED display using SSD1305 driver
// Copyright *spark audio-visual 2012

#include "spk_oled_ssd1305.h"
#include "spk_oled_gfx.h"
#include "mbed.h"

SPKDisplay::SPKDisplay(PinName mosiPin, PinName clkPin, PinName csPin, PinName dcPin, PinName resPin, Serial *debugSerial)
{
    spi = new SPI(mosiPin, NC, clkPin);
    spi->format(8,3);
    spi->frequency(2000000);
    
    cs = new DigitalOut(csPin);
    dc = new DigitalOut(dcPin);
    res = new DigitalOut(resPin);
    
    // Link up debug Serial object
    // Passing in shared object as debugging is shared between all DVI mixer functions
    debug = debugSerial;
    
    setup();
    
    clearBuffer();
    
    if (debug) debug->printf("SPKDisplay loaded\n\r");
}

void SPKDisplay::clearBuffer()
{
    memset(buffer, 0, bufferCount);
}

void SPKDisplay::imageToBuffer()
{
    memcpy(buffer, image, bufferCount);
}

void SPKDisplay::clearBufferRow(int row)
{
    // Range check
    if (row >= 8)
    {
        if (debug) debug->printf("SPKDisplay::clearBufferRow sent out of bounds row");
        return;
    }
    int bStart = row*bufferWidth;
    int bEnd = bStart + pixelWidth;

    for (int bPos = bStart; bPos <= bEnd; bPos++)
    {
        buffer[bPos] = 0x00;
    }
}

void SPKDisplay::horizLineToBuffer(int y)
{
    if (y >= pixelHeight)
    {
        if (debug) debug->printf("SPKDisplay::clearBufferRow sent out of bounds y");
        return;
    }
    
    int row = (y*pixInPage) / pixelHeight;
    int posInRow = y % pixInPage;
    
    int bStart = row*bufferWidth;
    int bEnd = bStart + pixelWidth;
    
    for (int bPos = bStart; bPos <= bEnd; bPos++)
    {
        // Need to bitwise OR as setting single bit (the line) in byte (the row)
        buffer[bPos] = buffer[bPos] | 0x01 << posInRow;
    } 
}

void SPKDisplay::textToBuffer(std::string message, int row)
{
    // Range check
    if (row >= 8) row = 7;
    int bStart = row*bufferWidth;
    int bEnd = bStart + pixelWidth;

    int bPos = bStart;
    for (int i = 0; i < message.size(); i++)
    {
        char character = message.at(i);
        
        // Is it outside the range we have glyphs for?
        if ((character < characterBytesStartChar) || (character > characterBytesEndChar))
        {
            // Treat as a space
            for (int j = 0; j < 5; j++)
            {
                if (bPos >= bEnd) break;
                buffer[bPos++] = 0x00;
            }
            
            // Warn if not
            if (debug)
            {
                if (character != ' ') debug->printf("No glyph for character %c at position %i", character, i);
            }
        }
        // If not, typeset it!
        else 
        {
            // Shift into our array's indexing
            character -= characterBytesStartChar;
            
            // Write each byte's vertical column of 8bits into the buffer.
            for (int j = 0; j < characterBytes[character][0]; j++)
            {
                if (bPos >= bEnd) break;
                buffer[bPos++] = characterBytes[character][j+1];
            }
            
            // Put 1px letter spacing at end
            if (bPos >= bEnd) break;
            buffer[bPos++] = 0x00; // 1 px letter spacing
        }
    }
}

void SPKDisplay::sendBuffer()
{
    // Select the device by seting chip select low
    *cs = 0;

    // Set to receive DATA not commands
    *dc = 1;

    for (int i = 0; i < bufferCount; i++)
    {
        spi->write(buffer[i]);
    }
    
    // Deselect the device
    *cs = 1;
}

void SPKDisplay::setup()
{
    // TASK: SCREEN OFF, Run pre-flight
    
    // Hard reset the OLED
    *res = 0;
    wait_ms(1);
    *res = 1;

    // Select the device by seting chip select low
    *cs = 0;
    
    // Set to receive COMMANDS not data
    *dc = 0;

    spi->write(0xAE); // set display off
    spi->write(0xD5); // set display clock divide ratio
    spi->write(0xA0);
    spi->write(0xA8); // set multiplex ratio
    spi->write(0x3F); 
    spi->write(0xD3); // set display offset
    spi->write(0x00);
    spi->write(0x40); // set display start line
    spi->write(0xAD); // set master configuration
    spi->write(0x8E);
    spi->write(0xD8); // set area color mode
    spi->write(0x05);
    spi->write(0xA1); // set segment re-map
    spi->write(0xC8); // set com output scan direction
    spi->write(0xDA); // set com pins hardware configuration
    spi->write(0x12);
    spi->write(0x91); // set look-up table
    spi->write(0x3F);
    spi->write(0x3F);
    spi->write(0x3F);
    spi->write(0x3F);
    spi->write(0x81); // set current control for bank 0
    spi->write(0x8F);
    spi->write(0xD9); // set pre-charge period
    spi->write(0xD2);
    spi->write(0xDB); //set vcomh deselect level
    spi->write(0x34);
    spi->write(0xA4); // set entire display on/off
    spi->write(0xA6); // set normal/inverse display
    
    spi->write(0x20); // page mode
    spi->write(0x00);
    
    // TASK: Clear screen's content buffer
    
    // Is this neccessary when switching command/data?
    *cs = 1;
    wait_ms(1);
    *cs = 0;
    
    // Set to receive DATA not commands
    *dc = 1;
        
    for (int i = 0; i < bufferCount; i++)
    {
        spi->write(0x00);
    }
    
    // TASK: SCREEN ON
    
    // Is this neccessary when switching command/data?
    *cs = 1;
    wait_ms(1);
    *cs = 0;
    
    // Set to receive COMMANDS not data
    *dc = 0;
    
    spi->write(0xAF); // set display on
  
    // Deselect the device
    *cs = 1;
}