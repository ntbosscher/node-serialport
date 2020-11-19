//
//  util.hpp
//  
//
//  Created by nate bosscher on 2020-11-18.
//

#ifndef util_h
#define util_h

#include <stdio.h>
#include <v8.h>
#include <nan.h>

#define TIMEOUT_PRECISION 10
#define ERROR_STRING_SIZE 1024
#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

void ErrorCodeToString(const char* prefix, int errorCode, char *errorStr);
std::ofstream logger(std::string id);

void setIfNotEmpty(v8::Local<v8::Object> item, std::string key, const char *value);
char *copySubstring(char *someString, int n);

#endif /* util_hpp */
