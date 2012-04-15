// *SPARK D-FUSER
// A project by *spark audio-visual
//
// 'DJ' controller styke RS232 Control for TV-One products
// Good for 1T-C2-750, others will need some extra work
//
// Copyright *spark audio-visual 2009-2011
//
// v10 - Port to mBed, keying redux - Apr'11
// v11 - Sign callbacks, code clean-up - Apr'11
// v12 - TVOne header split into two: defines and mbed class. v002 header updates pulled down. Removed sign callbacks, rewrite of debug and signing. - Apr'11
// v13 - Menu system for Resolution + Keying implemented, it writing to debug, it sending TVOne commands - Apr'11
// v14 - Fixes for new PCB - Oct'11
// v15 - TBZ PCB, OLED - Mar'12
// v16 - Comms menu, OSC. There in theory: lots of trouble from EthernetNetIf. NetServices better. But still silently crashes on creation of EthernetNetIf, despite (now) ample memory and code tested elsewhere (inc OSC + spkOLED). 
// vxx - TODO: EDID upload from USB mass storage
// vxx - TODO: EDID creation from resolution

#include "mbed.h"

#include "spk_tvone_mbed.h"
#include "spk_utils.h"
#include "spk_mRotaryEncoder.h"
#include "spk_oled_ssd1305.h"
#include "spk_oled_gfx.h"
#include "EthernetNetIf.h"
#include "mbedOSC.h"
#include "DmxArtNet.h"

#include <sstream>

#define kMenuLine1 3
#define kMenuLine2 4
#define kCommsStatusLine 6
#define kTVOneStatusLine 7

#define kOSCMbedPort 10000
#define kOSCMbedIPAddress 10,0,0,2
#define kOSCMbedSubnetMask 255,255,255,0
#define kOSCMbedGateway 10,0,0,1
#define kOSCMbedDNS 10,0,0,1

#define kArtNetBindIPAddress 2,0,0,100
#define kArtNetBroadcastAddress 2,255,255,255

//// DEBUG

// Comment out one or the other...
Serial *debug = new Serial(USBTX, USBRX); // For debugging via USB serial
// Serial *debug = NULL; // For release (no debugging)

//// mBED PIN ASSIGNMENTS

// Inputs
AnalogIn xFadeAIN(p19);    
AnalogIn fadeUpAIN(p20);
DigitalIn tapLeftDIN(p24);
DigitalIn tapRightDIN(p21);

SPKRotaryEncoder menuEnc(p17, p16, p15);

// Outputs
PwmOut fadeAPO(LED1);
PwmOut fadeBPO(LED2);

// SPKTVOne(PinName txPin, PinName rxPin, PinName signWritePin, PinName signErrorPin, Serial *debugSerial)
SPKTVOne tvOne(p28, p27, LED3, LED4, debug);
//SPKTVOne tvOne(p28, p27, LED3, LED4);

// SPKDisplay(PinName mosi, PinName clk, PinName cs, PinName dc, PinName res, Serial *debugSerial = NULL);
SPKDisplay screen(p5, p7, p8, p10, p9, debug);

// Menu 

SPKMenu *selectedMenu;
SPKMenu *lastSelectedMenu;
SPKMenuOfMenus mainMenu;
SPKMenuPayload resolutionMenu;
SPKMenuPayload mixModeMenu;
SPKMenuPayload commsMenu;

// Comms Objects
EthernetNetIf *ethernet = NULL;
OSCClass *osc = NULL;
OSCMessage recMessage;
DmxArtNet *artNet = NULL;

// Fade logic constants
const float xFadeTolerance = 0.05;
const float fadeUpTolerance = 0.05;

// A&B Fade as resolved percent
int fadeAPercent = 0;
int fadeBPercent = 0;

// Tap button states
bool tapLeftPrevious = false;
bool tapRightPrevious = false;

// Key mode parameters
int keyerParamsSet = -1; // last keyParams index uploaded to unit 
// {lumakey, chroma on blue [, to be extended as needed] }
// {minY, maxY, minU, maxU, minV, maxV }
int keyerParams[2][6] = 
{
    {0, 18, 128, 129, 128, 129}, // lumakey
    {41, 42, 240, 241, 109, 110} // chroma on blue
    // ...
};

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

void processArtNet(float &xFade, float &fadeUp) {


    screen.clearBufferRow(kCommsStatusLine);
    screen.textToBuffer("ArtNet activity", kCommsStatusLine);
    screen.sendBuffer();
    if (debug) debug->printf("ArtNet activity");
}


inline float fadeCalc (const float AIN, const float tolerance) {
    float pos ;
    if (AIN < tolerance) pos = 0;
    else if (AIN > 1.0 - tolerance) pos = 1;
    else pos = (AIN - tolerance) / (1 - 2*tolerance);
    if (debug && false) debug->printf("fadeCalc in: %f out: %f \r\n", AIN, pos);
    return pos;
}

bool setKeyParamsTo(int index) {   
    // Only spend the time uploading six parameters if we need to
    // Might want to bounds check here
    
    bool ok = false;
    
    if (index != keyerParamsSet)
    {
        ok =       tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinY, keyerParams[index][0]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxY, keyerParams[index][1]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinU, keyerParams[index][2]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxU, keyerParams[index][3]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMinV, keyerParams[index][4]); 
        ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerMaxV, keyerParams[index][5]);
        
        keyerParamsSet = index;
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
    screen.textToBuffer("SW beta.15",1);
    
    // Set menu structure
    mixModeMenu.title = "Mix Mode";
    enum { blend, additive, lumaKey, chromaKey1, chromaKey2, chromaKey3 }; // additive will require custom TVOne firmware.
    mixModeMenu.addMenuItem("Blend", blend, 0);
    mixModeMenu.addMenuItem("LumaKey", lumaKey, 0);
    mixModeMenu.addMenuItem("ChromaKey - Blue", chromaKey1, 0);
 
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
    enum { commsNone, commsOSC, commsArtNet, commsDMX}; 
    commsMenu.addMenuItem("None", commsNone, 0);
    commsMenu.addMenuItem("OSC", commsOSC, 0);
    commsMenu.addMenuItem("ArtNet", commsArtNet, 0);
    commsMenu.addMenuItem("DMX", commsDMX, 0);

    mainMenu.title = "Main Menu";
    mainMenu.addMenuItem(&mixModeMenu);
    mainMenu.addMenuItem(&resolutionMenu);
    mainMenu.addMenuItem(&commsMenu);
    
    selectedMenu = &mainMenu;
    lastSelectedMenu = &mainMenu;    
    
    // Misc I/O stuff
    
    fadeAPO.period(0.001);
    fadeBPO.period(0.001);
    
    // TVOne setup
    
    bool ok = false;
    
    // horrid, horrid HDCP
    ok = tvOne.setHDCPOff();

    std::string sendOK = ok ? "Sent: HDCP Off" : "Send Error: HDCP Off";

    // display menu and framing lines
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
    screen.textToBuffer(sendOK, kTVOneStatusLine);
    screen.sendBuffer();


    //// CONTROLS TEST

    while (0) {
        if (debug) debug->printf("xFade: %f, fadeOut: %f, tapLeft %i, tapRight: %i encPos: %i encChange:%i encHasPressed:%i \r\n" , xFadeAIN.read(), fadeUpAIN.read(), tapLeftDIN.read(), tapRightDIN.read(), menuEnc.getPos(), menuEnc.getChange(), menuEnc.hasPressed());
    }

    //// MIXER RUN

    while (1) {

        //// Task background things
        if (commsMenu.selectedPayload1() == commsOSC || commsMenu.selectedPayload1() == commsArtNet)
        {
            Net::poll();
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
                bool ok = false;
                std::string sentOK;
                std::stringstream sentMSG;
            
                // Set keying parameters
                switch (mixModeMenu.selectedPayload1()) {
                case lumaKey:
                    ok = setKeyParamsTo(0);
                    sentMSG << "Keyer Params 0, ";
                    break;
                case chromaKey1:
                    ok = setKeyParamsTo(1);
                    sentMSG << "Keyer Params 1, ";
                    break;
                }

                // Set keying on or off
                switch (mixModeMenu.selectedPayload1()) {
                case blend:
                case additive:
                    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, false);
                    sentMSG << "Keyer Off";
                    break;
                case lumaKey:
                case chromaKey1:
                case chromaKey2:
                case chromaKey3:
                    ok = ok && tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustKeyerEnable, true);
                    sentMSG << "Keyer On";
                    break;
                }
                
                if (ok) sentOK = "Sent:";
                else sentOK = "Send Error:";
                
                screen.clearBufferRow(kTVOneStatusLine);
                screen.textToBuffer(sentOK + sentMSG.str(), kTVOneStatusLine);
                
                if (debug) { debug->printf("Changing mix mode"); }
            }
            else if (selectedMenu == &resolutionMenu)
            {
                bool ok = false;
                
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
                std::string commsType = "Network: --";
                std::stringstream commsStatus;
            
                // Tear down any existing comms
                // This is the action of commsNone
                // And also clears the way for other comms actions
                if (osc) {delete osc; osc = NULL;}  
                if (ethernet) {delete ethernet; ethernet = NULL;}
                if (artNet) {delete artNet; artNet = NULL;}

                if (commsMenu.selectedPayload1() == commsOSC) 
                {
                    commsType = "OSC: ";                    
                    
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
                        // commsMenu = commsNone; //FIXME: this should set the selected menu item to none, but errors. wtf?
                        // break out of here. this setup should be a function that returns a boolean
                    }

                    osc = new OSCClass();
                    osc->setReceiveMessage(&recMessage);
                    osc->begin(kOSCMbedPort);
                    
                    commsStatus << "Listening on " << kOSCMbedPort;
                }
                else if (commsMenu.selectedPayload1() == commsArtNet) 
                {
                    commsType = "ArtNet: ";                    

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
                else if (commsMenu.selectedPayload1() == commsDMX) 
                {
                    
                }
                
                screen.clearBufferRow(kCommsStatusLine);
                screen.textToBuffer(commsType + commsStatus.str(), kCommsStatusLine);
            }
            else
            {
                if (debug) { debug->printf("Warning: No action identified"); }
            }
        }
        
        // Send any updates to the display
        screen.sendBuffer();
       
        
        //// MIX MIX MIX MIX MIX MIX MIX MIX MIXMIX MIX MIXMIX MIX MIXMIX MIX MIXMIX MIX MIXMIX MIX MIX

        bool updateFade = false;
        float xFade = 0;
        float fadeUp = 1;
        
        //// TASK: Process control surface
        
        // Get new states of tap buttons, remembering at end of loop() assign these current values to the previous variables
        const bool tapLeft = (tapLeftDIN) ? false : true;
        const bool tapRight = (tapRightDIN) ? false : true;
        
        // We're going to cache the analog in reads, as have seen wierdness otherwise
        const float xFadeAINCached = xFadeAIN.read();
        const float fadeUpAINCached = fadeUpAIN.read();
        
        // When a tap is depressed, we can ignore any move of the crossfader but not fade to black
        if (tapLeft || tapRight) 
        {
            // If both are pressed, which was not pressed in the last loop?
            if (tapLeft && tapRight) 
            {
                if (!tapLeftPrevious) xFade = 0;
                if (!tapRightPrevious) xFade = 1;
            }
            // If just one is pressed, is this it going high or the other going low?
            else if (tapLeft && (!tapLeftPrevious || tapRightPrevious)) xFade = 0;
            else if (tapRight && (!tapRightPrevious || tapLeftPrevious)) xFade = 1;
        } 
        else xFade = fadeCalc(xFadeAINCached, xFadeTolerance);

        fadeUp = 1.0 - fadeCalc(fadeUpAINCached, fadeUpTolerance);

        //// TASK: Process Network Comms
        if (commsMenu.selectedPayload1() == commsOSC)
        {
            if (osc->newMessage) 
            {
                osc->newMessage = false; // fixme!
                processOSC(xFade, fadeUp);
            }
        }

        if (commsMenu.selectedPayload1() == commsArtNet)
        {
            if (artNet->Work()) processArtNet(xFade, fadeUp);
        }

        // WISH: Really, we should have B at 100% and A fading in over that, with fade to black implemented as a fade in black layer on top of that correct mix.
        // There is no way to implement that though, and the alphas get messy, so this is the only way (afaik).
        
        // Calculate new A&B fade percents
        int newFadeAPercent = 0;
        int newFadeBPercent = 0;

        switch (mixModeMenu.selectedPayload1()) {
        case blend:
        case additive: 
            newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
            newFadeBPercent = xFade * fadeUp * 100.0;
            break;
        case lumaKey:
        case chromaKey1:
        case chromaKey2:
        case chromaKey3:
            newFadeAPercent = (1.0-xFade) * fadeUp * 100.0;
            newFadeBPercent = fadeUp * 100.0;
            break;
        }            
        
        // Send to TVOne if percents have changed
        if (newFadeAPercent != fadeAPercent) {
            fadeAPercent = newFadeAPercent;
            updateFade = true;
            
            fadeAPO = fadeAPercent / 100.0;
            tvOne.command(0, kTV1WindowIDA, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeAPercent);
        }

        if (newFadeBPercent != fadeBPercent) {
            fadeBPercent = newFadeBPercent;
            updateFade = true;
            
            fadeBPO = fadeBPercent / 100.0;
            tvOne.command(0, kTV1WindowIDB, kTV1FunctionAdjustWindowsMaxFadeLevel, fadeBPercent);
        }

        if (updateFade && debug) {
            debug->printf("\r\n"); 
            //debug->printf("xFade = %3f   fadeUp = %3f \r\n", xFadeAIN.read(), fadeUpAIN.read());
            debug->printf("xFade = %3f   fadeUp = %3f \r\n", xFadeAINCached, fadeUpAINCached);
            debug->printf("xFade = %3f   fadeUp = %3f   fadeA% = %i   fadeB% = %i \r\n", xFade, fadeUp, fadeAPercent, fadeBPercent);
        }
        
        // END OF LOOP - Reset
        tapLeftPrevious = tapLeft;
        tapRightPrevious = tapRight;
    }
}
