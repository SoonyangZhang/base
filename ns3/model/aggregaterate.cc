#include "aggregaterate.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include <unistd.h>
using namespace std;
namespace ns3{
NS_LOG_COMPONENT_DEFINE ("AggregateRate");
AggregateRate::AggregateRate()
:cb_(NULL)
,first_ts_(0){
    first_=true;
	char buf[FILENAME_MAX];
	memset(buf,0,FILENAME_MAX);
    string ratepath = string (getcwd(buf, FILENAME_MAX)) + "/traces/" + "mpvideo"+"_rate.txt";
	m_traceRateFile.open(ratepath.c_str(), std::fstream::out);
}
AggregateRate::~AggregateRate(){
	if(m_traceRateFile.is_open())
		m_traceRateFile.close();    
}
void AggregateRate::ChangeRate(uint8_t pid,uint32_t bitrate
		, uint8_t fraction_loss, uint32_t rtt){
	auto it=path_rate_table_.find(pid);
    double now=Simulator::Now().GetSeconds();
    if(first_){
        first_=false; 
        first_ts_=now;
    }
    double delta=now-first_ts_;
	if(it==path_rate_table_.end()){
		path_rate_table_.insert(std::make_pair(pid,bitrate));
	}else{
		it->second=bitrate;
	}
	uint32_t totalrate=0;
	for(auto it=path_rate_table_.begin();it!=path_rate_table_.end();it++){
		totalrate+=it->second;
	}
	if(cb_){
		cb_->ChangeRate(totalrate);
	}
    NS_LOG_INFO(std::to_string(pid)<<" "<<delta<<" "<<bitrate/1000);
	char line [255];
	memset(line,0,255);
	sprintf (line, "%16d %16f %16f",
			pid,delta,(double)bitrate/1000);
	m_traceRateFile<<line<<std::endl;
	memset(line,0,255);
	sprintf (line, "%16d %16f %16f",
			a_pid_,delta,(double)totalrate/1000);
	m_traceRateFile<<line<<std::endl;
    //NS_LOG_INFO(std::to_string(a_pid_)<<" "<<delta<<" "<<totalrate/1000);
   // printf("%d %d %d\n",pid,delta,bitrate/1000);
   // printf("%d %d %d\n",a_pid_,delta,totalrate/1000);
}
void AggregateRate::RegisterRateChangeCallback(RateChangeCallback*cb){
	cb_=cb;
}
}




