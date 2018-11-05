#include "mpreceiver.h"
#include "ns3/log.h"
#include "rtc_base/timeutils.h"
#include <stdio.h>
#include<memory.h>
namespace ns3{
NS_LOG_COMPONENT_DEFINE ("MultipathReceiver");
MultipathReceiver::MultipathReceiver(uint32_t uid)
:uid_(uid)
,min_bitrate_(MIN_BITRATE)
,max_bitrate_(MAX_BITRATE)
,seg_c_(0)
,deliver_(NULL),
stop_(false)
,last_deliver_ts_{0}
,min_fid_(0){
	bin_stream_init(&strm_);
	for(int i=0;i<2;i++){
		video_packet_t *v_packet=new video_packet_t();
		free_segs_.push(v_packet);
		seg_c_++;
	}
	first_packet_=false;
}
MultipathReceiver::~MultipathReceiver(){
	bin_stream_destroy(&strm_);
	video_packet_t *v_packet=NULL;
	while(!free_segs_.empty()){
		v_packet=free_segs_.front();
		free_segs_.pop();
		seg_c_--;
		delete v_packet;
	}
}
static uint32_t HBTIME=100;
void MultipathReceiver::HeartBeat(){
	if(hbTimer_.IsExpired()){
		Process();
		Time next=MilliSeconds(HBTIME);//500ms
		hbTimer_=Simulator::Schedule(next,&MultipathReceiver::HeartBeat,this);
	}
}
bool MultipathReceiver::RegisterDataSink(NetworkDataConsumer* c){
    bool ret=false;
    if(!deliver_){
    deliver_=c;
    ret=true;
    }
    return ret;    
}
Ptr<PathReceiver> MultipathReceiver::GetPathInfo(uint8_t pid){
	Ptr<PathReceiver> path=NULL;
    for(auto it=paths_.begin();it!=paths_.end();it++)
    {
        if((*it)->pid==pid){
        path=(*it);
        }
    }

	return path;
}
void MultipathReceiver::StopCallback(){
        Stop();
    }
bool MultipathReceiver::DeliverFrame(video_frame_t *f){
	bool ret=false;
	uint32_t fid=f->fid;
	if(deliver_){
		uint32_t size=0;
		uint32_t buf_size=0;
		uint32_t i=0;
		video_packet_t *packet=NULL;
		for(i=0;i<f->total;i++){
		packet=f->packets[i];
			if(packet){
				buf_size+=packet->seg.data_size;
			}
		}
		uint8_t *buf=new uint8_t[buf_size];
		uint32_t len=0;
		uint8_t offset=0;
		uint8_t *data;
		for(i=0;i<f->total;i++){
			packet=f->packets[i];
			if(packet){
				len=packet->seg.data_size;
				data=(uint8_t*)(packet->seg.data);
				memcpy(buf+offset,data,len);
				offset+=len;
			}
		}
		deliver_->ForwardUp(fid,buf,offset,f->recv,f->total);
		delete [] buf;
		ret=true;
		if(min_fid_<fid){
			min_fid_=fid;
		}
	}
	return ret;
}
void MultipathReceiver::PacketConsumed(video_frame_t *f){
	video_packet_t *packet=NULL;
	Ptr<PathReceiver> path=NULL;
	uint8_t pid=0;
	uint32_t packet_id=0;
	uint32_t i=0;
	for(i=0;i<f->total;i++){
		packet=f->packets[i];
		if(packet){
			pid=packet->pid;
			packet_id=packet->seg.packet_id;
			path=GetPathInfo(pid);
			if(path){
				path->Consume(packet_id);
			}
		}
	}
}
void MultipathReceiver::BuffCollection(video_frame_t*f){
	uint32_t i=0;
	video_packet_t *packet=NULL;
	for(i=0;i<f->total;i++){
		packet=f->packets[i];
		if(packet){
			memset(packet,0,sizeof(video_packet_t));
			free_segs_.push(packet);
			f->packets[i]=NULL;
		}
	}
}
void MultipathReceiver::UpdateDeliverTime(uint32_t ts){
	last_deliver_ts_=ts;
}
static uint32_t max_deliver_wait=500;
static uint32_t mem_id=0;
void  MultipathReceiver::CheckFrameDeliverBlock(uint32_t now){
	if(last_deliver_ts_==0){

	}else{
		uint32_t delta=now-last_deliver_ts_;
		if(delta>max_deliver_wait){
			if(!frame_cache_.empty()){
				video_frame_t *frame=NULL;
				auto it=frame_cache_.begin();
				frame=it->second;
                uint32_t fid=frame->fid;
                if(mem_id!=fid){printf("block %d %d\n",fid,frame->frame_type);}
                mem_id=fid;
				
			}
		}
	}
}
static uint32_t max_frame_waitting=500;
void MultipathReceiver::CheckDeliverFrame(){
	video_frame_t *frame=NULL;
	video_frame_t *waitting_for_delete=NULL;
	uint32_t now=rtc::TimeMillis();
	for(auto it=frame_cache_.begin();it!=frame_cache_.end();){
		frame=it->second;
		waitting_for_delete=NULL;
		if(frame->recv==frame->total){
			DeliverFrame(frame);
			PacketConsumed(frame);
			UpdateDeliverTime(now);
			waitting_for_delete=frame;
		}else if(frame->recv<frame->total){
			if(frame->waitting_ts!=-1){
                uint32_t delta=now-frame->ts;
				if(now>frame->waitting_ts||delta>max_frame_waitting){
					DeliverFrame(frame);
					PacketConsumed(frame);
					UpdateDeliverTime(now);
					waitting_for_delete=frame;
				}
			}else{

			}
		}
		if(waitting_for_delete){
			frame_cache_.erase(it++);
			BuffCollection(waitting_for_delete);
			delete waitting_for_delete;
		}else{
			break;
		}
	}
}
//true means not to deliver frame
bool MultipathReceiver::CheckLateFrame(uint32_t fid){
	bool ret=false;
	if(fid<=min_fid_){
        printf("late %d %d\n",fid,min_fid_);
		ret=true;
	}
	return ret;
}
//waitting or not waitting, the retransmission packet ,
//that's a hard question.
//waitting_ts=ts+max(path_i_waitting)
//path_i_waitting=s_send_ts+rtt+rtt_var
// fid=1,waitting=-1,must to get the full frame
static uint32_t MAX_WAITTING_RETRANS_TIME=500;//500ms
uint32_t MultipathReceiver::GetWaittingDuration(){
	uint32_t path_waitting=0;
	uint32_t temp;
	Ptr<PathReceiver> path=NULL;
	for(auto it=paths_.begin();it!=paths_.end();it++){
		path=*it;
		temp=path->GetWaittingDuration();
		if(temp>path_waitting){
			path_waitting=temp;
		}
	}
	path_waitting=SU_MIN(path_waitting,MAX_WAITTING_RETRANS_TIME);
	return path_waitting;
}
#include<stdio.h>
// implement the most simple way retransmission waititng time;
void MultipathReceiver::DeliverToCache(uint8_t pid,sim_segment_t* d){
    if(stop_){
    return ;
    }
    if(!first_packet_){
    	HeartBeat();
    	first_packet_=true;
    }
	video_packet_t *packet=NULL;
	uint32_t fid=d->fid;
	uint8_t ftype=d->ftype;
	uint16_t total=d->total;
	uint16_t index=d->index;
	if(CheckLateFrame(fid)){
		return;
	}
	video_frame_t *frame=NULL;
	{
		auto it=frame_cache_.find(fid);
		if(it!=frame_cache_.end()){
			frame=it->second;
		}else{
			frame=new video_frame_t();
            frame->fid=fid;
			frame->packets=(video_packet_t**)calloc(total,sizeof(sim_segment_t*));
			frame->recv=0;
			frame->total=total;
			frame->waitting_ts=0;
			frame->frame_type=ftype;
			if(ftype==1){
				frame->waitting_ts=-1;
			}
		}
	}
    if(frame->packets[index]!=0){
        uint32_t oldseq=frame->packets[index]->seg.packet_id;
        uint8_t oldpid=frame->packets[index]->pid;
        printf("fatal error %d %d %d %d \n",oldpid,pid,oldseq,d->packet_id);
        return;
    }    
	if(!free_segs_.empty()){
		packet=free_segs_.front();
		free_segs_.pop();
	}else{
		packet=new video_packet_t();
		seg_c_++;
	}
	uint32_t now=rtc::TimeMillis();
	packet->pid=pid;
	//packet->ts=now;
	sim_segment_t *seg=&(packet->seg);
	memcpy(seg,d,sizeof(sim_segment_t));  
    //if(frame->recv==0) 
    {
        frame->ts=now;
    }
	
	frame->recv++;
	frame->packets[index]=packet;
	frame_cache_.insert(std::make_pair(fid,frame));
	if(frame->recv<frame->total&&frame->waitting_ts!=-1){
		frame->waitting_ts=now+GetWaittingDuration();
	}
    CheckDeliverFrame();
}
void MultipathReceiver::RegisterPath(Ptr<PathReceiver> path){
	for(auto it=paths_.begin();it!=paths_.end();it++){
		if((*it)==path){
			return;
		}
	}
	paths_.push_back(path);
	path->RegisterReceiverInterface(this);
	path->SetClock(&clock_);
}
void MultipathReceiver::Process(){
    if(stop_){return ;}
    uint32_t now=Simulator::Now().GetMilliSeconds();
	CheckDeliverFrame();
	CheckFrameDeliverBlock(now);
}
void MultipathReceiver::Stop(){
	Ptr<PathReceiver> path=NULL;
	hbTimer_.Cancel();
	stop_=true;
	for(auto it=paths_.begin();it!=paths_.end();it++){
		path=(*it);
		path->Stop();
	}
}
}
