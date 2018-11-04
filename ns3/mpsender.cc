#include "mpsender.h"
#include "rtc_base/timeutils.h"
#include <stdio.h>
#include <string>
#include <list>
#include <assert.h>
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
namespace ns3{
NS_LOG_COMPONENT_DEFINE ("MultipathSender");
MultipathSender::MultipathSender(uint32_t uid)
:uid_(uid),pid_(1)
,frame_seed_(1)
,schedule_seed_(1)
,seg_c_(0),first_ts_(-1)
,min_bitrate_(MIN_BITRATE)
,start_bitrate_(START_BITRATE)
,max_bitrate_(MAX_BITRATE)
,scheduler_(NULL)
,rate_control_(NULL)
,stop_(false)
,source_(NULL)
{
}
MultipathSender::~MultipathSender(){
    //NS_LOG_INFO("seg"<<std::to_string(seg_c_));
	printf("dtor seg %d\n",seg_c_);
	sim_segment_t *seg=NULL;
	while(!pending_buf_.empty()){
		auto it=pending_buf_.begin();
		seg=it->second;
		pending_buf_.erase(it);
		delete seg;
	}
}
void MultipathSender::Process(){

}
void MultipathSender::StopCallback(){
    Stop();
}
void MultipathSender::Stop(){
	Ptr<PathSender> path=NULL;
	stop_=true;
	for(auto it=usable_path_.begin();it!=usable_path_.end();it++){
		path=it->second;
		path->Stop();
	}
    if(source_){
        source_->Stop();
    }
}
void MultipathSender::Connect(std::vector<InetSocketAddress> addr){
    int len=addr.size();
    int i=0;
	for(i=0;i<len;i+=2){
		for(auto it=syn_path_.begin();it!=syn_path_.end();it++){
            Ptr<PathSender> sender=it->second;
			InetSocketAddress senderaddr=
					sender->GetLocalAddress();
			if(senderaddr.GetIpv4()==addr[i].GetIpv4()){
                sender->peer_ip_=addr[i+1].GetIpv4();
                sender->peer_port_=addr[i+1].GetPort();
                sender->SendConnect();
				NS_LOG_INFO("match");
			}
		}
	}
}
void MultipathSender::AddAvailablePath(uint8_t pid){
	if(scheduler_){
		scheduler_->RegisterPath(pid);
	}
}
void MultipathSender::SchedulePendingBuf(){
	std::map<uint32_t,uint32_t> pending_info;
	{
		if(!pending_buf_.empty()){
			sim_segment_t *seg=NULL;
			uint32_t id=0;
			uint32_t len=0;
			for(auto it=pending_buf_.begin();it!=pending_buf_.end();it++){
				id=it->first;
				seg=it->second;
				len=seg->data_size+SIM_SEGMENT_HEADER_SIZE;
				pending_info.insert(std::make_pair(id,len));
			}
		}
	}
	if(!pending_info.empty()){
		if(scheduler_){
			scheduler_->IncomingPackets(pending_info);
		}
	}
}
#define MAX_SPLIT_NUMBER	1024
static uint16_t FrameSplit(uint16_t splits[], size_t size){
	uint16_t ret, i;
	uint16_t remain_size;

	if (size <= SIM_VIDEO_SIZE){
		ret = 1;
		splits[0] = size;
	}
	else{
		ret = size / SIM_VIDEO_SIZE;
		for (i = 0; i < ret; i++)
			splits[i] = SIM_VIDEO_SIZE;

		remain_size = size % SIM_VIDEO_SIZE;
		if (remain_size > 0){
			splits[ret] = remain_size;
			ret++;
		}
	}

	return ret;
}
//TODO
// the frame id should be set by up layer, to skip frame for rate control
//and for result comparison
void MultipathSender::SendVideo(uint8_t payload_type,int ftype,void *data,uint32_t size){
	if(stop_){return;}
	uint32_t timestamp=0;
	int64_t now=Simulator::Now().GetMilliSeconds();
	if(first_ts_==-1){
		first_ts_=now;
	}
	timestamp=now-first_ts_;
	uint8_t* pos;
	uint16_t splits[MAX_SPLIT_NUMBER], total=0;
	assert((size / SIM_VIDEO_SIZE) < MAX_SPLIT_NUMBER);
	total = FrameSplit(splits, size);
	pos = (uint8_t*)data;
	std::list<sim_segment_t*> buf;
	sim_segment_t *seg=NULL;
	uint32_t i=0;
	for(i=0;i<total;i++){
		seg=new sim_segment_t();
		buf.push_back(seg);
	}
	{
	    i=0;
		while(!buf.empty()){
				seg=buf.front();
				buf.pop_front();
				seg->packet_id=0;
				uint32_t schedule_id=schedule_seed_;
				schedule_seed_++;
				seg->fid = frame_seed_;
				seg->timestamp = timestamp;
				seg->ftype = ftype;
				seg->payload_type = payload_type;
				seg->index = i;
				seg->total = total;
				seg->remb = 1;
				seg->send_ts = 0;
				seg->transport_seq = 0;
				seg->data_size = splits[i];
				memcpy(seg->data, pos, seg->data_size);
				pos += splits[i];
				pending_buf_.insert(std::make_pair(schedule_id,seg));
				i++;
			}
	}
    frame_seed_++;
    SchedulePendingBuf();
}
void MultipathSender::OnNetworkChanged(uint8_t pid,
					  uint32_t bitrate_bps,
					  uint8_t fraction_loss,
					  int64_t rtt_ms){
	if(rate_control_){
		rate_control_->ChangeRate(pid,bitrate_bps,fraction_loss,rtt_ms);
	}
}
Ptr<PathSender> MultipathSender::GetPathInfo(uint8_t pid){
	Ptr<PathSender> info=NULL;
	auto it=usable_path_.find(pid);
	if(it!=usable_path_.end()){
		info=it->second;
	}
	return info;
}
//send packet id to path according to schedule;
void MultipathSender::PacketSchedule(uint32_t schedule_id,uint8_t pid){
	sim_segment_t *seg=NULL;
	{
		auto it=pending_buf_.find(schedule_id);
		if(it!=pending_buf_.end()){
			seg=it->second;
			pending_buf_.erase(it);
		}
	}
	if(seg){
		Ptr<PathSender> path=NULL;
		auto it=usable_path_.find(pid);
		if(it!=usable_path_.end()){
			path=it->second;
			path->put(seg);
		}else{
			NS_LOG_ERROR("fatal packet schedule error");
		}
	}
}
void  MultipathSender::SetSchedule(Schedule* s){
	scheduler_=s;
	scheduler_->SetSender(this);
}
void MultipathSender::SetRateControl(RateControl * c){
	rate_control_=c;
	rate_control_->SetSender(this);
}
void MultipathSender::RegisterPath(Ptr<PathSender> path){
	for(auto it=syn_path_.begin();it!=syn_path_.end();it++){
		Ptr<PathSender> p=it->second;
		if(p==path){
			return;
		}
	}
	path->pid=pid_;
	path->RegisterSenderInterface(this);
	path->SetClock(&clock_);
	syn_path_.insert(std::make_pair(path->pid,path));
	pid_++;
}
void MultipathSender::NotifyPathUsable(uint8_t pid){
	auto it=syn_path_.find(pid);
	Ptr<PathSender> path;
	if(it!=syn_path_.end()){
		path=it->second;
		syn_path_.erase(it);
		AddAvailablePath(pid);
		usable_path_.insert(std::make_pair(pid,path));
        if(source_){
            source_->Start();
        }
	}
}
}
