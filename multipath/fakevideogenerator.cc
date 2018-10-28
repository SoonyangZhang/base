#include "fakevideogenerator.h"
#include "log.h"
#include "rtc_base/timeutils.h"
#include <string>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("FakeVideoGenerator");
FakeVideoGenerator::FakeVideoGenerator(uint32_t minbitrate,uint32_t fs)
:min_bitrate_(minbitrate)
,fs_(fs)
,rate_(min_bitrate_)
,first_ts_(0)
,last_send_ts_(0)
,delta_(1)
,frame_id_(0)
,stop_(false)
,session_(NULL){
	delta_=1000/fs_;
}
void FakeVideoGenerator::ChangeRate(uint32_t bitrate){
	if(bitrate<min_bitrate_){
		rate_=min_bitrate_;
	}
	else{
		rate_=bitrate;
	}
	uint32_t now=rtc::TimeMillis();
	if(first_ts_==0){
		first_ts_=now;
	}
	uint32_t abs_time=now-first_ts_;
	NS_LOG_INFO("time "<<std::to_string(abs_time)<<" rate "<<std::to_string(rate_));
}
void FakeVideoGenerator::RegisterSession(SessionInterface *s){
	session_=s;
}
void FakeVideoGenerator::SendFrame(uint32_t now){
	last_send_ts_=now;
	uint32_t f_type=0;
	uint32_t len=0;
	if(frame_id_%10==0){
		f_type=1;
	}
	frame_id_++;
	len=delta_*rate_/(1000*8);
	if(session_){
		uint8_t *buf=new uint8_t[len];
		session_->SendVideo(0,f_type,buf,len);
		delete [] buf;
	}
}
void FakeVideoGenerator::HeartBeat(uint32_t now){
	if(stop_){ return ;}
	if(last_send_ts_==0){
		SendFrame(now);
	}else{
		if((now-last_send_ts_)>delta_){
		SendFrame(now);
		}
	}
}
}




