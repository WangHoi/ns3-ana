/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/ana-helper.h"
#include "ns3/mytrace.h"
#include "ns3/bwtrace.h"
#include "ns3/traffic-control-module.h"
#include <vector>

using namespace ns3;

const uint32_t IPV4_TCP_PACKET_OVERHEAD = 2 + 20 + 32; // P2P + IP + TCP header
const uint32_t DEFAULT_PACKET_SIZE = 1000;

std::vector<Ptr<Object> > objectHolder;

static void InstallTcp (Ptr<Node> sender, Ptr<Node> receiver, uint16_t port, float startTime,
                        float stopTime, std::string flowName);

int
main (int argc, char *argv[])
{
  bool verbose = true;
  uint64_t kbps = 120;
  uint32_t msDelay = 50;
  uint32_t msQdelay = 300;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("kbps", "Bottleneck bandwidth", kbps);
  cmd.AddValue ("msDelay", "One way delay (ms)", msDelay);
  cmd.AddValue ("msQdelay", "Queue delay (ms)", msQdelay);

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  const double endTime = 60;

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (kbps * 1000)));
  pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
  auto qsize = std::max<uint32_t>(DEFAULT_PACKET_SIZE, msQdelay * kbps / 8);
  pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize", QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, qsize)));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  // Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  // em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  // devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  uint16_t serverPort = 10000;
  Address serverAddress (InetSocketAddress (interfaces.GetAddress (1), serverPort));
  Ptr<AnaReceiver> anaReceiver = CreateObject<AnaReceiver> ();
  anaReceiver->Setup (serverPort);
  nodes.Get (1)->AddApplication (anaReceiver);
  anaReceiver->SetStartTime (Seconds (0.));
  anaReceiver->SetStopTime (Seconds (endTime));

  Ptr<UdpSocket> clientSocket =
      StaticCast<UdpSocket> (Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ()));
  // clientSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

  Ptr<AnaSender> anaSender = CreateObject<AnaSender> ();
  anaSender->Setup (clientSocket, serverAddress, 130, 50 * ((uint32_t)endTime - 2), DataRate ("1Mbps"));
  nodes.Get (0)->AddApplication (anaSender);
  anaSender->SetStartTime (Seconds (1.));
  anaSender->SetStopTime (Seconds (endTime));

  InstallTcp (nodes.Get (0), nodes.Get (1), 2345, 10, 50, "tcp1");
  // InstallTcp (nodes.Get (0), nodes.Get (1), 234, 10, 50, "tcp2");

  // disable tc for now, some bug in ns3 causes extra delay
  TrafficControlHelper tch;
  tch.Uninstall (devices);

  // devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
  pointToPoint.EnablePcapAll ("ana", false);

  Simulator::Stop (Seconds (endTime));

  MyTracer tracer;
  tracer.OpenTraceFile ("ana");
  anaSender->SetRateCallback (MakeCallback (&MyTracer::RateTrace, &tracer));
  anaSender->SetRateAckCallback (MakeCallback (&MyTracer::RateAckTrace, &tracer));
  anaSender->SetRttCallback (MakeCallback (&MyTracer::RttTrace, &tracer));
  anaReceiver->SetDelayCallback (MakeCallback (&MyTracer::DelayTrace, &tracer));
  anaReceiver->SetRateCallback (MakeCallback (&MyTracer::RateRecvTrace, &tracer));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

void
InstallTcp (Ptr<Node> sender, Ptr<Node> receiver, uint16_t port, float startTime, float stopTime, std::string flowName)
{
  // configure TCP source/sender/client
  auto serverAddr = receiver->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
  BulkSendHelper source{"ns3::TcpSocketFactory", InetSocketAddress{serverAddr, port}};
  // Set the amount of data to send in bytes. Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  source.SetAttribute ("SendSize", UintegerValue (DEFAULT_PACKET_SIZE));

#if 1
  auto clientApps = source.Install (sender);
  clientApps.Start (Seconds (startTime));
  clientApps.Stop (Seconds (stopTime));

  // configure TCP sink/receiver/server
  PacketSinkHelper sink{"ns3::TcpSocketFactory", InetSocketAddress{Ipv4Address::GetAny (), port}};
  auto serverApps = sink.Install (receiver);
  serverApps.Start (Seconds (startTime));
  serverApps.Stop (Seconds (stopTime));

  auto packetSink = DynamicCast<PacketSink> (serverApps.Get (0));
  auto trace = CreateObject<BwTracer> ();
  trace->SetPacketOverhead (IPV4_TCP_PACKET_OVERHEAD);
  trace->OpenTraceFile (flowName);
  trace->Start (Seconds (startTime));
  trace->Stop (Seconds (stopTime));
  packetSink->TraceConnectWithoutContext ("Rx", MakeCallback (&BwTracer::PacketTrace, trace));
  objectHolder.push_back(trace);
#endif
}
