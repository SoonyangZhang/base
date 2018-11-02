#include "mpsender.h"
#include "log.h"
#include "rtc_base/timeutils.h"
#include <stdio.h>
#include <string>
#include <list>
#include <assert.h>
namespace zsy{
NS_LOG_COMPONENT_DEFINE ("MultipathSender");
static uint32_t CONN_TIMEOUT=200000;//200ms
static uint32_t CONN_RETRY=3;
MultipathSender::MultipathSender(SessionInterface *session,uint32_t uid)
:uid_(uid),pid_(1)
,session_(session)
,frame_seed_(1)
,schedule_seed_(1)
,seg_c_(0),first_ts_(-1)
,cc_type_(gcc_transport)
,min_bitrate_(MIN_BITRATE)
,start_bitrate_(START_BITRATE)
,max_bitrate_(MAX_BITRATE)
,scheduler_(NULL)
,rate_control_(NULL)
,stop_(false)
,max_free_buf_th_(50){
	bin_stream_init(&strm_);
	pm_=new webrtc::ProcessThreadImpl("mpsender");
	pm_->Start();
}
MultipathSender::~MultipathSender(){
	bin_stream_destroy(&strm_);
    //NS_LOG_INFO("seg"<<std::to_string(seg_c_));
	printf("dtor seg %d\n",seg_c_);
	sim_segment_t *seg=NULL;
	while(!pending_buf_.empty()){
		auto it=pending_buf_.begin();
		seg=it->second;
		pending_buf_.erase(it);
		delete seg;
	}
	pm_->Stop();
	delete pm_;
}
void MultipathSender::Process(){
    if(stop_){return ;}
	if(!syn_path_.empty()){
		uint32_t now=rtc::TimeMillis();
		for(auto it=syn_path_.begin();it!=syn_path_.end();){
            PathSender *path=*it;
			if(path->con_c>CONN_RETRY){
				it=syn_path_.erase(it);
				uint8_t pid=path->pid;
				delete path;
				NS_LOG_INFO("path"<<pid<<"is unreachable");
			}else{
				uint32_t delta=now-path->con_ts;
				uint32_t backoff=path->con_c*CONN_TIMEOUT;
				if(delta>backoff){
					path->con_c++;
					path->con_ts=now;
					path->SendConnect(cc_type_,now);
				}
				it++;
			}
		}
	}
	uint32_t now=rtc::TimeMillis();
	PathSender *path=NULL;
	for(auto it=usable_path_.begin();it!=usable_path_.end();it++){
		path=it->second;
		path->HeartBeat(now);
	}
	SchedulePendingBuf();
}
void MultipathSender::Stop(){
	PathSender *path=NULL;
	stop_=true;
	for(auto it=usable_path_.begin();it!=usable_path_.end();it++){
		path=it->second;
		path->Stop();
	}
}
void MultipathSender::Connect(su_socket *fd,su_addr *src,su_addr *dst){
	PathSender *path=new PathSender(pm_,&clock_);
    path->RegisterSenderInterface(this);
	path->fd=*fd;
	path->src=*src;
    path->dst=*dst;
	uint32_t now=rtc::TimeMillis();
	path->state=path_conning;
	path->pid=pid_;
	pid_++;
	path->con_ts=now;
	path->con_c=1;
    path->SendConnect(cc_type_,now);
	syn_path_.push_back(path);
}
PathSender* MultipathSender::GetMinRttPath(){
	PathSender *path=NULL;
	uint32_t rtt=0;
	if(usable_path_.size()==1){
		auto it=usable_path_.begin();
		path=it->second;
	}else if(usable_path_.size()>1){
		auto it=usable_path_.begin();
		path=it->second;
		rtt=path->rtt_;
		for(;it!=usable_path_.end();it++){
			PathSender *temp=it->second;
			if(temp->rtt_<rtt){
				path=temp;
				rtt=temp->rtt_;
			}
		}
	}
	return path;
}
void MultipathSender::ProcessingMsg(su_socket *fd,su_addr *remote,sim_header_t*header,bin_stream_t *stream){
    if(stop_){return ;}
	uint8_t pid=header->ver;
	switch(header->mid){
	case SIM_CONNECT_ACK:{
		sim_connect_ack_t ack;
		if (sim_decode_msg(stream, header, &ack) != 0)
			return;
        printf("pid %d\n",pid);
        uint32_t now=rtc::TimeMillis();
        uint32_t rtt=now-ack.cid;
		//TODO cid can be use to updata rtt
		for(auto it=syn_path_.begin();it!=syn_path_.end();){
			PathSender *path=*it;
			if(path->pid==pid){
				path->state=path_conned;
				path->rtt_=rtt;
				path->min_rtt_=rtt;
				path->rtt_update_ts_=now;
				syn_path_.erase(it++);
				AddAvailablePath(path);
				session_->PathStateForward(NOTIFYMESSAGE::notify_con_ack,pid);
				NS_LOG_INFO("path "<<std::to_string(pid)<< "is usable rtt "<<std::to_string(rtt));
				break;
			}
			else{
				it++;
			}
		}
	}
	case SIM_SEG_ACK:{
		sim_segment_ack_t ack;
		if (sim_decode_msg(stream, header, &ack) != 0)
			return;
		InnerProcessSegAck(pid,&ack);
		break;
	}
	case SIM_FEEDBACK:{
		sim_feedback_t feedback;
		if (sim_decode_msg(stream, header, &feedback) != 0)
			return;
		InnerProcessFeedback(pid,&feedback);
		break;
	}
	}
}
void MultipathSender::AddAvailablePath(PathSender *path){
	path->ConfigureCongestion();
    uint8_t pid=path->pid;
	usable_path_.insert(std::make_pair(pid,path));
	if(scheduler_){
		scheduler_->RegisterPath(pid);
	}
}
void MultipathSender::ProcessPongMsg(uint8_t pid,uint32_t rtt,uint32_t now){
    if(stop_){return;}
	PathSender *path=GetPathInfo(pid);
	if(path){
		path->UpdateRtt(rtt,now);
	}
}
void MultipathSender::SchedulePendingBuf(){
	std::map<uint32_t,uint32_t> pending_info;
	{
		rtc::CritScope cs(&buf_mutex_);
		//packetid ->len
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
void MultipathSender::InnerProcessFeedback(uint8_t pid,sim_feedback_t* feedback){
	PathSender *path=GetPathInfo(pid);
	path->IncomingFeedBack(feedback);
}
void MultipathSender::InnerProcessSegAck(uint8_t pid,sim_segment_ack_t* ack){
	PathSender *path=NULL;
	path=GetPathInfo(pid);
	if(path){
		path->RecvSegAck(ack);
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
	int64_t now=rtc::TimeMillis();
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
		rtc::CritScope cs(&buf_mutex_);
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
}
void MultipathSender::OnNetworkChanged(uint8_t pid,
					  uint32_t bitrate_bps,
					  uint8_t fraction_loss,
					  int64_t rtt_ms){
	if(rate_control_){
		rate_control_->ChangeRate(pid,bitrate_bps,fraction_loss,rtt_ms);
	}
}
PathSender* MultipathSender::GetPathInfo(uint8_t pid){
	PathSender *info=NULL;
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
		rtc::CritScope cs(&buf_mutex_);
		auto it=pending_buf_.find(schedule_id);
		if(it!=pending_buf_.end()){
			seg=it->second;
			pending_buf_.erase(it);
		}
	}
	if(seg){
		PathSender *path=NULL;
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
}