/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ana-sender.h"
#include "ana-packet-tag.h"
#include <algorithm>

namespace ns3 {

namespace {
static const uint64_t SEND_INTERVAL = 20;
static const uint64_t TICK_INTERVAL = 1000;
static const uint64_t FS_MULT = 48;
static const Time BWCTL_INTERVAL = MilliSeconds (1000);
static const size_t CWND = 1500;
static const uint32_t MIN_PACKET_SIZE = 20; // 8kbps
static const uint32_t INIT_PACKET_SIZE = 130; // 64kbps
static const uint32_t PACKET_SIZE_STEP = 10; // 4kbps
} // namespace

NS_LOG_COMPONENT_DEFINE ("AnaSenderApplication");

NS_OBJECT_ENSURE_REGISTERED (AnaSender);

TypeId
AnaSender::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AnaSender")
                          .SetParent<Application> ()
                          .SetGroupName ("Applications")
                          .AddConstructor<AnaSender> ();
  return tid;
}

AnaSender::AnaSender ()
{
}
AnaSender::~AnaSender ()
{
  m_socket = nullptr;
}

void
AnaSender::Setup (Ptr<UdpSocket> socket, Address address, uint32_t maxPacketSize, uint32_t nPackets)
{
  m_socket = socket;
  m_peer = address;
  m_maxPacketSize = maxPacketSize;
  m_nPackets = nPackets;
  m_packetSize = std::min(m_maxPacketSize, std::max(MIN_PACKET_SIZE, INIT_PACKET_SIZE));

  m_socket->SetRecvCallback (MakeCallback (&AnaSender::HandleRead, this));
  m_packetHistory = CreateObject<SendPacketHistory> (this, Seconds (15), 100);
  m_inflightDataSizeHistory = CreateObject<HistoryBuffer<size_t>> (30);
}

void
AnaSender::StartApplication ()
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  Tick ();
  SendPacket ();
}
void
AnaSender::StopApplication ()
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_tickEvent.IsRunning ())
    {
      Simulator::Cancel (m_tickEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
}
int64_t
AnaSender::GetSendRate () const
{
  return (m_packetSize + ANA_PACKET_OVERHEAD) * 8 * 1000 / SEND_INTERVAL;
}

void
AnaSender::SendPacket ()
{
  const Time now = Simulator::Now ();
  m_packetHistory->CheckLoss (now);
  m_inflightDataSizeHistory->Add (m_packetHistory->m_inflightDataSize);

  if (m_packetsSent == 0)
    {
      NS_LOG_UNCOND (now.As (Time::S) << " Action " << BWctrlActionStr (m_lastBWctrlAction)
                                      << " PacketSize " << m_packetSize);
      m_rateCb (GetSendRate ());
    }

  if (now - m_lastBWctrlTime >= BWCTL_INTERVAL)
    {
      /*
      if (m_inflightDataSizeHistory->GetAvg () > CWND + CWND)
        {
          m_lastBWctrlAction = BWctrlAction::PAUSE;
        }
      else if (m_inflightDataSizeHistory->GetAvg () > CWND + CWND / 5)
        {
          m_lastBWctrlAction = BWctrlAction::DECR;
          m_packetSize = std::min (m_maxPacketSize,
                                   std::max (MIN_PACKET_SIZE, m_packetSize - m_packetSize / 3));
        }
      else if (m_inflightDataSizeHistory->GetAvg () < CWND - CWND / 5)
        {
          m_lastBWctrlAction = BWctrlAction::INCR;
          m_packetSize = std::min (m_maxPacketSize,
                                   std::max (MIN_PACKET_SIZE, m_packetSize + PACKET_SIZE_STEP));
        }
      else
        {
          m_lastBWctrlAction = BWctrlAction::KEEP;
        }
      */
      if (m_lossRate >= 0.1)
        {
          m_lastBWctrlAction = BWctrlAction::DECR;
#if 0
          m_packetSize = std::min (
              m_maxPacketSize,
              std::max (MIN_PACKET_SIZE,
                        m_packetSize - uint32_t (m_packetSize * (0.5 * m_lossRate))));
#else
          uint32_t dataSize = m_packetSize + ANA_PACKET_OVERHEAD;
          double decrRatio = (0.5 * m_lossRate);
          dataSize = std::min (
              m_maxPacketSize + ANA_PACKET_OVERHEAD,
              std::max (MIN_PACKET_SIZE + ANA_PACKET_OVERHEAD,
                        dataSize - uint32_t (dataSize * decrRatio)));
          m_packetSize = dataSize - ANA_PACKET_OVERHEAD;
#endif
        }
      else if (m_lossRate < 0.02)
        {
          m_lastBWctrlAction = BWctrlAction::INCR;
#if 0
          m_packetSize = std::min (m_maxPacketSize,
                                   std::max (MIN_PACKET_SIZE, m_packetSize + PACKET_SIZE_STEP));
#else
          uint32_t dataSize = m_packetSize + ANA_PACKET_OVERHEAD;
          dataSize = std::min (
              m_maxPacketSize + ANA_PACKET_OVERHEAD,
              std::max (MIN_PACKET_SIZE + ANA_PACKET_OVERHEAD, dataSize + PACKET_SIZE_STEP));
          m_packetSize = dataSize - ANA_PACKET_OVERHEAD;
#endif
        }
      else
        {
          m_lastBWctrlAction = BWctrlAction::KEEP;
        }
      m_rateCb (GetSendRate ());
      NS_LOG_UNCOND (now.As (Time::S) << " Action " << BWctrlActionStr (m_lastBWctrlAction)
                                      << " PacketSize " << m_packetSize);
      m_lastBWctrlTime = now;
    }

  if (m_lastBWctrlAction != BWctrlAction::PAUSE)
    {
      Ptr<Packet> packet = Create<Packet> (m_packetSize);
      AnaRtpTag tag;
      tag.m_time = now.GetMicroSeconds ();
      tag.m_seq = m_nextSeq;
      tag.m_timestamp = (uint32_t) m_nextTimestamp;
      packet->AddPacketTag (tag);
      m_socket->Send (packet);
      m_packetHistory->PacketSent (now, tag, m_packetSize);
      NS_LOG_UNCOND (now.As (Time::MS) << " Send Seq " << tag.m_seq);

      ++m_nextSeq;
    }
  else
    {
      NS_LOG_UNCOND ("Skip " << m_nextTimestamp);
    }

  m_nextTimestamp += SEND_INTERVAL * FS_MULT;

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
AnaSender::ScheduleTx ()
{
  if (m_running)
    {
      m_sendEvent =
          Simulator::Schedule (MilliSeconds (SEND_INTERVAL), &AnaSender::SendPacket, this);
    }
}
void
AnaSender::Tick ()
{
  int64_t rateAck = m_packetHistory->m_newlyAckDataSize * 8000 / TICK_INTERVAL;
  m_rateAckCb (rateAck);
  m_packetHistory->m_newlyAckDataSize = 0;

  NS_ASSERT (m_packetHistory->m_received <= m_packetHistory->m_expected);
  size_t expected = m_packetHistory->m_expected - m_packetHistory->m_expectedPrior;
  size_t received = m_packetHistory->m_received - m_packetHistory->m_receivedPrior;
  size_t loss = expected - received;
  double lossRate;
  if (expected == 0 || loss == 0)
    lossRate = 0;
  else
    lossRate = (double) loss / expected;
  m_lossRateCb (lossRate);
  NS_LOG_UNCOND (Simulator::Now ().As (Time::MS)
                 << " Loss " << loss << " Expected " << expected << " LossRate " << lossRate);
  m_packetHistory->m_expectedPrior = m_packetHistory->m_expected;
  m_packetHistory->m_receivedPrior = m_packetHistory->m_received;
  m_lossRate = lossRate;

  m_tickEvent = Simulator::Schedule (MilliSeconds (TICK_INTERVAL), &AnaSender::Tick, this);
}
void
AnaSender::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client received "
                              << packet->GetSize () << " bytes from "
                              << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port "
                              << InetSocketAddress::ConvertFrom (from).GetPort ());
      socket->GetSockName (localAddress);
      // m_rxTrace (packet);
      // m_rxTraceWithAddresses (packet, from, localAddress);
      AnaFeedbackTag tag;
      if (packet->PeekPacketTag (tag))
        {
          m_packetHistory->TransportFeedback (Simulator::Now (), tag);
          auto maxSeq = tag.GetMaxSeq ();
          auto inflight = m_nextSeq - 1 - maxSeq;
          NS_LOG_UNCOND ("Inflight " << inflight << "p, Size "
                                     << m_packetHistory->m_inflightDataSize);
        }
    }
}
/* -------------------------------------------------------------------------- */
SendPacketHistory::SendPacketHistory (AnaSender *sender, Time timeThrehold, uint16_t seqThrehold)
    : m_sender (sender), m_timeThrehold (timeThrehold), m_seqThrehold (seqThrehold)
{
}
void
SendPacketHistory::CheckLoss (Time time)
{
  for (auto &p : m_queue)
    {
      if (p.ackTime.IsZero () && time - p.sendTime >= MilliSeconds (1000))
        {
          p.ackTime = time;
          m_inflightDataSize -= p.dataSize;
          ++m_lossCount;
          p.timeoutLoss = true;
        }
    }
}
void
SendPacketHistory::PacketSent (Time time, const AnaRtpTag &tag, uint16_t packetSize)
{
  while (!m_queue.empty ())
    {
      auto &p = m_queue.front ();
      if (time - p.sendTime > m_timeThrehold)
        {
          m_queue.pop_front ();
        }
      else
        {
          break;
        }
    }
  uint16_t dataSize = ANA_PACKET_OVERHEAD + packetSize;
  m_queue.push_back ({time, Time (), tag, dataSize, false});
  ++m_packetCount;
  m_inflightDataSize += dataSize;

  // track highest sequence number
  if (m_packetCount == 1)
    {
      m_maxSeq = tag.m_seq;
    }
  else
    {
      WrapComp<uint16_t> comp;
      if (comp (m_maxSeq, tag.m_seq))
        {
          m_maxSeq = tag.m_seq;
        }
    }
}
void
SendPacketHistory::TransportFeedback (Time time, const AnaFeedbackTag &tag)
{
  NS_LOG_UNCOND (time.As (Time::MS) << " Feedback: AckSeq " << tag.GetMaxSeq ());
  uint16_t max_seq = tag.GetMaxSeq ();

  if (m_expected < (size_t) max_seq + 1)
    m_expected = max_seq + 1;

  WrapComp<uint16_t> seq_comp;
  for (auto &p : m_queue)
    {
      if (p.tag.m_seq == max_seq)
        {
          if (p.ackTime.IsZero ())
            {
              ++m_received;

              p.ackTime = time;
              m_inflightDataSize -= p.dataSize;
              m_newlyAckDataSize += p.dataSize;

              Time rtt = time - p.sendTime;
              m_sender->m_rttCb (rtt.GetMilliSeconds ());
            }
          break;
        }

      if (!seq_comp (p.tag.m_seq, max_seq))
        break;
    }

  for (auto seq : tag.m_ackSeq)
    {
      auto p = FindPacket (seq);
      if (p)
        {
          if (p->timeoutLoss)
            {
              ++m_received;

              p->timeoutLoss = false;
              --m_lossCount;
              ++m_falseLossCount;

              p->ackTime = time;
            }
          else if (p->ackTime.IsZero ())
            {
              ++m_received;

              p->ackTime = time;
              m_inflightDataSize -= p->dataSize;
              m_newlyAckDataSize += p->dataSize;
            }
        }
    }
}
SendPacketInfo *
SendPacketHistory::FindPacket (uint16_t seq)
{
  for (auto &p : m_queue)
    {
      if (p.tag.m_seq == seq)
        {
          return &p;
        }
    }
  return nullptr;
}
} // namespace ns3
