#include <unistd.h>
#include <memory.h>
#include <string>
#include <stdio.h>
#include "ns3/bwtrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Bw-Trace");

static const uint32_t TICK_INTERVAL = 20;
static const uint32_t SAMPLE_INTERVAL = 1000;

BwTracer::BwTracer ()
{
}
BwTracer::~BwTracer ()
{
  if (m_tickEvent.IsRunning ())
    Simulator::Cancel (m_tickEvent);
  
  if (m_traceFile.is_open ())
    m_traceFile.close ();
}
void BwTracer::SetPacketOverhead (int32_t packetOverhead)
{
  m_packetOverhead = packetOverhead;
}
void
BwTracer::OpenTraceFile (std::string filename)
{
  char buf[FILENAME_MAX];
  memset (buf, 0, FILENAME_MAX);
  string path = string (getcwd (buf, FILENAME_MAX)) + "/traces/" + filename + "_bw.txt";

  m_traceFile.open (path.c_str (), fstream::out);
}
void
BwTracer::CloseTraceFile ()
{
  if (m_traceFile.is_open ())
    m_traceFile.close ();
}
void
BwTracer::PacketTrace (Ptr<const Packet> packet, const Address &addr)
{
  int32_t dataSize = packet->GetSize () + m_packetOverhead;
  m_queue.push_back({Simulator::Now (), dataSize});
}
void
BwTracer::Start (Time time)
{
  m_startTime = time;
  m_tickEvent = Simulator::Schedule (time, &BwTracer::Tick, this);
}
void
BwTracer::Stop (Time time)
{
  m_stopTime = time;
}
void
BwTracer::Tick ()
{
  char line[255];
  memset (line, 0, 255);
  Time now = Simulator::Now ();
  int64_t rate = ComputeRate (now);
  sprintf (line, "%16f %16li", now.GetSeconds (), rate);
  m_traceFile << line << std::endl;

  m_dataSize = 0;
  m_tickEvent = Simulator::Schedule (MilliSeconds (TICK_INTERVAL), &BwTracer::Tick, this);
}
int64_t
BwTracer::ComputeRate (Time time)
{
  while (!m_queue.empty() && m_queue.front().time + MilliSeconds (SAMPLE_INTERVAL) < time)
    m_queue.pop_front();

  int64_t dataSize = 0;
  for (auto &p : m_queue)
    {
      if (p.time >= time)
        break;
      dataSize += p.dataSize;
    }
  
  int64_t sample_interval = SAMPLE_INTERVAL;
  if (time - m_startTime < MilliSeconds (SAMPLE_INTERVAL))
    sample_interval = (time - m_startTime).GetMilliSeconds ();
  if (sample_interval <= 0)
    return 0;
  
  return dataSize * 8000 / sample_interval;
}

} // namespace ns3
