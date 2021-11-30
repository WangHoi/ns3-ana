#ifndef LOSS_TRACE_H
#define LOSS_TRACE_H
#include "ns3/core-module.h"
#include "ns3/packet.h"
#include <iostream>
#include <fstream>
#include <string>
#include <deque>
namespace ns3
{
class LossTracer : public Object
{
public:
	LossTracer();
	~LossTracer();
	void OpenTraceFile(std::string filename);
	void CloseTraceFile();
	void LossTrace(double rate);

private:
	std::fstream m_traceFile;
};
}
#endif
