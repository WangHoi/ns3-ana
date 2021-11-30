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

class SendPacketHistory;
template <typename>
class HistoryBuffer;
class AnaSender : public Application
{
public:
  static TypeId GetTypeId ();

  AnaSender ();
  virtual ~AnaSender () override;

  void Setup (Ptr<UdpSocket> socket, Address address, uint32_t maxPacketSize, uint32_t nPackets);
  using RateCallback = Callback<void, int64_t>;
  void
  SetRateCallback(RateCallback cb)
  {
    m_rateCb = cb;
  }
  void
  SetRateAckCallback(RateCallback cb)
  {
    m_rateAckCb = cb;
  }
  int64_t GetSendRate() const;
  using RttCallback = Callback<void, int64_t>;
  void
  SetRttCallback (RttCallback cb)
  {
    m_rttCb = cb;
  }
  using LossRateCallback = Callback<void, double>;
  void
  SetLossRateCallback (LossRateCallback cb)
  {
    m_lossRateCb = cb;
  }

private:
  virtual void StartApplication () override;
  virtual void StopApplication () override;
  void SendPacket ();
  void Tick ();
  void ScheduleTx ();
  void HandleRead (Ptr<Socket> socket);

  Ptr<UdpSocket> m_socket;
  Address m_peer;
  uint32_t m_maxPacketSize = 0;
  uint32_t m_packetSize = 0;
  uint32_t m_nPackets = 0;
  EventId m_sendEvent;
  EventId m_tickEvent;
  bool m_running = false;
  uint32_t m_packetsSent = 0;
  double m_lossRate = 0.0;

  uint16_t m_nextSeq = 0;
  uint64_t m_nextTimestamp = 0;
  Ptr<SendPacketHistory> m_packetHistory;
  Ptr<HistoryBuffer<size_t> > m_inflightDataSizeHistory;
  Time m_lastBWctrlTime;
  enum class BWctrlAction {
    KEEP,
    INCR,
    DECR,
    PAUSE,
  };
  BWctrlAction m_lastBWctrlAction = BWctrlAction::KEEP;
  RateCallback m_rateCb;
  RateCallback m_rateAckCb;
  RttCallback m_rttCb;
  LossRateCallback m_lossRateCb;
  friend class SendPacketHistory;

  static const char *BWctrlActionStr(BWctrlAction action)
  {
    switch (action) {
    case BWctrlAction::KEEP:
      return "KEEP";
    case BWctrlAction::INCR:
      return "INCR";
    case BWctrlAction::DECR:
      return "DECR";
    case BWctrlAction::PAUSE:
      return "PAUSE";
    }
    NS_ASSERT(0);
  }
};

struct SendPacketInfo
{
  ns3::Time sendTime;
  ns3::Time ackTime;
  AnaRtpTag tag;
  uint16_t dataSize;
  bool timeoutLoss;
};
class SendPacketHistory : public Object
{
public:
  SendPacketHistory (AnaSender *sender, Time timeThrehold, uint16_t seqThrehold);
  ~SendPacketHistory () = default;

  void PacketSent (Time time, const AnaRtpTag &tag, uint16_t packetSize);
  void TransportFeedback (Time time, const AnaFeedbackTag &tag);
  void CheckLoss (Time time);

  AnaSender *m_sender;
  std::deque<SendPacketInfo> m_queue;
  const Time m_timeThrehold;
  const uint16_t m_seqThrehold;
  uint64_t m_packetCount = 0;
  uint16_t m_maxSeq = 0;
  size_t m_inflightDataSize = 0;
  size_t m_newlyAckDataSize = 0;
  size_t m_lossCount = 0;
  size_t m_falseLossCount = 0;

  size_t m_expected = 0;
  size_t m_expectedPrior = 0;
  size_t m_received = 0;
  size_t m_receivedPrior = 0;

private:
  SendPacketInfo *FindPacket (uint16_t seq);
};
template <typename T>
class HistoryBuffer : public Object
{
public:
  HistoryBuffer (size_t max) : m_max (max)
  {
  }
  void
  Clear ()
  {
    m_queue.clear ();
  }
  void
  Add (T value)
  {
    // NS_LOG_UNCOND (Simulator::Now ().As (Time::MS) << " Add " << value);
    if (m_queue.size () >= m_max)
      {
        m_queue.pop_front ();
      }
    m_queue.push_back (value);
  }
  T
  GetMin () const
  {
    T min = std::numeric_limits<T>::max ();
    for (auto v : m_queue)
      {
        if (v < min)
          {
            min = v;
          }
      }
    return min;
  }
  T
  GetMax () const
  {
    T max = std::numeric_limits<T>::min ();
    for (auto v : m_queue)
      {
        if (v > max)
          {
            max = v;
          }
      }
    return max;
  }
  T
  GetAvg () const
  {
    NS_ASSERT (!m_queue.empty ());
    int64_t total = 0;
    for (auto v : m_queue)
      {
        total += static_cast<int64_t> (v);
      }
    // NS_LOG_UNCOND ("Avg " << total << " " << m_queue.size ());
    return static_cast<T> (total / m_queue.size ());
  }

private:
  std::deque<T> m_queue;
  size_t m_max;
};
} // namespace ns3

#endif /* ANA_SENDER_H */
