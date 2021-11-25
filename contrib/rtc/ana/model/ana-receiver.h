/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef ANA_H
#define ANA_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ana-packet-tag.h"
#include <deque>

namespace ns3 {

class RecvPacketHistory;
class AnaReceiver : public Application
{
public:
  static TypeId GetTypeId (void);

  void Setup (uint16_t port);
  using DelayCallback = Callback<void, int64_t>;
  void
  SetDelayCallback (DelayCallback cb)
  {
    m_delayCb = cb;
  }

private:
  virtual void StartApplication () override;
  virtual void StopApplication () override;
  void HandleRead (Ptr<Socket> socket);

  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<UdpSocket> m_socket; //!< IPv4 Socket
  Ptr<RecvPacketHistory> m_packetHistory;
  DelayCallback m_delayCb;
};

struct RecvPacketInfo
{
  ns3::Time time;
  AnaRtpTag tag;
};
class RecvPacketHistory : public Object
{
public:
  RecvPacketHistory (Time timeThrehold, uint16_t seqThrehold);
  ~RecvPacketHistory () = default;

  void Add (Time time, const AnaRtpTag &tag);
  bool GetFeedback (AnaFeedbackTag &tag);

  std::deque<RecvPacketInfo> m_queue;
  const Time m_timeThrehold;
  const uint16_t m_seqThrehold;
  uint64_t m_packetCount = 0;
  uint16_t m_maxSeq = 0;
};

} // namespace ns3

#endif /* ANA_H */
