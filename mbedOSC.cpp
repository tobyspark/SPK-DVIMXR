/*
 mbedOSC.cpp 
*/                    
   
#include "mbed.h"
#include "mbedOSC.h"
#include "stdarg.h"

OSCMessage::OSCMessage() {
 // Initialize host address and port by default (as if this where the receiver message):
 //    host=new Host(IpAddr(10, 0, 0, 1), DEFAULT_RECEIVE_PORT, NULL);
}

/*
 Set PortNo for the OSC Message
 @param[in] _port PortNo (unsigned int)
 @return None
 */
void OSCMessage::setPort(uint16_t _port){
     host.setPort(_port);
}

/*
 Set IP Address of the OSC Message (for SENDING messages - for receiving this will be done when receiving something ) 
 param[in] <-- _ip pointer of IP Address array (byte *)
 Example: IP=192.168.0.99, then we have to do: ip[]={192,168,0,1}, then setIp(ip)
 */
void OSCMessage::setIp(uint8_t *_ip){
    host.setIp(IpAddr(_ip[0], _ip[1], _ip[2], _ip[3]));
}


/*!
 Set IP Address to the OSC Message container (not through pointer)
 Example: IP=192.168.0.99 => setIp(192,168,0,99)
 */
void OSCMessage::setIp(    uint8_t _ip1,
                        uint8_t _ip2,
                        uint8_t _ip3,
                        uint8_t _ip4 ){
    
    host.setIp(IpAddr(_ip1, _ip2, _ip3, _ip4));
}

const IpAddr& OSCMessage::getIp(){
    return host.getIp();
}


 const int& OSCMessage::getPort(){
    return host.getPort();
}


/*
 Gets the number of the OSC message address
 param[in] None
 return number of the OSC message address (byte)
 Examples: "/ard"      --> the number of the addresses is 1
           "/ard/test" --> the number of the addresses is 2
 Attention: the maximum number of addresses is 2 (MAX_ADDRESS)
*/
uint8_t    OSCMessage::getAddressNum(){
    
    return addressNum;
}


/*
 Gets the number of the OSC message args
 param[in] None
 return number of the args (byte)
 Example: "i" 123 --> number of the OSC message args is 1
          "if" 123 54.24 --> number of the OSC message args is 2
 Attention: the maximum number of args is 2 (MAX_ARG)
 */
uint8_t    OSCMessage::getArgNum(){
    
    return argNum;
}


/*
 Gets the address string of the OSC message
 param [in] <-- _index is the index of the address string (byte)
 return pointer of the address string (char *)
 @note ex. "/ard/test"<br>
 getAddress(0) = "/ard"<br>
 getAddress(1) = "/test"
 @attention It is maximum number of the addresses is 2<br>
 In this case "/ard/test1/test2"<br>
 ignore it after "/test2"
 */
char * OSCMessage::getAddress(uint8_t _index){
    if(_index>MAX_ADDRESS) _index=MAX_ADDRESS-1;
    return address[_index];
    
}


/*
 Gets the TopAddress string of the OSC message (this is just the address with index 0)
 param[in] None
 return pointer of the TopAddress string (char *), i.e. address[0]
 Example: In the case "/ard/test", getTopAddress() = "/ard" (WITH the slash "/") 
 */
char * OSCMessage::getTopAddress(){
    
    return getAddress(0);
    
}

/*
 Gets the "SubAddress" string of the OSC message (this is just the address with index 1)
 param[in] None
 return pointer of the SubAddress string (char *), i.e. address[1]
 Example: in the case "/ard/test", getSubAddress() = "/test" (WITH the slash "/") 
 */
char * OSCMessage::getSubAddress(){
    
    return getAddress(1);
    
}

/*
 Gets the TypeTag string (with index) of the OSC message
 param[in] <--_index is the index of the TypeTag string (byte)
 return: TypeTag char (char)
 Example: in the case of a total typetag string equal to "if", getTypeTag(0) = 'i' and getTypeTag(1) = 'f'
 Attention: MAX_ARG is maximum number of the args, if the index argument is larger, it will be constrained to this max. 
 */
char  OSCMessage::getTypeTag(uint8_t _index){
    if(_index>MAX_ARG) _index=MAX_ARG-1;
    return typeTag[_index];
}

/*
 Get the args of the OSC message with an integer value
 param[in] <--_index is (an int, or uint8_t), corresponding to the index of the args (byte)
 return: integer value (long, or int32_t)
 Example: in the case "if" 123 54.24, getArgInt(0) = 123
 Noe: "i" is integer, but the return type is "long"
 Note: When a index is bigger than the number of the args, it is set to the number of the args
 */
int32_t OSCMessage::getArgInt(uint8_t _index){
    int32_t *value;
    if(_index > argNum) _index=argNum;
    value = (int32_t *)arg[_index]; // cast to int32_t
    return *value;
}

/*
 Get the args of the OSC message with a float value
 param[in] <--_index is the index of the args
 return: float value (double)
 note: In this case "if" 123 54.24, getArgFloat(1) = 54.24
 attention: arg declared as float, but return value cast as "double"
 attention: When index is bigger than the number of the args, it is set to the number of the args
 */
double OSCMessage::getArgFloat(uint8_t _index){
    double *value;
    if(_index > argNum) _index=argNum;
    value = (double *)arg[_index];
    return *value;
}

/*
 Set TopAddress string of OSC Message 
 param[in] <-- _address is a string pointer for the TopAddress String (char *). NOTE: is this a good idea? why not pass as const, and do allocation here?
 return: None
 Example: if the complete address string is "/ard/test", we set the topaddress as follows: char top[]="/ard" (allocation done here!), then setTopAddress(top)
 */
void OSCMessage::setTopAddress(char *_address){
    address[0]=_address;
    address[1]=0;
    addressNum=1; // Note: this "erases" the subaddress! (is this a good idea?)
}

/*
 Set SubAddress string of the OSC Message
 param[in] <-- _address is a string pointer for the SubAddress String (char *)
 return: None
 Example:  if the complete address string is "/ard/test", we set the subaddress as follows: char sub[]="/test" (allocation done here!), then setSubAddress(sub)
 Attention: we should call first setTopAddress, and then setSubAddress. The order is important. This does not seems like a good idea...
 */
void OSCMessage::setSubAddress(char *_address){
    address[1]=_address;
    addressNum=2; // Note: this assumes the top address was already set!
}


/*
 Set the complete Address string of the OSC Message (top and sub addresses)
 param[in] <-- _topAddress and _subAddress are the string pointers to top and sub addresses (char *)
 return: None
 Example: in the case "/ard/test", we need to do: char top[]="/ard", char sub[]="/test", and then setAddress(top,sub)
 Reminder: in this implementation, the maximum number of addresses is MAX_ADDRESS=2
 */
void OSCMessage::setAddress(char *_topAddress,char *_subAddress){
    setTopAddress(_topAddress);
    setSubAddress(_subAddress);
    addressNum=2; // (unnecessary...)
}

/*
 Set address string using index (here 0 or 1)
 Example: "/ard/test", char adr[]="/ard", setAddress(0,adr), char adr2[]="/test", setAddress(1,adr)
 */
void OSCMessage::setAddress(uint8_t _index, char *_address){
    if(_index>MAX_ADDRESS) _index=MAX_ADDRESS-1;
    address[_index]=_address;
    addressNum=_index+1;
}


/*
 Set TypeTag and args to the OSC Message container
 @param[in] types TypeTag string "i"(integer) or"f"(float) (char *)
 @param[in] ... Pointer of the Args(variable argument) ..
 @return None
 @Example: 
 (1) integer 123: (NOTE: integers are LONG)
 long v1=123; sendMes.setArgs("i",&v1)
 (2)integer:123 and float:52.14
 long v1=123; double v2=52.14; sendMes.setArgs("if",&v1,&v2)
 Attention: in this implementation, the maximum number of the args is 2
 (if setArgs("iff",&v1,&v2,&v3), data is ignored after &v3)
 */
void OSCMessage::setArgs(char *types,...){
    
    va_list argList;
    
    argNum = strlen(types);
    if(argNum>MAX_ARG) argNum=MAX_ARG-1;
    
    va_start( argList, types );
    for(uint8_t i=0 ; i < argNum ; i++){
        
        typeTag[i]=types[i];
        
        switch(types[i]) {
            case 'i':
                arg[i]=(uint32_t *)va_arg(argList, uint32_t *);
                break;
            case 'f':
                arg[i]=va_arg(argList, double *);
                break;
        }
        
    }
    
}

// ================================================================================================================================================
// ====================================  OSCClass for sending and receiving OSC messages using UDP protocol =======================================
// ================================================================================================================================================
//The class define an object wrapping the UDP functions to send and receive OSC messages

OSCClass::OSCClass(){
    udpRec.setOnEvent(this, &OSCClass::onUDPSocketEvent);
    newMessage=false;
}

/*
 This sets "binds" the received message to the receiver container of the communication object
 param[in]<--_mes is a pointer to the "receiveing" OSC message (OSCMessage *)
 */
OSCClass::OSCClass(OSCMessage *_mes){
    udpRec.setOnEvent(this, &OSCClass::onUDPSocketEvent);
    receiverMessage = _mes; // note: receiverMessage MUST be a pointer to the message, because we will modify things in it
    newMessage=false;
}

/*
 This initializes the OSC communication object with default receiving port (DEFAULT_REC_PORT)
 param[in]: None
 return: None
 */
void OSCClass::begin()
{    
  // setup receiver udp socket:
  udpRec.bind(receiverMessage->host);
}

/*
 Initialize an OSC object with arbitrary listening port
 param[in] <-- _recievePort, is the listening ("receiving") Port No (unsigned int)
 return: None
 */
void OSCClass::begin(uint16_t _recievePort)
{
  receiverMessage->host.setPort(_recievePort);
  // setup receiver udp socket:
  udpRec.bind(receiverMessage->host);
}

/*
 Set a OSC receive message container
 param[in] _mes Pointer to the OSC receive message container (OSCMessage *)
 return None
 */
void OSCClass::setReceiveMessage(OSCMessage *_mes){
    receiverMessage = _mes;
}

/*
 callback function when an upd message arrives (it will be transformed as OSC message)
 */
void OSCClass::onUDPSocketEvent(UDPSocketEvent e)
{
  switch(e)
  {
  case UDPSOCKET_READABLE: //The only event for now
    //char buf[256] = {0};
    Host auxhost;
    buflength = udpRec.recvfrom( rcvBuff, 256, &auxhost ); // QUESTION: auxhost should be equal to the receiver host I guess...
    if ( buflength > 0 ) {
      //printf("\r\nFrom %d.%d.%d.%d:\r\n", host.getIp()[0], host.getIp()[1], host.getIp()[2], host.getIp()[3]);   
      decodePacket(receiverMessage); // convert to OSC message, and save it in receiverMessage
      newMessage=true;
      
      messageReceivedCallback.call();
    }
  break;
  }
}

/*
 Decode UDP packet and save it in the OSCMessage structure
 */
void OSCClass::decodePacket( OSCMessage *_mes) {
    
    //uint16_t    lenBuff;
    uint8_t        d;    
    uint8_t        messagePos=0;    
    uint8_t        adrCount=0;
    uint8_t        adrMesPos=0;    
    uint8_t        packetCount=0;
    uint8_t        packetPos=4;
    
    
    //W5100.writeSn(socketNo, SnIR, SnIR::RECV);
    //lenBuff=recvfrom(socketNo, rcvBuff, 1, receiverMessage->ip, &receiverMessage->port);    
    
    receiverMessage->address[0]=tempAddress[0];

    //(1) address process start =========================================
    do{
        d=rcvBuff[messagePos];

        
        if( (d=='/') && (messagePos>0) ){

            if(adrCount<MAX_ADDRESS){
                tempAddress[adrCount][adrMesPos]=0;

                adrCount++;
                adrMesPos=0;

                receiverMessage->address[adrCount]=tempAddress[adrCount];
            }

        }
        
        if(adrCount<MAX_ADDRESS){
        //Added this in to remove the slashes out of final output
        if(d!='/'){
        tempAddress[adrCount][adrMesPos]=d;            
    
        if(packetCount>3)  {
            packetCount=0;
            packetPos+=4;
        }
        
        adrMesPos++;
        }
        }
        messagePos++;
        packetCount++;
        
    }while(d!=0);

    
    if(adrCount<MAX_ADDRESS) adrCount++;
    receiverMessage->addressNum=adrCount;
    
    messagePos=packetPos;

    //(2) type tag process starts =========================================
    packetCount=0;
    packetPos+=4;

    uint8_t  typeTagPos=0;
    uint8_t     tempArgNum=0;

    while(rcvBuff[messagePos]!=0 ){
            
        if(rcvBuff[messagePos] != ',') {
        
                if(typeTagPos<MAX_ARG){
                    receiverMessage->typeTag[tempArgNum]=rcvBuff[messagePos];
                    tempArgNum++;
                }
                typeTagPos++;
                
            }
        
        packetCount++;
        
        if(packetCount>3)  {
            packetCount=0;
            packetPos+=4;
        }
        
        messagePos++;
    }
    
    receiverMessage->argNum=tempArgNum;

    messagePos=packetPos;

    //(3) tempArg process starts =========================================
    for(int i=0;i<tempArgNum;i++){
        
        adrMesPos=3;

        receiverMessage->arg[i]=tempArg[i];
    
        for(int j=0;j<4;j++){

            tempArg[i][adrMesPos]=rcvBuff[messagePos];

            messagePos++;
            adrMesPos--;
        }
    
    }


}


/*
 Get the received OSC message (note: this is another way to access the message directly from the OSCClass object).
 The advantage is that we will signal that we read the message, and will be able to query if a NEW message arrived
 (Alternatively, one could have a function pointer to pass to the OSC object, that will be called each time a new packet is received: TO DO) 
 */
OSCMessage * OSCClass::getMessage(){
    newMessage=false; // this indicate the user READ the message
    return receiverMessage;
}

/*
 Send an OSC Message (message contain the host ip and port where the message data has to be sent)
 param[in] _mes Pointer to the OSC message container (OSCMessage *)
 return None
 */
void OSCClass::sendOsc( OSCMessage *_mes )
{
    uint8_t lengthEnd;
    uint8_t lengthStart;    
    char  buff[128];
    
    sendContainer = _mes;
    
    //&#12496;&#12483;&#12501;&#12449;&#21021;&#26399;&#20516;
    buff[0]=0;
    
    //1) Add name spaces:
    for(int i=0;i<sendContainer->addressNum;i++){
        
        strcat(buff,sendContainer->address[i]); // note: an address is for instance: "/test" (including the "/")
        
    }

    // pad with 0s to align in multiples of 4:
    lengthStart=strlen(buff);
    lengthEnd=lengthStart+(4-(lengthStart%4));
    for(int i=lengthStart ; i<lengthEnd; i++){
        buff[i]=0;  
    }

    lengthStart=lengthEnd;
    
    //2) Add TypeTag:
    buff[lengthEnd++]=','; // Note: type tag is for instance: ",if"
    for(int i=0;i<sendContainer->argNum;i++){
        buff[lengthEnd++]=sendContainer->typeTag[i];
    }
    
    // pad with 0s to align in multiples of 4:
    lengthStart=lengthEnd;
    lengthEnd=lengthStart+(4-(lengthStart%4));
    for(int i=lengthStart ; i<lengthEnd; i++){
        buff[i]=0;
    }
    
    //3) add argument values (Note: here only big endian):
    uint8_t *v;
    for(int i=0;i<sendContainer->argNum;i++){
        uint8_t valuePos=3;
        v=(uint8_t *)sendContainer->arg[i];

        buff[lengthEnd++]=v[valuePos--];
        buff[lengthEnd++]=v[valuePos--];
        buff[lengthEnd++]=v[valuePos--];
        buff[lengthEnd++]=v[valuePos]; 
        
    }
    
    //4) Send udp packet: 
    //sendto(    socketNo, (uint8_t *)buff, lengthEnd, sendContainer->ip, sendContainer->port );
    udpSend.sendto(buff , lengthEnd, &(sendContainer->host));
}


/*
 flush a receive buffer
void OSCClass::flush() {    
    while ( available() ){}
}
*/

/*
 Stop OSC communication (in fact, only the receiver - the server side)
 */
void OSCClass::stop() {
    //close( socketNo );
    udpSend.resetOnEvent(); // disables callback
}


