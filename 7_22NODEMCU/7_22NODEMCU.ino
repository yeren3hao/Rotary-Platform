//NodeMCU
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>   // include SoftwareSerial heading
SoftwareSerial mySerial(14,12);//RX=d5,TX=d6

#define server_ip "bemfa.com" //Bemfa cloud server address
#define server_port "8344" //address port:8344

#define wifi_name  "vivo"     //my wifi name
#define wifi_password   "sbwhj111"  //WIFI passwprd
String UID = "46e62e1f12fa422ebee36f41d5e056db";  //my own UID
String TOPIC = "myplatform";         //my topic

//**************************************************//
#define MAX_PACKETSIZE 512 //Maximum number of bytes
#define KEEPALIVEATIME 60*1000 //Set the heartbeat value to 60s
WiFiClient TCPclient;
String TcpClient_Buff = "";//An initialization string that receives data from the server
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;//heartbeat
unsigned long preTCPStartTick = 0;//connection
bool preTCPConnected = false;

int set_tilt = 30; //APP set tilt angle
int set_speed = 150; //APP set rotary speed
int set_cycles = 1;//APP设 set rotary cycle

void doWiFiTick();
void startSTA();

void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);

void turnOnLed();
void turnOffLed();

/*
  * Send data to TCP server
 */
void sendtoTCPServer(String p){
  if (!TCPclient.connected()) 
  {
    Serial.println("Client is not readly");
    return;
  }
  TCPclient.print(p);
  preHeartTick = millis();//The heartbeat timer starts, and data needs to be sent every 60 seconds
}

/*
  * Initialize and establish a connection with the server
*/
void startTCPClient(){
  if(TCPclient.connect(server_ip, atoi(server_port))){
    Serial.print("\nConnected to server:");
    Serial.printf("%s:%d\r\n",server_ip,atoi(server_port));
    
    String tcpTemp="";  //Initialize the string
    tcpTemp = "cmd=1&uid="+UID+"&topic="+TOPIC+"\r\n"; //Build subscription directives
    sendtoTCPServer(tcpTemp); //Send a subscription instruction
    tcpTemp="";//clear
    preTCPConnected = true;
    TCPclient.setNoDelay(true);
  }
  else{
    Serial.print("Failed connected to server:");
    Serial.println(server_ip);
    TCPclient.stop();
    preTCPConnected = false;
  }
  preTCPStartTick = millis();
}

/*
  * Check data, send heartbeat
*/
void doTCPClientTick(){
 //Check whether it is disconnected, disconnect and reconnect
   if(WiFi.status() != WL_CONNECTED) return;
  if (!TCPclient.connected()) {//Disconnect and reconnect
  if(preTCPConnected == true){
    preTCPConnected = false;
    preTCPStartTick = millis();
    Serial.println();
    Serial.println("TCP Client disconnected.");
    TCPclient.stop();
  }
  else if(millis() - preTCPStartTick > 1*1000)//reconnect
    startTCPClient();
  }
  else
  {
    if (TCPclient.available()) {//receive data
      char c =TCPclient.read();
      TcpClient_Buff +=c;
      TcpClient_BuffIndex++;
      TcpClient_preTick = millis();
      
      if(TcpClient_BuffIndex>=MAX_PACKETSIZE - 1){
        TcpClient_BuffIndex = MAX_PACKETSIZE-2;
        TcpClient_preTick = TcpClient_preTick - 200;
      }
 
    }
    if(millis() - preHeartTick >= KEEPALIVEATIME){//Keep heart beating
      preHeartTick = millis();
      Serial.println("--Keep alive:");
      sendtoTCPServer("ping\r\n"); //To send a heartbeat, the command needs to end \r\n
    }
  }
 
  if((TcpClient_Buff.length() >= 1) && (millis() - TcpClient_preTick>=200))
   {//data ready
    TCPclient.flush();
    Serial.println(TcpClient_Buff); //Print the received message
    String getTopic = "";
    String getMsg = "";
    String getTemp = "";
    int tempIndex = 0; //Assign an initial value to the temp position
          //At this time, receiving a push command, which is approximately cmd=2&uid=xxx&topic=light002&msg=off  cmd=2&uid=xxx&topic=light002&msg=settemp#60
           if((TcpClient_Buff.indexOf("&msg=on") > 0)) {
              turnOnSwitch();
              //////The string matches, detects whether the sent string TcpClient_Buff contains &msg=off, and if so, stops the device
           }else if((TcpClient_Buff.indexOf("&msg=off") > 0)) {
            turnOffSwitch();
           } 
          int topicIndex = TcpClient_Buff.indexOf("&topic=")+7; //C language string lookup, find &topic= position, and move 7 bits
          int msgIndex = TcpClient_Buff.indexOf("&msg=");//C language string lookup, find &msg= position
          getTopic = TcpClient_Buff.substring(topicIndex,msgIndex);//C language string interception, interception to topic,
          getMsg = TcpClient_Buff.substring(msgIndex+5);

          if(tempIndex = getMsg.indexOf("?")>3){//C language string lookup, find "?" If it is with '?', it indicates that the tilt angle is set
          getTemp = getMsg.substring(tempIndex+7);
          Serial.println(getTemp); 
          set_tilt=getTemp.toInt();
          Serial.printf("get_tilt:%d\r\n",set_tilt); 
          //Serial.print("get_tilt"+set_tilt);  
          mySerial.printf("?get_tilt:%d\r\n",set_tilt);
          }

          if(tempIndex = getMsg.indexOf("!")>3){//C language string lookup, find'!' If it is with '!', it indicates that the rotation speed is set
          getTemp = getMsg.substring(tempIndex+8);
          Serial.println(getTemp);   
          set_speed=getTemp.toInt();
          Serial.printf("get_speed:%d\r\n",set_speed);  
          //Serial.print("get_speed"+set_speed);   
          mySerial.printf("!get_speed:%d\r\n",set_speed);
          }
          
          if(tempIndex = getMsg.indexOf("*")>3){//C language string lookup, find '*' position If it is with '*', it indicates that the rotary cycles is set
          getTemp = getMsg.substring(tempIndex+9);
          Serial.println(getTemp);  
          set_cycles=getTemp.toInt();
          Serial.printf("get_cycles:%d\r\n",set_cycles); 
          //Serial.print("get_cycles"+set_cycles);  
          mySerial.printf("*get_cycles:%d\r\n",set_cycles);
          }

          
            
   TcpClient_Buff="";
   TcpClient_BuffIndex = 0;

  }
}
/*
  * Initialize wifi connection
*/
void startSTA(){
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_name, wifi_password);
}

/**************************************************************************
                                 WIFI
***************************************************************************/
/*
  WiFiTick
  Check if you need to initialize WiFi
  Check whether WiFi is connected and start TCP Client if the connection is successful
*/
void doWiFiTick(){
  static bool startSTAFlag = false;
  static bool taskStarted = false;
  static uint32_t lastWiFiCheckTick = 0;

  if (!startSTAFlag) {
    startSTAFlag = true;
    startSTA();
  }

  //  1s reconnection if it is not connected
  if ( WiFi.status() != WL_CONNECTED ) {
    if (millis() - lastWiFiCheckTick > 1000) {
      lastWiFiCheckTick = millis();
    }
  }
  //The connection was successfully established
  else {
    if (taskStarted == false) {
      taskStarted = true;
      Serial.print("\r\nGet IP Address: ");
      Serial.println(WiFi.localIP());
      startTCPClient();
    }
  }
}
//turn on device
void turnOnSwitch(){
  Serial.println("Turn ON");
  mySerial.print("N");
}
//turn off device
void turnOffSwitch(){
  Serial.println("Turn OFF");
  mySerial.print("F");
}

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  Serial.println("Beginning...");
}

void loop() {
  doWiFiTick();
  doTCPClientTick();
}
