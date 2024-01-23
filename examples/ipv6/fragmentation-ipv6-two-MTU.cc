/*
 * Copyright (c) 2008-2009 Strasbourg University
 * Copyright (c) 2013 Universita' di Firenze
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 *         Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 * Modified by Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
// //
// //     Src     n0   r    n1    Dst
// //             |    _    |
// //     MTU     ====|_|====     MTU
// //     5000       router       1500
// //
// // - Tracing of queues and packet receptions to file "fragmentation-ipv6-two-mtu.tr"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/ipv6-static-routing-helper.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FragmentationIpv6TwoMtuExample");

int
main(int argc, char** argv)
{
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("Ipv6L3Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv6L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6Interface", LOG_LEVEL_ALL);
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_ALL);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_ALL);
    }
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);

    NS_LOG_INFO("Create nodes.");
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> r = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();

    NodeContainer net1(n0, r);
    NodeContainer net2(r, n1);
    NodeContainer all(n0, r, n1);

    NS_LOG_INFO("Create IPv6 Internet Stack");
    InternetStackHelper internetv6;
    internetv6.Install(all);

    NS_LOG_INFO("Create channels.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer d2 = csma.Install(net2);
    csma.SetDeviceAttribute("Mtu", UintegerValue(5000));
    NetDeviceContainer d1 = csma.Install(net1);

    NS_LOG_INFO("Create networks and assign IPv6 Addresses.");
    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i1 = ipv6.Assign(d1);
    i1.SetForwarding(1, true);
    i1.SetDefaultRouteInAllNodes(1);
    ipv6.SetBase(Ipv6Address("2001:2::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i2 = ipv6.Assign(d2);
    i2.SetForwarding(0, true);
    i2.SetDefaultRouteInAllNodes(0);

    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(&std::cout);
    Ipv6RoutingHelper::PrintRoutingTableAt(Seconds(0), n0, routingStream);

    /* Create a UdpEchoClient and UdpEchoServer application to send packets from n0 to n1 via r */
    uint32_t packetSize = 4096;
    uint32_t maxPacketCount = 5;

    UdpEchoClientHelper echoClient(i2.GetAddress(1, 1), 42);
    echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));
    echoClient.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    ApplicationContainer clientApps = echoClient.Install(net1.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(20.0));

    UdpEchoServerHelper echoServer(42);
    ApplicationContainer serverApps = echoServer.Install(net2.Get(1));
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(Seconds(30.0));

    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("fragmentation-ipv6-two-mtu.tr"));
    csma.EnablePcapAll(std::string("fragmentation-ipv6-two-mtu"), true);

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
