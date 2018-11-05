#include "fakevideogenerator.h"
#include "rtc_base/timeutils.h"
#include <string>
#include "ns3/log.h"
#include "ns3/simulator.h"
namespace ns3{
NS_LOG_COMPONENT_DEFINE ("FakeVideoGenerator");
FakeVideoGenerator::FakeVideoGenerator(uint32_t minbitrate,uint32_t fs)
:min_bitrate_(minbitrate)
,fs_(fs)
,rate_(min_bitrate_)
,first_ts_(0)
,last_send_ts_(0)
,delta_(1)
,frame_id_(0)
,running_(false)
,sender_(NULL){
	delta_=1000/fs_;
	ratecontrol_.RegisterRateChangeCallback(this);
}
void FakeVideoGenerator::ChangeRate(uint32_t bitrate){
	if(bitrate<min_bitrate_){
		rate_=min_bitrate_;
	}
	else{
		rate_=bitrate;
	}
	uint32_t now=Simulator::Now().GetMilliSeconds();
	if(first_ts_==0){
		first_ts_=now;
	}
	uint32_t elapse=now-first_ts_;
	NS_LOG_INFO("t "<<elapse<<" r "<<rate_);
}
void FakeVideoGenerator::RegisterSender(SenderInterface *s){
	sender_=s;
	sender_->RegisterPacketSource(this);
	sender_->SetRateControl(&ratecontrol_);
	sender_->SetSchedule(&schedule_);
}
void FakeVideoGenerator::SendFrame(){
	if(!running_){
		return;
	}
	uint32_t now=Simulator::Now().GetMilliSeconds();
	last_send_ts_=now;
	uint32_t f_type=0;
	uint32_t len=0;
	if(frame_id_==0){
		f_type=1;
	}
	frame_id_++;
	len=delta_*rate_/(1000*8);
    uint32_t packet_size=len/1000;
    //printf("packet_size %d\t%d\n",frame_id_-1,packet_size);
	if(sender_){
		uint8_t *buf=new uint8_t[len];
		sender_->SendVideo(0,f_type,buf,len);
		delete [] buf;
	}
}
void FakeVideoGenerator::Generate()
{
	if(m_timer.IsExpired())
	{
		SendFrame();
		Time next=MilliSeconds(delta_);
		m_timer=Simulator::Schedule(next,&FakeVideoGenerator::Generate,this);
	}
}
void FakeVideoGenerator::Start(){
	if(!running_){
		m_timer=Simulator::ScheduleNow(&FakeVideoGenerator::Generate,this);
	}
	running_=true;
}
void FakeVideoGenerator::Stop(){
	running_=false;
	m_timer.Cancel();
}
}




