#include "roundrobinschedule.h"
namespace ns3{
RoundRobinSchedule::RoundRobinSchedule(){
    last_index_=0;
}
void RoundRobinSchedule::IncomingPackets(std::map<uint32_t,uint32_t>&packets){
	uint8_t total=pids_.size();
	for(auto it=packets.begin();it!=packets.end();it++){
		uint32_t packet_id=it->first;
		//uint32_t index=(uniform_.GetInteger())%total;
        uint32_t index=last_index_%total;
        last_index_++;
		uint8_t pid=pids_[index];
		sender_->PacketSchedule(packet_id,pid);
	}
}
void RoundRobinSchedule::RetransPackets(std::map<uint32_t,uint32_t>&packets){

}
void RoundRobinSchedule::RegisterPath(uint8_t pid){
	pids_.push_back(pid);
}
void RoundRobinSchedule::UnregisterPath(uint8_t pid){
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




