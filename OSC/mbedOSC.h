/* mbed OSC Library
 This is an Open Sound Control library for the mbed, created to be compatible with Recotana's OSCClass library (http://recotana.com) for the
 Arduino with Ethernet shield. It also uses parts of the OSC Transceiver(Sender/Receiver) code by xshige
 written by: Alvaro Cassinelli, October 2011
 tweaked by: Toby Harris / *spark audio-visual, March 2012

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License version 2.1 as published by the Free Software Foundation.
 Open Sound Control  http://opensoundcontrol.org/
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


/** Container class for OSC messages (receiving or sending)
 @note mbedOSC version 0.1 Specification (similar to Recotana's OSCClass library)
 Example of an OSC message: "/mbed/test1, if 50 32.4"
 ie. "Address TypeTag Args"
 Address : max 2
    "/ard"
    "/ard/output"
    --address[0]="/ard"       :max 15character
    --address[1]="/output"    :max 15character
 TypeTag : max 2
    "i" - long or unsigned long
    "f" - double
 arg    :    max 2
 (Note: The byte string as seen here is not sent as UDP packet directly - there are no spaces, and arguments are in binary, BIG ENDIAN)
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
        /** Create a container for an OSC message to be received or sent */
        OSCMessage();

        /** Return the IpAddr object */   
        const IpAddr& getIp();
        /** Return the port */
        const int&     getPort();
    
/** Gets the address string of the OSC message
 *
 * @param[in] _index The index of the address string (byte)
 * @return pointer of the address string (char *)
 * @note ex. "/ard/test"<br>
 * getAddress(0) = "/ard"<br>
 * getAddress(1) = "/test"
 * @attention It is maximum number of the addresses is 2<br>
 * In this case "/ard/test1/test2"<br>
 * ignore it after "/test2"
 */
        char        *getAddress(uint8_t _index);    //retturn address
        
/** Gets the TopAddress string of the OSC message (this is just the address with index 0)
 @return pointer of the TopAddress string (char *), i.e. address[0]
 Example: In the case "/ard/test", getTopAddress() = "/ard" (WITH the slash "/") 
 */        
        char        *getTopAddress();    //return address[0] :"/ard"

/**
 Gets the "SubAddress" string of the OSC message (this is just the address with index 1)
 @return pointer of the SubAddress string (char *), i.e. address[1]
 Example: in the case "/ard/test", getSubAddress() = "/test" (WITH the slash "/") 
 */
        char        *getSubAddress();    //return address[1] :"/test"

/**
 Gets the number of the OSC message address
 @return number of the OSC message address (byte)
 Examples: "/ard"      --> the number of the addresses is 1
           "/ard/test" --> the number of the addresses is 2
 Attention: the maximum number of addresses is 2 (MAX_ADDRESS)
*/
        uint8_t         getAddressNum();    //return 2        
    
/**
 Gets the TypeTag string (with index) of the OSC message
 @param[in] _index The index of the TypeTag string (byte)
 @return: TypeTag char (char)
 Example: in the case of a total typetag string equal to "if", getTypeTag(0) = 'i' and getTypeTag(1) = 'f'
 Attention: MAX_ARG is maximum number of the args, if the index argument is larger, it will be constrained to this max. 
 */
        char         getTypeTag(uint8_t _index);    //_index=0 ->'i'
                                                    //_index=1 ->'f'

/**
 Gets the number of the OSC message args
 @return number of the args (byte)
 Example: "i" 123 --> number of the OSC message args is 1
          "if" 123 54.24 --> number of the OSC message args is 2
 Attention: the maximum number of args is 2 (MAX_ARG)
 */
        uint8_t         getArgNum();    //return 2
    
/**
 Get the args of the OSC message with an integer value
 @param[in] _index An int or uint8_t corresponding to the index of the args (byte)
 @return: integer value (long, or int32_t)
 Example: in the case "if" 123 54.24, getArgInt(0) = 123
 Noe: "i" is integer, but the return type is "long"
 Note: When a index is bigger than the number of the args, it is set to the number of the args
 */
        int32_t         getArgInt(uint8_t _index);        //_index=0 -> 123

/**
 Get the args of the OSC message with a float value
 @param[in] _index The index of the args
 @return: float value (double)
 note: In this case "if" 123 54.24, getArgFloat(1) = 54.24
 attention: arg declared as float, but return value cast as "double"
 attention: When index is bigger than the number of the args, it is set to the number of the args
 */
        double         getArgFloat(uint8_t _index);    //_index=1 -> 54.21
    
    
/**
 Set TopAddress string of OSC Message 
 @param[in] _address A string pointer for the TopAddress String (char *). NOTE: is this a good idea? why not pass as const, and do allocation here?
 Example: if the complete address string is "/ard/test", we set the topaddress as follows: char top[]="/ard" (allocation done here!), then setTopAddress(top)
 */
        void setTopAddress(char *_address);        //set address[0]

/**
 Set SubAddress string of the OSC Message
 @param[in] _address A string pointer for the SubAddress String (char *)
 Example:  if the complete address string is "/ard/test", we set the subaddress as follows: char sub[]="/test" (allocation done here!), then setSubAddress(sub)
 Attention: we should call first setTopAddress, and then setSubAddress. The order is important. This does not seems like a good idea...
 */
        void setSubAddress(char *_address);        //set address[1]

/**
 Set the complete Address string of the OSC Message (top and sub addresses)
 @param[in] _topAddress, _subAddress The string pointers to top and sub addresses (char *)
 Example: in the case "/ard/test", we need to do: char top[]="/ard", char sub[]="/test", and then setAddress(top,sub)
 Reminder: in this implementation, the maximum number of addresses is MAX_ADDRESS=2
 */
        void setAddress(char *_topAddress,     
                        char *_subAddress);

/**
 Set address string using index (here 0 or 1)
 Example: "/ard/test", char adr[]="/ard", setAddress(0,adr), char adr2[]="/test", setAddress(1,adr)
 */
        void setAddress(uint8_t _index,        //set 0,address[0]
                        char *_address);    
                                            //set 1,address[1]

/**
 Set IP Address of the OSC Message (for SENDING messages - for receiving this will be done when receiving something ) 
 @param[in] _ip Pointer of IP Address array (byte *)
 Example: IP=192.168.0.99, then we have to do: ip[]={192,168,0,1}, then setIp(ip)
 */    
        void setIp( uint8_t *_ip );    //set ip

/**
 Set IP Address to the OSC Message container (not through pointer)
 Example: IP=192.168.0.99 => setIp(192,168,0,99)
 */    
        void setIp(uint8_t _ip1,    //set(192,
                   uint8_t _ip2,    //      168,
                   uint8_t _ip3,    //    0,
                   uint8_t _ip4);    //    100)

        /*
         Set PortNo for the OSC Message
         @param[in] _port PortNo (unsigned int)
         @return None
         */
        void setPort( uint16_t _port );
    
/**
 Set TypeTag and args to the OSC Message container
 @param[in] types TypeTag string "i"(integer) or"f"(float) (char *)
 @param[in] ... Pointer of the Args(variable argument) ..
 @Example: 
 (1) integer 123: (NOTE: integers are LONG)
 long v1=123; sendMes.setArgs("i",&v1)
 (2)integer:123 and float:52.14
 long v1=123; double v2=52.14; sendMes.setArgs("if",&v1,&v2)
 Attention: in this implementation, the maximum number of the args is 2
 (if setArgs("iff",&v1,&v2,&v3), data is ignored after &v3)
 */
        void setArgs( char *types , ... );    //set ("if",&v1,&v2)
    
        friend class OSCClass;
    
};



/* ====================================  OSCClass for sending and receiving OSC messages using UDP protocol ===================================== */

#include "UDPSocket.h"

/** Wraps the UDP functions to send and receive OSC messages */
class OSCClass {
    
private:
    
    UDPSocket udpRec,udpSend;
    char   rcvBuff[256]; // raw buffer for UDP packets (udpRec.recvfrom( buf, 256, &host ) ))
    int   buflength;
    
    OSCMessage *receiverMessage;
    OSCMessage *sendContainer;
    
    char         tempAddress[MAX_ADDRESS][16];
    uint8_t      tempArg[MAX_ARG][4];    
    
    void onUDPSocketEvent(UDPSocketEvent e);
    
    void decodePacket( OSCMessage *_mes); // makes OSC message from packet

public:
    
    friend class UDPSocket;
    
    /** Create an object to send and receive OSC messages */
    OSCClass();
    
/**
 This sets "binds" the received message to the receiver container of the communication object
 @param[in] _mes A pointer to the "receiveing" OSC message (OSCMessage *)
 */
    OSCClass(OSCMessage *_mes); // set the receiver message container
        
/**
 This initializes the OSC communication object with default receiving port (DEFAULT_REC_PORT) 
 */
    void begin();

/**
 Initialize an OSC object with arbitrary listening port
 @param[in] _recievePort The listening ("receiving") Port No (unsigned int)
 */
    void begin(uint16_t _recievePort);

/**
 Stop OSC communication (in fact, only the receiver - the server side)
 */
    void stop();
    
/**
 Returns whether there is new OSC data in the receiver message container.
 */
    bool newMessage;

/**
 Set a OSC receive message container
 @param[in] _mes Pointer to the OSC receive message container (OSCMessage *)
 */
    void setReceiveMessage( OSCMessage *_mes ); //set receive OSCmessage container (note: the message has a "host" object from which we get the upd packets)

/**
 Get the received OSC message (note: this is another way to access the message directly from the OSCClass object).
 The advantage is that we will signal that we read the message, and will be able to query if a NEW message arrived
 (Alternatively, one could have a function pointer to pass to the OSC object, that will be called each time a new packet is received: TO DO) 
 */
    OSCMessage    *getMessage();    //return received OSCmessage    
    
/**
 Send an OSC Message (message contain the host ip and port where the message data has to be sent)
 @param[in] _mes Pointer to the OSC message container (OSCMessage *)
 */
    void sendOsc( OSCMessage *_mes ); //set&send OSCmessage (note: it will be sent to the host defined in the message container)

/**
 A function pointer to be set by host program that will be called on receipt of an OSC message
 @code
 osc.messageReceivedCallback.attach(&processOSC);
 @endcode
 */
    FunctionPointer messageReceivedCallback;
};

#endif
