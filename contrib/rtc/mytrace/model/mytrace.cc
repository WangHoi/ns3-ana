#include <unistd.h>
#include <memory.h>
#include <string>
#include <stdio.h>
#include "ns3/mytrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
using namespace std;
namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("My-Trace");
MyTracer::MyTracer ()
{
}
MyTracer::~MyTracer ()
{
  if (m_traceRateFile.is_open ())
    m_traceRateFile.close ();
  if (m_traceDelayFile.is_open ())
    m_traceDelayFile.close ();
  if (m_traceRateAckFile.is_open ())
    m_traceRateAckFile.close ();
  if (m_traceRateRecvFile.is_open ())
    m_traceRateRecvFile.close ();
  if (m_traceRttFile.is_open ())
    m_traceRttFile.close ();
}
void
MyTracer::OpenTraceFile (std::string filename)
{
  char buf[FILENAME_MAX];
  memset (buf, 0, FILENAME_MAX);
  string delaypath = string (getcwd (buf, FILENAME_MAX)) + "/traces/" + filename + "_delay.txt";

  memset (buf, 0, FILENAME_MAX);
  string ratepath = string (getcwd (buf, FILENAME_MAX)) + "/traces/" + filename + "_rate.txt";

  memset (buf, 0, FILENAME_MAX);
  string rateAckPath = string (getcwd (buf, FILENAME_MAX)) + "/traces/" + filename + "_rateAck.txt";

  memset (buf, 0, FILENAME_MAX);
  string rateRecvPath =
      string (getcwd (buf, FILENAME_MAX)) + "/traces/" + filename + "_rateRecv.txt";

  memset (buf, 0, FILENAME_MAX);
  string rttpath = string (getcwd (buf, FILENAME_MAX)) + "/traces/" + filename + "_rtt.txt";

  m_traceRateFile.open (ratepath.c_str (), fstream::out);
  m_traceRateAckFile.open (rateAckPath.c_str (), fstream::out);
  m_traceDelayFile.open (delaypath.c_str (), fstream::out);
  m_traceRateRecvFile.open (rateRecvPath.c_str (), fstream::out);
  m_traceRttFile.open (rttpath.c_str (), fstream::out);
}
void
MyTracer::CloseTraceFile ()
{
  if (m_traceRateFile.is_open ())
    m_traceRateFile.close ();
  if (m_traceDelayFile.is_open ())
    m_traceDelayFile.close ();
  if (m_traceRateAckFile.is_open ())
    m_traceRateAckFile.close ();
  if (m_traceRateRecvFile.is_open ())
    m_traceRateRecvFile.close ();
  if (m_traceRttFile.is_open ())
    m_traceRttFile.close ();
}
void
MyTracer::RateTrace (int64_t rate)
{
  char line[255];
  memset (line, 0, 255);
  double now = Simulator::Now ().GetSeconds ();
  sprintf (line, "%16f %16li", now, rate);
  m_traceRateFile << line << std::endl;
}
void
MyTracer::RateAckTrace (int64_t rate)
{
  char line[255];
  memset (line, 0, 255);
  double now = Simulator::Now ().GetSeconds ();
  sprintf (line, "%16f %16li", now, rate);
  m_traceRateAckFile << line << std::endl;
}
void
MyTracer::RateRecvTrace (int64_t rate)
{
  char line[255];
  memset (line, 0, 255);
  double now = Simulator::Now ().GetSeconds ();
  sprintf (line, "%16f %16li", now, rate);
  m_traceRateRecvFile << line << std::endl;
}
void
MyTracer::DelayTrace (int64_t delay)
{
  char line[255];
  memset (line, 0, 255);
  double now = Simulator::Now ().GetSeconds ();
  //Time temp=MilliSenconds(delay);
  sprintf (line, "%16f %16li", now, delay);
  m_traceDelayFile << line << std::endl;
}
void
MyTracer::RttTrace (int64_t rtt)
{
  char line[255];
  memset (line, 0, 255);
  double now = Simulator::Now ().GetSeconds ();
  //Time temp=MilliSenconds(delay);
  sprintf (line, "%16f %16li", now, rtt);
  m_traceRttFile << line << std::endl;
}
} // namespace ns3
