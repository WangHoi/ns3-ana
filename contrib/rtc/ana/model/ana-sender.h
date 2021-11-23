/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef ANA_SENDER_H
#define ANA_SENDER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ana-packet-tag.h"
#include <deque>

namespace ns3 {

class AnaSender : public Application
{
public:
  static TypeId GetTypeId ();
  
  AnaSender ();
  virtual ~AnaSender () override;
  
  void Setup (Ptr<UdpSocket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication () override;
  virtual void StopApplication () override;
  void SendPacket ();
  void ScheduleTx ();
  void HandleRead (Ptr<Socket> socket);

  Ptr<UdpSocket> m_socket;
  Address m_peer;
  uint32_t m_packetSize = 0;
  uint32_t m_nPackets = 0;
  DataRate m_dataRate;
  EventId m_sendEvent;
  bool m_running = false;
  uint32_t m_packetsSent = 0;
  
  uint16_t m_nextSeq = 0;
  uint64_t m_nextTimestamp = 0;
};

struct SendPacketInfo
{
  ns3::Time sendTime;
  ns3::Time ackTime;
  AnaRtpTag tag;
};
class SendPacketHistory : public Object
{
public:
  SendPacketHistory (Time timeThrehold, uint16_t seqThrehold);
  ~SendPacketHistory () = default;

  void Prepare (Time time, const AnaRtpTag &tag);
  void Update (Time time, const AnaFeedbackTag &tag);

  std::deque<SendPacketInfo> m_queue;
  const Time m_timeThrehold;
  const uint16_t m_seqThrehold;
  uint64_t m_packetCount = 0;
  uint16_t m_maxSeq = 0;

private:
  SendPacketInfo *FindPacket(uint16_t seq);
};

} // namespace ns3

#endif /* ANA_SENDER_H */
