#ifndef G_CLOCK_H
#define  G_CLOCK_H
#include"rtc_base/timeutils.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
namespace ns3
{
class GClock:public rtc::ClockInterface
{
public:
	GClock(){}
	~GClock(){}
	int64_t TimeNanos() const override
	{
		return Simulator::Now().GetNanoSeconds();
	}
};
static GClock f_clock;
void SetClockForWebrtc()
{
	rtc::SetClockForTesting(&f_clock);
}
int64_t GetMilliSecondsFromWebrtc()//just for testing;
{
		return rtc::TimeMillis();
}
}
#endif
