#ifndef PACKET_GENERATOR_H
#define PACKET_GENERATOR_H
#include "processmodule.h"
//#include "modules/congestion_controller/include/network_changed_observer.h"
#include "modules/congestion_controller/include/send_side_congestion_controller.h"
#include "target.h"
#include"ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/callback.h"
namespace ns3
{
class PacketGenerator:public webrtc::SendSideCongestionController::Observer
{
public:
	PacketGenerator(){}
	~PacketGenerator(){}
	typedef Callback<void,float> RateCallback;
    	void SetRateCallback(RateCallback cb){m_rateCb=cb;}
	void SetTarget(Target *target){m_target=target;}
	void SetFs(uint32_t fs){m_fs=fs;}
	void SetBitRate(uint32_t bitRate){m_bitRate=bitRate;}
	void Start();
	void Stop();
	void SetSource(uint32_t ssrc){m_ssrc=ssrc;}
	virtual void OnNetworkChanged(uint32_t bitrate_bps,
	                              uint8_t fraction_loss,  // 0 - 255.
	                              int64_t rtt_ms,
	                              int64_t probing_interval_ms);
private:
	void Generate();
	Target *m_target;
	uint32_t m_ssrc{0};//for log purpose
	uint32_t m_fs{40};
	uint32_t m_bitRate{200000};
	EventId m_timer;
    	RateCallback m_rateCb;
};	
}
#endif
