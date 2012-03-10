// THIS IS AS PER spk_dvimxr_v07 AND IS OUT OF DATE, NEEDS APHEX TWIN FIXES

// *spark audio-visual
// RS232 Control for TV-One products
// Good for 1T-C2-750, others will need some extra work
// Copyright *spark audio-visual 2009-2010

#include "spk_tvone_mbed.h"
#include "mbed.h"

SPKTVOne::SPKTVOne(PinName txPin, PinName rxPin, PinName signWritePin, PinName signErrorPin, Serial *debugSerial)
{
    // Create Serial connection for TVOne unit comms
    // Creating our own as this is exclusively for TVOne comms
    serial = new Serial(txPin, rxPin);
    serial->baud(57600);
    
    if (signWritePin != NC) writeDO = new DigitalOut(signWritePin);
    else writeDO = NULL;
    
    if (signErrorPin != NC) errorDO = new DigitalOut(signErrorPin);
    else errorDO = NULL;
    
    // Link up debug Serial object
    // Passing in shared object as debugging is shared between all DVI mixer functions
    debug = debugSerial;
}

bool SPKTVOne::command(uint8_t channel, uint8_t window, int32_t func, int32_t payload) 
{
  char i;
  
  // TASK: Sign start of serial command write
  if (writeDO) *writeDO = 1;
  
  // TASK: discard anything waiting to be read
  while (serial->readable()) {
    serial->getc();
  }
  
  // TASK: Create the bytes of command

  uint8_t cmd[8];
  uint8_t checksum = 0;

  // CMD
  cmd[0] = 1<<2; // write
  // CHA
  cmd[1] = channel;
  // WINDOW
  cmd[2] = window;
  // OUTPUT & FUNCTION
  //            cmd[3]  cmd[4]
  // output 0 = 0000xxx xxxxxxx
  // function = xxxXXXX XXXXXXX
  cmd[3] = func >> 8;
  cmd[4] = func & 0xFF;
  // PAYLOAD
  cmd[5] = (payload >> 16) & 0xFF;
  cmd[6] = (payload >> 8) & 0xFF;
  cmd[7] = payload & 0xFF;

  // TASK: Write the bytes of command to RS232 as correctly packaged 20 characters of ASCII

  for (i=0; i<8; i++) 
  {
    checksum += cmd[i];
  }
  
  serial->printf("F%02X%02X%02X%02X%02X%02X%02X%02X%02X\r", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], checksum);
  
  // TASK: Check the unit's return string, to enable return to main program as soon as unit is ready

  // Handling the timing of this return is critical to effective control.
  // Returning the instant something is received back overloads the processor, as does anything until the full 20 char acknowledgement.
  // TVOne turn out to say that receipt of the ack doesn't guarantee the unit is ready for the next command. 
  // According to the manual, operations typically take 30ms, and to simplify programming you can throttle commands to every 100ms.
  // 100ms is too slow for us. Going with returning after 30ms if we've received an acknowledgement, returning after 100ms otherwise.

  int ack[20];
  int safePeriod = 100;
  int clearPeriod = 30;
  bool ackReceived = false;
  bool success = false;
  Timer timer;

  timer.start();
  i = 0;
  while (timer.read_ms() < safePeriod) {
    if (serial->readable())
        {
            ack[i] = serial->getc();
            i++;
            if (i >= 20) 
            {
                ackReceived = true;
                if (ack[0] == 'F' && ack[1] == '4') // TVOne start of message, acknowledgement with no error, rest will be repeat of sent command
                {
                    success = true;
                }
            }
        }
    if (ackReceived && (timer.read_ms() > clearPeriod)) break;
  }
  timer.stop();
  
  // TASK: Sign end of write
  
  if (writeDO) *writeDO = 0;
  
  if (!success) {
        if (errorDO) {
            signErrorTimeout.detach();
            signErrorTimeout.attach(this, &SPKTVOne::signErrorOff, 0.25);
            *errorDO = 1;
        }
        
        if (debug) {
            debug->printf("Serial command write error. Time from write finish: %ims \r\n", timer.read_ms());
        }
  };

  return success;
}

void SPKTVOne::setCustomResolutions() 
{
  set1920x480(kTV1ResolutionTripleHeadVGAp60);
  set1600x600(kTV1ResolutionDualHeadSVGAp60);
}

bool SPKTVOne::setHDCPOff() 
{
  bool ok = false;

  // Turn HDCP off on the output
  ok =       command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsHDCPRequired, 0);
  ok = ok && command(0, kTV1WindowIDA, kTV1FunctionAdjustOutputsHDCPStatus, 0);
  // Likewise on inputs A and B
  ok = ok && command(0, kTV1WindowIDA, kTV1FunctionAdjustSourceHDCPAdvertize, 0);
  ok = ok && command(0, kTV1WindowIDA, kTV1FunctionAdjustSourceHDCPAdvertize, 0);
  ok = ok && command(0, kTV1WindowIDB, kTV1FunctionAdjustSourceHDCPStatus, 0);
  ok = ok && command(0, kTV1WindowIDB, kTV1FunctionAdjustSourceHDCPStatus, 0);
  
  return ok;
}

void SPKTVOne::set1920x480(int resStoreNumber) 
{
  command(0, 0, kTV1FunctionAdjustResolutionImageToAdjust, resStoreNumber);
  command(0, 0, kTV1FunctionAdjustResolutionInterlaced, 0);
  command(0, 0, kTV1FunctionAdjustResolutionFreqCoarseH, 31400);
  command(0, 0, kTV1FunctionAdjustResolutionFreqFineH, 31475);
  command(0, 0, kTV1FunctionAdjustResolutionActiveH, 1920);
  command(0, 0, kTV1FunctionAdjustResolutionActiveV, 480);
  command(0, 0, kTV1FunctionAdjustResolutionStartH, 192); 
  command(0, 0, kTV1FunctionAdjustResolutionStartV, 32); 
  command(0, 0, kTV1FunctionAdjustResolutionCLKS, 2400); 
  command(0, 0, kTV1FunctionAdjustResolutionLines, 525);
  command(0, 0, kTV1FunctionAdjustResolutionSyncH, 240);
  command(0, 0, kTV1FunctionAdjustResolutionSyncV, 5); 
  command(0, 0, kTV1FunctionAdjustResolutionSyncPolarity, 0);
}

void SPKTVOne::set1600x600(int resStoreNumber) 
{
  command(0, 0, kTV1FunctionAdjustResolutionImageToAdjust, resStoreNumber);
  command(0, 0, kTV1FunctionAdjustResolutionInterlaced, 0);
  command(0, 0, kTV1FunctionAdjustResolutionFreqCoarseH, 37879);
  command(0, 0, kTV1FunctionAdjustResolutionFreqFineH, 37879);
  command(0, 0, kTV1FunctionAdjustResolutionActiveH, 1600);
  command(0, 0, kTV1FunctionAdjustResolutionActiveV, 600);
  command(0, 0, kTV1FunctionAdjustResolutionStartH, 160); 
  command(0, 0, kTV1FunctionAdjustResolutionStartV, 1); 
  command(0, 0, kTV1FunctionAdjustResolutionCLKS, 2112); 
  command(0, 0, kTV1FunctionAdjustResolutionLines, 628);
  command(0, 0, kTV1FunctionAdjustResolutionSyncH, 192);
  command(0, 0, kTV1FunctionAdjustResolutionSyncV, 14); 
  command(0, 0, kTV1FunctionAdjustResolutionSyncPolarity, 0);
}

void SPKTVOne::signErrorOff() {
    *errorDO = 0;
}
