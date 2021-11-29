#ifndef BW_TRACE_H
#define BW_TRACE_H
#include "ns3/core-module.h"
#include "ns3/packet.h"
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
namespace ns3
{
class BwTracer : public Object
{
public:
	BwTracer();
	~BwTracer();
	void OpenTraceFile(std::string filename);
	void CloseTraceFile();
  void SetPacketOverhead (int32_t packetOverhead);
	void PacketTrace(Ptr<const Packet> packet, const Address &addr);
  void Start (Time time);
  void Stop (Time time);

private:
  void Tick ();
  int64_t ComputeRate (Time time);

	std::fstream m_traceFile;
  int32_t m_packetOverhead = 0;
  int32_t m_dataSize = 0;
  Time m_startTime;
  Time m_stopTime;
  EventId m_tickEvent;
  struct Item {
    Time time;
    int32_t dataSize;
  };
  std::deque<Item> m_queue;
};
}
#endif
