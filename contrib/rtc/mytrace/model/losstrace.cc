#include <unistd.h>
#include <memory.h>
#include <string>
#include <stdio.h>
#include "ns3/losstrace.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Loss-Trace");

LossTracer::LossTracer ()
{
}
LossTracer::~LossTracer ()
{
  if (m_traceFile.is_open ())
    m_traceFile.close ();
}
void
LossTracer::OpenTraceFile (std::string filename)
{
  char buf[FILENAME_MAX];
  memset (buf, 0, FILENAME_MAX);
  string path = string (getcwd (buf, FILENAME_MAX)) + "/traces/" + filename + "_loss.txt";

  m_traceFile.open (path.c_str (), fstream::out);
}
void
LossTracer::CloseTraceFile ()
{
  if (m_traceFile.is_open ())
    m_traceFile.close ();
}
void
LossTracer::LossTrace (double rate)
{
  char line[255];
  memset (line, 0, 255);
  Time now = Simulator::Now ();
  sprintf (line, "%16lf %16lf", now.GetSeconds (), rate);
  m_traceFile << line << std::endl;
}

} // namespace ns3
