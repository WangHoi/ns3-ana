/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ana-helper.h"

namespace ns3 {

AnaSenderHelper::AnaSenderHelper (uint16_t port)
{
  // m_factory.SetTypeId (AnaSender::GetTypeId ());
  // SetAttribute ("Port", UintegerValue (port));
}

void 
AnaSenderHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
AnaSenderHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
AnaSenderHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
AnaSenderHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
AnaSenderHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<AnaSender> ();
  node->AddApplication (app);

  return app;
}

}

