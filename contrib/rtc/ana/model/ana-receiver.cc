/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ana-receiver.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AnaReceiverApplication");

NS_OBJECT_ENSURE_REGISTERED (AnaReceiver);

TypeId
AnaReceiver::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::AnaReceiver")
                          .SetParent<Application> ()
                          .SetGroupName ("Applications")
                          .AddConstructor<AnaReceiver> ();
  return tid;
}

void
AnaReceiver::Setup (uint16_t port)
{
  m_port = port;
}
void
AnaReceiver::StartApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket == nullptr)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = StaticCast<UdpSocket> (Socket::CreateSocket (GetNode (), tid));
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&AnaReceiver::HandleRead, this));
  m_packetHistory = Create<RecvPacketHistory> (Seconds (15), 32);
}
void
AnaReceiver::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
    }
}
void
AnaReceiver::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      const Time now = Simulator::Now ();
      socket->GetSockName (localAddress);
      // m_rxTrace (packet);
      // m_rxTraceWithAddresses (packet, from, localAddress);
      NS_LOG_INFO ("At time " << now.As (Time::S) << " server received "
                              << packet->GetSize () << " bytes from "
                              << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port "
                              << InetSocketAddress::ConvertFrom (from).GetPort ());

      AnaRtpTag rtpTag;
      if (packet->PeekPacketTag (rtpTag))
        {
          Time delay = now - MicroSeconds (rtpTag.m_time);
          NS_LOG_UNCOND (rtpTag.m_timestamp / 48 << " Delay " << delay.As (Time::MS));
          m_packetHistory->Add (now, rtpTag);
        }

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      NS_LOG_LOGIC ("Echoing packet");
      packet = Create<Packet> (0);
      AnaFeedbackTag feedbackTag;
      if (m_packetHistory->GetFeedback (feedbackTag))
        {
          packet->AddPacketTag (feedbackTag);
        }
      socket->SendTo (packet, 0, from);

      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server sent "
                              << packet->GetSize () << " bytes to "
                              << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port "
                              << InetSocketAddress::ConvertFrom (from).GetPort ());
    }
}

/* -------------------------------------------------------------------------- */

RecvPacketHistory::RecvPacketHistory (Time timeThrehold, uint16_t seqThrehold)
    : m_timeThrehold (timeThrehold), m_seqThrehold (seqThrehold)
{
}
void
RecvPacketHistory::Add (Time time, const AnaRtpTag &tag)
{
  while (!m_queue.empty ())
    {
      auto &p = m_queue.front ();
      if (time - p.time > m_timeThrehold)
        {
          m_queue.pop_front ();
        }
      else
        {
          break;
        }
    }
  m_queue.push_back ({time, tag});
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
bool
RecvPacketHistory::GetFeedback (AnaFeedbackTag &tag)
{
  if (m_packetCount == 0)
    {
      return false;
    }
  tag.m_ackSeq.clear ();
  tag.m_ackSeq.insert (m_maxSeq);
  for (auto &p : m_queue)
    {
      WrapComp<uint16_t> comp;
      if (comp (p.tag.m_seq, m_maxSeq) <= m_seqThrehold)
        {
          tag.m_ackSeq.insert (p.tag.m_seq);
        }
    }
  return true;
}

} // namespace ns3
