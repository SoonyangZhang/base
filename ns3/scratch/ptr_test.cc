#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include <iostream>
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "rtc_base/timeutils.h"
#include <unistd.h>
#include "ns3/mp-video-module.h"
#include "ns3/webrtc-ns3-module.h"

using namespace ns3;
using namespace std;
NS_LOG_COMPONENT_DEFINE ("ptr-test");
const uint32_t DEFAULT_PACKET_SIZE = 1000;
static int ip=1;
static NodeContainer BuildExampleTopo (uint64_t bps,
                                       uint32_t msDelay,
                                       uint32_t msQdelay)
{
    NodeContainer nodes;
    nodes.Create (2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", DataRateValue  (DataRate (bps)));
    pointToPoint.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (msDelay)));
    auto bufSize = std::max<uint32_t> (DEFAULT_PACKET_SIZE, bps * msQdelay / 8000);
    pointToPoint.SetQueue ("ns3::DropTailQueue",
                           "Mode", StringValue ("QUEUE_MODE_BYTES"),
                           "MaxBytes", UintegerValue (bufSize));
    NetDeviceContainer devices = pointToPoint.Install (nodes);

    InternetStackHelper stack;
    stack.Install (nodes);
    Ipv4AddressHelper address;
    std::string nodeip="10.1."+std::to_string(ip)+".0";
    ip++;
    address.SetBase (nodeip.c_str(), "255.255.255.0");
    address.Assign (devices);

    // Uncomment to capture simulated traffic
    // pointToPoint.EnablePcapAll ("rmcat-example");

    // disable tc for now, some bug in ns3 causes extra delay
    TrafficControlHelper tch;
    tch.Uninstall (devices);

    return nodes;
}
uint64_t startTime=0;
uint64_t stopTime=200;
#define MIN_SEND_BITRATE (20 * 8 * 1000)
#include "rtc_base/logging.h"

int main(){
    rtc::LogMessage::SetLogToStderr(true);
    LogComponentEnable("ptr-test",LOG_LEVEL_ALL);
    //LogComponentEnable("MultipathSender",LOG_LEVEL_ALL);
    //LogComponentEnable("PathSender",LOG_LEVEL_ALL);
    //LogComponentEnable("PathReceiver",LOG_LEVEL_ALL);
    //LogComponentEnable("FakeVideoGenerator",LOG_LEVEL_ALL);
    LogComponentEnable("ScaleSchedule",LOG_LEVEL_ALL);
    LogComponentEnable("AggregateRate",LOG_LEVEL_ALL);
    SetClockForWebrtc();//that's a must
	Ptr<PathSender> spath1=CreateObject<PathSender>();
	Ptr<PathReceiver> rpath1=CreateObject<PathReceiver>();
	MultipathSender sender(11);
    MultipathReceiver receiver(12);
    NodeContainer nodes=BuildExampleTopo(2000000,100,200);
    nodes.Get(0)->AddApplication (spath1);
    nodes.Get(1)->AddApplication (rpath1);
    spath1->Bind(1234);
    rpath1->Bind(4321);

	Ptr<PathSender> spath2=CreateObject<PathSender>();
	Ptr<PathReceiver> rpath2=CreateObject<PathReceiver>();
    NodeContainer nodes2=BuildExampleTopo(1000000,150,200);
    nodes2.Get(0)->AddApplication (spath2);
    nodes2.Get(1)->AddApplication (rpath2);
    spath2->Bind(1234);
    rpath2->Bind(4321);


    sender.RegisterPath(spath1);
    receiver.RegisterPath(rpath1);

    sender.RegisterPath(spath2);
    receiver.RegisterPath(rpath2);

    FakeVideoGenerator source(MIN_SEND_BITRATE,30);
    source.RegisterSender(&sender);
    
    FakeVideoConsumer sink;
    receiver.RegisterDataSink(&sink);
    std::vector<InetSocketAddress> addr;
    addr.push_back(spath1->GetLocalAddress());
    addr.push_back(rpath1->GetLocalAddress());

    addr.push_back(spath2->GetLocalAddress());
    addr.push_back(rpath2->GetLocalAddress());
    spath1->SetStartTime (Seconds (startTime));
    spath1->SetStopTime (Seconds (stopTime));
    rpath1->SetStartTime (Seconds (startTime));
    rpath1->SetStopTime (Seconds (stopTime));

    spath2->SetStartTime (Seconds (startTime));
    spath2->SetStopTime (Seconds (stopTime));
    rpath2->SetStartTime (Seconds (startTime));
    rpath2->SetStopTime (Seconds (stopTime));
    sender.Connect(addr);
    Simulator::Stop (Seconds(stopTime + 10.0));
    Simulator::Run ();
    Simulator::Destroy();
    
    
}
