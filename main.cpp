/* *SPARK D-FUSER
 * A project by Toby Harris
 *
 * 'DJ' controller styke RS232 Control for TV-One products
 * Good for 1T-C2-750, others will need some extra work
 *
 * www.sparkav.co.uk/dvimixer
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
 * vxx - TODO: Set keying values from controller, requires a guided, step-through process for user
 * vxx - TODO: Defaults load/save from USB mass storage
 * vxx - TODO: EDID passthrough override
 * vxx - TODO: EDID upload from USB mass storage
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

#define kSPKDFSoftwareVersion "21"

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

// NETWORKING

#define kOSCMbedPort 10000
#define kOSCMbedIPAddress 10,0,0,2
#define kOSCMbedSubnetMask 255,255,255,0
#define kOSCMbedGateway 10,0,0,1
#define kOSCMbedDNS 10,0,0,1

#define kArtNetBindIPAddress 2,0,0,100
#define kArtNetBroadcastAddress 2,255,255,255

#define kDMXInChannelXFade 0
#define kDMXInChannelFadeUp 1
#define kDMXOutChannelXFade 0
#define kDMXOutChannelFadeUp 1

// 8.3 format filename only, no subdirs
#define kSPKDFSettingsFilename "SPKDF.ini"

#define kStringBufferLength 30

//// DEBUG

// Comment out one or the other...
Serial *debug = new Serial(USBTX, USBRX); // For debugging via USB serial
//Serial *debug = NULL; // For release (no debugging)

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

// Saved Settings
SPKSettings settings;

// Menu 
SPKMenu *selectedMenu;
SPKMenu mainMenu;
SPKMenu resolutionMenu;

SPKMenu mixModeMenu;
SPKMenu mixModeAdditiveMenu; 
enum { mixBlend, mixAdditive, mixKey }; // additive will require custom TVOne firmware.
int mixMode = mixBlend; // Start with safe mix mode, and test to get out of it. Safe mode will work with inputs missing and without hold frames.
float fadeCurve = 0.0f; // 0 = "X", ie. as per blend, 1 = "/\", ie. as per additive  <-- pictograms!

SPKMenu commsMenu;
enum { commsNone, commsOSC, commsArtNet, commsDMXIn, commsDMXOut};
int commsMode = commsNone;

SPKMenu advancedMenu;
enum { advancedHDCPOn, advancedHDCPOff, advancedEDIDPassthrough, advancedEDIDInternal, advancedTestSources, advancedConformProcessor, advancedLoadDefaults, advancedSetResolutions };

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

// Key mode parameters
int keyerParamsSet = -1; // last keyParams index uploaded to unit 

// TVOne input sources stable flag
bool tvOneRGB1Stable = false;
bool tvOneRGB2Stable = false;

// TVOne behaviour flags
bool tvOneHDCPOn = false;
bool tvOneEDIDPassthrough = false;
const int32_t EDIDPassthroughSlot = 7;


void processOSCIn(float &xFade, float &fadeUp) {
    string statusMessage;
    
    if (!strcmp( receiveMessage.getTopAddress() , "dvimxr" )) 
    {
        statusMessage = "OSC: /dvimxr";
        if (!strcmp( receiveMessage.getSubAddress() , "xFade" )) 
        {
            if (receiveMessage.getArgNum() == 1)
                if (receiveMessage.getTypeTag(0) == 'f')
                {
                    xFade = receiveMessage.getArgFloat(0);
                    char buffer[15];
                    snprintf(buffer, kStringBufferLength, "/xFade %1.2f", xFade);
                    statusMessage += buffer;
                }
        }
        else if (!strcmp( receiveMessage.getSubAddress() , "fadeUp" ))
        {
            if (receiveMessage.getArgNum() == 1)
                if (receiveMessage.getTypeTag(0) == 'f')
                {
                    fadeUp = receiveMessage.getArgFloat(0);
                    char buffer[15];
                    snprintf(buffer, kStringBufferLength, "/fadeUp %1.2f", fadeUp);
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

void processOSCOut(float &xFade, float &fadeUp) 
{
    char statusMessageBuffer[kStringBufferLength];

    sendMessage.setAddress("dvimxr", "xFadeFadeUp");
    sendMessage.setArgs("ff", &xFade, &fadeUp);
    osc->sendOsc(&sendMessage);
    
    screen.clearBufferRow(kCommsStatusLine);
    snprintf(statusMessageBuffer, kStringBufferLength, "OSC Out: xF %.2f fUp %.2f", xFade, fadeUp);
    screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);

    if (debug) debug->printf(statusMessageBuffer);
}

void processArtNetIn(float &xFade, float &fadeUp) 
{
    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer("ArtNet activity", kCommsStatusLine);

    if (debug) debug->printf("ArtNet activity");
}

void processArtNetOut(float &xFade, float &fadeUp) 
{
    char statusMessageBuffer[kStringBufferLength];

    int xFadeDMX = xFade*255;
    int fadeUpDMX = fadeUp*255;
    
    // Universe 0, Channel 0 = xFade, Channel 1 = fadeUp
    char dmxData[2] = {xFadeDMX, fadeUpDMX};
    artNet->Send_ArtDmx(0, 0, dmxData, 2);
    
    screen.clearBufferRow(kCommsStatusLine);
    snprintf(statusMessageBuffer, kStringBufferLength, "A'Net Out: xF%3i fUp %3i", xFadeDMX, fadeUpDMX);
    screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);

    if (debug) debug->printf(statusMessageBuffer);
}

void processDMXIn(float &xFade, float &fadeUp) 
{
    char statusMessageBuffer[kStringBufferLength];

    int xFadeDMX = dmx->get(kDMXInChannelXFade);
    int fadeUpDMX = dmx->get(kDMXInChannelFadeUp);

    xFade = (float)xFadeDMX/255;
    fadeUp = (float)fadeUpDMX/255;

    screen.clearBufferRow(kCommsStatusLine);
    snprintf(statusMessageBuffer, kStringBufferLength, "DMX In: xF %3i fUp %3i", xFadeDMX, fadeUpDMX);
    screen.textToBuffer(statusMessageBuffer, kCommsStatusLine);

    if (debug) debug->printf(statusMessageBuffer);
}

void processDMXOut(float &xFade, float &fadeUp) 
{
    char statusMessageBuffer[kStringBufferLength];

    int xFadeDMX = xFade*255;
    int fadeUpDMX = fadeUp*255;
    
    dmx->put(kDMXOutChannelXFade, xFadeDMX);
    dmx->put(kDMXOutChannelFadeUp, fadeUpDMX);
    
    screen.clearBufferRow(kCommsStatusLine);
    snprintf(statusMessageBuffer, kStringBufferLength, "DMX Out: xF %3i fUp %3i", xFadeDMX, fadeUpDMX);
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
   
    if (debug) debug->printf("HandleTVOneSources: RGB1: %i, RGB2: %i, sourceA: %#x, sourceB: %#i \r\n", RGB1, RGB2, sourceA, sourceB);
   
    string tvOneDetectString;
    if (!ok)
    {
        tvOneDetectString = "TVOne: link failed";
    }
    else if (!RGB1 || !RGB2) 
    {
        if (!RGB1 && !RGB2) tvOneDetectString = "TVOne: no sources";
        else if (!RGB1)     tvOneDetectString = "TVOne: no right source";
        else if (!RGB2)     tvOneDetectString = "TVOne: no left source";
    }
    else
    {
        tvOneDetectString = "TVOne: OK";
    }
    
    screen.clearBufferRow(kTVOneStatusLine);
    screen.textToBuffer(tvOneDetectString, kTVOneStatusLine);
    
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
        
    return ok;       
}   

bool setKeyParamsTo(int index) 
{   
    // Only spend the time uploading six parameters if we need to
    // Might want to bounds check here
    
    bool ok;
    
    if (index != keyerParamsSet)
    {
        ok =       tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinY, settings.keyerParamSet(index)[SPKSettings::minY]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxY, settings.keyerParamSet(index)[SPKSettings::maxY]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinU, settings.keyerParamSet(index)[SPKSettings::minU]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxU, settings.keyerParamSet(index)[SPKSettings::maxU]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinV, settings.keyerParamSet(index)[SPKSettings::minV]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxV, settings.keyerParamSet(index)[SPKSettings::maxV]);
        
        keyerParamsSet = index;
    }
    else
    {
        ok = true;
    }
    
    return ok;
}

void actionMixMode()
{
    if (debug) debug->printf("Changing mix mode \r\n");

    bool ok = true;
    string sentOK;
    char sentMSGBuffer[kStringBufferLength];

    // Set Keyer
    if (mixMode < mixKey)
    {
        // Set Keyer Off. Quicker to set and fail than to test for on and then turn off
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, false);
        
        if (mixMode == mixBlend)    
        {
            // Turn off Additive Mixing on output
            ok = tvOne.command(0, kTV1WindowIDA, 0x298, 0);
            snprintf(sentMSGBuffer, kStringBufferLength, "Blend");
        }
        if (mixMode == mixAdditive) 
        {   
            // First set B to what you'd expect for additive; it may be left at 100 if optimised blend mixing was previous mixmode.
            ok =       tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeBPercent);
            // Then turn on Additive Mixing
            ok = ok && tvOne.command(0, kTV1WindowIDA, 0x298, 1);
            snprintf(sentMSGBuffer, kStringBufferLength, "Additive");
        }                
    }
    else
    {
        int index = mixMode - mixKey;

        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, true);
        ok = ok && setKeyParamsTo(index);
 
        snprintf(sentMSGBuffer, kStringBufferLength, "Keyer On with %i", index);
    }

    if (ok) sentOK = "Sent:";
    else sentOK = "Send Error:";
    
    screen.clearBufferRow(kTVOneStatusLine);
    screen.textToBuffer(sentOK + sentMSGBuffer, kTVOneStatusLine);
}


bool conformProcessor()
{
    bool ok = true;
                    
    int32_t on = 1;
    int32_t off = 0;
    
    // Independent output
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionMode, 2);
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsOutputEnable, on);
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsLockMethod, off);
                    
    // Make sure our windows exist
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsEnable, on);
    ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsEnable, on);
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsLayerPriority, 0);
    ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsLayerPriority, 1);                
        
    // Assign inputs to windows, so that left on the crossfader is left on the processor viewed from front
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceRGB2);
    ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsWindowSource, kTV1SourceRGB1);
    
    // Set scaling to fit source within output, maintaining aspect ratio
    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustWindowsZoomLevel, 100);
    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDB, kTV1FunctionAdjustWindowsZoomLevel, 100);
    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustWindowsShrinkEnable, off);
    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDB, kTV1FunctionAdjustWindowsShrinkEnable, off);
    int32_t fit = 1;
    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceAspectCorrect, fit);
    ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceAspectCorrect, fit);
    
    // On source loss, hold on the last frame received.
    int32_t freeze = 1;
    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceOnSourceLoss, freeze);
    ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceOnSourceLoss, freeze);
    
    // Set resolution and fade levels for maximum chance of being seen
    int32_t slot = tvOneEDIDPassthrough ? EDIDPassthroughSlot : 5;
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsOutputResolution, kTV1ResolutionVGA);
    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
    ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, 100);
    ok = ok && tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, 100);
    
    // Upload Matrox EDID to mem4 (ie. index 3). Use this EDID slot when setting Matrox resolutions.
    char edidData[256];
    int i;
    {
        LocalFileSystem local("local");
        FILE *file = fopen("/local/matroxe.did", "r"); // 8.3, avoid .bin as mbed executable
        for ( i=0; i<256; i++)
        {
            int edidByte = fgetc(file);
            if (edidByte == EOF) break;
            else edidData[i] = edidByte;
        }
        fclose(file);
    }
    ok = ok && tvOne.uploadEDID(edidData, i, 3);
    
    // Save current state for power on
    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionPowerOnPresetStore, 1);
    
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
    mixModeMenu.addMenuItem(SPKMenuItem("Crossfade", &mixModeAdditiveMenu));
    for (int i=0; i < settings.keyerSetCount(); i++)
    {
        mixModeMenu.addMenuItem(SPKMenuItem(settings.keyerParamName(i), mixKey+i));
    }
    mixModeMenu.addMenuItem(SPKMenuItem("Back to Main Menu", &mainMenu));
}

void setCommsMenuItems()
{
    if (rj45Mode == rj45Ethernet) 
    {
        commsMenu.title = "Network Mode [Ethernet]";
        commsMenu.clearMenuItems();
        commsMenu.addMenuItem(SPKMenuItem("None", commsNone));
        commsMenu.addMenuItem(SPKMenuItem("OSC", commsOSC));
        commsMenu.addMenuItem(SPKMenuItem("ArtNet", commsArtNet));
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
    setMixModeMenuItems();
    
    mixModeAdditiveMenu.title = "Crossfade";
    mixModeAdditiveMenu.addMenuItem(SPKMenuItem("[title overridden]", &mixModeMenu, true));
 
    resolutionMenu.title = "Resolution";
    setResolutionMenuItems();

    commsMenu.title = "Network Mode";
    setCommsMenuItems();
    
    advancedMenu.title = "Troubleshooting"; 
    advancedMenu.addMenuItem(SPKMenuItem("HDCP Off", advancedHDCPOff));
    advancedMenu.addMenuItem(SPKMenuItem("HDCP On", advancedHDCPOn));
    advancedMenu.addMenuItem(SPKMenuItem("EDID Passthrough", advancedEDIDPassthrough)); // have global setting of passthrough that overrides resolution sets and is saved with conform processor
    advancedMenu.addMenuItem(SPKMenuItem("EDID Internal", advancedEDIDInternal));
    advancedMenu.addMenuItem(SPKMenuItem("Test Processor Sources", advancedTestSources));
    advancedMenu.addMenuItem(SPKMenuItem("Conform Processor", advancedConformProcessor));
    if (settingsAreCustom) advancedMenu.addMenuItem(SPKMenuItem("Revert Controller", advancedLoadDefaults));
    advancedMenu.addMenuItem(SPKMenuItem("Back to Main Menu", &mainMenu));
    
    mainMenu.title = "Main Menu";
    mainMenu.addMenuItem(SPKMenuItem(mixModeMenu.title, &mixModeMenu));
    mainMenu.addMenuItem(SPKMenuItem(resolutionMenu.title, &resolutionMenu));
    mainMenu.addMenuItem(SPKMenuItem(commsMenu.title, &commsMenu));
    mainMenu.addMenuItem(SPKMenuItem(advancedMenu.title, &advancedMenu));
      
    selectedMenu = &mainMenu;
      
    // Misc I/O stuff
    
    fadeAPO.period(0.001);
    fadeBPO.period(0.001);
    
    // Test for TV One connectivity and determine unit type
    // TODO: Determine and fall back if not dfuser firmware?
    // TODO: Use software version to select resolution slots?
    // TODO: Use product / board type to select TVOne conform type?
    // kTV1FunctionReadSoftwareVersion
    // kTV1FunctionReadProductType
    // kTV1FunctionReadBoardType
    
    // Display menu and framing lines
    screen.horizLineToBuffer(kMenuLine1*pixInPage - 1);
    screen.clearBufferRow(kMenuLine1);
    screen.textToBuffer(selectedMenu->title, kMenuLine1);
    screen.clearBufferRow(kMenuLine2);
    screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    screen.horizLineToBuffer(kMenuLine2*pixInPage + pixInPage);
    screen.horizLineToBuffer(kCommsStatusLine*pixInPage - 1);
    screen.clearBufferRow(kTVOneStatusLine);
    screen.textToBuffer("TVOne: OK", kTVOneStatusLine); // handleTVOneSources will update this
    
    // If we do not have two solid sources, act on this as we rely on the window having a source for crossfade behaviour
    // Once we've had two solid inputs, don't check any more as we're ok as the unit is set to hold on last frame.
    bool ok = handleTVOneSources();
    
    // Update display before starting mixer loop
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
            if (rj45Mode == rj45Ethernet) screen.textToBuffer("RJ45: Ethernet Engaged", kCommsStatusLine);
            if (rj45Mode == rj45DMX) screen.textToBuffer("RJ45: DMX Engaged", kCommsStatusLine);
        }

        //// MENU
        
        int menuChange = menuEnc.getChange();
        
        // Update GUI
        if (menuChange != 0)
        {
            if (selectedMenu->selectedItem().handlingControls)
            {
                if (selectedMenu == &mixModeAdditiveMenu)
                {
                    fadeCurve += menuChange * 0.05;
                    if (fadeCurve > 1.0f) fadeCurve = 1.0f;
                    if (fadeCurve < 0.0f) fadeCurve = 0.0f;
                    
                    int newMixMode = (fadeCurve > 0.0f) ? mixAdditive: mixBlend;

                    if (newMixMode != mixMode)
                    {
                        mixMode = newMixMode;
                        actionMixMode();
                    }

                    screen.clearBufferRow(kMenuLine2);
                    screen.textToBuffer("Blend [ ----- ] Add", kMenuLine2);
                    screen.characterToBuffer('X', 38 + fadeCurve*20.0f, kMenuLine2);
                    
                    if (debug) debug->printf("Fade curve changed by %i to %f \r\n", menuChange, fadeCurve);
                }
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
                
                if (debug)
                {
                    debug->printf("\r\n");
                    debug->printf("%s \r\n", selectedMenu->title.c_str());
                    debug->printf("%s \r\n", selectedMenu->selectedString().c_str());
                }
                
                // Are we changing menus that should have a command attached?
                if (selectedMenu == &mixModeAdditiveMenu)
                {
                    screen.clearBufferRow(kMenuLine2);
                    screen.textToBuffer("Blend [ ----- ] Add", kMenuLine2);
                    screen.characterToBuffer('X', 38 + fadeCurve*20.0f, kMenuLine2);
                    
                    mixMode = fadeCurve > 0 ? mixAdditive : mixBlend;
                    actionMixMode();
                }
            }
            // With that out of the way, we should be actioning a specific menu's payload?
            else if (selectedMenu == &mixModeMenu)
            {
                mixMode = mixModeMenu.selectedItem().payload.command[0];
                actionMixMode();
            }
            else if (selectedMenu == &resolutionMenu)
            {
                bool ok = true;
                
                ok =       tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsOutputResolution, resolutionMenu.selectedItem().payload.command[0]);
                
                int32_t slot = tvOneEDIDPassthrough ? EDIDPassthroughSlot : resolutionMenu.selectedItem().payload.command[1];
                
                ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
                ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
                
                string sentOK;
                if (ok) sentOK = "Sent: ";
                else sentOK = "Send Error: ";
                
                char sentMSGBuffer[kStringBufferLength];
                snprintf(sentMSGBuffer, kStringBufferLength,"Res %i, EDID %i", resolutionMenu.selectedItem().payload.command[0], resolutionMenu.selectedItem().payload.command[1]);
                
                screen.clearBufferRow(kTVOneStatusLine);
                screen.textToBuffer(sentOK + sentMSGBuffer, kTVOneStatusLine);
                
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
                    
                    ethernet = new EthernetNetIf(
                    IpAddr(kOSCMbedIPAddress), 
                    IpAddr(kOSCMbedSubnetMask), 
                    IpAddr(kOSCMbedGateway), 
                    IpAddr(kOSCMbedDNS)  
                    );
                  
                    EthernetErr ethError = ethernet->setup();
                    if(ethError)
                    {
                        if (debug) debug->printf("Ethernet setup error, %d", ethError);
                        snprintf(commsStatusBuffer, kStringBufferLength, "Ethernet setup failed");
                        commsMenu = commsNone;
                        // break out of here. this setup should be a function that returns a boolean
                    }

                    osc = new OSCClass();
                    osc->setReceiveMessage(&receiveMessage);
                    osc->begin(kOSCMbedPort);
                    
                    snprintf(commsStatusBuffer, kStringBufferLength, "Listening on %i", kOSCMbedPort);
                }
                else if (commsMenu.selectedItem().payload.command[0] == commsArtNet) 
                {
                    commsMode = commsArtNet;
                    commsTypeString = "ArtNet: ";                    

                    artNet = new DmxArtNet();
                    
                    artNet->BindIpAddress = IpAddr(kArtNetBindIPAddress);
                    artNet->BCastAddress = IpAddr(kArtNetBroadcastAddress);
                
                    artNet->InitArtPollReplyDefaults();
                
                    artNet->ArtPollReply.PortType[0] = 128; // Bit 7 = Set is this channel can output data from the Art-Net Network.
                    artNet->ArtPollReply.GoodOutput[0] = 128; // Bit 7 = Set – Data is being transmitted.
                    artNet->ArtPollReply.PortType[2] = 64; // Bit 6 = Set if this channel can input onto the Art-NetNetwork.
                    artNet->ArtPollReply.GoodInput[2] = 128; // Bit 7 = Set – Data received.
                
                    artNet->Init();
                    artNet->SendArtPollReply(); // announce to art-net nodes
                    
                    snprintf(commsStatusBuffer, kStringBufferLength, "Listening");
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
                if (advancedMenu.selectedItem().payload.command[0] == advancedHDCPOff)
                {
                    bool ok;
                    
                    ok = tvOne.setHDCPOn(false);
                    
                    std::string sendOK = ok ? "Sent: HDCP Off" : "Send Error: HDCP Off";
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(sendOK, kTVOneStatusLine);
                }
                else if (advancedMenu.selectedItem().payload.command[0] == advancedHDCPOn)
                {
                    bool ok;
                    
                    ok = tvOne.setHDCPOn(true);
                    
                    std::string sendOK = ok ? "Sent: HDCP On" : "Send Error: HDCP On";
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(sendOK, kTVOneStatusLine);
                }
                else if (advancedMenu.selectedItem().payload.command[0] == advancedEDIDPassthrough)
                {
                    tvOneEDIDPassthrough = true;
                    
                    bool ok = true;
                    
                    int32_t slot = tvOneEDIDPassthrough ? EDIDPassthroughSlot : resolutionMenu.selectedItem().payload.command[1];
                
                    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
                    ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
                    
                    std::string sendOK = ok ? "Sent: EDID. Next:conform?" : "Send Error: EDID";
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(sendOK, kTVOneStatusLine);
                }
                else if (advancedMenu.selectedItem().payload.command[0] == advancedEDIDInternal)
                {
                    tvOneEDIDPassthrough = false;
                    
                    bool ok = true;
                    
                    int32_t slot = tvOneEDIDPassthrough ? EDIDPassthroughSlot : resolutionMenu.selectedItem().payload.command[1];
                
                    ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
                    ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, slot);
                    
                    std::string sendOK = ok ? "Sent: EDID. Next:conform?" : "Send Error: EDID";
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(sendOK, kTVOneStatusLine);
                }
                else if (advancedMenu.selectedItem().payload.command[0] == advancedTestSources)
                {   
                    tvOneRGB1Stable = false;
                    tvOneRGB2Stable = false;
                    handleTVOneSources();
                }
                else if (advancedMenu.selectedItem().payload.command[0] == advancedConformProcessor)
                {
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer("Conforming...", kTVOneStatusLine);
                    screen.sendBuffer();
                    
                    bool ok = conformProcessor();
                    
                    std::string sendOK = ok ? "Conform success" : "Send Error: Conform";
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(sendOK, kTVOneStatusLine);
                }
                else if (advancedMenu.selectedItem().payload.command[0] == advancedLoadDefaults)
                {
                    settings.loadDefaults();
                    setMixModeMenuItems();
                    setResolutionMenuItems();
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer("Controller reverted", kTVOneStatusLine);
                }
                else if (advancedMenu.selectedItem().payload.command[0] == advancedSetResolutions)
                {
                    bool ok;
                    ok = tvOne.setCustomResolutions();
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(ok ? "Resolutions set" : "Res' could not be set", kTVOneStatusLine);
                }
            }
            else
            {
                if (debug) { debug->printf("Warning: No action identified"); }
            }
        }
        
        // Send any updates to the display
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

        //// TASK: Process Network Comms In, ie. modify TVOne state
        if (commsMode == commsOSC)
        {
            if (osc->newMessage) 
            {
                osc->newMessage = false; // fixme!
                processOSCIn(xFade, fadeUp);
            }
        }

        if (commsMode == commsArtNet)
        {
            if (artNet->Work()) processArtNetIn(xFade, fadeUp);
        }

        if (commsMode == commsDMXIn)
        {
            processDMXIn(xFade, fadeUp);
        }

        if (commsMode == commsDMXOut)
        {
            processDMXOut(xFade, fadeUp);
        }
        
        // Calculate new A&B fade percents
        int newFadeAPercent = 0;
        int newFadeBPercent = 0;

        if (mixMode == mixBlend) 
        {
            if (fadeUp < 1.0)
            {
                // we need to set fade level of both windows as there is no way AFAIK to implement fade to black as a further window on top of A&B
                newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
                newFadeBPercent = xFade * fadeUp * 100.0;
            }
            else
            {
                // we can optimise and just fade A in and out over a fully up B, doubling the rate of fadeA commands sent.
                newFadeAPercent = (1.0-xFade) * 100.0;
                newFadeBPercent = 100.0;
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
        else if (mixMode >= mixKey)
        {
            newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
            newFadeBPercent = fadeUp * 100.0;
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
        
        
        //// TASK: Process Network Comms Out, ie. send out any fade updates
        if (commsMode == commsOSC && updateFade)
        {
            processOSCOut(xFade, fadeUp);
        }

        if (commsMode == commsArtNet && updateFade)
        {
            processArtNetOut(xFade, fadeUp);
        }

        if (commsMode == commsDMXOut && updateFade)
        {
            processDMXOut(xFade, fadeUp);
        }
        
        // Housekeeping
        if (tvOne.millisSinceLastCommandSent() > 1500)
        {
            // We should check up on any source that hasn't ever been stable 
            if (!tvOneRGB1Stable || !tvOneRGB2Stable) handleTVOneSources();
        }
    }
}