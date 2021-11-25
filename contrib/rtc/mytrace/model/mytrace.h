#ifndef MY_TRACE_H
#define MY_TRACE_H
#include <iostream>
#include <fstream>
#include <string>
namespace ns3
{
class MyTracer
{
public:
	MyTracer();
	~MyTracer();
	void OpenTraceFile(std::string filename);
	void CloseTraceFile();
	void RateTrace(int64_t rate);
	void DelayTrace(int64_t delay);
private:
	std::fstream m_traceRateFile;//
	std::fstream m_traceDelayFile;
};
}
#endif
