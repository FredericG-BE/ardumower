#ifndef __ESP8266_H__
#define __ESP8266_H__


#include <Arduino.h>
#include "comms.h"

#define RECDATA_MAX_HEADER_LEN  20

class Esp8266 :public comms
{
  public:
    void initialise(String connectionString);
    void checkAccessPointConnection(void);
    boolean sendCmd(const char* cmd, const char* expectedReply="OK", uint16_t timeoutMs=1000);
    boolean waitFor(const char* expectedMsg, uint16_t timeoutMs);
    boolean sendData(const char* data, boolean newline = false);

    void print(void);
    void println(void);
    void print(const char* data);
    void println(const char* data);
    void print(boolean b);
    void println(uint8_t value);
    void println(float value);
    void print(int& value);
    void println(int& value);
    size_t print(const __FlashStringHelper *ifsh);
    size_t println(const __FlashStringHelper *ifsh);
    void print(String s);
    void println(String s);



    size_t available(void);
    char read(void);
    void flush(void);



  private:
    void resetDataReception(void);
    bool bRecData;
    char recDataHeader[RECDATA_MAX_HEADER_LEN];
    uint8_t recDataHeaderLen;
    uint16_t recDataLen;
    uint16_t recDataCnt;

};

#endif //__ESP8266_H__

