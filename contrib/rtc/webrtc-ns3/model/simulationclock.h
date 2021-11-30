#ifndef SIMULATION_CLOCK_H
#define SIMULATION_CLOCK_H
#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "system_wrappers/include/clock.h"
namespace ns3
{

class SimulationClock:public webrtc::Clock
{
public:
	SimulationClock(){}
	~SimulationClock(){}
	int64_t TimeInMilliseconds() const override;
	int64_t TimeInMicroseconds() const override;
	webrtc::NtpTime CurrentNtpTime() const override;
	int64_t CurrentNtpInMilliseconds() const override;
};
}
#endif
