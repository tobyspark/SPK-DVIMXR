// *spark audio-visual and v002
// RS232 Control for TV-One products
// Good for 1T-C2-750, others will need some extra work
// Copyright *spark audio-visual / v002 2009-2011

#ifndef SPKTVOne_h
#define SPKTVOne_h

// Comment out the following firmware defines as appropriate
#define kTV1Firmware362

// Sources - Note only higher end models have more than 2 in....
//#pragma mark -
//#pragma mark Channel / Sources

#define kTV1SourceRGB1                                0x10
#define kTV1SourceRGB2                                0x11    
#define kTV1SourceRGB3                                0x12 
#define kTV1SourceRGB4                                0x13 
#define kTV1SourceRGB5                                0x14 
#define kTV1SourceRGB6                                0x15 

// Window
//#pragma mark -
//#pragma mark Window IDs

#define kTV1WindowIDA                                'A' // aka 0x41 
#define kTV1WindowIDB                                'B'
#define kTV1WindowIDZ                                'Z'
#define kTV1WindowIDLogoA                            'a'    // aka 0x61
#define kTV1WindowIDLogoB                            'b'

//#pragma mark -
//#pragma mark Functions

// Complete function list for verbosity. We will probably only be using a small small subset. Useful to have in one place.

// Preset & Mixer Mode Functions
#define kTV1FunctionMode                            0x109    // Values: 0 = Switcher, 1 = Independant, 2 = Dual PIP
#define kTV1FunctionPreset                            0x225    // Values: 0 - 9 (Preset 1 to 10) Set the current preset for the following functions.     
#define kTV1FunctionPresetLoad                        0x226    // Values: 1 to Load - switches back to 0 after a load
#define kTV1FunctionPresetStore                        0x227    // Values: 1 to Store - switches back to 0 after a store
#define kTV1FunctionPresetErase                        0x228    // Values: 1 to Erase - switches back to 0 after erase

// Adjust output Functions
//#pragma mark -
#define kTV1FunctionAdjustOutputsOutputEnable        0x170    // Values: 0 = Blanked 1 = Active
#define kTV1FunctionAdjustOutputsLockSource            0x149    // Values: 0x10 to 0x1F = RGB1 - RGB 16 (see manual for other values corresponding to SDI, CV etc)
#define kTV1FunctionAdjustOutputsLockMethod            0x10A    // Values: 0 = Off, 1 = Genlock, 2 = Lock & Mix (see manual for other values)
#define kTV1FunctionAdjustOutputsLockHShift            0x14A    // Values: -4096 - 4096
#define kTV1FunctionAdjustOutputsLockVShift            0x14B    // Values: -4096 - 4096
#define kTV1FunctionAdjustOutputsOutputResolution    0x083    // Values: 0 - 1000 (which resolution, "pragma mark" Resolutions for list.
#define kTV1FunctionAdjustOutputsOutputImageTypeA    0x0E2    // Values: 0 = RGBHV, 2 = RGBsB, 3 = YUV, 4 = tlYUV, 7 = tlRGB (Analog)
#define kTV1FunctionAdjustOutputsOutputImageTypeD    0x16C    // Values: 0 = RGBHV, 3 = YUV, 9 = Not Available (?) (Digital)
#define kTV1FunctionAdjustOutputsHDCPRequired        0x233    // Values: 0 = Off, 1 = On (if display supports it)
#define kTV1FunctionAdjustOutputsHDCPStatus            0x234    // Values: 0 = Unavailable, 1 = Supported, 2 = Active, 3 = Repeater Supported, 4 = Repeater Active
#define kTV1FunctionAdjustOutputsBackgroundY        0x13B    // Values: 16-235
#define kTV1FunctionAdjustOutputsBackgroundU        0x13C    // Values: 16-235
#define kTV1FunctionAdjustOutputsBackgroundV        0x13D    // Values: 16-235
#define kTV1FunctionAdjustOutputsSDIOptimization    0x197    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustOutputsOutputStandard        0x101    // Values: 0 = NTSC/PAL, 1 = PAL-M / PAL-N, 2 = SECAM
#define kTV1FunctionAdjustOutputsCVYCIRE            0x133    // Values: -0.75 - 12.5
#define kTV1FunctionAdjustOutputsCVYCHue            0x139    // Values: -22 - 22
#define kTV1FunctionAdjustOutputsSCHPhase            0x085    // Values: -180 - 180 
#define kTV1FunctionAdjustOutputsLumaBandwidth        0x134    // Values: 0 = Low, 1 = Medium, 2 = High
#define kTV1FunctionAdjustOutputsChromaBandwidth    0x135    // Values: 0 = Low, 1 = Medium, 2 = High
#define kTV1FunctionAdjustOutputsOutputChromaDelay    0x137    // Values: -4 - 3
#define kTV1FunctionAdjustOutputsPalWSS                0x130    // Values: 0-8 (see manual for value explanation) 
#define kTV1FunctionAdjustOutputsTake                0x11E    // Values: 0->1 Perform a Preview -> Program transition (?)
#define kTV1FunctionAdjustOutputsVolume                0x201    // Values: -16 - 15 (-16 = Mute)
// Left out additional functions specific to SDI Audio Channel adjustment

// Adjust Windows Functions -  Note for window functions, you must specify a Window to work on via the below value
//#pragma mark -

#define kTV1FunctionAdjustWindowsWindowSource        0x82    // Values: 0x10 to 0x1F = RGB1 to RGB16
#define kTV1FunctionAdjustWindowsSelectUniSource    0x241    // Values: 0xE0 to OxEF for universal source 1 - 16
#define kTV1FunctionAdjustWindowsSourceResolution    0xF8    // Values: (Read only) Resolution #
#define kTV1FunctionAdjustWindowsEnable                0x12B    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustWindowsZoomLevel            0x86    // Values: 100 - 1000
#define kTV1FunctionAdjustWindowsZoomLevelH            0x103    // Values: 100 - 1000 (only if advanced aspect ratio mode is enabled)
#define kTV1FunctionAdjustWindowsZoomLevelV            0x105    // Values: 100 - 1000 (only if advanced aspect ratio mode is enabled)
#define kTV1FunctionAdjustWindowsAspectRationIn        0x107    // Values: 0.1:1 - 9.99:1 (read only)
#define kTV1FunctionAdjustWindowsZoomPanH            0x9F    // Values: 0 - 100
#define kTV1FunctionAdjustWindowsZoomPanV            0xA0    // Values: 0 - 100
#define kTV1FunctionAdjustWindowsImageFreeze        0x9C    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustWindowsCropH                0x223    // Values: 0 - 100
#define kTV1FunctionAdjustWindowsCropV                0x224    // Values: 0 - 100
#define kTV1FunctionAdjustWindowsOutShiftH            0xAD    // Values: -4096 - 4096
#define kTV1FunctionAdjustWindowsOutShiftV            0xAE    // Values: -4096 - 4096
#define kTV1FunctionAdjustWindowsShrinkLevel        0x87    // Values: 10 - 100
#define kTV1FunctionAdjustWindowsShrinkLevelH        0x104    // Values: 10 - 100 (only if advanced aspect ratio mode is enabled)
#define kTV1FunctionAdjustWindowsShrinkLevelV        0x106    // Values: 10 - 100 (only if advanced aspect ratio mode is enabled)
#define kTV1FunctionAdjustWindowsShrinkEnable        0x18E    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustWindowsShrinkPosH            0xDA    // Values: 0 - 100
#define kTV1FunctionAdjustWindowsShrinkPosV            0xDB    // Values: 0 - 100
#define kTV1FunctionAdjustWindowsInTopLeftH            0x21B    // ??
#define kTV1FunctionAdjustWindowsInTopLeftV            0x21D    // ??
#define kTV1FunctionAdjustWindowsInSizeH            0x21C    // ??
#define kTV1FunctionAdjustWindowsInSizeV            0x21E    // ??
#define kTV1FunctionAdjustWindowsOutTopLeftH        0x21F    // ??
#define kTV1FunctionAdjustWindowsOutTopLeftV        0x221    // ??
#define kTV1FunctionAdjustWindowsOutSizeH            0x220    // ??
#define kTV1FunctionAdjustWindowsOutTopLeft            0x222    // ??
#define kTV1FunctionAdjustWindowsAspectChange        0x190    // Values: 0 = Normal, 1 = Letterbox, 2 = PillarBox
#define kTV1FunctionAdjustWindowsAspectAdjust        0x102    // Values: 0 = Simple, 1 = Advanced
#define kTV1FunctionAdjustWindowsFlickerReduction    0x92    // Values: 0 = Off, 1 = Low, 2 = Medium, 3 = High
#define kTV1FunctionAdjustWindowsImageSmoothing        0xA1    // Values: 0 = Off, 1 = Medium, 2 = High
#define kTV1FunctionAdjustWindowsImageFlip            0x95    // Values: 0 = Off, 1 = Horizontal, 2 = Vertical, 3 = H & V
#define kTV1FunctionAdjustWindowsTemporalInterp        0x229    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustWindowsMaxFadeLevel        0x10F    // Values: 0 - 100
#define kTV1FunctionAdjustWindowsFadeOutIn            0x193    // Values -1 = Fade Out, 0 = No Action, 1 = Fade In
#define kTV1FunctionAdjustWindowsLayerPriority        0x144    // Values: 0 - 5
#define kTV1FunctionAdjustWindowsHeadPhonVolume        0xFD    // Vaules: -16 - 15 (-16 = Mute)
#define kTV1FunctionAdjustWindowsAudioVolume        0x206    // Vaules: -128 - 127 (
#define kTV1FunctionAdjustWindowsAudioVolumeEnable    0x206    // Vaules: 0 = Off, 1 = On

// Adjust Keyer
//#pragma mark -

#define kTV1FunctionAdjustKeyerEnable                0x127    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustKeyerMinY                    0xAF    // Values: 0 - 255        
#define kTV1FunctionAdjustKeyerMinU                    0xB0    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerMinV                    0xB1    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerMaxY                    0xB2    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerMaxU                    0xB3    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerMaxV                    0xB4    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerSoftnessY            0x121    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerSoftnessU            0x123    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerSoftnessV            0x125    // Values: 0 - 255
#define kTV1FunctionAdjustKeyerInvertY                0x122    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustKeyerInvertU                0x124    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustKeyerInvertV                0x126    // Values: 0 = Off, 1 = On (Typo in documentation, noted as 0x156) 
#define kTV1FunctionAdjustKeyerSwap                    0x144    // Values: 0 = Off, 1 = On

// Edge Blending (not used in TVOne 1T-C2-750, therefore not bothered with for now)
//#define kTV1FunctionAdjustEdgeBlendXXX

// Adjust Logos
//#pragma mark -

#define kTV1FunctionAdjustLogoEnable                0x12B    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustLogoNumber                0x143    // Values: 0 - 9 (Logo Selection)
#define kTV1FunctionAdjustLogoOutShiftH                0xAD    // Values: 0 - 100
#define kTV1FunctionAdjustLogoOutShiftV                0xAE    // Values: 0 - 100
#define kTV1FunctionAdjustLogoMaxFadeLevel            0x10F    // Values: 0 - 100
#define kTV1FunctionAdjustLogoLayerPriority            0x144    // Values: 0 - 5

// Adjust Borders
//#pragma mark -

#define kTV1FunctionAdjustBorderEnable                0x150    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustBorderSizeH                0x151    // Values: 0 - 99
#define kTV1FunctionAdjustBorderSizeV                0x152    // Values: 0 - 99
#define kTV1FunctionAdjustBorderOffsetH                0x153    // Values: 0 - 99
#define kTV1FunctionAdjustBorderOffsetV                0x154    // Values: 0 - 99
#define kTV1FunctionAdjustBorderY                    0x155    // Values: 16 - 235
#define kTV1FunctionAdjustBorderU                    0x156    // Values: 16 - 240    
#define kTV1FunctionAdjustBorderV                    0x157    // Values: 16 - 240
#define kTV1FunctionAdjustBorderOpacity                0x158    // Values: 0 - 100 ( 0 = Transparent, 100 = Opaque)

// Adjust Sources - Note: These functions require the Channel parameter to be properly set. Not all functions take all channels.
//#pragma mark -

#define kTV1FunctionAdjustSourceTestCard            0xDC    // Values: 0 - 10
#define kTV1FunctionAdjustSourceAutoSet                0xFE    // Values: 1 = Start AutoSet Procedure
#define kTV1FunctionAdjustSourceAspectCorrect        0x240    // Values: 0 = Fill, 1 = Aspect, 2 = H-Fit, 3 = V-Fit, 4 = 1:1
#define kTV1FunctionAdjustSourceEDID                0x243    // Values: 0 - 7 (edid entry number)
#define kTV1FunctionAdjustSourceEDIDCapureID        0x244    // Values: 0 - 7 (edid entry number) - entry to grab into
#define kTV1FunctionAdjustSourceEditCaptureGrab        0x245    // Values: 1 performs grab of Edid to currently set EDIDCapureID
#define kTV1FunctionAdjustSourceHDCPAdvertize        0x237    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustSourceHDCPStatus            0x238    // Values: 0 = Inactive, 1 = Active (read only)
#define kTV1FunctionAdjustSourcePositionH            0xB6    // Values: -100 - 100
#define kTV1FunctionAdjustSourcePositionV            0xB7    // Values: -100 - 100
#define kTV1FunctionAdjustSourceSizeH                0xDE    // Values: -100 - 100
#define kTV1FunctionAdjustSourceSizeV                0xDF    // Values: -100 - 100
#define kTV1FunctionAdjustSourceAudioXXX                    // Ignored for now In source, Option IN source Volume, Balance 
#define kTV1FunctionAdjustSourceOnSourceLoss        0xA3    // Values: 0 = Show, 1 = Freeze, 2 = Blue, 3 = Black, 4 = Remove
#define kTV1FunctionAdjustSourceSourceStable        0x22A    // Values: 0 = Unstable, 1 = Stable
#define kTV1FunctionAdjustSourcePixelPhase            0x91    // Values: 0 - 31
#define kTV1FunctionAdjustSourceRGBInType            0xC1    // Values: 0 = Auto, 1 = D-RGB, 2 = D-YUV, 3 = A-RGB, 4 = A-YUV (Digital / Analog)
#define kTV1FunctionAdjustSourceRGBContributionR    0xC5    // Values: 75 - 150
#define kTV1FunctionAdjustSourceRGBContributionG    0xC6    // Values: 75 - 150    
#define kTV1FunctionAdjustSourceRGBContributionB    0xC7    // Values: 75 - 150
#define kTV1FunctionAdjustSourceYUVSetup            0x23E    // Values: 0 = 0 IRE, 1 = 7.5 IRE
#define kTV1FunctionAdjustSourceDeInterlace            0xB8    // Values: 0 = Normal, 1 = Auto, 2 = Film 3:2, 3 = Motion Compensation Low, 4 = Motion Compensation Medium, 5 = Motion Compensation High, 6 = Frame / Bob
#define kTV1FunctionAdjustSourceFilmMode            0xE3    // Values: 0 = Not Detected 1 = Detected (read only) 
#define kTV1FunctionAdjustSourceDiagonalInterp        0x22B    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustSourceNoiseReduction        0x23F    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustSourceBrightness            0xBB    // Values: 0 - 180    
#define kTV1FunctionAdjustSourceContrast            0xBC    // Values: 0 - 180
#define kTV1FunctionAdjustSourceSaturation            0xB9    // Values: 0 - 180
#define kTV1FunctionAdjustSourceHue                    0xBA    // Values: -180 - 180
#define kTV1FunctionAdjustSourceSharpness            0x80    // Values: -7 - 7
#define kTV1FunctionAdjustSourceLumaDelay            0xBD    // Values: -4 -3
#define kTV1FunctionAdjustSourceFieldSwap            0xC9    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustSourceFieldOffset            0x196    // Values: = 0 - 7
#define kTV1FunctionAdjustSourceAudioChannel1                // Ignored for now
#define kTV1FunctionAdjustSourceAudioChannel2                // Ignored for now
#define kTV1FunctionAdjustSourceAudioChannel3                // Ignored for now
#define kTV1FunctionAdjustSourceAudioChannel4                // Ignored for now
#define kTV1FunctionAdjustSourceAudioChannel5                // Ignored for now
#define kTV1FunctionAdjustSourceAudioChannel6                // Ignored for now
#define kTV1FunctionAdjustSourceAudioChannel7                // Ignored for now

// Adjust Audio
//#pragma mark -
#define kTV1FunctionAdjustAudioXXX                            // Ignored for now

// Adjust Transitions
//#pragma mark -
#define kTV1FunctionAdjustTransitionType            0x112    // Values: 0 = Cut, 1 = Fade, 2 = Wipe, 3 = Push
#define kTV1FunctionAdjustTransitionFadeTime        0xF5    // Values: 0 - 50 (0 - 5.0 seconds)
#define kTV1FunctionAdjustTransitionWipeType        0x145    // Values: 0 = Left -> Right 1 = Right -> Left, 2 = Up -> Down, 3 = Down -> Up, 4 = Diagonal, 5 = Diamond 
#define kTV1FunctionAdjustTransitionWipeSize        0x146    // Values: 10 - 2000

// Adjust Resolutions

/* 
 Note: You MUST set the 'Image to adjust' value to the correct value first, 
 and only then change the other values - otherwise you may be adjusting the wrong entry. 
 The user should not adjust the 'Image to adjust' entry using the front panel whilst also accessing it via RS232
 */

//#pragma mark -
#define kTV1FunctionAdjustResolutionImageToAdjust    0x81    // Values: 0 - 1000 - the preset you wish to manipulate
#define kTV1FunctionAdjustResolutionInterlaced        0xCA    // Values: 0 = Off, 1 = On
#define kTV1FunctionAdjustResolutionFreqCoarseH        0xBE    // Values: 10,000 - 200,000
#define kTV1FunctionAdjustResolutionFreqFineH        0xBF    // Values: 10,000 - 200,000
#define kTV1FunctionAdjustResolutionActiveH            0x96    // Values: 64 - 2047
#define kTV1FunctionAdjustResolutionActiveV            0x97    // Values: 64 - 2047
#define kTV1FunctionAdjustResolutionStartH            0x8B    // Values: 0 - 1023
#define kTV1FunctionAdjustResolutionStartV            0x8C    // Values: 0 - 1023
#define kTV1FunctionAdjustResolutionCLKS            0x8E    // Values: 64 - 4095
#define kTV1FunctionAdjustResolutionLines            0xBE    // Values: 64 - 2047
#define kTV1FunctionAdjustResolutionSyncH            0x8F    // Values: 8 - 1023
#define kTV1FunctionAdjustResolutionSyncV            0x90    // Values: 1 - 1023
#define kTV1FunctionAdjustResolutionSyncPolarity    0x94    // Values: 0 - 3 (++, +-. -+. --)

// Adjust Misc
//#pragma mark -
#define kTV1FunctionAdjustFrontPanelLock            0xFC    // Values: 0 = Unlocked, 1 = Locked

// Resolutions
//#pragma mark -
//#pragma mark Resolutions

// Selected Common Resolutions and resolution Descriptions. For a complete list, use the menu system in your TV1

// Res #27 640x480, 59.97Hz
// Res #28 640x480, 60Hz
// Res #99 1920x1080, 60Hz
// Res #104 = 1920x1200, 60Hz

#define kTV1ResolutionVGA                    0x8            
#define kTV1ResolutionNTSC                    0xF                    
#define kTV1ResolutionPAL                    0x10            
#define kTV1ResolutionSVGA                    0x12        
#define kTV1ResolutionXGAp5994                0x1B                    
#define kTV1ResolutionXGAp60                0x1C                    
#define kTV1ResolutionXGAp75                0x1D                     
#define kTV1Resolution720p2398                0x27                
#define kTV1Resolution720p24                0x28            
#define kTV1Resolution720p25                0x29            
#define kTV1Resolution720p2997                0x2A                
#define kTV1Resolution720p30                0x2B            
#define kTV1Resolution720p50                0x2C            
#define kTV1Resolution720p5994                0x2D            
#define kTV1Resolution720p60                0x2E            
#define kTV1ResolutionWXGA5by3p60            0x30                         
#define kTV1ResolutionWXGA5by3p75            0x31                         
#define kTV1ResolutionWXGA16by10p60            0x34                         
#define kTV1ResolutionWXGA16by10p75            0x35         
#define kTV1ResolutionSGAp60                0x3A                    
#define kTV1ResolutionSGAp75                0x3B        
#define kTV1ResolutionWSXGAp60                0x40        
#define kTV1ResolutionUXGAp60                0x47                    
#define kTV1ResolutionUXGAp75                0x4A                    
#define kTV1ResolutionUXGAp85                0x4B
#define kTV1ResolutionWSXGAPLUSp60     0x53

#ifndef kTV1Firmware362
 #define kTV1Resolution1080p60                0x6D
 #define kTV1Resolution1080p75                0x66        
 #define kTV1ResolutionWUXGAp60                0x69                        
 #define kTV1ResolutionWUXGAp75                0x6C                        
 #define kTV1ResolutionWUXGAp85                0x6D
#else
 #define kTV1Resolution1080p2398                0x60            
 #define kTV1Resolution1080p24                0x62              
 #define kTV1Resolution1080p25                0x64            
 #define kTV1Resolution1080p2997                0x65         
 #define kTV1Resolution1080p30                0x66        
 #define kTV1Resolution1080p50                0x67        
 #define kTV1Resolution1080p5996                0x69        
 #define kTV1Resolution1080p60                0x6A                
 #define kTV1Resolution1080p75                0x6D        
 #define kTV1ResolutionWUXGAp60                0x70                        
 #define kTV1ResolutionWUXGAp75                0x73                     
 #define kTV1ResolutionWUXGAp85                0x74
#endif

#define kTV1ResolutionDescriptionVGA                "VGA (640x480) @ 60Hz"
#define kTV1ResolutionDescriptionNTSC                "NTSC (720x480 @ 59.95Hz)"                        
#define kTV1ResolutionDescriptionPAL                "PAL (720x576 @ 50Hz)"    
#define kTV1ResolutionDescriptionSVGA                "SVGA (800x600) @ 60Hz"    
#define kTV1ResolutionDescriptionXGAp5994            "XGA (1024x768) @ 59.94Hz"                        
#define kTV1ResolutionDescriptionXGAp60                "XGA (1024x768) @ 60Hz"                            
#define kTV1ResolutionDescriptionXGAp75                "XGA (1024x768) @ 75Hz"                            
#define kTV1ResolutionDescription720p2398            "720p HD (1280x720) @ 23.98Hz"                    
#define kTV1ResolutionDescription720p24                "720p HD (1280x720) @ 24Hz"                        
#define kTV1ResolutionDescription720p25                "720p HD (1280x720) @ 25Hz"                        
#define kTV1ResolutionDescription720p2997            "720p HD (1280x720) @ 29.97Hz"                    
#define kTV1ResolutionDescription720p30                "720p HD (1280x720) @ 30Hz"                        
#define kTV1ResolutionDescription720p50                "720p HD (1280x720) @ 50Hz"                        
#define kTV1ResolutionDescription720p5994            "720p HD (1280x720) @ 59.95Hz"                     
#define kTV1ResolutionDescription720p60                "720p HD (1280x720) @ 60Hz"                        
#define kTV1ResolutionDescriptionWXGA5by3p60        "WXGA (1280x768) @ 60Hz"                        
#define kTV1ResolutionDescriptionWXGA5by3p75        "WXGA (1280x768) @ 75Hz"                        
#define kTV1ResolutionDescriptionWXGA16by10p60        "WXGA (1280x800) @ 60Hz"                        
#define kTV1ResolutionDescriptionWXGA16by10p75        "WXGA (1280x800) @ 75Hz"        
#define kTV1ResolutionDescriptionSGAp60                "SGA (1280x1024) @ 60Hz"                         
#define kTV1ResolutionDescriptionSGAp75                "SGA (1280x1024) @ 75Hz"         
#define kTV1ResolutionDescriptionWSXGAp60            "WSXGA (1440x900) @ 60Hz"
#define kTV1ResolutionDescriptionUXGAp60            "UXGA (1600x1200) @ 60Hz"                         
#define kTV1ResolutionDescriptionUXGAp75            "UXGA (1600x1200) @ 75Hz"                         
#define kTV1ResolutionDescriptionUXGAp85            "UXGA (1600x1200) @ 85Hz"
#define kTV1ResolutionDescriptionWSXGAPLUSp60     "WSXGA+ (1680x1050) @ 60Hz"
#define kTV1ResolutionDescription1080p2398        "1080p (1920x1080) @ 23.98Hz"                     
#define kTV1ResolutionDescription1080p24            "1080p (1920x1080) @ 24Hz"                        
#define kTV1ResolutionDescription1080p25            "1080p (1920x1080) @ 25Hz"                        
#define kTV1ResolutionDescription1080p2997        "1080p (1920x1080) @ 29.97Hz"                    
#define kTV1ResolutionDescription1080p30            "1080p (1920x1080) @ 30Hz"                        
#define kTV1ResolutionDescription1080p50            "1080p (1920x1080) @ 50Hz"                        
#define kTV1ResolutionDescription1080p5996        "1080p (1920x1080) @ 59.94Hz"
#define kTV1ResolutionDescription1080p60            "1080p (1920x1080) @ 60Hz"                        
#define kTV1ResolutionDescription1080p75      "1080p (1920x1080) @ 75Hz"    
#define kTV1ResolutionDescriptionWUXGAp60            "WUXGA (1920x1200) @ 60Hz"                        
#define kTV1ResolutionDescriptionWUXGAp75            "WUXGA (1920x1200) @ 75Hz"                        
#define kTV1ResolutionDescriptionWUXGAp85            "WUXGA (1920x1200) @ 85Hz"    

#define kTV1Resolution2Kp60                0x71        
#define kTV1ResolutionDescription2Kp60                "2K (2048x1080) @ 60Hz"    
#define kTV1ResolutionDoubleWXGA            0x73        
#define kTV1ResolutionDescriptionDoubleWXGA            "DWXGA (2880x900) @ 60Hz"                                        

// Triplehead resolutions
// Note these are currently either added to the resolution list (#123-125) via CorioTools
// Previously these were written over existing resolutions (#112-114)

#define kTV1ResolutionDualHeadSVGAp60        0x7B        
#define kTV1ResolutionDescriptionDualHeadSVGAp60    "Dual Head SVGA (1600x600) @ 60Hz"        

#define kTV1ResolutionDualHeadXGAp60    0x7C
#define kTV1ResolutionDescriptionDualHeadXGAp60   "Dual Head XGA (2048x768) @ 60 Hz"

#define kTV1ResolutionTripleHeadVGAp60        0x7D        
#define kTV1ResolutionDescriptionTripleHeadVGAp60    "Triple Head VGA (1920x480) @ 60Hz"    

#endif
