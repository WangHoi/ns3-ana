/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/ana-helper.h"

using namespace ns3;

int
main (int argc, char *argv[])
{
  bool verbose = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "Tell application to log if true", verbose);

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Kbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("50ms"));

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
  nodes.Get (1)->AddApplication(anaReceiver);
  anaReceiver->SetStartTime (Seconds (0.));
  anaReceiver->SetStopTime (Seconds (100.));

  Ptr<UdpSocket> clientSocket = StaticCast<UdpSocket> (Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ()));
  // clientSocket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&CwndChange));

  Ptr<AnaSender> anaSender = CreateObject<AnaSender> ();
  anaSender->Setup (clientSocket, serverAddress, 130, 1000, DataRate ("1Mbps"));
  nodes.Get (0)->AddApplication (anaSender);
  anaSender->SetStartTime (Seconds (1.));
  anaSender->SetStopTime (Seconds (100.));

  // devices.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
  pointToPoint.EnablePcapAll ("ana", false);

  Simulator::Stop (Seconds (100));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
