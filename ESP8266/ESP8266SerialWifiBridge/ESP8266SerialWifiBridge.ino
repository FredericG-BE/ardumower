/*
  Ardumower (www.ardumower.de)
  Copyright (c) 2013-2015 by Alexander Grau
  Copyright (c) 2013-2015 by Sven Gennat
  Copyright (c) 2014 by Maxime Carpentieri    
  Copyright (c) 2014-2015 by Stefan Manteuffel
  Copyright (c) 2015 by Uwe Zimprich
  Copyright (c) 2015 by Frederic Goddeeris
  Private-use only! (you need to ask for a commercial-use)
 
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
  Private-use only! (you need to ask for a commercial-use)
*/



#include <ESP8266WiFi.h>
#include <Ticker.h>

// When ECHOTEST is enabled, each received {xxx} will be echoed as [xxx]
// #define ECHOTEST
 
#define BAUDRATE 19200

#define MAX_CONFIG_LEN  100
#define MSG_HEADER "[WSB]"
#define VESRION "v0.9"
#define CONFIG_MSG_START "config:"
#define WELCOME_MSG MSG_HEADER " ESP8266 Serial WIFI Bridge " VESRION

typedef struct {
  const char* name;
  const char* valueStr;
} param_t;


typedef struct ledSequence_t{
  uint8_t onTicks;
  uint8_t offTicks;
} ledSequence_t;

struct {
  boolean on;
  uint8_t counter;
  const ledSequence_t* currSequence;
} ledStatus;



char configMsg[MAX_CONFIG_LEN];
param_t params[] = { 
  {"SSID",""},
  {"Password",""},
  {"IPAddress",""},
  {"Gateway",""},
  {"Subnet",""},
};
#define PARAMID_SSID    0
#define PARAMID_PASSWD  1
#define PARAMID_LOCALIP  2
#define PARAMID_GATEWAY 3
#define PARAMID_SUBNET  4
#define NBPARAMS (sizeof(params)/sizeof(params[0]))

const ledSequence_t ledSeq_startup        =   {1,10};             
const ledSequence_t ledSeq_waitForConfig  =   {1,1};      
const ledSequence_t ledSeq_connecting  =      {3,3};      
const ledSequence_t ledSeq_connected  =       {1,0};      
const ledSequence_t ledSeq_clientConnected  = {10,1};      
      
WiFiServer server(8080);
WiFiClient client;
bool clientConnected = false;

WiFiServer monitorServer(23);
WiFiClient monitorClient;
bool monitorConnected = false;

Ticker ledTicker;


void setLed(boolean on) {
  ledStatus.on = on;
  digitalWrite(2, on?HIGH:LOW);
}

void setLedSequence(const struct ledSequence_t& ledSeq) {
  ledStatus.currSequence = &ledSeq;
  ledStatus.counter = 0;
  setLed(true);
}

void onLedTicker(void) {
  ledStatus.counter++;
  if (ledStatus.on) {
    if (ledStatus.counter >= ledStatus.currSequence->onTicks) {
      if (ledStatus.currSequence->offTicks) {
        setLed(false);
      } else {
        // Stay ON
      }
      ledStatus.counter = 0;
    }
  } else {
    if (ledStatus.counter >= ledStatus.currSequence->offTicks) {
      setLed(true);
      ledStatus.counter = 0;
    }
  } 
}

void flushInput(void) {
  while(Serial.available())
    Serial.read();
}

void waitForParams(void) {
  boolean done = false;

  // Loop until a valid config message is received  
  while (! done) {
  
    // Loop until a line starting with "config:" is received
    uint8_t configMsgLen = 0; 
    boolean msgComplete = false; 
    while (! msgComplete) {
      if (Serial.available()) {
        char ch = Serial.read();
        if (ch == '\n' || ch == '\r') {
          msgComplete = true;
          configMsg[configMsgLen] = 0;  // Zero terminate
        } else {
          configMsg[configMsgLen] = ch;
          configMsgLen++;
          if (configMsgLen > MAX_CONFIG_LEN) {
            Serial.println(MSG_HEADER " ERR: Config too long");
            configMsgLen = 0; // discard all we have got so far  
          }
          if (memcmp(configMsg, CONFIG_MSG_START, min(strlen(CONFIG_MSG_START), configMsgLen)) != 0 ) {
            configMsgLen = 0; // discard all we have got so far 
          }
        }
      }
      yield();    
    }
    
    // Analyze the message
    int i;
    if (strlen(configMsg) >= strlen(CONFIG_MSG_START)) {
      // Split message into parameters
      char* p = configMsg + sizeof(CONFIG_MSG_START)-1;  // start after the "config:"
      for (i=0; i<NBPARAMS && *p; i++) {
        params[i].valueStr = p;
        p = strchr(p, ',');
        if (p) {
          *p = 0;
          p++;
        } else {
          break;
        }
      }
      if (i==NBPARAMS-1) {
        // Correct number of parameters, done
        done = true;
        Serial.println(MSG_HEADER " OK");
      } else {
        Serial.println(MSG_HEADER " ERR: Not enough parameters");
        configMsgLen = 0;
      }
    } else {
      Serial.println(MSG_HEADER " ERR: Expected \"" CONFIG_MSG_START "...\"");
    }
  
    delay(100);
    flushInput();
  }
  
}

void printParams(void) {
  int i;
  Serial.println(MSG_HEADER " Configuration parameters:");
  for (i=0; i<NBPARAMS; i++) {
    Serial.print(MSG_HEADER "    ");
    Serial.print(params[i].name);
    Serial.print(" = \"");
    Serial.print(params[i].valueStr);
    Serial.println("\"");
  }
}

void str2IpAddr(const char* str, IPAddress* ip) {
  int i;
  for (i=0; i<4; i++) {
    (*ip)[i]=atoi(str);
    str=strchr(str,'.');
    if (str)
      str++;
    else
      break;
  }
}


#ifdef ECHOTEST
char echotestMsg[20];
uint8_t echotestMsgLen = 0;
void echotestProcess(char ch) {
  if (ch == '{') {
    echotestMsg[0] = '[';
    echotestMsgLen = 1;
  } else if (ch == '}') {
    echotestMsg[echotestMsgLen] = ']';
    echotestMsgLen++;
    Serial.println();
    serverClients[0].write((uint8_t*)echotestMsg, echotestMsgLen);
    echotestMsgLen = 0;
  } else {
    if (echotestMsgLen > 0) {
      echotestMsg[echotestMsgLen] = ch;
      echotestMsgLen++;
    }
  }
}
#endif

#define DIR_UNDEF  0
#define DIR_W2S    1
#define DIR_S2W    2

void monitorHeader(uint8_t dir) {
  static uint8_t curr_dir = DIR_UNDEF;
  if (curr_dir != dir) {
    curr_dir = dir; 
    if (dir == DIR_W2S)
      monitorClient.print("\nW > S : ");
    else
      monitorClient.print("\nW < S : ");
  }
}

void doMonitor(char ch, uint8_t dir) {
  monitorHeader(dir);
  monitorClient.print(ch);
}

void doMonitor(const uint8_t* buf, size_t buflen, uint8_t dir) {
  monitorHeader(dir);
  monitorClient.write(buf, buflen);
}

void setup() {
  // Configure Serial Port
  Serial.begin(BAUDRATE);

  // Configure LED
  pinMode(2, OUTPUT);
  setLedSequence(ledSeq_startup);
  //ledTicker.attach(0.1, onLedTicker); 
   
  // Welcome message
  delay(500);
  Serial.println("\n\n");
  Serial.println(WELCOME_MSG);
  
  // Get configuration message
  setLedSequence(ledSeq_waitForConfig);
  waitForParams();
  printParams();
  
  // Configue the ESP8266
  setLedSequence(ledSeq_connecting);
  WiFi.begin(params[PARAMID_SSID].valueStr, params[PARAMID_PASSWD].valueStr);
  if (strlen(params[PARAMID_LOCALIP].valueStr) > 0) {
    IPAddress localIp;
    IPAddress gateway;
    IPAddress subnet;
    
    str2IpAddr(params[PARAMID_LOCALIP].valueStr, &localIp);
    str2IpAddr(params[PARAMID_GATEWAY].valueStr, &gateway);
    str2IpAddr(params[PARAMID_SUBNET].valueStr, &subnet);
    
    WiFi.config(localIp, gateway, subnet);
  }
  
  // Waiting for connection
  uint8 connectCnt = 0; 
  while (WiFi.status() != WL_CONNECTED ) {
    Serial.print(MSG_HEADER " Connecting ..."); 
    Serial.println(connectCnt); 
    connectCnt++;
    delay(500);
  }
  
  // Connected
  setLedSequence(ledSeq_connected);
  Serial.print(MSG_HEADER " CONNECTED! ");
  Serial.print(" IP address: ");
  Serial.println(WiFi.localIP());
  
  // Start server
  server.begin();
  server.setNoDelay(true);
  monitorServer.begin();
  monitorServer.setNoDelay(true);
}

uint8_t inbuf[100];
size_t bufLen = 0;

void loop() {
  setLed(false);
  
  // Handle Client connection
  if (clientConnected) {
    if (!client.connected())  {
      // Client is disconnected
      client.stop();
      clientConnected = false;
      setLedSequence(ledSeq_connected);
      Serial.println(MSG_HEADER " Client Disconnected");
    }
  }
  if (server.hasClient()) {
    // A new client tries to connect
    if (!clientConnected) {
      // OK accept the client
      client = server.available();
      clientConnected = true;
      setLedSequence(ledSeq_clientConnected);
      Serial.println(MSG_HEADER " Client Connected");  
    } else {
      // A client is already connetced, refuse
      WiFiClient serverClient = server.available();
      serverClient.stop();
    }
  }
  
  // Handle Monitor connection
  if (monitorConnected) {
    if (!monitorClient.connected())  {
      // monitor is disconnected
      monitorClient.stop();
      monitorConnected = false;
      Serial.println(MSG_HEADER " Monitor Disconnected");
    }
  }
  if (monitorServer.hasClient()) {
    // monitor tries to connect
    if (monitorConnected) {
      monitorClient.stop();
    }
    // OK accept the monitor
    monitorClient = monitorServer.available();
    monitorConnected = true;
    Serial.println(MSG_HEADER " Monitor Connected");
    monitorClient.println(WELCOME_MSG);
    
  }
  
 
  // Send all bytes received form the client to the Serial Port
  if (clientConnected){
    if(client.available()){
      char buf[200];
      size_t bytesAvailable = min(client.available(), sizeof(buf));
      for (size_t i=0; i<bytesAvailable; i++) {
        buf[i] = client.read();
      }  
      yield();
      Serial.write(buf, bytesAvailable);
      yield();
      if (monitorConnected) doMonitor((uint8_t*)buf, bytesAvailable, DIR_W2S);
    }
  }  
  
  
  // What is received on the Serial Port is sent to the client
//  if (Serial.available()>0){
//    setLed(true);
//    size_t len = Serial.available();//min(Serial.available(), 50);
//    Serial.print("I");
//    Serial.print(len);
//    uint8_t sbuf[len];
//    Serial.readBytes(sbuf, len);
//    if (clientConnected){
//      client.print(len);
//      client.print(" ");
//      //client.write(sbuf, len);
//      //if (monitorConnected) doMonitor(sbuf, len, DIR_S2W);
//      yield();
//    }
//    setLed(false);
//  }

  if (Serial.available()>0){
    
    size_t len = Serial.available();//min(Serial.available(), 50);
    Serial.readBytes(inbuf+bufLen, len);
    bufLen += len;
    
    if (inbuf[bufLen] == '}') {
        setLed(true);
        if (clientConnected){
        //  client.print(len);
        //  client.print(" ");
        uint8_t sbuf[6];
           //client.write(sbuf, len);
           bufLen = 0;
          //if (monitorConnected) doMonitor(sbuf, len, DIR_S2W);
          yield();
        }
        setLed(false);
    }
  }
  
}
