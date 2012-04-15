#include "mbed.h"
#include "mbedOSC.h"

//// ETHERNET

// Ethernet can be created with *either* an address assigned by DHCP or a static IP address. Uncomment the define line for DHCP
//#define DHCP
#ifdef DHCP
EthernetNetIf eth;
#else
EthernetNetIf eth(
    IpAddr(10,0,0,2), //IP Address
    IpAddr(255,255,255,0), //Network Mask
    IpAddr(10,0,0,1), //Gateway
    IpAddr(10,0,0,1)  //DNS
);
#endif

//// OSC

// The object to do the work of sending and receiving
OSCClass osc;

// The message objects to send and receive with
OSCMessage recMes;
OSCMessage sendMes;

// Setting - The port we're listening to on the mbed for OSC messages
int  mbedListenPort  = 10000;

// Setting - The address and port we're going to send to, from the mbed
uint8_t destIp[]  = { 10, 0, 0, 1};
int  destPort = 12000;

//// mbed input

DigitalIn button(p21);
bool buttonLastState;

//// Our messageReceivedCallback function
void processOSC() {

    // If this function has been called, the OSC message just received will have been parsed into our recMes OSCMessage object
    // Note we can access recMes here, outside of the main loop, as we created it as a global variable.

    // TASK: If this message one we want, do something about it.
    // In this example we're listening for messages with a top address of "mbed".
    // Note the strcmp function returns 0 if identical, so !strcmp is true if the two strings are the same
    if ( !strcmp( recMes.getAddress(0) , "mbed" ) ) {
        printf("OSC Message received addressed to mbed \r\n");
        if ( !strcmp( recMes.getAddress(1) , "test1" ) )
            printf("Received subAddress= test1 \r\n");

        // Send some osc message:
        sendMes.setTopAddress("/working...");
        osc.sendOsc(&sendMes);
    }
}

////  M A I N
int main() {

    //// TASK: Set up the Ethernet port
    printf("Setting up ethernet...\r\n");
    EthernetErr ethErr = eth.setup();
    if (ethErr) {
        printf("Ethernet Failed to setup. Error: %d\r\n", ethErr);
        return -1;
    }
    printf("Ethernet OK\r\n");

    //// TASK: Set up OSC message sending

    // In the OSC message container we've made for send messages, set where we want it to go:
    sendMes.setIp( destIp );
    sendMes.setPort( destPort );

    //// TASK: Set up OSC message receiving

    // In the OSC send/receive object...
    // Set the OSC message container for it to parse received messages into
    osc.setReceiveMessage(&recMes);

    // Tell it to begin listening for OSC messages at the port specified (the IP address we know already, it's the mbed's!).
    osc.begin(mbedListenPort);

    // Rather than constantly checking to see whether there are new messages waiting, the object can call some code of ours to run when a message is received.
    // This line does that, attaching a callback function we've written before getting to this point, in this case it's called processOSC
    // For more info how this works, see http://mbed.org/cookbook/FunctionPointer
    osc.messageReceivedCallback.attach(&processOSC);

    //// TASK: Prime button change detection
    buttonLastState = button;

    //// TASK: GO!

    // We've finished setting up, now loop this forever...
    while (true) {
        // This polls the network connection for new activity, without keeping on calling this you won't receive any OSC!
        Net::poll();

        // Has the button changed?
        if (button != buttonLastState) {
            // If so, lets update the lastState variable and then send an OSC message
            buttonLastState = button;
            
            sendMes.setTopAddress("/mbed");
            sendMes.setSubAddress("/button");
            sendMes.setArgs("i", (long)button); // The payload will be the button state as an integer, ie. 0 or 1. We need to cast to 'long' for ints (and 'double' for floats).// The payload will be the button state as an integer, ie. 0 or 1. We need to cast to 'long' for ints (and 'double' for floats).
            osc.sendOsc(&sendMes);
            
            printf("Sent OSC message /mbed/button \r\n");
        }

        // ... Do whatever needs to be done by your mbed otherwise. If an OSC message is received, your messageReceivedCallback will run (in this case, processOSC()).
    }
}