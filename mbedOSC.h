/* mbedOSC.h
 This is an OSC library for the mbed, created to be compatible with Recotana's OSCClass library (http://recotana.com) for the
 Arduino with Ethernet shield. I have also used parts of the OSC Transceiver(Sender/Receiver) code by xshige
 written by: Alvaro Cassinelli, 7.10.2011
 tweaked by: Toby Harris / *spark audio-visual, 13.4.2012

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License version 2.1 as published by the Free Software Foundation.
 Open Sound Control  http://opensoundcontrol.org/
 
 mbedOSC version 0.1 Specification (similar to Recotana's OSCClass library)

 ********
 Address : max 2
     "/ard"
    "/ard/output"    --address[0]="/ard"        :max 15character
                    --address[1]="/output"    :max 15character

 *******
 TypeTag    :    max 2
 
 "i" - long or unsigned long
 "f" - double
 
 ******* 
 arg    :    max 2
 
 *******
 Example of an OSC message: "/mbed/test1, if 50 32.4" (Note: this is not the byte string
 sent as UDP packet - there are no spaces, and arguments are in binary, BIG ENDIAN)
*/

#ifndef mbedOSC_h
#define mbedOSC_h

#include "mbed.h"
#include "EthernetNetIf.h"
#include "UDPSocket.h"

// setup IP of destination (computer):
#define DEFAULT_SEND_PORT 12000
//Host sendHost(IpAddr(10, 0, 0, 1), DEFAULT_SEND_PORT, NULL); // Send Port
// set IP of origin of UDP packets - the mbed acts as a SERVER here, and needs to bind the socket to the "client" (the computer)
#define DEFAULT_RECEIVE_PORT 57130
//Host recHost(IpAddr(10, 0, 0, 1), DEFAULT_RECEIVE_PORT, NULL);  // Receive Port
//UDPSocket udpRec,udpSend;


#define MAX_ADDRESS    2
#define MAX_ARG        2

#define TYPE_INT    1
#define TYPE_FLOAT    2


/*
Container class for OSC messages (receiving or sending)
*/
class OSCMessage{
    
    private:
    
        char        *address[MAX_ADDRESS]; // these are strings (as char*)
        uint8_t         addressNum; // current number of addresses in the message (ex: "/ard/test" --> the number of the addresses is 2)
    
        char         typeTag[MAX_ARG];
    
        void        *arg[MAX_ARG];
        uint8_t         argNum;
    
        // Information about the connection:    
        //uint8_t         ip[4];            
        //uint16_t     port;
        Host host; 
    
    public:
    
        OSCMessage();
    
        const IpAddr& getIp();    // return IpAddr object
        const int&     getPort(); // return port
    
        //ex. address patern "/adr/test"
        //    address[2]={"/ard" , "/test"}
        char        *getAddress(uint8_t _index);    //retturn address
        char        *getTopAddress();    //return address[0] :"/ard"
        char        *getSubAddress();    //return address[1] :"/test"
        uint8_t         getAddressNum();    //return 2        
    
        // 'i': long(int32_t)
        // 'f': double
        //ex 'if' 123 54.21
        char         getTypeTag(uint8_t _index);    //_index=0 ->'i'
                                                    //_index=1 ->'f'

        uint8_t         getArgNum();    //return 2
    
        int32_t         getArgInt(uint8_t _index);        //_index=0 -> 123
        double         getArgFloat(uint8_t _index);    //_index=1 -> 54.21
    
    
        void setTopAddress(char *_address);        //set address[0]
        void setSubAddress(char *_address);        //set address[1]
        void setAddress(char *_topAddress,     
                        char *_subAddress);
        void setAddress(uint8_t _index,        //set 0,address[0]
                        char *_address);    
                                            //set 1,address[1]
    
        void setIp( uint8_t *_ip );    //set ip
    
        void setIp(uint8_t _ip1,    //set(192,
                   uint8_t _ip2,    //      168,
                   uint8_t _ip3,    //    0,
                   uint8_t _ip4);    //    100)
    
        void setPort( uint16_t _port );
    
        //ex. long   v1=100
        //    double v2=123.21
        void setArgs( char *types , ... );    //set ("if",&v1,&v2)
    
        friend class OSCClass;
    
};



/* ====================================  OSCClass for sending and receiving OSC messages using UDP protocol ===================================== */

#include "UDPSocket.h"

class OSCClass {
    
private:
    
    UDPSocket udpRec,udpSend;
    char   rcvBuff[256]; // raw buffer for UDP packets (udpRec.recvfrom( buf, 256, &host ) ))
    int   buflength;
    
    OSCMessage *receiverMessage;
    OSCMessage *sendContainer;
    
    char         tempAddress[MAX_ADDRESS][16];
    uint8_t      tempArg[MAX_ARG][4];    
    
    void decodePacket( OSCMessage *_mes); // makes OSC message from packet

public:
    
    OSCClass();
    OSCClass(OSCMessage *_mes); // set the receiver message container
    void onUDPSocketEvent(UDPSocketEvent e);
        
    //init osc 
    void begin();
    void begin(uint16_t _recievePort);
    void stop();
    
    //new OSC data in the receiver message container: 
    bool newMessage;

    void setReceiveMessage( OSCMessage *_mes ); //set receive OSCmessage container (note: the message has a "host" object from which we get the upd packets)
    OSCMessage    *getMessage();    //return received OSCmessage    

    //buffer clear
    //void flush();    
    
    //OSC send
    void sendOsc( OSCMessage *_mes ); //set&send OSCmessage (note: it will be sent to the host defined in the message container)

    //to be set by host program, will be called on receipt of an OSC message
    FunctionPointer messageReceivedCallback;
};

#endif
