/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/ana-helper.h"
#include "ns3/mytrace.h"
#include "ns3/bwtrace.h"
#include "ns3/losstrace.h"
#include "ns3/traffic-control-module.h"
#include <vector>

using namespace ns3;

const uint32_t IPV4_TCP_PACKET_OVERHEAD = 2 + 20 + 32; // P2P + IP + TCP header
const uint32_t DEFAULT_PACKET_SIZE = 1000;

static void InstallTcp (Ptr<Node> sender, Ptr<Node> receiver, uint16_t port, float startTime,
                        float stopTime, BwTracer *bwtracer);
static void InstallAna (Ptr<Node> sender, Ptr<Node> receiver, uint16_t port, float startTime,
                        float stopTime, MyTracer *mytracer, LossTracer *ltracer);

int
main (int argc, char *argv[])
{
  bool verbose = true;
  bool tcpSideFlow = false;
  bool anaSideFlow = false;
  uint64_t kbps = 120;
  uint32_t msDelay = 50;
  uint32_t msQdelay = 300;
  const double startTime = 0;
  const double stopTime = 60;

  // std::vector<Ptr<Object>> objectHolder;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);
  cmd.AddValue ("kbps", "Bottleneck bandwidth", kbps);
  cmd.AddValue ("msDelay", "One way delay (ms)", msDelay);
  cmd.AddValue ("msQdelay", "Queue delay (ms)", msQdelay);
  cmd.AddValue ("tcp", "Tcp side flow", tcpSideFlow);
  cmd.AddValue ("ana", "ANA side flow", anaSideFlow);

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (kbps * 1000)));
  pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
  auto qsize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, msQdelay * kbps / 8);
  pointToPoint.SetQueue ("ns3::DropTailQueue", "MaxSize",
                         QueueSizeValue (QueueSize (QueueSizeUnit::BYTES, qsize)));

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

  MyTracer tracer;
  tracer.OpenTraceFile ("ana");
  LossTracer lossTracer;
  lossTracer.OpenTraceFile ("ana");
  InstallAna (nodes.Get (0), nodes.Get (1), 10000, startTime, stopTime, &tracer, &lossTracer);

  BwTracer bwtracer;
  bwtracer.SetPacketOverhead (IPV4_TCP_PACKET_OVERHEAD);
  bwtracer.OpenTraceFile ("tcp1");
  if (tcpSideFlow)
    InstallTcp (nodes.Get (0), nodes.Get (1), 2345, 10, 40, &bwtracer);

  MyTracer tracer1;
  tracer1.OpenTraceFile ("ana1");
  LossTracer lossTracer1;
  lossTracer1.OpenTraceFile ("ana1");
  if (anaSideFlow)
    InstallAna (nodes.Get (0), nodes.Get (1), 10010, startTime + 10, stopTime, &tracer1, &lossTracer1);

  // InstallTcp (nodes.Get (0), nodes.Get (1), 234, 10, 50, "tcp2");

  // disable tc for now, some bug in ns3 causes extra delay
  TrafficControlHelper tch;
  tch.Uninstall (devices);

  // devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
  pointToPoint.EnablePcapAll ("ana", false);

  Simulator::Stop (Seconds (stopTime + 10));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

void
InstallTcp (Ptr<Node> sender, Ptr<Node> receiver, uint16_t port, float startTime, float stopTime,
            BwTracer *tracer)
{
  // configure TCP source/sender/client
  auto serverAddr = receiver->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
  BulkSendHelper source{"ns3::TcpSocketFactory", InetSocketAddress{serverAddr, port}};
  // Set the amount of data to send in bytes. Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (0));
  source.SetAttribute ("SendSize", UintegerValue (DEFAULT_PACKET_SIZE));

  auto clientApps = source.Install (sender);
  clientApps.Start (Seconds (startTime));
  clientApps.Stop (Seconds (stopTime));

  // configure TCP sink/receiver/server
  PacketSinkHelper sink{"ns3::TcpSocketFactory", InetSocketAddress{Ipv4Address::GetAny (), port}};
  auto serverApps = sink.Install (receiver);
  serverApps.Start (Seconds (startTime));
  serverApps.Stop (Seconds (stopTime));

  auto packetSink = DynamicCast<PacketSink> (serverApps.Get (0));
  tracer->SetPacketOverhead (IPV4_TCP_PACKET_OVERHEAD);
  tracer->Start (Seconds (startTime));
  tracer->Stop (Seconds (stopTime));
  packetSink->TraceConnectWithoutContext ("Rx", MakeCallback (&BwTracer::PacketTrace, tracer));
}
void
InstallAna (Ptr<Node> sender, Ptr<Node> receiver, uint16_t port, float startTime, float stopTime,
            MyTracer *mytracer, LossTracer *ltracer)
{
  Ptr<AnaReceiver> anaReceiver = CreateObject<AnaReceiver> ();
  anaReceiver->Setup (port);
  receiver->AddApplication (anaReceiver);
  anaReceiver->SetStartTime (Seconds (startTime));
  anaReceiver->SetStopTime (Seconds (stopTime));

  Ptr<UdpSocket> senderSocket =
      StaticCast<UdpSocket> (Socket::CreateSocket (sender, UdpSocketFactory::GetTypeId ()));
  // clientSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

  Ptr<AnaSender> anaSender = CreateObject<AnaSender> ();
  Ptr<Ipv4> ipv4 = receiver->GetObject<Ipv4> ();
  Ipv4Address peerIp = ipv4->GetAddress (1, 0).GetLocal ();
  Address peerAddress (InetSocketAddress (peerIp, port));
  anaSender->Setup (senderSocket, peerAddress, 130, uint32_t (50 * (stopTime - startTime)));
  sender->AddApplication (anaSender);
  anaSender->SetStartTime (Seconds (startTime));
  anaSender->SetStopTime (Seconds (stopTime));

  anaSender->SetRateCallback (MakeCallback (&MyTracer::RateTrace, mytracer));
  anaSender->SetRateAckCallback (MakeCallback (&MyTracer::RateAckTrace, mytracer));
  anaSender->SetRttCallback (MakeCallback (&MyTracer::RttTrace, mytracer));
  anaReceiver->SetDelayCallback (MakeCallback (&MyTracer::DelayTrace, mytracer));
  anaReceiver->SetRateCallback (MakeCallback (&MyTracer::RateRecvTrace, mytracer));
  anaSender->SetLossRateCallback (MakeCallback (&LossTracer::LossTrace, ltracer));
}
