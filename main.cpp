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
 * vxx - TODO: Keying values load from USB mass storage
 * vxx - TODO: Set keying values from controller, requires a guided, step-through process for user
 * vxx - TODO: Defaults load/save from USB mass storage
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

#include <sstream>

#define kSPKDFSoftwareVersion "beta.19"

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
SPKMenu *lastSelectedMenu;
SPKMenuOfMenus mainMenu;
SPKMenuPayload resolutionMenu;
SPKMenuPayload mixModeMenu;
SPKMenuPayload advancedMenu;

enum { mixBlend, mixAdditive, mixKey }; // additive will require custom TVOne firmware.
int mixMode = mixBlend;
SPKMenuPayload commsMenu;
enum { commsNone, commsOSC, commsArtNet, commsDMXIn, commsDMXOut};
int commsMode = commsNone;
enum { advancedHDCPOn, advancedHDCPOff, advancedSelfTest };

// RJ45 Comms
enum { rj45Ethernet = 0, rj45DMX = 1}; // These values from circuit
int rj45Mode = -1;
EthernetNetIf *ethernet = NULL;
OSCClass *osc = NULL;
OSCMessage recMessage;
DmxArtNet *artNet = NULL;
DMX *dmx = NULL;

// Fade logic constants
const float xFadeTolerance = 0.05;
const float fadeUpTolerance = 0.05;

// A&B Fade as resolved percent
int fadeAPercent = 0;
int fadeBPercent = 0;

// Tap button states
bool tapLeftWasFirstPressed = false;

// Key mode parameters
int keyerParamsSet = -1; // last keyParams index uploaded to unit 

void processOSC(float &xFade, float &fadeUp) {
    std::stringstream statusMessage;
    statusMessage.setf(ios::fixed,ios::floatfield);
    statusMessage.precision(2);
    
    if (!strcmp( recMessage.getTopAddress() , "dvimxr" )) 
    {
        statusMessage << "OSC: /dvimxr";
        if (!strcmp( recMessage.getSubAddress() , "xFade" )) 
            if (recMessage.getArgNum() == 1)
                if (recMessage.getTypeTag(0) == 'f')
                {
                    double newXFade = recMessage.getArgFloat(0);
                    statusMessage << "/xFade " << newXFade;
                    xFade = newXFade;
                }
        else if (!strcmp( recMessage.getSubAddress() , "fadeUp" ))
            if (recMessage.getArgNum() == 1)
                if (recMessage.getTypeTag(0) == 'f')
                {
                    double newFadeUp = recMessage.getArgFloat(0);
                    statusMessage << "/fadeUp " << newFadeUp;
                    xFade = newFadeUp;
                }
        else statusMessage << recMessage.getSubAddress() << " - Ignoring";
    }
    else
    {
        statusMessage << "OSC: " << recMessage.getTopAddress() << " - Ignoring";
    }
    
    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer(statusMessage.str(), kCommsStatusLine);
    screen.sendBuffer();
    if (debug) debug->printf("%s \r\n", statusMessage.str().c_str());
    
}

void processArtNet(float &xFade, float &fadeUp) 
{
    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer("ArtNet activity", kCommsStatusLine);
    screen.sendBuffer();
    if (debug) debug->printf("ArtNet activity");
}

void processDMXIn(float &xFade, float &fadeUp) 
{
    std::stringstream statusMessage;

    int xFadeDMX = dmx->get(kDMXInChannelXFade);
    int fadeUpDMX = dmx->get(kDMXInChannelFadeUp);

    xFade = (float)xFadeDMX/255;
    fadeUp = (float)fadeUpDMX/255;

    screen.clearBufferRow(kCommsStatusLine);
    statusMessage << "DMX In: xF " << xFadeDMX << " fUp " << fadeUpDMX;
    screen.textToBuffer(statusMessage.str(), kCommsStatusLine);
    screen.sendBuffer();
    if (debug) debug->printf(statusMessage.str().c_str());
}

void processDMXOut(float &xFade, float &fadeUp) 
{
    std::stringstream statusMessage;

    int xFadeDMX = xFade*255;
    int fadeUpDMX = fadeUp*255;
    
    dmx->put(kDMXOutChannelXFade, xFadeDMX);
    dmx->put(kDMXOutChannelFadeUp, fadeUpDMX);
    
    screen.clearBufferRow(kCommsStatusLine);
    statusMessage << "DMX Out: xF " << xFadeDMX << " fUp " << fadeUpDMX;
    screen.textToBuffer(statusMessage.str(), kCommsStatusLine);
    screen.sendBuffer();
    if (debug) debug->printf(statusMessage.str().c_str());
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

bool setKeyParamsTo(int index) 
{   
    // Only spend the time uploading six parameters if we need to
    // Might want to bounds check here
    
    bool ok;
    
    if (index != keyerParamsSet)
    {
        ok =       tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinY, settings.keyerParamSet(index)[0]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxY, settings.keyerParamSet(index)[1]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinU, settings.keyerParamSet(index)[2]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxU, settings.keyerParamSet(index)[3]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinV, settings.keyerParamSet(index)[4]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxV, settings.keyerParamSet(index)[5]);
        
        keyerParamsSet = index;
    }
    else
    {
        ok = true;
    }
    
    return ok;
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
    screen.imageToBuffer(spkDisplayLogo);
    screen.textToBuffer("SPK:D-Fuser",0);
    screen.textToBuffer(string("SW ") + kSPKDFSoftwareVersion,1);
    screen.sendBuffer();
    
    // Load saved settings
/* CRAZY, see note in spk_settings.h   
    if (settings.load(kSPKDFSettingsFilename)) 
    {screen.textToBuffer("Settings Read",2); screen.sendBuffer();}
    else 
    {screen.textToBuffer("Settings NOT Read",2); screen.sendBuffer();}
*/    
    // Set menu structure
    mixModeMenu.title = "Mix Mode";
    mixModeMenu.addMenuItem("Blend", mixBlend, 0);
    for (int i=0; i < settings.keyerSetCount(); i++)
    {
        mixModeMenu.addMenuItem(settings.keyerParamName(i), mixKey+i, 0);
    }
 
    resolutionMenu.title = "Resolution";
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionVGA, kTV1ResolutionVGA, 5);
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionSVGA, kTV1ResolutionSVGA, 5);
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionXGAp60, kTV1ResolutionXGAp60, 5);
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionWSXGAPLUSp60, kTV1ResolutionWSXGAPLUSp60, 5);
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionWUXGAp60, kTV1ResolutionWUXGAp60, 5);
    resolutionMenu.addMenuItem(kTV1ResolutionDescription720p60, kTV1Resolution720p60, 5);
    resolutionMenu.addMenuItem(kTV1ResolutionDescription1080p60, kTV1Resolution1080p60, 5);
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionDualHeadSVGAp60, kTV1ResolutionDualHeadSVGAp60, 0);
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionDualHeadXGAp60, kTV1ResolutionDualHeadXGAp60, 0);
    resolutionMenu.addMenuItem(kTV1ResolutionDescriptionTripleHeadVGAp60, kTV1ResolutionTripleHeadVGAp60, 0);

    commsMenu.title = "Network Mode"; 
    commsMenu.addMenuItem("None", commsNone, 0);
    commsMenu.addMenuItem("OSC", commsOSC, 0);
    commsMenu.addMenuItem("ArtNet", commsArtNet, 0);
    commsMenu.addMenuItem("DMX In", commsDMXIn, 0);
    commsMenu.addMenuItem("DMX Out", commsDMXOut, 0);

    advancedMenu.title = "Troubleshooting"; 
    advancedMenu.addMenuItem("HDCP Off", advancedHDCPOff, 0);
    advancedMenu.addMenuItem("HDCP On", advancedHDCPOn, 0);
    advancedMenu.addMenuItem("Start Self-Test", advancedSelfTest, 0);

    mainMenu.title = "Main Menu";
    mainMenu.addMenuItem(&mixModeMenu);
    mainMenu.addMenuItem(&resolutionMenu);
    mainMenu.addMenuItem(&commsMenu);
    mainMenu.addMenuItem(&advancedMenu);
    
    selectedMenu = &mainMenu;
    lastSelectedMenu = &mainMenu;    
    
    // Misc I/O stuff
    
    fadeAPO.period(0.001);
    fadeBPO.period(0.001);
    
    // TODO: Test for TVOne connectivity here and display in status line
    
    // Display menu and framing lines
    screen.horizLineToBuffer(kMenuLine1*pixInPage - 1);
    screen.clearBufferRow(kMenuLine1);
    screen.textToBuffer(selectedMenu->title, kMenuLine1);
    screen.clearBufferRow(kMenuLine2);
    screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
    screen.horizLineToBuffer(kMenuLine2*pixInPage + pixInPage);
    screen.horizLineToBuffer(kCommsStatusLine*pixInPage - 1);
    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer(commsMenu.selectedString(), kCommsStatusLine);
    screen.clearBufferRow(kTVOneStatusLine);
    screen.sendBuffer();


    //// CONTROLS TEST

    while (0) {
        if (debug) debug->printf("xFade: %f, fadeOut: %f, tapLeft %i, tapRight: %i encPos: %i encChange:%i encHasPressed:%i \r\n" , xFadeAIN.read(), fadeUpAIN.read(), tapLeftDIN.read(), tapRightDIN.read(), menuEnc.getPos(), menuEnc.getChange(), menuEnc.hasPressed());
    }

    //// MIXER RUN

    while (1) 
    {
        //// Task background things
        if (ethernet && rj45Mode == rj45Ethernet)
        {
            if (debug) debug->printf("net poll");
            Net::poll();
        }

        //// RJ45 SWITCH
        
        if (rj45ModeDIN != rj45Mode)
        {
            // update state
            rj45Mode = rj45ModeDIN;
            if (rj45Mode == rj45Ethernet) commsMenu.title = "Network Mode [Ethernet]";
            if (rj45Mode == rj45DMX) commsMenu.title = "Network Mode [DMX]";
            
            // cancel old comms
            commsMode = commsNone;
            commsMenu = commsMode;
            
            // refresh display
            if (selectedMenu == &commsMenu) screen.textToBuffer(selectedMenu->title, kMenuLine1);
            if (rj45Mode == rj45Ethernet) screen.textToBuffer("RJ45: Ethernet Engaged", kCommsStatusLine);
            if (rj45Mode == rj45DMX) screen.textToBuffer("RJ45: DMX Engaged", kCommsStatusLine);
        }

        //// MENU
        
        int menuChange = menuEnc.getChange();
        
        // Update GUI
        if (menuChange != 0)
        {
            if (debug) debug->printf("Menu changed by %i\r\n", menuChange);
            
            *selectedMenu = selectedMenu->selectedIndex() + menuChange;
            
            // update OLED line 2 here
            screen.clearBufferRow(kMenuLine2);
            screen.textToBuffer(selectedMenu->selectedString(), kMenuLine2);
            
            if (debug) debug->printf("%s \r\n", selectedMenu->selectedString().c_str());
        }
        
        // Action menu item
        if (menuEnc.hasPressed()) 
        {
            if (debug) debug->printf("Action Menu Item!\r\n");
        
            // Are we changing menus?
            if (selectedMenu->type() == menuOfMenus) 
            {
                // point selected menu to the new menu
                // FIXME. Make this function abstract virtual of base class or get dynamic_cast working. BTW: C++ sucks / Obj-c rocks / Right now.
                if (selectedMenu == &mainMenu) selectedMenu = mainMenu.selectedMenu();
                else if (debug) debug->printf("FIXME: You've missed a SPKMenuOfMenus");
                
                // reset the selection within that menu to the first position
                (*selectedMenu) = 0;
                
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
            }
            // Are we cancelling?
            else if (selectedMenu->type() == payload && selectedMenu->selectedIndex() == 0)           
            {
                selectedMenu = lastSelectedMenu;
                
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
            }
            // With that out of the way, we should be actioning a specific menu's payload?
            else if (selectedMenu == &mixModeMenu)
            {
                mixMode = mixModeMenu.selectedPayload1();
            
                bool ok = true;
                std::string sentOK;
                std::stringstream sentMSG;
            
                // Set Keyer
                if (mixModeMenu.selectedPayload1() < mixKey)
                {
                    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, false);
                    sentMSG << "Keyer Off";                
                }
                else
                {
                    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, true);
                    sentMSG << "Keyer On";
                    
                    int index = mixModeMenu.selectedPayload1() - mixKey;
                    ok = ok && setKeyParamsTo(index);
                    sentMSG << " with " << index;
                }

                if (ok) sentOK = "Sent:";
                else sentOK = "Send Error:";
                
                screen.clearBufferRow(kTVOneStatusLine);
                screen.textToBuffer(sentOK + sentMSG.str(), kTVOneStatusLine);
                
                if (debug) { debug->printf("Changing mix mode"); }
            }
            else if (selectedMenu == &resolutionMenu)
            {
                bool ok = true;
                
                ok =       tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsOutputResolution, resolutionMenu.selectedPayload1());
                ok = ok && tvOne.command(kTV1SourceRGB1, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, resolutionMenu.selectedPayload2());
                ok = ok && tvOne.command(kTV1SourceRGB2, kTV1WindowIDA, kTV1FunctionAdjustSourceEDID, resolutionMenu.selectedPayload2());
                
                std::string sentOK;
                if (ok) sentOK = "Sent: ";
                else sentOK = "Send Error: ";
                
                std::stringstream sentMSG;
                sentMSG << "Res " << resolutionMenu.selectedPayload1() << ", EDID " << resolutionMenu.selectedPayload2();
                
                screen.clearBufferRow(kTVOneStatusLine);
                screen.textToBuffer(sentOK + sentMSG.str(), kTVOneStatusLine);
                
                if (debug) { debug->printf("Changing resolution"); }
            }
            else if (selectedMenu == &commsMenu)
            {
                std::string commsTypeString = "Network: --";
                std::stringstream commsStatus;
            
                // Tear down any existing comms
                // This is the action of commsNone
                // And also clears the way for other comms actions
                if (osc)        {delete osc; osc = NULL;}  
                if (ethernet)   {delete ethernet; ethernet = NULL;}
                if (artNet)     {delete artNet; artNet = NULL;}
                if (dmx)        {delete dmx; dmx = NULL;}
                
                // Ensure we can't change to comms modes the hardware isn't switched to
                if (rj45Mode == rj45DMX && (commsMenu.selectedPayload1() == commsOSC || commsMenu.selectedPayload1() == commsArtNet))
                {
                    commsTypeString = "RJ45 not in Ethernet mode";
                }
                else if (rj45Mode == rj45Ethernet && (commsMenu.selectedPayload1() == commsDMXIn || commsMenu.selectedPayload1() == commsDMXOut))
                {
                    commsTypeString = "RJ45 not in DMX mode";
                }
                // Action!
                else if (commsMenu.selectedPayload1() == commsOSC) 
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
                        commsStatus << "Ethernet setup failed";
                        commsMenu = commsNone;
                        // break out of here. this setup should be a function that returns a boolean
                    }

                    osc = new OSCClass();
                    osc->setReceiveMessage(&recMessage);
                    osc->begin(kOSCMbedPort);
                    
                    commsStatus << "Listening on " << kOSCMbedPort;
                }
                else if (commsMenu.selectedPayload1() == commsArtNet) 
                {
                    commsMode = commsArtNet;
                    commsTypeString = "ArtNet: ";                    

                    artNet = new DmxArtNet();
                    
                    artNet->BindIpAddress = IpAddr(kArtNetBindIPAddress);
                    artNet->BCastAddress = IpAddr(kArtNetBroadcastAddress);
                
                    artNet->InitArtPollReplyDefaults();
                
                    artNet->ArtPollReply.PortType[0] = 128; // output
                    artNet->ArtPollReply.PortType[2] = 64; // input
                    artNet->ArtPollReply.GoodInput[2] = 4;
                
                    artNet->Init();
                    artNet->SendArtPollReply(); // announce to art-net nodes
                    
                    commsStatus << "Listening";
                }
                else if (commsMenu.selectedPayload1() == commsDMXIn) 
                {
                    commsMode = commsDMXIn;
                    commsTypeString = "DMX In: ";
                    
                    dmxDirectionDOUT = 0;
                    
                    dmx = new DMX(kMBED_RS485_TTLTX, kMBED_RS485_TTLRX);
                }
                else if (commsMenu.selectedPayload1() == commsDMXOut) 
                {
                    commsMode = commsDMXOut;
                    commsTypeString = "DMX Out: ";
                    
                    dmxDirectionDOUT = 1;
                    
                    dmx = new DMX(kMBED_RS485_TTLTX, kMBED_RS485_TTLRX);
                }
                                
                screen.clearBufferRow(kCommsStatusLine);
                screen.textToBuffer(commsTypeString + commsStatus.str(), kCommsStatusLine);
            }
            else if (selectedMenu == &advancedMenu)
            {
                if (advancedMenu.selectedPayload1() == advancedHDCPOff)
                {
                    bool ok = false;
                    
                    ok = tvOne.setHDCPOn(false);
                    
                    std::string sendOK = ok ? "Sent: HDCP Off" : "Send Error: HDCP Off";
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(sendOK, kTVOneStatusLine);
                }
                else if (advancedMenu.selectedPayload1() == advancedHDCPOn)
                {
                    bool ok = false;
                    
                    ok = tvOne.setHDCPOn(true);
                    
                    std::string sendOK = ok ? "Sent: HDCP On" : "Send Error: HDCP On";
                    
                    screen.clearBufferRow(kTVOneStatusLine);
                    screen.textToBuffer(sendOK, kTVOneStatusLine);
                }
                else if (advancedMenu.selectedPayload1() == advancedSelfTest)
                {
                    /* SELF TEST - Pixels
                     * Clicking ‘self-test’ menu will display a solid lit screen. Check all pixels lit. 
                     * Verified: Display
                     */
                    
                    screen.imageToBuffer(spkDisplayAllPixelsOn);
                    screen.sendBuffer();
                    
                    while(!menuEnc.hasPressed())
                    {
                        // do nothing, wait for press
                    }
                    
                    /* SELF TEST - Mixing Controls
                     * Clicking again will prompt to check crossfader, fade to black and tap buttons. Check movement of physical controls against 0.0-1.0 values on- screen. 
                     * Verified: Mixing controls.
                     */
                     
                    screen.clearBuffer();
                    screen.textToBuffer("Self test - Mixing Controls", 0);
  
                    while(!menuEnc.hasPressed())
                    {
                        stringstream xFadeReadOut; 
                        stringstream fadeToBlackReadOut;
                        stringstream tapsReadOut;
                        
                        xFadeReadOut.precision(2);
                        fadeToBlackReadOut.precision(2);
                        tapsReadOut.precision(1);
                        
                        xFadeReadOut << "Crossfade: " << xFadeAIN.read();
                        fadeToBlackReadOut << "Fade to black: " << fadeUpAIN.read();
                        tapsReadOut << "Tap left: " << tapLeftDIN.read() << " right: " << tapRightDIN.read();
                        
                        screen.clearBufferRow(1);
                        screen.clearBufferRow(2);
                        screen.clearBufferRow(3);
                        
                        screen.textToBuffer(xFadeReadOut.str(), 1);
                        screen.textToBuffer(fadeToBlackReadOut.str(), 2);
                        screen.textToBuffer(tapsReadOut.str(), 3);
                        screen.sendBuffer();
                    }
                    
                    /* SELF TEST - RS232
                     * Click the controller menu control. Should see ‘RS232 test’ prompt and test message. Ensure PC is displaying the test message. 
                     * Verified: RS232 connection.
                     */
                     
                    screen.clearBuffer();
                    screen.textToBuffer("Self test - RS232", 0);
                    screen.sendBuffer();
                    
                    while(!menuEnc.hasPressed())
                    {
                        screen.textToBuffer("TODO!", 1);
                        screen.sendBuffer();
                    }
                    
                    /* SELF TEST - DMX
                     * Click the controller menu control. Should see ‘DMX test’ prompt and test message. Ensure PC is displaying the test message. 
                     * Verified: RS485 connection and DMX library.
                     */
                     
                    screen.clearBuffer();
                    screen.textToBuffer("Self test - DMX", 0);
                    screen.sendBuffer();
                    
                    while(!menuEnc.hasPressed())
                    {
                        screen.textToBuffer("TODO!", 1);
                        screen.sendBuffer();
                    }
                    
                    /* SELF TEST - OSC
                     * Click the controller menu control. Should see ‘OSC test’ prompt and test message. Ensure PC is displaying the test message. 
                     * Verified: Ethernet connection and OSC library.
                     */
                     
                    screen.clearBuffer();
                    screen.textToBuffer("Self test - DMX", 0);
                    screen.sendBuffer();
                    
                    while(!menuEnc.hasPressed())
                    {
                        screen.textToBuffer("TODO!", 1);
                        screen.sendBuffer();
                    }

                    /* SELF TEST - Exit!
                     * To do this, we could just do nothing but we'd need to recreate screen and comms as they were. 
                     * Instead, lets just restart the mbed
                     */
                     
                    screen.clearBuffer();
                    screen.textToBuffer("Self test complete", 0);
                    screen.textToBuffer("Press to restart controller", 1);
                    screen.sendBuffer();
                    
                    while(!menuEnc.hasPressed()) {}                    
                    
                    mbed_reset();
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
        
        // We're going to cache the analog in reads, as have seen wierdness otherwise
        const float xFadeAINCached = 1-xFadeAIN.read();
        const float fadeUpAINCached = fadeUpAIN.read();
        
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
        else xFade = fadeCalc(xFadeAINCached, xFadeTolerance);

        fadeUp = 1.0 - fadeCalc(fadeUpAINCached, fadeUpTolerance);

        //// TASK: Process Network Comms
        if (commsMode == commsOSC)
        {
            if (osc->newMessage) 
            {
                osc->newMessage = false; // fixme!
                processOSC(xFade, fadeUp);
            }
        }

        if (commsMode == commsArtNet)
        {
            if (artNet->Work()) processArtNet(xFade, fadeUp);
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
            // we need to set fade level of both windows according to the fade curve profile (yet to implement - to do when tvone supply additive capability)
            newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
            newFadeBPercent = xFade * fadeUp * 100.0;
        }
        else if (mixMode >= mixKey)
        {
            newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
            newFadeBPercent = fadeUp * 100.0;
        }            
        
        // Send to TVOne if percents have changed
        // We want to send the higher first, otherwise black flashes can happen on taps
        if (newFadeAPercent != fadeAPercent && newFadeAPercent >= newFadeBPercent) 
        {
            fadeAPercent = newFadeAPercent;
            updateFade = true;
            
            fadeAPO = fadeAPercent / 100.0;
            tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeAPercent);
        }

        if (newFadeBPercent != fadeBPercent) 
        {
            fadeBPercent = newFadeBPercent;
            updateFade = true;
            
            fadeBPO = fadeBPercent / 100.0;
            tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeBPercent);
        }

        if (newFadeAPercent != fadeAPercent && newFadeAPercent < newFadeBPercent) 
        {
            fadeAPercent = newFadeAPercent;
            updateFade = true;
            
            fadeAPO = fadeAPercent / 100.0;
            tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeAPercent);
        }

        if (updateFade && debug) 
        {
            debug->printf("\r\n"); 
            //debug->printf("xFade = %3f   fadeUp = %3f \r\n", xFadeAIN.read(), fadeUpAIN.read());
            debug->printf("xFade = %3f   fadeUp = %3f \r\n", xFadeAINCached, fadeUpAINCached);
            debug->printf("xFade = %3f   fadeUp = %3f   fadeA% = %i   fadeB% = %i \r\n", xFade, fadeUp, fadeAPercent, fadeBPercent);
        }

    }
}
