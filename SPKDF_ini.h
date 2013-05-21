##########################################################################
#  _____ _____ _____ _____ _____    ____  _____ _____ _____ _____ _____  #
# |   __|  _  |  _  | __  |  |  |  |    \|   __|  |  |   __|   __| __  | #
# |__   |   __|     |    -|    -|  |  |  |   __|  |  |__   |   __|    -| #
# |_____|__|  |__|__|__|__|__|__|  |____/|__|  |_____|_____|_____|__|__| #
#                                                                        #
######################################### A PROJECT BY TOBY HARRIS #######

### KEYS
#
# Name = What is shown in menu
# MinY...MaxV = As per TVOne keyer settings. 
# Note these are super sensitive, one laptop will have slightly different 
#  numbers to another to achieve otherwise the same -- ie. pure blue -- key.
#
# Keying advice from the 1T-C2-750 Manual: The Min/Max parameters are used to 
# select what range of Y (luminance/grey-scale) values are made transparent 
# within the selected window/lock source. In order to key out part of an image, 
# start with the max value and increase it until the required lighter parts 
# within the window/lock source disappear. Then adjust the min level to bring 
# back any darker parts of the image.

[Key1]
Name = Lumakey
MinY = 0
MaxY = 18
MinU = 128
MaxU = 129
MinV = 128
MaxV = 129

[Key2]
Name = Chromakey
MinY = 30
MaxY = 35
MinU = 237
MaxU = 242
MinV = 114
MaxV = 121

# Edit the above, or add your own keys here, up to Key99

### RESOLUTIONS
#
# Name = What is shown in menu
# Number = Resolution number in TVOne. ie. what would be set in Menu > Outputs > Set Resolution
# EDID Number = The EDID to use on the inputs, which what your computer will think its connected to. ie. what would be set in Menu > Windows > Display Emul. EDID
#
# EDID numbers are as follows
# 0 = Mem1, 1 = Mem2, 2 = Mem3, 
# 3 = Mem4 which we use for Matrox EDID, its uploaded by the controller as part of 'Conform Processor'
# 4 = 3D, 5 = HDMI, 6 = DVI, 7 = Monitor Passthrough

[Resolution1]
Name = VGA (640x480)
Number = 8
EDIDNumber = 6

[Resolution2]
Name = SVGA (800x600)
Number = 18
EDIDNumber = 6

[Resolution3]
Name = XGA (1024x768)
Number = 28
EDIDNumber = 6

[Resolution4]
Name = WSXGA+ (1650x1050)
Number = 85
EDIDNumber = 6

[Resolution5]
Name = WUXGA (1920x1200)
Number = 115
EDIDNumber = 6

[Resolution6]
Name = HD 720P24 (1280x720)
Number = 40
EDIDNumber = 5

[Resolution7]
Name = HD 720P50 (1280x720)
Number = 44
EDIDNumber = 5

[Resolution8]
Name = HD 720P60 (1280x720)
Number = 48
EDIDNumber = 5

[Resolution9]
Name = HD 1080P24 (1920x1080)
Number = 101
EDIDNumber = 5

[Resolution10]
Name = HD 1080P50 (1920x1080)
Number = 106
EDIDNumber = 5

[Resolution11]
Name = HD 1080P60 (1920x1080)
Number = 109
EDIDNumber = 5

[Resolution12]
Name = Dual head SVGA (1600x600)
Number = 75
EDIDNumber = 3

[Resolution13]
Name = Dual head XGA (2048x768)
Number = 123
EDIDNumber = 3

[Resolution14]
Name = Triple head VGA (1920x480)
Number = 90
EDIDNumber = 3

# Edit the above, or add your own keys here, up to Resolution99

# End of SPKDF.ini -- Ensure there is a blank line below this.
