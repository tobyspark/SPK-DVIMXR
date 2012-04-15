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

void OSCMessage::setPort(uint16_t _port){
     host.setPort(_port);
}


void OSCMessage::setIp(uint8_t *_ip){
    host.setIp(IpAddr(_ip[0], _ip[1], _ip[2], _ip[3]));
}



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



uint8_t    OSCMessage::getAddressNum(){
    
    return addressNum;
}


uint8_t    OSCMessage::getArgNum(){
    
    return argNum;
}



char * OSCMessage::getAddress(uint8_t _index){
    if(_index>MAX_ADDRESS) _index=MAX_ADDRESS-1;
    return address[_index];
    
}



char * OSCMessage::getTopAddress(){
    
    return getAddress(0);
    
}


char * OSCMessage::getSubAddress(){
    
    return getAddress(1);
    
}


char  OSCMessage::getTypeTag(uint8_t _index){
    if(_index>MAX_ARG) _index=MAX_ARG-1;
    return typeTag[_index];
}


int32_t OSCMessage::getArgInt(uint8_t _index){
    int32_t *value;
    if(_index > argNum) _index=argNum;
    value = (int32_t *)arg[_index]; // cast to int32_t
    return *value;
}


double OSCMessage::getArgFloat(uint8_t _index){
    double *value;
    if(_index > argNum) _index=argNum;
    value = (double *)arg[_index];
    return *value;
}


void OSCMessage::setTopAddress(char *_address){
    address[0]=_address;
    address[1]=0;
    addressNum=1; // Note: this "erases" the subaddress! (is this a good idea?)
}


void OSCMessage::setSubAddress(char *_address){
    address[1]=_address;
    addressNum=2; // Note: this assumes the top address was already set!
}



void OSCMessage::setAddress(char *_topAddress,char *_subAddress){
    setTopAddress(_topAddress);
    setSubAddress(_subAddress);
    addressNum=2; // (unnecessary...)
}


void OSCMessage::setAddress(uint8_t _index, char *_address){
    if(_index>MAX_ADDRESS) _index=MAX_ADDRESS-1;
    address[_index]=_address;
    addressNum=_index+1;
}



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

OSCClass::OSCClass(OSCMessage *_mes){
    udpRec.setOnEvent(this, &OSCClass::onUDPSocketEvent);
    receiverMessage = _mes; // note: receiverMessage MUST be a pointer to the message, because we will modify things in it
    newMessage=false;
}

void OSCClass::begin()
{    
  // setup receiver udp socket:
  udpRec.bind(receiverMessage->host);
}


void OSCClass::begin(uint16_t _recievePort)
{
  receiverMessage->host.setPort(_recievePort);
  // setup receiver udp socket:
  udpRec.bind(receiverMessage->host);
}


void OSCClass::setReceiveMessage(OSCMessage *_mes){
    receiverMessage = _mes;
}

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



OSCMessage * OSCClass::getMessage(){
    newMessage=false; // this indicate the user READ the message
    return receiverMessage;
}


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


void OSCClass::stop() {
    //close( socketNo );
    udpSend.resetOnEvent(); // disables callback
}


