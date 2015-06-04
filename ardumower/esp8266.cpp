#include "esp8266.h"
#include "drivers.h"

#define SerialESP8266 Serial1

void Esp8266::initialise(String connectionString) {
  Console.print(F("ESP8266 Init..."));
  SerialESP8266.begin(115200);

  if (sendCmd("AT")) {
    if (sendCmd("AT+RST", "ready", 3000)) {
      if (sendCmd("AT+CWMODE=1", "no change")) {
        if (sendCmd("AT+CIPMODE=0")) {
          if (sendCmd("AT+CIPMUX=1")) {
            if (sendCmd((String("AT+CWJAP=")+connectionString).c_str(), "OK", 5000)) {
              if (sendCmd("AT+CIPSERVER=1,8080", "OK")) {
                 Console.print(F("ESP8266Init OK\n"));
              return;
              }
            }
          }
        }
      }
    }
  }
  Console.print(F("ESP8266 Init FAILED\n"));

  bRecData = false;
  recDataHeaderLen = 0;
  recDataLen = 0;
  recDataCnt = 0;
}

void Esp8266::checkAccessPointConnection(void) {
  sendCmd("AT+CIFSR");
}


boolean Esp8266::sendCmd(const char* cmd, const char* expectedReply, uint16_t timeoutMs) {
  while (SerialESP8266.available())
    SerialESP8266.read();

  Console.print(F("ESP8266 -> "));
  Console.println(cmd);
  SerialESP8266.print(cmd);
  SerialESP8266.print(F("\r\n"));

  return waitFor(expectedReply, timeoutMs);
}


boolean Esp8266::waitFor(const char* expMsg, uint16_t timeoutMs) {
  unsigned long startTime;
  uint8_t matchCount = 0;
  uint8_t expMsgLen = strlen(expMsg);

  Console.print(F("ESP8266 <- "));
  startTime = millis();
  while ( ((uint16_t)(millis()-startTime) < timeoutMs) &&  (matchCount < expMsgLen) ){
    if (SerialESP8266.available()) {
      char in = SerialESP8266.read();
      if (in == '\n' || in == '\r')
        in = '_';
      Console.print(in);
      if (in == expMsg[matchCount]) {
        matchCount++;
      } else {
        matchCount = 0;
      }
    }
  }
  if (matchCount == expMsgLen) {
    Console.println(F("  [OK]"));
    return true;
  } else {
    Console.println(F("  [TIMEOUT]"));
    return false;
  }
}

boolean Esp8266::sendData(const char* data, boolean newline) {
  char cmd[30];
  sprintf(cmd, "AT+CIPSEND=0,%d", strlen(data)+2+newline?1:0);
  if (sendCmd(cmd, ">"), 5000) {
      sendCmd(data, "OK", 5000);
  }
}


void Esp8266::print(const char* data) {
  sendData(data);
}

void Esp8266::println(const char* data) {
  sendData(data, true);
}

size_t Esp8266::print(const __FlashStringHelper *ifsh) {
  char str[2];

  str[1] = 0;

  const char PROGMEM *p = (const char PROGMEM *)ifsh;
    size_t n = 0;
    while (1) {
      unsigned char c = pgm_read_byte(p++);
      if (c == 0) break;
      n += 1;
      str[0] = c;
      sendData(str);
    }
    return n;
}

size_t Esp8266::println(const __FlashStringHelper *ifsh) {
  print(ifsh);
  println("");
  return 1;
}


void Esp8266::print(String s) {
  print(s.c_str());
}

void Esp8266::println(String s) {
  println(s.c_str());
}

void Esp8266::print(int& value) {
  char str[10];
  itoa(value, str, 10);
  sendData(str);
}

void Esp8266::println(int& value) {
  char str[10];
    itoa(value, str, 10);
    sendData(str, true);
}

void Esp8266::print(boolean b) {
  sendData(b?"True":"False");
}

void Esp8266::print(void) {
  sendData("");
}

void Esp8266::println(void) {
  sendData("", true);
}

void Esp8266::println(float value) {
  sendData("TODOFloat", true);
}

void Esp8266::println(unsigned char c) {
  char str[2];
  str[0] =c;
   str[1] = 0;
   sendData(str, true);
}


void Esp8266::resetDataReception(void) {
  recDataHeaderLen = 0;
  bRecData = false;
}

size_t Esp8266::available(void)
{
  if (!bRecData) {
    while (SerialESP8266.available() && !bRecData) {
      char c=SerialESP8266.read();
      Console.print(F("ESP8266 <- "));
      Console.print(c);
      if (recDataHeaderLen < RECDATA_MAX_HEADER_LEN) {
        recDataHeader[recDataHeaderLen] = c;
        recDataHeaderLen++;
        if (memcmp(recDataHeader, "+IPD", min(4, recDataHeaderLen)) != 0) {
          recDataHeaderLen = 0;
        } else {
          if (c==':') {
            if (recDataHeaderLen > 5) {
              Console.print("GOT HEADER!");
              recDataHeaderLen--;
              recDataHeader[recDataHeaderLen] = 0;
              Console.print(recDataHeader);
              Console.print(" ");
              bRecData = true;
              recDataCnt = 0;
              char* p = strchr(recDataHeader+5, ',');
              if (p) {
                recDataLen = atoi(p+1);
                Console.print(recDataLen);
              } else {
                resetDataReception();
              }
            } else {
              resetDataReception();
            }
          }
        }
      } else {
        resetDataReception();
      }
      Console.println("");
    }
  }

  if (bRecData) {
    if (SerialESP8266.available()) {
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

char Esp8266::read(void) {
  if (bRecData && SerialESP8266.available()) {
    char c = SerialESP8266.read();
    recDataCnt++;
    if (recDataCnt >= recDataLen) {
      resetDataReception();
    }
    Console.print("yealding ");
    Console.println(c);
    return c;
  } else {
    return 0;
  }
}

void Esp8266::flush(void) {

}

