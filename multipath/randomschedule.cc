#include "randomschedule.h"
#include "path.h"
#include <stdlib.h>
#include <time.h>
namespace zsy{
RandomSchedule::RandomSchedule(){
	srand(time(NULL));
    int64_t stream=rand();
    stream=(stream<<32)+rand();
	uniform_.SetStream(stream);
}
void RandomSchedule::IncomingPackets(std::map<uint32_t,uint32_t>&packets){
	uint8_t total=pids_.size();
	for(auto it=packets.begin();it!=packets.end();it++){
		uint32_t packet_id=it->first;
		uint32_t index=(uniform_.GetInteger())%total;
		uint8_t pid=pids_[index];
		sender_->PacketSchedule(packet_id,pid);
	}
}
void RandomSchedule::RetransPackets(std::map<uint32_t,uint32_t>&packets){

}
void RandomSchedule::RegisterPath(uint8_t pid){
	pids_.push_back(pid);
}
void RandomSchedule::UnregisterPath(uint8_t pid){
	for(auto it=pids_.begin();it!=pids_.end();){
		if((*it)==pid){
			it=pids_.erase(it);
			break;
		}
		else{
			it++;
		}
	}
}
}
