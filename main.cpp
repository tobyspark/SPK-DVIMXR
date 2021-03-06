/* *SPARK D-FUSER
 * A project by Toby Harris
 *
 * 'DJ' controller style RS232 Control for TV-One products
 * Good for 1T-C2-750, others will need some extra work
 *
 * www.tobyz.net/projects/dvi-mixer
 */

/* Copyright (c) 2011 Toby Harris, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
 * and associated documentation files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, 
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
/* ROADMAP / HISTORY
 * v10 - Port to mBed, keying redux - Apr'11
 * v11 - Sign callbacks, code clean-up - Apr'11
 * v12 - TVOne header split into two: defines and mbed class. v002 header updates pulled down. Removed sign callbacks, rewrite of debug and signing. - Apr'11
 * v13 - Menu system for Resolution + Keying implemented, it writing to debug, it sending TVOne commands - Apr'11
 * v14 - Fixes for new PCB - Oct'11
 * v15 - TBZ PCB, OLED - Mar'12
 * v16 - Comms menu, OSC, ArtNet - April'12
 * v17 - RJ45 - May'12
 * v18 - DMX - July'12
 * v19 - TVOne mixing comms further optimised - August'12
 * v20 - Keying values and resolutions load from USB mass storage - September'12
 * v21 - Mixing behaviour upgrade: blend-additive as continuum, test cards on startup if no valid source - October'12
 * v22 - EDID passthrough override and EDID upload from USB mass storage
 * v23 - Set keying values from controller
 * v24 - Conform uploads SIS image; now once firmware is loaded controller is all that is required
 * v25 - UX work
 * v26 - Tweaks: Network in works with hands-on controls, EDID Change message, Fit/Fill
 * v27 - Rework Keying UX, having current key saved in processor and loading in presets.
 * v28 - Network tweaks. Reads OSC and ArtNet network info from .ini
 * v29 - Keying: Can change between Left over Right and Right over Left
 * v30 - Matrox Dual/Triplehead - support for analogue edition timings (currently only analog DH2Go XGA)
 * vxx - TODO: Writes back to .ini on USB mass storage: keyer updates, comms, hdcp, edid internal/passthrough, ...?
 * vxx - TODO: EDID creation from resolution
 */
 
#include "mbed.h"

#include "spk_tvone_mbed.h"
#include "spk_utils.h"
#include "spk_mRotaryEncoder.h"
#include "spk_oled_ssd1305.h"
#include "spk_oled_gfx.h"
#include "spk_settings.h"
#include "EthernetNetIf.h"
#include "mbedOSC.h"
#include "DmxArtNet.h"
#include "DMX.h"
#include "filter.h"

#define kSPKDFSoftwareVersion "30"

// MBED PINS

#define kMBED_AIN_XFADE     p20
#define kMBED_AIN_FADEUP    p19
#define kMBED_DIN_TAP_L     p24
#define kMBED_DIN_TAP_R     p23
#define kMBED_ENC_SW        p15
#define kMBED_ENC_A         p16
#define kMBED_ENC_B         p17

#define kMBED_RS232_TTLTX   p13
#define kMBED_RS232_TTLRX   p14

#define kMBED_OLED_MOSI     p5
#define kMBED_OLED_SCK      p7
#define kMBED_OLED_CS       p8
#define kMBED_OLED_RES      p9
#define kMBED_OLED_DC       p10

#define kMBED_DIN_ETHLO_DMXHI       p30
#define kMBED_DOUT_RS485_TXHI_RXLO  p29
#define kMBED_RS485_TTLTX           p28
#define kMBED_RS485_TTLRX           p27

// DISPLAY

#define kMenuLine1 3
#define kMenuLine2 4
#define kCommsStatusLine 6
#define kTVOneStatusLine 7
#define kTVOneStatusMessageHoldTime 5

// 8.3 format filename only, no subdirs
#define kSPKDFSettingsFilename "SPKDF.ini"

#define kStringBufferLength 30

//// DEBUG

// Comment out one or the other...
//Serial *debug = new Serial(USBTX, USBRX); // For debugging via USB serial
Serial *debug = NULL; // For release (no debugging)

//// SOFT RESET

extern "C" void mbed_reset();

//// mBED PIN ASSIGNMENTS

// Inputs
AnalogIn xFadeAIN(kMBED_AIN_XFADE);    
AnalogIn fadeUpAIN(kMBED_AIN_FADEUP);
DigitalIn tapLeftDIN(kMBED_DIN_TAP_L);
DigitalIn tapRightDIN(kMBED_DIN_TAP_R);
medianFilter xFadeFilter(9);
medianFilter fadeUpFilter(9);

SPKRotaryEncoder menuEnc(kMBED_ENC_A, kMBED_ENC_B, kMBED_ENC_SW);

DigitalIn rj45ModeDIN(kMBED_DIN_ETHLO_DMXHI);

// Outputs
PwmOut fadeAPO(LED1);
PwmOut fadeBPO(LED2);

DigitalOut dmxDirectionDOUT(kMBED_DOUT_RS485_TXHI_RXLO);

// SPKTVOne(PinName txPin, PinName rxPin, PinName signWritePin, PinName signErrorPin, Serial *debugSerial)
SPKTVOne tvOne(kMBED_RS232_TTLTX, kMBED_RS232_TTLRX, LED3, LED4, debug);

// SPKDisplay(PinName mosi, PinName clk, PinName cs, PinName dc, PinName res, Serial *debugSerial = NULL);
SPKDisplay screen(kMBED_OLED_MOSI, kMBED_OLED_SCK, kMBED_OLED_CS, kMBED_OLED_DC, kMBED_OLED_RES, debug);
SPKMessageHold tvOneStatusMessage;

// Saved Settings
SPKSettings settings;

// Menu 
SPKMenu *selectedMenu;
SPKMenu mainMenu;
SPKMenu resolutionMenu;

SPKMenu mixModeMenu;
SPKMenu mixModeAdditiveMenu;
SPKMenu mixModeUpdateKeyMenu; 
enum { mixBlend, mixAdditive, mixKeyLeft, mixKeyRight };
int mixKeyPresetStartIndex = 100; // need this hard coded as mixBlend and mixAdditive are now combined into the same menu item
int mixKeyWindow = kTV1WindowIDA; // The window we've last used to key
int mixMode = mixBlend; // Start with safe mix mode, and test to get out of it. Safe mode will work with inputs missing and without hold frames.
int mixModeOld = mixMode;
float fadeCurve = 0.0f; // 0 = "X", ie. as per blend, 1 = "/\", ie. as per additive  <-- pictograms!

SPKMenu commsMenu;
enum { commsNone, commsOSC, commsArtNet, commsDMXIn, commsDMXOut};
int commsMode = commsNone;

SPKMenu troubleshootingMenu;
SPKMenu troubleshootingMenuHDCP;
SPKMenu troubleshootingMenuEDID;
SPKMenu troubleshootingMenuAspect;
SPKMenu troubleshootingMenuMatrox;
SPKMenu troubleshootingMenuReset;

SPKMenu advancedMenu;
enum { advancedConformUploadProcessor, advancedSetResolutions };

// RJ45 Comms
enum { rj45Ethernet = 0, rj45DMX = 1}; // These values from circuit
int rj45Mode = -1;
EthernetNetIf *ethernet = NULL;
OSCClass *osc = NULL;
OSCMessage sendMessage;
OSCMessage receiveMessage;
DmxArtNet *artNet = NULL;
DMX *dmx = NULL;

// Fade logic constants
const float xFadeTolerance = 0.05;
const float fadeUpTolerance = 0.05;

// A&B Fade as resolved percent
int fadeAPercent = 0;
int fadeBPercent = 0;
int oldFadeAPercent = 0;
int oldFadeBPercent = 0;

// Tap button states
bool tapLeftWasFirstPressed = false;

// Comms In fade state
float commsXFade = -1;
float commsFadeUp = -1;
float oldXFade = 0;
float oldFadeUp = 0;
bool  commsInActive = false;

// TVOne input sources stable flag
bool tvOneRGB1Stable = false;
bool tvOneRGB2Stable = false;

// TVOne behaviour flags
bool tvOneHDCPOn = false;
bool tvOneEDIDPassthrough = false;
const int32_t EDIDPassthroughSlot = 7;

bool processOSCIn() 
{
    bool updateFade = false;

    if (osc->newMessage) 
    {
        osc->newMessage = false; // fixme!
        
        string statusMessage;
        
        if (!strcmp( receiveMessage.getTopAddress() , "dvimxr" )) 
        {
            statusMessage = "OSC: /dvimxr";
            if (!strcmp( receiveMessage.getSubAddress() , "xFade" )) 
            {
                if (receiveMessage.getArgNum() == 1)
                    if (receiveMessage.getTypeTag(0) == 'f')
                    {
                        commsXFade = receiveMessage.getArgFloat(0);
                        updateFade = true;
                        
                        char buffer[15];
                        snprintf(buffer, 15, "/xFade %1.2f", commsXFade);
                        statusMessage += buffer;
                    }
            }
            else if (!strcmp( receiveMessage.getSubAddress() , "fadeUp" ))
            {
                if (receiveMessage.getArgNum() == 1)
                    if (receiveMessage.getTypeTag(0) == 'f')
                    {
                        commsFadeUp = receiveMessage.getArgFloat(0);
                        updateFade = true;
                        
                        char buffer[15];
                        snprintf(buffer, 15, "/fadeUp %1.2f", commsFadeUp);
                        statusMessage += buffer;
                    }
            }
            else if (!strcmp( receiveMessage.getSubAddress() , "xFadeFadeUp" ))
            {
                if (receiveMessage.getArgNum() == 2)
                    if (receiveMessage.getTypeTag(0) == 'f' && receiveMessage.getTypeTag(1) == 'f')
                    {
                        commsXFade  = receiveMessage.getArgFloat(0);
                        commsFadeUp = receiveMessage.getArgFloat(1);
                        updateFade = true;
                        
                        char buffer[15];
                        snprintf(buffer, 15, "/... %1.2f %1.2f", commsXFade, commsFadeUp);
                        statusMessage += buffer;
                    }
            }
            else 
            {
                statusMessage += receiveMessage.getSubAddress();
                statusMessage += " - Ignoring";
            }
        }
        else
        {
            statusMessage = "OSC: ";
            statusMessage += receiveMessage.getTopAddress();
            statusMessage += " - Ignoring";
        }
        
        screen.clearBufferRow(kCommsStatusLine);
        screen.textToBuffer(statusMessage, kCommsStatusLine);
    
        if (debug) debug->printf("%s \r\n", statusMessage.c_str());
    }
    
    return updateFade;
}

void processOSCOut(const float &xFade, const float &fadeUp) 
{
    sendMessage.setAddress("dvimxr", "xFadeFadeUp");
    sendMessage.setArgs("ff", &xFade, &fadeUp);
    osc->sendOsc(&sendMessage);
    
    char statusMessageBuffer[kStringBufferLength];
    snprintf(statusMessageBuffer, kStringBufferLength, "OSC Out: xF %.2f fUp %.2f", xFade, fadeUp);
    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);

    if (debug) debug->printf(statusMessageBuffer);
}

bool processArtNetIn() 
{
    if (artNet->Work()) 
    {
        int xFadeDMX = artNet->DmxIn[settings.artNet.universe][settings.dmx.inChannelXFade];
        int fadeUpDMX = artNet->DmxIn[settings.artNet.universe][settings.dmx.inChannelFadeUp];
    
        commsXFade  = (float)xFadeDMX/255;
        commsFadeUp = (float)fadeUpDMX/255;
    
        char statusMessageBuffer[kStringBufferLength];
        snprintf(statusMessageBuffer, kStringBufferLength, "A'Net In: xF%3i fUp %3i", xFadeDMX, fadeUpDMX);
        screen.clearBufferRow(kCommsStatusLine);
        screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);
    
        if (debug) debug->printf("ArtNet activity");
        
        return true;
    }
    
    return false;
}

void processArtNetOut(const float &xFade, const float &fadeUp) 
{
    int xFadeDMX = xFade*255;
    int fadeUpDMX = fadeUp*255;
    
    // Create array for all 512 DMX channels in the universe
    char dmxData[512];
    
    // Don't set every other channel to 0, rather channel values we already have
    memcpy(dmxData, artNet->DmxIn[settings.artNet.universe], 512);
        
    // Set fade channels
    dmxData[settings.dmx.outChannelXFade] = xFadeDMX;
    dmxData[settings.dmx.outChannelFadeUp] = fadeUpDMX;
    
    // Send
    artNet->Send_ArtDmx(settings.artNet.universe, 0, dmxData, 512);

    char statusMessageBuffer[kStringBufferLength];
    snprintf(statusMessageBuffer, kStringBufferLength, "A'Net Out: xF%3i fUp %3i", xFadeDMX, fadeUpDMX);
    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);

    if (debug) debug->printf(statusMessageBuffer);
}

bool processDMXIn() 
{
    int xFadeDMX = dmx->get(settings.dmx.inChannelXFade);
    int fadeUpDMX = dmx->get(settings.dmx.inChannelFadeUp);

    if (((float)xFadeDMX/255 != commsXFade) && ((float)fadeUpDMX/255 != commsFadeUp))
    {
        commsXFade = (float)xFadeDMX/255;
        commsFadeUp = (float)fadeUpDMX/255;
    
        char statusMessageBuffer[kStringBufferLength];
        snprintf(statusMessageBuffer, kStringBufferLength, "DMX In: xF %3i fUp %3i", xFadeDMX, fadeUpDMX);
        screen.clearBufferRow(kCommsStatusLine);
        screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);
    
        if (debug) debug->printf(statusMessageBuffer);
        
        return true;
    }
    
    return false;
}

void processDMXOut(const float &xFade, const float &fadeUp) 
{
    int xFadeDMX = xFade*255;
    int fadeUpDMX = fadeUp*255;
    
    dmx->put(settings.dmx.outChannelXFade, xFadeDMX);
    dmx->put(settings.dmx.outChannelFadeUp, fadeUpDMX);
    
    char statusMessageBuffer[kStringBufferLength];
    snprintf(statusMessageBuffer, kStringBufferLength, "DMX Out: xF %3i fUp %3i", xFadeDMX, fadeUpDMX);
    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);

    if (debug) debug->printf(statusMessageBuffer);
}

inline float fadeCalc (const float AIN, const float tolerance) 
{
    float pos ;
    if (AIN < tolerance) pos = 0;
    else if (AIN > 1.0 - tolerance) pos = 1;
    else pos = (AIN - tolerance) / (1 - 2*tolerance);
    if (debug && false) debug->printf("fadeCalc in: %f out: %f \r\n", AIN, pos);
    return pos;
}

bool handleTVOneSources()
{
    static int notOKCounter = 0;
    
    bool ok = true;

    int32_t payload = 0;
    
    ok = ok && tvOne.readCommand(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceSourceStable, payload);
    bool RGB1 = (payload == 1);    
    
    ok = ok && tvOne.readCommand(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceSourceStable, payload);
    bool RGB2 = (payload == 1);
   
    ok = ok && tvOne.readCommand(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsWindowSource, payload);
    int sourceA = payload;
   
    ok = ok && tvOne.readCommand(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsWindowSource, payload);
    int sourceB = payload;
   
    if (debug) debug->printf("HandleTVOneSources: RGB1: %i, RGB2: %i, sourceA: %#x, sourceB: %#x \r\n", RGB1, RGB2, sourceA, sourceB);
    
    string tvOneDetectString = "TVOne: ";

    if (ok)
    {
        string right = RGB1 ? "Live" : (tvOneRGB1Stable ? "Hold" : "Logo");
        string left  = RGB2 ? "Live" : (tvOneRGB2Stable ? "Hold" : "Logo");
        
        tvOneDetectString += "L: ";
        tvOneDetectString += left;
        tvOneDetectString += " R: ";
        tvOneDetectString += right;
    }
        
    tvOneStatusMessage.addMessage(tvOneDetectString);
    
    // Assign appropriate source depending on whether DVI input is good
    // If that assign command completes ok, and the DVI input is good, finally flag the unit has had a live source
    // Note any further losses on this input will be handled by the unit holding the last frame, so we don't need to switch back to SIS.
    if (ok && !tvOneRGB1Stable)
    {
        if (RGB1 && (sourceB != kTV1SourceRGB1)) ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceRGB1);
        if (!RGB1 && (sourceB != kTV1SourceSIS2)) ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceSIS2); // Wierd: Can't set to SIS1 sometimes.
        if (ok && RGB1) tvOneRGB1Stable = true;
    }
    if (ok && !tvOneRGB2Stable)
    {
        if (RGB2 && (sourceA != kTV1SourceRGB2)) ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceRGB2);
        if (!RGB2 && (sourceA != kTV1SourceSIS2)) ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceSIS2);
        if (ok && RGB2) tvOneRGB2Stable = true;
    } 
    
    // It seems there is an occasional RS232 choke around power-on of the processor.
    // Hard to reproduce, doubly so when in debug mode, this may fix. 
    if (ok)
    {
        notOKCounter = 0;
        tvOne.resetCommandPeriods();
    }
    else 
    {
        notOKCounter++;
        if (notOKCounter == 5)
        {
            tvOne.increaseCommandPeriods(200);
        }
        if (notOKCounter == 6)
        {
            tvOne.resetCommandPeriods();
        }
        if (notOKCounter % 15 == 0)
        {
            tvOneStatusMessage.addMessage("TVOne: Resetting link", 2.0f);
            screen.textToBuffer(tvOneStatusMessage.message(), kTVOneStatusLine);
            screen.sendBuffer();
            
            tvOne.increaseCommandPeriods(1500);
        }
        if (notOKCounter % 15 == 1)
        {
            tvOne.resetCommandPeriods();
        }
    }
    
    return ok;
}   

void actionMixMode(bool reset = false)
{
    if (debug) debug->printf("Changing mix mode \r\n");

    bool ok = true;
    string sentOK;
    char* sentMSGBuffer = "Blend";

    // Perform unset before set, in case mixMode = mixModeOld

    if (mixModeOld == mixAdditive || reset)
    {
        // Turn off Additive Mixing on output
        if (tvOne.getProcessorType().version == 423)
        {
            ok = ok && tvOne.command(0, kTV1WindowIDA, 0x298, 0);
        }
    }
    if (mixMode == mixAdditive) 
    {   
        sentMSGBuffer = "Additive";
    
        // First set B to what you'd expect for additive; it may be left at 100 if optimised blend mixing was previous mixmode.
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeBPercent);
        // Then turn on Additive Mixing
        if (tvOne.getProcessorType().version == 423)
        {
            ok = ok && tvOne.command(0, kTV1WindowIDA, 0x298, 1);
        }
    }

    if (mixModeOld == mixKeyLeft || reset)
    {
        // Turn off Keyer
        tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, false);
    }
    
    if (mixMode == mixKeyLeft)
    {
        sentMSGBuffer = "Key L over R";
        mixKeyWindow = kTV1WindowIDA;
        
        // Turn on Keyer
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, true);
    }

    if (mixModeOld == mixKeyRight || reset)
    {
        // Restore window positions
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsLayerPriority, 0);
        
        // Turn off Keyer
        tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustKeyerEnable, false); // Checkme: if check for success, returns failure errantly?
    }    
    if (mixMode == mixKeyRight)
    {
        sentMSGBuffer = "Key R over L";
        mixKeyWindow = kTV1WindowIDB;
        
        // Turn on Keyer
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustKeyerEnable, true);
        
        
        // Set window B above window A
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsLayerPriority, 0);
    }
    
    if (ok) 
    {
        mixModeOld = mixMode;
        sentOK = "Sent: ";
    }
    else 
    {
        mixMode = mixModeOld;
        sentOK = "Send Error: ";
    }
    
    tvOneStatusMessage.addMessage(sentOK + sentMSGBuffer, kTVOneStatusMessageHoldTime);
}

bool checkTVOneMixStatus()
{
    bool ok = true;
    
    int32_t payload;
    
    // Mix Mode
    bool mixModeNeedsAction = false;
    bool additiveOn = false, keyLeftOn = false, keyRightOn = false;
    int  windowAPriority = -1;
    
    if (mixMode == mixBlend)    { additiveOn = false; keyLeftOn = false; keyRightOn = false; windowAPriority = 0;}
    if (mixMode == mixAdditive) { additiveOn = true;  keyLeftOn = false; keyRightOn = false; windowAPriority = 0;}
    if (mixMode == mixKeyLeft)  { additiveOn = false; keyLeftOn = true;  keyRightOn = false; windowAPriority = 0;}
    if (mixMode == mixKeyRight) { additiveOn = false; keyLeftOn = false; keyRightOn = true;  windowAPriority = 1;}
    
    if (tvOne.getProcessorType().version == 423)
    {
        payload = -1;
        ok = ok && tvOne.readCommand(0, kTV1WindowIDA, 0x298, payload);
        if (payload != additiveOn) mixModeNeedsAction = true;
    }
    
    payload = -1;
    ok = ok && tvOne.readCommand(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, payload);
    if (payload != keyLeftOn) mixModeNeedsAction = true;

    payload = -1;
    ok = ok && tvOne.readCommand(0, kTV1WindowIDB, kTV1FunctionAdjustKeyerEnable, payload);
    if (payload != keyRightOn) mixModeNeedsAction = true;
    
    payload = -1;
    ok = ok && tvOne.readCommand(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsLayerPriority, payload);
    if (payload != windowAPriority) mixModeNeedsAction = true;

    if (ok && mixModeNeedsAction) 
    {
        if (debug) debug->printf("Check TVOne Mix Status requiring mixMode action. mixMode: %i \r\n", mixMode);
        actionMixMode(true);
    }
    
    // Check Fade
    payload = -1;
    ok = ok && tvOne.readCommand(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, payload);
    if (ok && (payload != fadeAPercent))
    {
        if (debug) debug->printf("Check TVOne Mix Status requiring fadeA action");
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeAPercent);
    }
    
    payload = -1;
    ok = ok && tvOne.readCommand(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, payload);
    if (ok && (payload != fadeBPercent))
    {
        if (debug) debug->printf("Check TVOne Mix Status requiring fadeB action");
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeBPercent);
    }
    
    return ok;
}

bool conformProcessor()
{
    bool ok;
                    
    int32_t on = 1;
    int32_t off = 0;
    
    for (int i=0; i < 3; i++)
    {
        // Independent output
        ok =       tvOne.command(0, kTV1WindowIDA, kTV1FunctionMode, 2);
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsOutputEnable, on);
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsLockMethod, off);
                        
        // Make sure our windows exist
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsEnable, on);
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsEnable, on);
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsLayerPriority, 0);
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsLayerPriority, 1);
        
        // Turn off borders on those windows                
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustBorderEnable, off);
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustBorderEnable, off);
            
        // Assign inputs to windows, so that left on the crossfader is left on the processor viewed from front
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceRGB2);
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceRGB1);
        
        // Set scaling to fit source within output, maintaining aspect ratio
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsZoomLevel, 100);
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsZoomLevel, 100);
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsShrinkEnable, off);
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsShrinkEnable, off);
        ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceAspectCorrect, SPKTVOne::aspectFit);
        ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceAspectCorrect, SPKTVOne::aspectFit);
        ok = ok && tvOne.command(kTV1SourceSIS1, kTV1WindowIDA, kTV1FunctionAdjustSourceTestCard, 1);
        ok = ok && tvOne.command(kTV1SourceSIS1, kTV1WindowIDA, kTV1FunctionAdjustSourceAspectCorrect, SPKTVOne::aspect1to1);
        ok = ok && tvOne.command(kTV1SourceSIS2, kTV1WindowIDA, kTV1FunctionAdjustSourceTestCard, 1);
        ok = ok && tvOne.command(kTV1SourceSIS2, kTV1WindowIDA, kTV1FunctionAdjustSourceAspectCorrect, SPKTVOne::aspect1to1);
        
        // On source loss, hold on the last frame received.
        int32_t freeze = 1;
        ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceOnSourceLoss, freeze);
        ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceOnSourceLoss, freeze);
        
        // Set resolution and fade levels for maximum chance of being seen
        ok = ok && tvOne.setResolution(kTV1ResolutionVGA, 5);
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, 50);
        ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, 100);
        
        // Set evil, evil HDCP off
        ok = ok && tvOne.setHDCPOn(false);
    
        if (ok) break;
        else tvOne.increaseCommandPeriods(500);
    }
    
    if (ok)
    {
        // Save current state in preset one
        tvOne.command(0, kTV1WindowIDA, kTV1FunctionPreset, 1);          // Set Preset 1
        tvOne.command(0, kTV1WindowIDA, kTV1FunctionPresetStore, 1);     // Store
        
        // Save current state for power on
        tvOne.command(0, kTV1WindowIDA, kTV1FunctionPowerOnPresetStore, 1);
    }
    
    tvOne.resetCommandPeriods();
    
    return ok;
}

bool uploadToProcessor()
{
    bool ok = true;

    LocalFileSystem local("local");
    FILE *file;
    
    // Upload EDIDs
    if (tvOne.getProcessorType().version < 415)
    {
        if (debug) debug->printf("Skipping EDID upload as unsupported on detected TV One firmware\r\n");
    }
    else
    {
        // Upload Matrox EDID to mem4 (ie. index 3). Use this EDID slot when setting Matrox resolutions.
        file = fopen("/local/matroxe.did", "r"); // 8.3, avoid .bin as mbed executable extension
        if (file)
        {
            ok = ok && tvOne.uploadEDID(file, 3);   
            fclose(file);
        }
        else
        {
            if (debug) debug->printf("Could not open Matrox EDID file 'matroxe.did'\r\n");
        }
        
        // Upload Datapath X4 EDID to mem3 (ie. index 2). Use this EDID slot when setting X4 resolutions.
        file = fopen("/local/x4e.did", "r"); // 8.3, avoid .bin as mbed executable extension
        if (file)
        {
            ok = ok && tvOne.uploadEDID(file, 2);   
            fclose(file);
        }
        else
        {
            if (debug) debug->printf("Could not open X4 EDID file 'x4e.did'\r\n");
        }
    }

    // Upload Logo to SIS2. Use this (minimal) image when no sources are connected.
    {
        file = fopen("/local/spark.dat", "r"); // 8.3, avoid .bin as mbed executable extension
        if (file)
        {
            ok = ok && tvOne.uploadImage(file, 0);   
            fclose(file);
        }
        else
        {
            if (debug) debug->printf("Could not open image file 'spark.dat'");
        }
    }    
    
    return ok;
}

void setResolutionMenuItems()
{
    resolutionMenu.clearMenuItems();
    for (int i=0; i < settings.resolutionsCount(); i++)
    {
        resolutionMenu.addMenuItem(SPKMenuItem(settings.resolutionName(i), settings.resolutionIndex(i), settings.resolutionEDIDIndex(i)));
    }
    resolutionMenu.addMenuItem(SPKMenuItem("Back to Main Menu", &mainMenu));
}

void setMixModeMenuItems()
{
    mixModeMenu.clearMenuItems();
    
    if (tvOne.getProcessorType().version == 423 || tvOne.getProcessorType().version == -1)
    {
        mixModeAdditiveMenu.title = "Crossfade";
        mixModeMenu.addMenuItem(SPKMenuItem(mixModeAdditiveMenu.title, &mixModeAdditiveMenu));
    }
    else
    {
        mixModeMenu.addMenuItem(SPKMenuItem("Blend", mixBlend));
    }
    
    mixModeMenu.addMenuItem(SPKMenuItem("Key: Left over Right", mixKeyLeft));
    mixModeMenu.addMenuItem(SPKMenuItem("Key: Right over Left", mixKeyRight));
    mixModeMenu.addMenuItem(SPKMenuItem("Key: Tweak key values", &mixModeUpdateKeyMenu));
    
    // Load in presets from settings. Index 0 is the "live" key read from TVOne, so we ignore here.
    if (settings.keyerSetCount() > 1)
        for (int i=1; i < settings.keyerSetCount(); i++)
        {
            mixModeMenu.addMenuItem(SPKMenuItem("Key Preset: " + settings.keyerParamName(i), mixKeyPresetStartIndex + i));
        }
    
    mixModeMenu.addMenuItem(SPKMenuItem("Back to Main Menu", &mainMenu));
}

void setCommsMenuItems()
{
    if (rj45Mode == rj45Ethernet) 
    {
        commsMenu.title = "Network Mode [Ethernet]";
        commsMenu.clearMenuItems();
        switch (commsMode) // Cannot switch between OSC and Artnet once one is selected (crash in EthernetIf deconstructor?), so this.
        {
            case commsNone:        
                commsMenu.addMenuItem(SPKMenuItem("None", commsNone));
                commsMenu.addMenuItem(SPKMenuItem("OSC", commsOSC));
                commsMenu.addMenuItem(SPKMenuItem("ArtNet", commsArtNet));
                break;
            case commsOSC:
                commsMenu.addMenuItem(SPKMenuItem("OSC", commsOSC));
                break;
            case commsArtNet:
                commsMenu.addMenuItem(SPKMenuItem("ArtNet", commsArtNet));
                break;
        }
        commsMenu.addMenuItem(SPKMenuItem("Back to Main Menu", &mainMenu));
        commsMenu = 0;
    }
    else if (rj45Mode == rj45DMX) 
    {
        commsMenu.title = "Network Mode [DMX]";
        commsMenu.clearMenuItems();
        commsMenu.addMenuItem(SPKMenuItem("None", commsNone));
        commsMenu.addMenuItem(SPKMenuItem("DMX In", commsDMXIn));
        commsMenu.addMenuItem(SPKMenuItem("DMX Out", commsDMXOut));
        commsMenu.addMenuItem(SPKMenuItem("Back to Main Menu", &mainMenu));
        commsMenu = 0;
    }
}

void mixModeAdditiveMenuHandler(int change, bool action)
{
    fadeCurve += change * 0.05f;
    if (fadeCurve > 1.0f) fadeCurve = 1.0f;
    if (fadeCurve < 0.0f) fadeCurve = 0.0f;
    
    mixMode = (fadeCurve > 0.001f) ? mixAdditive: mixBlend;

    screen.clearBufferRow(kMenuLine2);
    screen.textToBuffer("Blend [ ----- ] Add", kMenuLine2);
    screen.characterToBuffer('X', 38 + fadeCurve*20.0f, kMenuLine2);
    
    if (debug) debug->printf("Fade curve changed by %i to %f \r\n", change, fadeCurve);
    
    if (action)
    {
        selectedMenu = &mixModeMenu;
        
        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer(selectedMenu->title, kMenuLine1);
        screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    }
}

void troubleshootingMenuHDCPHandler(int change, bool action)
{
    static int currentHDCP;
    static int state = 0;

    if (change == 0 && !action)
    {
        // We check the control not the status, as status depends on connection etc.
        
        int32_t payloadOutput = -1;
        tvOne.readCommand(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsHDCPRequired, payloadOutput);
        
        int32_t payload1 = -1;
        tvOne.readCommand(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceHDCPAdvertize, payload1);
        
        int32_t payload2 = -1;
        tvOne.readCommand(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceHDCPAdvertize, payload2);
   
        if ((payloadOutput == payload1) && (payload1 == payload2) && (payload2 == 0)) 
        {
            currentHDCP = 0; // Change to on
        }
        else if ((payloadOutput == payload1) && (payload1 == payload2) && (payload2 == 1))
        {
            currentHDCP = 1; // Change to off
        }
        else
        {
            currentHDCP = -1; // Change to off
        }
        
        if (debug) debug->printf("HDCP detected O: %i 1: %i 2: %i", payloadOutput, payload1, payload2);
    }
    
    state += change;
    if (state > 1) state = 1;
    if (state < 0) state = 0;
    
    char paramLine[kStringBufferLength];
    screen.clearBufferRow(kMenuLine2);
    
    const char* current = currentHDCP == -1 ? "Mixed" : ( currentHDCP == 1 ? "On" : "Off");
    
    if (state == 0) snprintf(paramLine, kStringBufferLength, "%s. Set: [%s/      ]?", current, currentHDCP == 0 ? "On " : "Off" );
    else           snprintf(paramLine, kStringBufferLength, "%s. Set: [   /Cancel]?", current);
    screen.textToBuffer(paramLine, kMenuLine2);

    if (action)
    {
        if (state == 0)
        {
            screen.clearBufferRow(kTVOneStatusLine);
            screen.textToBuffer("Setting HDCP...", kTVOneStatusLine);
            screen.sendBuffer();
        
            // Do the action
            bool ok = tvOne.setHDCPOn(currentHDCP == 0);
            
            if (ok) tvOne.command(0, kTV1WindowIDA, kTV1FunctionPowerOnPresetStore, 1);
            
            std::string sendOK = ok ? "Sent: HDCP " : "Send Error: HDCP ";
            sendOK += currentHDCP == 0 ? "On" : "Off";
            
            tvOneStatusMessage.addMessage(sendOK, kTVOneStatusMessageHoldTime);
        }
        
        // Get back to menu
        selectedMenu = &troubleshootingMenu;
        
        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer(selectedMenu->title, kMenuLine1);
        screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    }
}

void troubleshootingMenuEDIDHandler(int change, bool action)
{
    static int currentEDIDPassthrough;
    static int currentEDID;
    static int state = 0;
    
    if (change == 0 && !action)
    {
        currentEDID = tvOne.getEDID();
        
        if (currentEDID == -1) currentEDIDPassthrough = -1;
        else currentEDIDPassthrough = (currentEDID == EDIDPassthroughSlot) ? 1 : 0;
    }
    
    state += change;
    if (state > 1) state = 1;
    if (state < 0) state = 0;
        
    char paramLine[kStringBufferLength];
    screen.clearBufferRow(kMenuLine2);
    
    const char* current = currentEDIDPassthrough == -1 ? "Mixed" : ( currentEDIDPassthrough == 1 ? "Thru" : "Internal");
    
    if (state == 0) snprintf(paramLine, kStringBufferLength, "%s. Set: [%s/      ]?", current, currentEDIDPassthrough == 0 ? "Thru" : "Int");
    else           snprintf(paramLine, kStringBufferLength, "%s. Set: [   /Cancel]?", current);      
    screen.textToBuffer(paramLine, kMenuLine2);
    
    if (action)
    {
        if (state == 0)
        {
            screen.clearBufferRow(kTVOneStatusLine);
            screen.textToBuffer("Setting EDID...", kTVOneStatusLine);
            screen.sendBuffer();
        
            // Do the action
            tvOneEDIDPassthrough = currentEDIDPassthrough == 0;
            
            bool ok = true;
            std::string message;
            
            int newEDID = tvOneEDIDPassthrough ? EDIDPassthroughSlot : resolutionMenu.selectedItem().payload.command[1];
            
            if (newEDID != currentEDID)
            {
                ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, newEDID);
                ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, newEDID);
                if (ok) message = "Sent: EDID";
                else    message = "Send Error: EDID";
            }
            else        message = "EDID already set";
        
            if (ok) tvOne.command(0, kTV1WindowIDA, kTV1FunctionPowerOnPresetStore, 1);
            
            tvOneStatusMessage.addMessage(message, kTVOneStatusMessageHoldTime);
            
            // This is WHACK. Can't believe it's both needed and officially in the 1T-C2-750 manual
            if ((newEDID != currentEDID) && ok) tvOneStatusMessage.addMessage("EDID: Processor Off+On?", kTVOneStatusMessageHoldTime);
        }
            
        // Get back to menu
        selectedMenu = &troubleshootingMenu;
        
        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer(selectedMenu->title, kMenuLine1);
        screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    }
}

void troubleshootingMenuAspectHandler(int change, bool action)
{
    static int state = 0;

    if (change == 0 && !action)
    {
        switch (tvOne.getAspect())
        {
            case SPKTVOne::aspectFit : state = 0; break;
            case SPKTVOne::aspectHFill : state = 1; break;
            case SPKTVOne::aspectVFill : state = 1; break;
            case SPKTVOne::aspectSPKFill : state = 1; break;
            case SPKTVOne::aspect1to1 : state = 2; break;
        }
    }
    
    state += change;
    if (state > 3) state = 3;
    if (state < 0) state = 0;
    
    screen.clearBufferRow(kMenuLine2);
    switch (state) 
    {
        case 0: screen.textToBuffer("Set: [Fit/    /   /      ]", kMenuLine2); break;
        case 1: screen.textToBuffer("Set: [   /Fill/   /      ]", kMenuLine2); break;
        case 2: screen.textToBuffer("Set: [   /    /1:1/      ]", kMenuLine2); break;
        case 3: screen.textToBuffer("Set: [   /    /   /Cancel]", kMenuLine2); break;
    }
      
    if (action)
    {
        if (state != 3)
        {
            screen.clearBufferRow(kTVOneStatusLine);
            screen.textToBuffer("Setting Aspect...", kTVOneStatusLine);
            screen.sendBuffer();
        
            // Do the action
            bool ok = false;
            switch (state) 
            {
                case 0: ok = tvOne.setAspect(SPKTVOne::aspectFit); break;
                case 1: ok = tvOne.setAspect(SPKTVOne::aspectSPKFill); break;
                case 2: ok = tvOne.setAspect(SPKTVOne::aspect1to1); break;
            }
            if (ok) tvOne.command(0, kTV1WindowIDA, kTV1FunctionPowerOnPresetStore, 1);
            
            std::string sendOK = ok ? "Sent: " : "Send Error: ";
            switch (state) 
            {
                case 0: sendOK += "Aspect Fit "; break;
                case 1: sendOK += "Aspect Fill"; break;
                case 2: sendOK += "Aspect 1:1"; break;
            }
            tvOneStatusMessage.addMessage(sendOK, kTVOneStatusMessageHoldTime);
        }
            
        // Get back to menu
        selectedMenu = &troubleshootingMenu;
        
        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer(selectedMenu->title, kMenuLine1);
        screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    }
}

void troubleshootingMenuMatroxHandler(int change, bool action)
{
    static int state = 0;

    if (change == 0 && !action)
    {
        // TODO: Find processor state. Test for a known timing value?
    }
    
    state += change;
    if (state > 2) state = 2;
    if (state < 0) state = 0;
    
    screen.clearBufferRow(kMenuLine2);
    switch (state) 
    {
        case 0: screen.textToBuffer("Set: [Digital/      /      ]", kMenuLine2); break;
        case 1: screen.textToBuffer("Set: [       /Analog/      ]", kMenuLine2); break;
        case 2: screen.textToBuffer("Set: [       /      /Cancel]", kMenuLine2); break;
    }
      
    if (action)
    {
        if (state != 2)
        {
            screen.clearBufferRow(kTVOneStatusLine);
            screen.textToBuffer("Configuring...", kTVOneStatusLine);
            screen.sendBuffer();
        
            // Do the action
            bool ok = false;
            switch (state) 
            {
                case 0: ok = tvOne.setMatroxResolutions(true); break;
                case 1: ok = tvOne.setMatroxResolutions(false); break;
            }
            
            std::string sendOK = ok ? "Sent: " : "Send Error: ";
            switch (state) 
            {
                case 0: sendOK += "Digital Timings"; break;
                case 1: sendOK += "Analogue Timings"; break;
            }
            tvOneStatusMessage.addMessage(sendOK, kTVOneStatusMessageHoldTime);
        }
            
        // Get back to menu
        selectedMenu = &troubleshootingMenu;
        
        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer(selectedMenu->title, kMenuLine1);
        screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    }
}

void mixModeUpdateKeyMenuHandler(int menuChange, bool action)
{
    static int actionCount = 0;
    static int state = 0;
    
    if (mixMode == mixKeyLeft)  mixKeyWindow = kTV1WindowIDA;
    if (mixMode == mixKeyRight) mixKeyWindow = kTV1WindowIDB;
    
    if (action) actionCount++;
    
    if (actionCount == 0) 
    {
        settings.editingKeyerSetIndex = 0;
        
        bool ok;
        int minY, maxY, minU, maxU, minV, maxV;
    
        ok =       tvOne.readCommand(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinY, minY); 
        ok = ok && tvOne.readCommand(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxY, maxY); 
        ok = ok && tvOne.readCommand(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinU, minU); 
        ok = ok && tvOne.readCommand(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxU, maxU); 
        ok = ok && tvOne.readCommand(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinV, minV); 
        ok = ok && tvOne.readCommand(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxV, maxV);
        
        if (ok)
        {
            settings.setEditingKeyerSetValue(SPKSettings::minY, minY);
            settings.setEditingKeyerSetValue(SPKSettings::maxY, maxY);
            settings.setEditingKeyerSetValue(SPKSettings::minU, minU);
            settings.setEditingKeyerSetValue(SPKSettings::maxU, maxU);
            settings.setEditingKeyerSetValue(SPKSettings::minV, minV);
            settings.setEditingKeyerSetValue(SPKSettings::maxV, maxV);
        }
        else
        {
        printf("failed to load\r\n");
            tvOneStatusMessage.addMessage("Failed to read key values", kTVOneStatusMessageHoldTime);
        }
        
        actionCount = 1;
    }
    if (actionCount == 1)
    {
        state += menuChange;

        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Tweak or start over?", kMenuLine1);
        
        char paramLine[kStringBufferLength];
        
        if (state % 2)  snprintf(paramLine, kStringBufferLength, "[%3i/%3i][%3i/%3i][%3i/%3i]", 0, 
                                                                                                255, 
                                                                                                0, 
                                                                                                255,
                                                                                                0, 
                                                                                                255);
        else            snprintf(paramLine, kStringBufferLength, "[%3i/%3i][%3i/%3i][%3i/%3i]", settings.editingKeyerSetValue(SPKSettings::minY), 
                                                                                                settings.editingKeyerSetValue(SPKSettings::maxY), 
                                                                                                settings.editingKeyerSetValue(SPKSettings::minU), 
                                                                                                settings.editingKeyerSetValue(SPKSettings::maxU),
                                                                                                settings.editingKeyerSetValue(SPKSettings::minV), 
                                                                                                settings.editingKeyerSetValue(SPKSettings::maxV));
        screen.textToBuffer(paramLine, kMenuLine2); 
    }
    if (actionCount == 2)
    {
        if (state % 2)
        {
            settings.setEditingKeyerSetValue(SPKSettings::minY,0);
            settings.setEditingKeyerSetValue(SPKSettings::maxY,255);
            settings.setEditingKeyerSetValue(SPKSettings::minU,0);
            settings.setEditingKeyerSetValue(SPKSettings::maxU,255);
            settings.setEditingKeyerSetValue(SPKSettings::minV,0);
            settings.setEditingKeyerSetValue(SPKSettings::maxV,255);
            tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinY, 0);
            tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxY, 255);
            tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinU, 0);
            tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxU, 255);
            tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinV, 0);
            tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxV, 255);
        }
        
        actionCount = 3;
    }
    if (actionCount == 3)
    {
        int value = settings.editingKeyerSetValue(SPKSettings::maxY);
        value += menuChange;
        if (value < 0) value = 0;
        if (value > 255) value = 255;
        settings.setEditingKeyerSetValue(SPKSettings::maxY, value);

        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Down until unmasked", kMenuLine1);
        
        char paramLine[kStringBufferLength];
        snprintf(paramLine, kStringBufferLength, "[   /%3i][   /   ][   /   ]", value);
        screen.textToBuffer(paramLine, kMenuLine2);
        
        tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxY, value);   
    }
    else if (actionCount == 4)
    {
        int value = settings.editingKeyerSetValue(SPKSettings::minY);
        value += menuChange;
        if (value < 0) value = 0;
        if (value > settings.editingKeyerSetValue(SPKSettings::maxY)) value = settings.editingKeyerSetValue(SPKSettings::maxY);
        settings.setEditingKeyerSetValue(SPKSettings::minY, value);

        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Up until unmasked", kMenuLine1);
        
        char paramLine[kStringBufferLength];
        snprintf(paramLine, kStringBufferLength, "[%3i/%3i][   /   ][   /   ]", value,
                                                                                settings.editingKeyerSetValue(SPKSettings::maxY));
        screen.textToBuffer(paramLine, kMenuLine2);
        
        tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinY, value); 
    }
    else if (actionCount == 5)
    {
        int value = settings.editingKeyerSetValue(SPKSettings::maxU);
        value += menuChange;
        if (value < 0) value = 0;
        if (value > 255) value = 255;
        settings.setEditingKeyerSetValue(SPKSettings::maxU, value);

        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Down until unmasked", kMenuLine1);
        
        char paramLine[kStringBufferLength];
        snprintf(paramLine, kStringBufferLength, "[%3i/%3i][   /%3i][   /   ]", settings.editingKeyerSetValue(SPKSettings::minY), 
                                                                                settings.editingKeyerSetValue(SPKSettings::maxY),  
                                                                                value);
        screen.textToBuffer(paramLine, kMenuLine2);
        
        tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxU, value); 
    }
    else if (actionCount == 6)
    {
        int value = settings.editingKeyerSetValue(SPKSettings::minU);
        value += menuChange;
        if (value < 0) value = 0;
        if (value > settings.editingKeyerSetValue(SPKSettings::maxU)) value = settings.editingKeyerSetValue(SPKSettings::maxU);
        settings.setEditingKeyerSetValue(SPKSettings::minU, value);

        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Up until unmasked", kMenuLine1);
        
        char paramLine[kStringBufferLength];
        snprintf(paramLine, kStringBufferLength, "[%3i/%3i][%3i/%3i][   /   ]", settings.editingKeyerSetValue(SPKSettings::minY), 
                                                                                settings.editingKeyerSetValue(SPKSettings::maxY), 
                                                                                value,
                                                                                settings.editingKeyerSetValue(SPKSettings::maxU));
        screen.textToBuffer(paramLine, kMenuLine2);
        
        tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinU, value);
    }
    else if (actionCount == 7)
    {
        int value = settings.editingKeyerSetValue(SPKSettings::maxV);
        value += menuChange;
        if (value < 0) value = 0;
        if (value > 255) value = 255;
        settings.setEditingKeyerSetValue(SPKSettings::maxV, value);

        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Down until unmasked", kMenuLine1);
        
        char paramLine[kStringBufferLength];
        snprintf(paramLine, kStringBufferLength, "[%3i/%3i][%3i/%3i][   /%3i]", settings.editingKeyerSetValue(SPKSettings::minY), 
                                                                                settings.editingKeyerSetValue(SPKSettings::maxY), 
                                                                                settings.editingKeyerSetValue(SPKSettings::minU), 
                                                                                settings.editingKeyerSetValue(SPKSettings::maxU),  
                                                                                value);
        screen.textToBuffer(paramLine, kMenuLine2);
        
        tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxV, value);    
    }
    else if (actionCount == 8)
    {
        int value = settings.editingKeyerSetValue(SPKSettings::minV);
        value += menuChange;
        if (value < 0) value = 0;
        if (value > settings.editingKeyerSetValue(SPKSettings::maxV)) value = settings.editingKeyerSetValue(SPKSettings::maxV);
        settings.setEditingKeyerSetValue(SPKSettings::minV, value);

        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Up until unmasked", kMenuLine1);
        
        char paramLine[kStringBufferLength];
        snprintf(paramLine, kStringBufferLength, "[%3i/%3i][%3i/%3i][%3i/%3i]", settings.editingKeyerSetValue(SPKSettings::minY), 
                                                                                settings.editingKeyerSetValue(SPKSettings::maxY), 
                                                                                settings.editingKeyerSetValue(SPKSettings::minU), 
                                                                                settings.editingKeyerSetValue(SPKSettings::maxU),
                                                                                value, 
                                                                                settings.editingKeyerSetValue(SPKSettings::maxV));
        screen.textToBuffer(paramLine, kMenuLine2);
        
        tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinV, value);    
    }
    else if (actionCount == 9)
    {
        // Save settings
        tvOne.command(0, mixKeyWindow, kTV1FunctionPowerOnPresetStore, 1);
    
        // Get back to menu
        actionCount = 0;
        state = 0;
        selectedMenu = &mixModeMenu;
        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer(selectedMenu->title, kMenuLine1);
        screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    }
}

void troubleshootingMenuResetHandler(int menuChange, bool action)
{
    static int actionCount = 0;
    
    if (action) actionCount++;
    
    if (actionCount == 0)
    {
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Follow instructions... [+]", kMenuLine2);  
    }
    if (actionCount == 1)
    {
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("On Processor find...   [+]", kMenuLine2);  
    }
    if (actionCount == 2)
    {
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("MENU+STANDBY buttons[+]", kMenuLine2);
    }
    if (actionCount == 3)
    {
        Timer timer;
        timer.start();
        
        while (timer.read_ms() < 16000)
        {
            screen.clearBufferRow(kMenuLine2);
            char messageBuffer[kStringBufferLength];
            snprintf(messageBuffer, kStringBufferLength,"Hold buttons for [%i s]", 15 - (timer.read_ms() / 1000));
            screen.textToBuffer(messageBuffer, kMenuLine2);
            screen.sendBuffer();
        }
        
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Hold buttons for [+]", kMenuLine2);
    }
    if (actionCount == 4)
    {
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Updating processor [-]", kMenuLine2);
        screen.clearBufferRow(kTVOneStatusLine);
        screen.textToBuffer("Sending...", kTVOneStatusLine);
        screen.sendBuffer();
    
        bool ok = conformProcessor();
        
        std::string sendOK = ok ? "TVOne: Reset success" : "Send Error: Reset";
    
        tvOneStatusMessage.addMessage(sendOK, kTVOneStatusMessageHoldTime);
        
        tvOneRGB1Stable = false;
        tvOneRGB2Stable = false;
        handleTVOneSources();
        
        actionCount++;
    }
    if (actionCount == 5)
    {
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer("Reset complete [DONE]", kMenuLine2);
    }
    if (actionCount == 6)
    {
        // Get back to menu
        actionCount = 0;
        selectedMenu = &troubleshootingMenu;
        screen.clearBufferRow(kMenuLine1);
        screen.clearBufferRow(kMenuLine2);
        screen.textToBuffer(selectedMenu->title, kMenuLine1);
        screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    }
}

int main() 
{
    if (debug) 
    { 
        debug->printf("\r\n\r\n");
        debug->printf("*spark d-fuser -----------\r\n");
        debug->printf(" debug channel\r\n");
    }
    
    // Set display font
    screen.fontStartCharacter = &characterBytesStartChar;
    screen.fontEndCharacter = &characterBytesEndChar;
    screen.fontCharacters = characterBytes;
    
    // Splash screen
    string softwareLine = "SW ";
    softwareLine += kSPKDFSoftwareVersion;
    screen.imageToBuffer(spkDisplayLogo);
    screen.textToBuffer("SPK:D-Fuser",0);
    screen.textToBuffer(softwareLine,1);
    screen.sendBuffer();
    
    // Load saved settings
    bool settingsAreCustom = false;
    settingsAreCustom = settings.load(kSPKDFSettingsFilename);
    if (settingsAreCustom) 
    {
        softwareLine += "; ini OK";
        screen.textToBuffer(softwareLine, 1); 
    }
    
    // Set menu structure
    mixModeMenu.title = "Mix Mode";
    mixModeAdditiveMenu.addMenuItem(SPKMenuItem(&mixModeAdditiveMenuHandler));
    mixModeUpdateKeyMenu.addMenuItem(SPKMenuItem(&mixModeUpdateKeyMenuHandler));

    setMixModeMenuItems();

    resolutionMenu.title = "Resolution";
    setResolutionMenuItems();

    commsMenu.title = "Network Mode";
    setCommsMenuItems();
    
    advancedMenu.title = "Advanced Commands";
    advancedMenu.addMenuItem(SPKMenuItem("Processor full conform", advancedConformUploadProcessor));
    advancedMenu.addMenuItem(SPKMenuItem("Back to Troubleshooting Menu", &troubleshootingMenu));
    
    troubleshootingMenu.title = "Troubleshooting"; 
    troubleshootingMenuHDCP.title = "HDCP - Can Block DVI";
    troubleshootingMenuHDCP.addMenuItem(SPKMenuItem(&troubleshootingMenuHDCPHandler));
    troubleshootingMenu.addMenuItem(SPKMenuItem(troubleshootingMenuHDCP.title, &troubleshootingMenuHDCP));
    troubleshootingMenuEDID.title = "EDID - Advertises Res's";
    troubleshootingMenuEDID.addMenuItem(SPKMenuItem(&troubleshootingMenuEDIDHandler));
    troubleshootingMenu.addMenuItem(SPKMenuItem(troubleshootingMenuEDID.title, &troubleshootingMenuEDID));
    troubleshootingMenuAspect.title = "Aspect - Mismatched Res";
    troubleshootingMenuAspect.addMenuItem(SPKMenuItem(&troubleshootingMenuAspectHandler));
    troubleshootingMenu.addMenuItem(SPKMenuItem(troubleshootingMenuAspect.title, &troubleshootingMenuAspect));
    troubleshootingMenuMatrox.title = "Matrox - Red Light";
    troubleshootingMenuMatrox.addMenuItem(SPKMenuItem(&troubleshootingMenuMatroxHandler));
    troubleshootingMenu.addMenuItem(SPKMenuItem(troubleshootingMenuMatrox.title, &troubleshootingMenuMatrox));
    troubleshootingMenuReset.title = "Output - Mixing Wrong";
    troubleshootingMenuReset.addMenuItem(SPKMenuItem(&troubleshootingMenuResetHandler));
    troubleshootingMenu.addMenuItem(SPKMenuItem(troubleshootingMenuReset.title, &troubleshootingMenuReset));
    troubleshootingMenu.addMenuItem(SPKMenuItem(advancedMenu.title, &advancedMenu));
    troubleshootingMenu.addMenuItem(SPKMenuItem("Back to Main Menu", &mainMenu));
    
    mainMenu.title = "Main Menu";
    mainMenu.addMenuItem(SPKMenuItem(mixModeMenu.title, &mixModeMenu));
    mainMenu.addMenuItem(SPKMenuItem(resolutionMenu.title, &resolutionMenu));
    mainMenu.addMenuItem(SPKMenuItem(commsMenu.title, &commsMenu));
    mainMenu.addMenuItem(SPKMenuItem(troubleshootingMenu.title, &troubleshootingMenu));
      
    selectedMenu = &mainMenu;
      
    // Misc I/O stuff
    
    fadeAPO.period(0.001);
    fadeBPO.period(0.001);

    // If we do not have two solid sources, act on this as we rely on the window having a source for crossfade behaviour
    // Once we've had two solid inputs, don't check any more as we're ok as the unit is set to hold on last frame.
    handleTVOneSources();
    
    // Processor can have been power-on saved with a keyer on, lets revert
    checkTVOneMixStatus();
    
    // Display menu and framing lines
    screen.horizLineToBuffer(kMenuLine1*pixInPage - 1);
    screen.clearBufferRow(kMenuLine1);
    screen.textToBuffer(selectedMenu->title, kMenuLine1);
    screen.clearBufferRow(kMenuLine2);
    screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    screen.horizLineToBuffer(kMenuLine2*pixInPage + pixInPage);
    screen.horizLineToBuffer(kCommsStatusLine*pixInPage - 1);
    screen.clearBufferRow(kTVOneStatusLine);
    screen.textToBuffer(tvOneStatusMessage.message(), kTVOneStatusLine);
    screen.sendBuffer();
    
    //// CONTROLS TEST

    while (0) {
        if (debug) debug->printf("xFade: %f, fadeOut: %f, tapLeft %i, tapRight: %i encPos: %i encChange:%i encHasPressed:%i \r\n" , xFadeAIN.read(), fadeUpAIN.read(), tapLeftDIN.read(), tapRightDIN.read(), menuEnc.getPos(), menuEnc.getChange(), menuEnc.hasPressed());
    }

    //// MIXER RUN

    while (1) 
    {    
        //// Task background things
        if ((osc || artNet) && rj45Mode == rj45Ethernet)
        {
            Net::poll();
        }

        //// RJ45 SWITCH
        
        if (rj45ModeDIN != rj45Mode)
        {
            if (debug) debug->printf("Handling RJ45 mode change\r\n");   

            // update state
            rj45Mode = rj45ModeDIN;
            
            setCommsMenuItems();
            
            // cancel old comms
            commsMode = commsNone;
            commsMenu = commsMode;
            
            // refresh display
            if (selectedMenu == &commsMenu) 
            {
                screen.clearBufferRow(kMenuLine1);
                screen.clearBufferRow(kMenuLine2);
                screen.textToBuffer(selectedMenu->title, kMenuLine1);
                screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
            }
            if (rj45Mode == rj45Ethernet) screen.textToBuffer("RJ45: Ethernet Mode", kCommsStatusLine);
            if (rj45Mode == rj45DMX) screen.textToBuffer("RJ45: DMX Mode", kCommsStatusLine);
        }

        //// MENU
        
        int menuChange = menuEnc.getChange();
        
        // Update GUI
        if (menuChange != 0)
        {
            if (selectedMenu->selectedItem().type == SPKMenuItem::hasHandler)
            {
                selectedMenu->selectedItem().payload.handler(menuChange, false);
            }
            else
            {
                if (debug) debug->printf("Menu changed by %i\r\n", menuChange);
                
                *selectedMenu = selectedMenu->selectedIndex() + menuChange;
                
                // update OLED line 2 here
                screen.clearBufferRow(kMenuLine2);
                screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
                
                if (debug) debug->printf("%s \r\n", selectedMenu->selectedString().c_str());
            }    
        }
        
        // Action menu item
        if (menuEnc.hasPressed()) 
        {
            if (debug) debug->printf("Action Menu Item!\r\n");
                    
            // Are we changing menus?
            if (selectedMenu->selectedItem().type == SPKMenuItem::changesToMenu) 
            {
                // If we're exiting the menu, we should set its selected index back to the menu's beginning...
                SPKMenu* menuToReset = selectedMenu->selectedItem().payload.menu == &mainMenu? selectedMenu : NULL;
             
                // point selected menu pointer to the new menu pointer
                selectedMenu = selectedMenu->selectedItem().payload.menu;
                
                // ...doing this, of course, after we've used the value
                if (menuToReset) *menuToReset = 0;
                
                // update OLED lines 1&2
                screen.clearBufferRow(kMenuLine1);
                screen.clearBufferRow(kMenuLine2);
                screen.textToBuffer(selectedMenu->title, kMenuLine1);
                screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
                
                if (selectedMenu->selectedItem().type == SPKMenuItem::hasHandler)
                {
                    selectedMenu->selectedItem().payload.handler(0, false);
                }
                
                if (debug)
                {
                    debug->printf("\r\n");
                    debug->printf("%s \r\n", selectedMenu->title.c_str());
                    debug->printf("%s \r\n", selectedMenu->selectedString().c_str());
                }
            }    
            else if (selectedMenu->selectedItem().type == SPKMenuItem::hasHandler)
            {
                selectedMenu->selectedItem().payload.handler(0, true);
            }
            // With that out of the way, we should be actioning a specific menu's payload?
            else if (selectedMenu == &mixModeMenu)
            {
                int mixModeMenuPayload = mixModeMenu.selectedItem().payload.command[0];

                if (mixModeMenuPayload < mixKeyPresetStartIndex)
                {
                    mixMode = mixModeMenuPayload;
                    
                    if (mixMode == mixModeOld) tvOneStatusMessage.addMessage("Mix mode already active", kTVOneStatusMessageHoldTime);
                }
                else
                { 
                    int keySetIndex = mixModeMenuPayload - mixKeyPresetStartIndex;
                    
                    if (keySetIndex > 0) // Key set 0 is now the "live" set, we read from processor rather than write to it.
                    {
                        bool ok;
                        ok =       tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinY, settings.keyerParamSet(keySetIndex)[SPKSettings::minY]); 
                        ok = ok && tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxY, settings.keyerParamSet(keySetIndex)[SPKSettings::maxY]); 
                        ok = ok && tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinU, settings.keyerParamSet(keySetIndex)[SPKSettings::minU]); 
                        ok = ok && tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxU, settings.keyerParamSet(keySetIndex)[SPKSettings::maxU]); 
                        ok = ok && tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMinV, settings.keyerParamSet(keySetIndex)[SPKSettings::minV]); 
                        ok = ok && tvOne.command(0, mixKeyWindow, kTV1FunctionAdjustKeyerMaxV, settings.keyerParamSet(keySetIndex)[SPKSettings::maxV]);
                        
                        tvOne.command(0, kTV1WindowIDA, kTV1FunctionPowerOnPresetStore, 1);
                        
                        tvOneStatusMessage.addMessage(ok ? "Loaded: " + settings.keyerParamName(keySetIndex) + " values" : "Send error: keyer values", kTVOneStatusMessageHoldTime);
                    }
                }
            }
            else if (selectedMenu == &resolutionMenu)
            {
                screen.clearBufferRow(kTVOneStatusLine);
                screen.textToBuffer("Setting Resolution...", kTVOneStatusLine);
                screen.sendBuffer();
                
                bool ok;
                int oldEDID = tvOne.getEDID();
                int newEDID = tvOneEDIDPassthrough ? EDIDPassthroughSlot : resolutionMenu.selectedItem().payload.command[1];
                
                ok = tvOne.setResolution(resolutionMenu.selectedItem().payload.command[0], newEDID);
                
                // Save new resolution and EDID into TV One unit for power-on. Cycling TV One power sometimes needed for EDID. Pffft.
                if (ok) tvOne.command(0, kTV1WindowIDA, kTV1FunctionPowerOnPresetStore, 1);
                
                string message;
                if (ok)
                {
                    if (oldEDID == newEDID) message = "Sent: Resolution";
                    else                    message = "Sent: Resolution + EDID";
                }
                else                        message = "Send Error: Resolution";
                tvOneStatusMessage.addMessage(message, kTVOneStatusMessageHoldTime, kTVOneStatusMessageHoldTime);
                
                // This is WHACK. Can't believe it's both needed and officially in the 1T-C2-750 manual
                if ((oldEDID != newEDID) && ok) tvOneStatusMessage.addMessage("EDID: Processor Off+On?", kTVOneStatusMessageHoldTime);
                
                if (debug) { debug->printf("Changing resolution"); }
            }
            else if (selectedMenu == &commsMenu)
            {
                string commsTypeString = "Network:";
                char commsStatusBuffer[kStringBufferLength] = "--";
            
                // Tear down any existing comms
                // This is the action of commsNone
                // And also clears the way for other comms actions
                commsMode = commsNone;
                /* // These don't take well to being destroyed. So while it was a nice idea to be able to swap between None, OSC and Artnet, now the GUI won't allow it, and we don't need this.
                if (osc)        {delete osc; osc = NULL;}  
                if (ethernet)   {delete ethernet; ethernet = NULL;}
                if (artNet)     
                {
                    artNet->ArtPollReply.NumPorts = 0; 
                    strcpy(artNet->ArtPollReply.NodeReport, "Shutdown");
                    artNet->SendArtPollReply();
                    artNet->Done(); 
                    delete artNet; 
                    artNet = NULL;
                }
                */
                if (dmx)        {delete dmx; dmx = NULL;}
                
                // Ensure we can't change to comms modes the hardware isn't switched to
                if (rj45Mode == rj45DMX && (commsMenu.selectedItem().payload.command[0] == commsOSC || commsMenu.selectedItem().payload.command[0] == commsArtNet))
                {
                    commsTypeString = "RJ45 not in Ethernet mode";
                }
                else if (rj45Mode == rj45Ethernet && (commsMenu.selectedItem().payload.command[0] == commsDMXIn || commsMenu.selectedItem().payload.command[0] == commsDMXOut))
                {
                    commsTypeString = "RJ45 not in DMX mode";
                }
                // Action!
                else if (commsMenu.selectedItem().payload.command[0] == commsOSC) 
                {
                    commsMode = commsOSC;
                    commsTypeString = "OSC: ";
                    
                    if (!ethernet)
                    {
                        if (settings.osc.DHCP)
                        {
                            ethernet = new EthernetNetIf("dvimxr");
                        }
                        else
                        {
                            ethernet = new EthernetNetIf(
                                settings.osc.controllerAddress, 
                                settings.osc.controllerSubnetMask, 
                                settings.osc.controllerGateway, 
                                settings.osc.controllerDNS  
                            );
                        }
                        
                        EthernetErr ethError = ethernet->setup();
                        if(ethError)
                        {
                            if (debug) debug->printf("Ethernet setup error, %d", ethError);
                            snprintf(commsStatusBuffer, kStringBufferLength, "Ethernet setup failed");
                            commsMenu = commsNone;
                            // break out of here. this setup should be a function that returns a boolean
                        }
                    }
                    
                    if (!osc)
                    {
                        osc = new OSCClass();
                        osc->setReceiveMessage(&receiveMessage);
                        osc->begin(settings.osc.sendPort);
                        
                        uint8_t sendAddress[] = {settings.osc.sendAddress[0], settings.osc.sendAddress[1], settings.osc.sendAddress[2], settings.osc.sendAddress[3]};
                        sendMessage.setIp( sendAddress );
                        sendMessage.setPort( settings.osc.sendPort );
                    }
                    
                    IpAddr ethIP = ethernet->getIp();
                    snprintf(commsStatusBuffer, kStringBufferLength, "In %u.%u.%u.%u:%u", ethIP[0], ethIP[1], ethIP[2], ethIP[3], settings.osc.controllerPort);
                
                    setCommsMenuItems(); // remove non-OSC from menu. we're locked in.
                }
                else if (commsMenu.selectedItem().payload.command[0] == commsArtNet) 
                {
                    commsMode = commsArtNet;
                    commsTypeString = "ArtNet: ";                    

                    artNet = new DmxArtNet();
                    
                    artNet->BindIpAddress = settings.artNet.controllerAddress;
                    artNet->BCastAddress = settings.artNet.broadcastAddress;
                
                    artNet->InitArtPollReplyDefaults();
                
                    artNet->ArtPollReply.PortType[0] = 128; // Bit 7 = Set is this channel can output data from the Art-Net Network.
                    artNet->ArtPollReply.GoodOutput[0] = 128; // Bit 7 = Set – Data is being transmitted.
                    artNet->ArtPollReply.PortType[2] = 64; // Bit 6 = Set if this channel can input onto the Art-NetNetwork.
                    artNet->ArtPollReply.GoodInput[2] = 128; // Bit 7 = Set – Data received.
                
                    artNet->Init();
                    artNet->SendArtPollReply(); // announce to art-net nodes
                    
                    snprintf(commsStatusBuffer, kStringBufferLength, "Listening");
                    
                    setCommsMenuItems(); // remove non-ArtNet from menu. we're locked in.
                }
                else if (commsMenu.selectedItem().payload.command[0] == commsDMXIn) 
                {
                    commsMode = commsDMXIn;
                    commsTypeString = "DMX In: ";
                    
                    dmxDirectionDOUT = 0;
                    
                    dmx = new DMX(kMBED_RS485_TTLTX, kMBED_RS485_TTLRX);
                }
                else if (commsMenu.selectedItem().payload.command[0] == commsDMXOut) 
                {
                    commsMode = commsDMXOut;
                    commsTypeString = "DMX Out: ";
                    
                    dmxDirectionDOUT = 1;
                    
                    dmx = new DMX(kMBED_RS485_TTLTX, kMBED_RS485_TTLRX);
                }
                                
                screen.clearBufferRow(kCommsStatusLine);
                screen.textToBuffer(commsTypeString + commsStatusBuffer, kCommsStatusLine);
            }
            else if (selectedMenu == &advancedMenu)
            {
                if (advancedMenu.selectedItem().payload.command[0] == advancedConformUploadProcessor)
                {
                    bool ok = true;
                
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer("Uploading...", kTVOneStatusLine);
                    screen.sendBuffer();
                    
                    ok = ok && uploadToProcessor();                    
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer("Conforming...", kTVOneStatusLine);
                    screen.sendBuffer();
                    
                    ok = ok && conformProcessor();
                    
                    std::string sendOK = ok ? "Conform success" : "Send Error: Conform";
                    
                    tvOneStatusMessage.addMessage(sendOK, kTVOneStatusMessageHoldTime, 600);
                }
//                else if (advancedMenu.selectedItem().payload.command[0] == advancedSetResolutions)
//                {
//                    bool ok;
//                    ok = tvOne.uploadCustomResolutions();
//                    
//                    tvOneStatusMessage.addMessage(ok ? "Resolutions set" : "Res' could not be set", kTVOneStatusMessageHoldTime);
//                }
            }
            else
            {
                if (debug) { debug->printf("Warning: No action identified"); }
            }
        }

        // Send any updates to the display
        screen.clearBufferRow(kTVOneStatusLine);
        screen.textToBuffer(tvOneStatusMessage.message(), kTVOneStatusLine);
        screen.sendBuffer();
        
        //// MIX MIX MIX MIX MIX MIX MIX MIX MIX MIX MIX MIXMIX MIX MIXMIX MIX MIX MIX MIX MIXMIX MIX MIX

        bool updateFade = false;
        float xFade = 0;
        float fadeUp = 1;
        
        //// TASK: Process control surface
        
        // Get new states of tap buttons, remembering at end of loop() assign these current values to the previous variables
        const bool tapLeft = !tapLeftDIN;
        const bool tapRight = !tapRightDIN;
        
        // We're taking a further median of the AINs on top of mbed libs v29.
        // This takes some values from last passes and most from now. With debug off, seem to need median size > 5
        xFadeFilter.process(xFadeAIN.read());
        fadeUpFilter.process(fadeUpAIN.read());
        xFadeFilter.process(xFadeAIN.read());
        fadeUpFilter.process(fadeUpAIN.read());
        xFadeFilter.process(xFadeAIN.read());
        fadeUpFilter.process(fadeUpAIN.read());
        xFadeFilter.process(xFadeAIN.read());
        fadeUpFilter.process(fadeUpAIN.read());
        const float xFadeAINCached = xFadeFilter.process(xFadeAIN.read());
        const float fadeUpAINCached = fadeUpFilter.process(fadeUpAIN.read());
        
        // When a tap is depressed, we can ignore any move of the crossfader but not fade to black
        if (tapLeft || tapRight) 
        {
            // If both are pressed, take to the one that is new, ie. not the first pressed.
            if (tapLeft && tapRight) 
            {
                xFade = tapLeftWasFirstPressed ? 1 : 0;
            }
            // If just one is pressed, take to that and remember which is pressed
            else if (tapLeft) 
            {
                xFade = 0;
                tapLeftWasFirstPressed = 1;
            }
            else if (tapRight) 
            {
                xFade = 1;
                tapLeftWasFirstPressed = 0;
            }
        }
        else xFade = 1.0 - fadeCalc(xFadeAINCached, xFadeTolerance);

        fadeUp = 1.0 - fadeCalc(fadeUpAINCached, fadeUpTolerance);

        //// TASK: Process Network Comms In, allowing hands-on controls to override
        if ((commsMode == commsOSC) || (commsMode == commsArtNet) || (commsMode == commsDMXIn))
        {
            bool commsIn = false;

            switch (commsMode)
            {
                case commsOSC:      commsIn = processOSCIn(); break;
                case commsArtNet:   commsIn = processArtNetIn(); break;
                case commsDMXIn:    commsIn = processDMXIn(); break;
            }
        
            if (commsIn)
            {
                // Store hands-on control positions to compare for change later
                commsInActive = true;
                oldXFade = xFade;
                oldFadeUp = fadeUp;
            }
            else if (commsInActive)
            {
                // If no comms in update this loop, hold to the last unless hands-on controls have moved significantly
                bool movement = (fabs(oldXFade-xFade) > 0.1) || (fabs(oldFadeUp-fadeUp) > 0.1);
                if (movement) 
                {   
                    commsInActive = false;
                    commsXFade = -1;
                    commsFadeUp = -1;
                }
            }
        
            if (commsInActive)
            {
                if (commsXFade >= 0)    xFade = commsXFade;
                if (commsFadeUp >= 0)   fadeUp = commsFadeUp;   
            }
        }

        // Calculate new A&B fade percents
        int newFadeAPercent = 0;
        int newFadeBPercent = 0;

        if (mixMode == mixBlend) 
        {
            // This is the correct algorithm for blend where window A occludes B.
            // Who knew a crossfade could be so tricky. The level of B has to be factored by what A is letting through.
            // ie. if fully faded up, top window = xfade, bottom window = 100%
            // This will however look very wrong if A is not occluding B, ie. mismatched aspect ratios.
            if (xFade > 0) // avoids div by zero (if xFade = 0 and fadeUp = 1, B sum = 0 / 0)
            {
                newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
                newFadeBPercent = (xFade*fadeUp) / (1.0 - fadeUp + xFade*fadeUp) * 100.0;
            }
            else
            {
                newFadeAPercent = fadeUp * 100.0;
                newFadeBPercent = 0;
            }
        }
        else if (mixMode == mixAdditive)
        {
            // we need to set fade level of both windows according to the fade curve profile
            float newFadeA = (1.0-xFade) * (1.0 + fadeCurve);
            float newFadeB = xFade * (1 + fadeCurve);
            if (newFadeA > 1.0) newFadeA = 1.0;
            if (newFadeB > 1.0) newFadeB = 1.0;
            
            newFadeAPercent = newFadeA * fadeUp * 100.0;
            newFadeBPercent = newFadeB * fadeUp * 100.0;
        }
        else if (mixMode == mixKeyLeft)
        {
            newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
            newFadeBPercent = fadeUp * 100.0;
        }
        else if (mixMode == mixKeyRight)
        {
            newFadeAPercent = fadeUp * 100.0;
            newFadeBPercent = xFade * fadeUp * 100.0;
        }
        
        //// TASK: Send to TVOne if percents have changed
        
        // No amount of median filtering is stopping flipflopping between two adjacent percents, so...
        bool fadeAPercentHasChanged;
        bool fadeBPercentHasChanged;
        if (oldFadeAPercent == newFadeAPercent && (newFadeAPercent == fadeAPercent - 1 || newFadeAPercent == fadeAPercent + 1))
            fadeAPercentHasChanged = false;
        else
            fadeAPercentHasChanged = newFadeAPercent != fadeAPercent;
        if (oldFadeBPercent == newFadeBPercent && (newFadeBPercent == fadeBPercent - 1 || newFadeBPercent == fadeBPercent + 1))
            fadeBPercentHasChanged = false;
        else
            fadeBPercentHasChanged = newFadeBPercent != fadeBPercent;
        
        // If changing mixMode from additive, we want to do this before updating fade values
        if (mixMode != mixModeOld && mixModeOld == mixAdditive) actionMixMode();
        
        // We want to send the higher first, otherwise black flashes can happen on taps
        if (fadeAPercentHasChanged && newFadeAPercent >= newFadeBPercent) 
        {
            oldFadeAPercent = fadeAPercent;
            fadeAPercent = newFadeAPercent;
            updateFade = true;
            
            fadeAPO = fadeAPercent / 100.0;
            tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeAPercent);
        }
        if (fadeBPercentHasChanged) 
        {
            oldFadeBPercent = fadeBPercent;
            fadeBPercent = newFadeBPercent;
            updateFade = true;
            
            fadeBPO = fadeBPercent / 100.0;
            tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeBPercent);
        }
        if (fadeAPercentHasChanged && newFadeAPercent < newFadeBPercent) 
        {
            oldFadeAPercent = fadeAPercent; 
            fadeAPercent = newFadeAPercent;
            updateFade = true;
            
            fadeAPO = fadeAPercent / 100.0;
            tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeAPercent);
        }
        if (updateFade && debug) 
        {
            //debug->printf("xFade = %3f   fadeUp = %3f \r\n", xFadeAIN.read(), fadeUpAIN.read());
            debug->printf("xFade = %3f   fadeUp = %3f \r\n", xFadeAINCached, fadeUpAINCached);
            debug->printf("xFade = %3f   fadeUp = %3f   fadeA% = %i   fadeB% = %i \r\n", xFade, fadeUp, fadeAPercent, fadeBPercent);
            debug->printf("\r\n"); 
        }
        
        // If changing mixMode to additive, we want to do this after updating fade values
        if (mixMode != mixModeOld) actionMixMode();
                
        //// TASK: Process Network Comms Out, ie. send out any fade updates
        if (commsMode == commsOSC && updateFade && !commsInActive)
        {
            processOSCOut(xFade, fadeUp);
        }

        if (commsMode == commsArtNet && updateFade && !commsInActive)
        {
            processArtNetOut(xFade, fadeUp);
        }

        if (commsMode == commsDMXOut && updateFade && !commsInActive)
        {
            processDMXOut(xFade, fadeUp);
        }
        
        //// TASK: Housekeeping
        
        if (tvOne.millisSinceLastCommandSent() > tvOne.getCommandTimeoutPeriod() + 1000)
        {
            // Lets check on our sources
            handleTVOneSources();
            
            // Lets check on our fade levels
            checkTVOneMixStatus();
        }
    }
}