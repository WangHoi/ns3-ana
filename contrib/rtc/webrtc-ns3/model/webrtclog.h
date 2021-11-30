#ifndef WEBRTC_LOG_H
#define WEBRTC_LOG_H
#include "rtc_base/logging.h"
namespace ns3
{
class LogSinkConsole:public rtc::LogSink 
{
public: 
    LogSinkConsole(){}
    ~LogSinkConsole(){}
    void OnLogMessage(const std::string &message) override;
    bool Init();
};
}
#endif
