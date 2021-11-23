/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ana-sender.h"
#include "ana-packet-tag.h"

namespace ns3 {

namespace {
static const uint64_t SEND_INTERVAL = 20;
static const uint64_t FS_MULT = 48;
}

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
AnaSender::Setup (Ptr<UdpSocket> socket, Address address, uint32_t packetSize, uint32_t nPackets,
                  DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;

  m_socket->SetRecvCallback (MakeCallback (&AnaSender::HandleRead, this));
}

void
AnaSender::StartApplication ()
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
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

  if (m_socket)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
}
void
AnaSender::SendPacket ()
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  AnaRtpTag tag;
  tag.m_seq = m_nextSeq;
  tag.m_timestamp = (uint32_t) m_nextTimestamp;
  packet->AddPacketTag (tag);
  m_socket->Send (packet);

  ++m_nextSeq;
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
      Time tNext (MilliSeconds (SEND_INTERVAL));
      m_sendEvent = Simulator::Schedule (tNext, &AnaSender::SendPacket, this);
    }
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
          auto maxSeq = tag.GetMaxSeq ();
          auto inflight = m_nextSeq - 1 - maxSeq;
          NS_LOG_UNCOND ("Inflight " << inflight << " packets.");
        }
    }
}
/* -------------------------------------------------------------------------- */
SendPacketHistory::SendPacketHistory (Time timeThrehold, uint16_t seqThrehold)
    : m_timeThrehold (timeThrehold), m_seqThrehold (seqThrehold)
{
}
void
SendPacketHistory::Prepare (Time time, const AnaRtpTag &tag)
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
  m_queue.push_back ({time, Time (), tag});
  ++m_packetCount;
  
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
SendPacketHistory::Update (Time time, const AnaFeedbackTag &tag)
{
  for (auto seq : tag.m_ackSeq) {
    auto p = FindPacket (seq);
    if (p && p->ackTime.IsZero ())
      {
        p->ackTime = time;
      }
  }
}
SendPacketInfo *
SendPacketHistory::FindPacket(uint16_t seq)
{
  for (auto &p : m_queue) {
    if (p.tag.m_seq == seq) {
      return &p;
    }
  }
  return nullptr;
}
} // namespace ns3
