#include "./Util.h"
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>

const char kPathSeparator =
#ifdef _WIN32
                            '\\';
#else
                            '/';
#endif

bool enableVerboseLogging;
std::string loggingDirectory = "";
std::mutex muVerboseLogging;

bool verboseLoggingEnabled() {
  muVerboseLogging.lock();
  auto enabled = enableVerboseLogging;
  muVerboseLogging.unlock();

  return enabled;
}

std::string getLoggingDirectory() {
  muVerboseLogging.lock();
  auto dir = loggingDirectory;
  muVerboseLogging.unlock();

  return dir;
}

void configureLogging(bool _enabled, std::string _dir) {
  muVerboseLogging.lock();
  enableVerboseLogging = _enabled;
  loggingDirectory = _dir;
  muVerboseLogging.unlock();
}

const char *copySubstring(const char *someString, int n) {
    std::string str(someString);
    return str.substr(0, n).c_str();
}

void setIfNotEmpty(v8::Local<v8::Object> item, std::string key, const char *value) {
    v8::Local<v8::String> v8key = Nan::New<v8::String>(key).ToLocalChecked();
    if (strlen(value) > 0) {
        Nan::Set(item, v8key, Nan::New<v8::String>(value).ToLocalChecked());
    } else {
        Nan::Set(item, v8key, Nan::Undefined());
    }
}

std::string bufferToHex(char* buffer, int len) {
  std::stringstream output;

  for(int i = 0; i < len; i++) {
      output << std::hex << "0x" << (((int)(buffer[i])) & 0xff) << " ";
  }

  return output.str();
}

std::ofstream defaultLogger() {
  return logger("default");
}

std::ofstream logger(std::string id) {
    return std::ofstream(getLoggingDirectory() + kPathSeparator + "faster-serialport-log." + id + ".txt", std::ofstream::app);
}

v8::Local<v8::Value> getValueFromObject(v8::Local<v8::Object> options, std::string key) {
  v8::Local<v8::String> v8str = Nan::New<v8::String>(key).ToLocalChecked();
  return Nan::Get(options, v8str).ToLocalChecked();
}

int getIntFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::Int32>(getValueFromObject(options, key)).ToLocalChecked()->Value();
}

bool getBoolFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::Boolean>(getValueFromObject(options, key)).ToLocalChecked()->Value();
}

v8::Local<v8::String> getStringFromObj(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<v8::String>(getValueFromObject(options, key)).ToLocalChecked();
}

double getDoubleFromObject(v8::Local<v8::Object> options, std::string key) {
  return Nan::To<double>(getValueFromObject(options, key)).FromMaybe(0);
}

int currentMs() {
  return clock() / (CLOCKS_PER_SEC / 1000);
}

std::string wStr2Char(wchar_t *buf)
{
    // std::wstring wstr(buf);
    // std::string cstr(wstr.begin(), wstr.end());

    char* out = (char*)malloc(wcslen(buf)+1);
    sprintf(out, "%ls", buf);

    auto str = std::string(out);
    free(out);

    return str;
}

std::ofstream perfLogger;
std::string tracking;
int trackingStart = 0;
std::mutex muPerfLogger;

void logPerf(std::string value) {
  auto now = currentMs();

  muPerfLogger.lock();

  if(!perfLogger) {
    perfLogger = logger("performance");
  }

  perfLogger << now << " " << value;

  if(tracking == value) {
    perfLogger << " (" << (now - trackingStart) << "ms)";
    tracking = "";
    trackingStart = 0;
  } else {
    tracking = value;
    trackingStart = now;
  }

  perfLogger << "\n";
  perfLogger.flush();

  muPerfLogger.unlock();
}