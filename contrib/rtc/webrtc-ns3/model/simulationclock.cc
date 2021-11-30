#include "simulationclock.h"
#include "ns3/log.h"
namespace ns3
{
NS_LOG_COMPONENT_DEFINE("WebrtcSimulationClock");
int64_t SimulationClock::TimeInMilliseconds() const
{
	return Simulator::Now().GetMilliSeconds();
}
int64_t SimulationClock::TimeInMicroseconds() const
{
	return Simulator::Now().GetMicroSeconds();
}
webrtc::NtpTime SimulationClock::CurrentNtpTime() const
{
	NS_LOG_FUNCTION("this should not happen");
	int64_t now_ms =Simulator::Now().GetMilliSeconds();
	uint32_t seconds = (now_ms / 1000) + webrtc::kNtpJan1970;
	uint32_t fractions =
    	static_cast<uint32_t>((now_ms % 1000) * webrtc::kMagicNtpFractionalUnit / 1000);
	return webrtc::NtpTime(seconds, fractions);
}
int64_t SimulationClock::CurrentNtpInMilliseconds() const
{
	NS_LOG_FUNCTION("this should not happen");
	return Simulator::Now().GetMilliSeconds() + 1000 * static_cast<int64_t>(webrtc::kNtpJan1970);
}
}
