#include "ana-packet-tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

TypeId
AnaRtpTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AnaRtpTag")
                          .SetParent<Tag> ()
                          .AddConstructor<AnaRtpTag> ()
                          .AddAttribute ("Timestamp", "Rtp Timestamp", EmptyAttributeValue (),
                                         MakeUintegerAccessor (&AnaRtpTag::m_timestamp),
                                         MakeUintegerChecker<uint32_t> ())
                          .AddAttribute ("Seq", "Rtp Sequence Number", EmptyAttributeValue (),
                                         MakeUintegerAccessor (&AnaRtpTag::m_seq),
                                         MakeUintegerChecker<uint16_t> ());
  return tid;
}
TypeId
AnaRtpTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
AnaRtpTag::GetSerializedSize (void) const
{
  return 600;
}
void
AnaRtpTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_timestamp);
  i.WriteU16 (m_seq);
}
void
AnaRtpTag::Deserialize (TagBuffer i)
{
  m_timestamp = i.ReadU32 ();
  m_seq = i.ReadU16 ();
}
void
AnaRtpTag::Print (std::ostream &os) const
{
  os << "Rtp [Timestamp:" << m_timestamp;
  os << ", Seq:" << m_seq;
  os << "] ";
}

/* -------------------------------------------------------------------------- */
TypeId
AnaFeedbackTag::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::AnaFeedbackTag").SetParent<Tag> ().AddConstructor<AnaFeedbackTag> ();
  return tid;
}
TypeId
AnaFeedbackTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
AnaFeedbackTag::GetSerializedSize (void) const
{
  return 2 + m_ackSeq.size () * 2;
}
void
AnaFeedbackTag::Serialize (TagBuffer i) const
{
  i.WriteU16 ((uint16_t) m_ackSeq.size ());
  for (auto seq : m_ackSeq)
    {
      i.WriteU16 (seq);
    }
}
void
AnaFeedbackTag::Deserialize (TagBuffer i)
{
  uint16_t len = i.ReadU16 ();
  m_ackSeq.clear ();
  for (uint16_t j = 0; j < len; ++j)
    {
      uint16_t seq = i.ReadU16 ();
      m_ackSeq.insert (seq);
    }
}
void
AnaFeedbackTag::Print (std::ostream &os) const
{
  os << "Feedback [";
  for (auto it = m_ackSeq.begin (); it != m_ackSeq.end (); ++it)
    {
      if (it != m_ackSeq.begin ())
        {
          os << ", ";
        }
      os << *it;
    }
  os << "] ";
}
bool
AnaFeedbackTag::IsEmpty () const
{
  return m_ackSeq.empty ();
}
uint16_t
AnaFeedbackTag::GetMaxSeq () const
{
  NS_ASSERT (!m_ackSeq.empty ());
  return *m_ackSeq.rbegin ();
}

} // namespace ns3
