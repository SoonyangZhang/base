#include "rate_record.h"
#include "rtc_base/timeutils.h"
namespace zsy{
RateRecord::RateRecord(rtc::Ns3TaskQueue *queue,std::string f)
:q_(queue),fp_(NULL),first_flag_(true),running_(true){
	fp_=fopen(f.c_str(),"w+");
}
RateRecord::~RateRecord(){
	if(fp_){
		fclose(fp_);
	}
}
void RateRecord::Stop(){
	running_=false;
}
void RateRecord::ChangeBitrate(uint32_t rate){
	if(!running_){return;}
	uint32_t now=rtc::TimeMillis();
	if(first_flag_){
		first_flag_=false;
		last_=now;
	}
	uint32_t delta=now-last_;
	q_->PostTask([this,rate,delta]{
		if(fp_){
			fprintf(fp_,"%d\t%d\n",delta,rate);
		}
	});
}


}

