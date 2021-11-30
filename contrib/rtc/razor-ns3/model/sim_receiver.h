/*
 * sim_receiver.h
 *
 *  Created on: 2018.4.27
 *      Author: zsy
 */

#ifndef SIM_RECEIVER_H
#define SIM_RECEIVER_H
#include "razor_header.h"
#include "razor_api.h"
#include "ns3/socket.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/callback.h"
#include<vector>
namespace ns3
{
typedef struct
{
	uint32_t			fid;
	uint32_t			last_seq;
	uint32_t			ts;
	int					frame_type;

	int					seg_number;
	sim_segment_t**		segments;
}sim_frame_t;
enum{
	buffer_waiting = 0,
	buffer_playing
};
class SimReceiver
{
public:
	SimReceiver();
	~SimReceiver();
	void SetHeader(sim_header_t sheader);
	void SetFeedBack(Callback<void,Ptr<Packet>>feedback);
	static void SendFeedback(void*handler,const uint8_t* payload, int payload_size);
	void RecvFakeData(sim_segment_t seg);
	uint32_t GetFakeData();//just some length;
	void Reset();
	void SetBitrate(uint32_t minBitRate,uint32_t maxBitRate);
	void HeartBeat();
	void StatsRecvRate();
	void UpdateRtt(uint32_t rtt,uint32_t rtt_var);
	typedef Callback<void,uint32_t> LossSeqCallback;
	void SetLossSeqTrace(LossSeqCallback cb){
		m_lossSeqCb=cb;
	}
private:
	uint32_t GetIndex(uint32_t i);
//the important part is not about video consuming,so ignore it
	uint32_t m_minBitRate;//80000 10KB
	uint32_t m_maxBitRate;//16000000 2MB
	uint32_t m_bufferSize;//1024
	std::vector<sim_frame_t> m_frameBuffer;
	uint32_t m_minfid;
	uint32_t m_maxfid;
	uint32_t m_framets;		/*�Ѿ����ŵ������Ƶʱ���*/
	uint64_t m_playts;		/*��ǰϵͳʱ�ӵ�ʱ���*/

	uint32_t m_maxts;//maximium ts in buffer
	uint32_t m_frameTime;//frame interval
	uint32_t m_waitTime;//max buffer waiting time
	bool m_recvFirstPacket;
	uint8_t m_state;
	razor_receiver_t*	m_cc;
	sim_header_t m_sHeader;
	uint32_t m_statsDataLen;//stastics data length
	//int64_t m_lastStatsTime;
	EventId m_heartbeatEvent;
	uint32_t m_heartbeat;// in 5 milliseconds
	EventId m_statsRateEvent;
	uint32_t m_rateStatsInterval;//in 100ms;
	Callback<void,Ptr<Packet>> m_feedback;
	uint32_t m_base_seq{0};
	LossSeqCallback m_lossSeqCb;
};
}

#endif /* SIM_RECEIVER_H */
