#pragma once
#include "ns3/tag.h"
#include "ns3/packet.h"
#include <set>

namespace ns3 {

class AnaRtpTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  uint32_t m_timestamp; //!< 48khz
  uint16_t m_seq;
};

template <typename T>
class WrapComp
{
public:
  enum {
    THREHOLD = 1 << (sizeof (T) * 8 - 1),
  };
  bool
  operator() (const T &left, const T &right) const
  {
    if (left < right)
      {
        return (right - left) < THREHOLD;
      }
    else if (left == right)
      {
        return false;
      }
    else
      {
        return (left - right) >= THREHOLD;
      }
  }
};

class AnaFeedbackTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  uint16_t GetMaxSeq () const;
  bool IsEmpty () const;

  std::set<uint16_t> m_ackSeq;
};

} // namespace ns3
