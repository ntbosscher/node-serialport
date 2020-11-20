//
//  util.hpp
//  
//
//  Created by nate bosscher on 2020-11-18.
//

#ifndef util_h
#define util_h

#include <stdio.h>
#include <fstream>
#include <nan.h>
#include <string.h>

#define strncasecmp strnicmp

enum SerialPortParity {
  SERIALPORT_PARITY_NONE  = 1,
  SERIALPORT_PARITY_MARK  = 2,
  SERIALPORT_PARITY_EVEN  = 3,
  SERIALPORT_PARITY_ODD   = 4,
  SERIALPORT_PARITY_SPACE = 5
};

enum SerialPortStopBits {
  SERIALPORT_STOPBITS_ONE      = 1,
  SERIALPORT_STOPBITS_ONE_FIVE = 2,
  SERIALPORT_STOPBITS_TWO      = 3
};

#define TIMEOUT_PRECISION 10
#define ERROR_STRING_SIZE 1024
#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

void ErrorCodeToString(const char* prefix, int errorCode, char *errorStr);

void setIfNotEmpty(v8::Local<v8::Object> item, std::string key, const char *value);
char *copySubstring(char *someString, int n);

v8::Local<v8::Value> getValueFromObject(v8::Local<v8::Object> options, std::string key);
int getIntFromObject(v8::Local<v8::Object> options, std::string key);
bool getBoolFromObject(v8::Local<v8::Object> options, std::string key);
v8::Local<v8::String> getStringFromObj(v8::Local<v8::Object> options, std::string key);
double getDoubleFromObject(v8::Local<v8::Object> options, std::string key);

std::ofstream logger(std::string id);

int currentMs();

std::string wStr2Char(wchar_t *buf);
char *guid2Str(const GUID *id, char *out);

void logPerf(std::string value);

#endif /* util_hpp */
