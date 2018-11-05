#include "scaleschedule.h"
#include "pathsender.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <map>
namespace ns3{
NS_LOG_COMPONENT_DEFINE("ScaleSchedule");
void ScaleSchedule::IncomingPackets(std::map<uint32_t,uint32_t>&packets){
	uint32_t totalrate=0;
	uint32_t packet_num=packets.size();
	std::map<uint8_t,uint32_t> pathrate;
	if(pids_.size()==1){
		RoundRobin(packets);
		return;
	}
	for(auto it=pids_.begin();it!=pids_.end();it++){
		uint8_t pid=(*it);
		Ptr<PathSender> path=sender_->GetPathInfo(pid);
		if(path->rate_!=0){
			uint32_t rate=path->rate_;
			totalrate+=rate;
			pathrate.insert(std::make_pair(pid,rate));
		}
	}
	int useable_path=pathrate.size();
	uint8_t *path_array=new uint8_t [useable_path];
	int i=0;
	int path_counter=0;
	std::map<uint8_t,uint32_t> pathpacket;
	uint32_t remain=packet_num;
	{
		auto it=pathrate.begin();
		double rate=(double)it->second;
		for(i=0;i<useable_path;i++){
			uint32_t num=(uint32_t)(remain*rate/totalrate+0.5);
			uint8_t pid=it->first;
			if((i+1)==useable_path){
				if(remain>0){
					pathpacket.insert(std::make_pair(pid,remain));
					path_array[path_counter]=pid;
					path_counter++;
				}
			break;
			}else{
				if(remain>0){
					pathpacket.insert(std::make_pair(pid,num));
					path_array[path_counter]=pid;
					path_counter++;
				}
			}
			it++;
			remain-=num;
		}
	}
	if(totalrate==0){
		RoundRobin(packets);
	}else{
		auto it=packets.begin();
		for(i=0;i<path_counter;i++){
			int j=0;
			uint8_t temp_pid=path_array[i];
			auto pathit=pathpacket.find(temp_pid);
			uint32_t path_i_num=pathit->second;
			for(j=0;j<path_i_num;j++){
				sender_->PacketSchedule(it->first,temp_pid);
				it++;
			}
		}
		if(it!=packets.end()){
			NS_LOG_ERROR("fatal error");
		}
	}
	delete path_array;
}
void ScaleSchedule::RoundRobin(std::map<uint32_t,uint32_t>&packets){
	uint8_t total=pids_.size();
    uint32_t last_index=0;
	for(auto it=packets.begin();it!=packets.end();it++){
		uint32_t packet_id=it->first;
        uint32_t index=last_index%total;
        last_index++;
		uint8_t pid=pids_[index];
		sender_->PacketSchedule(packet_id,pid);
	}
}
void ScaleSchedule::RetransPackets(std::map<uint32_t,uint32_t>&packets){

}
void ScaleSchedule::RegisterPath(uint8_t pid){
	for(auto it=pids_.begin();it!=pids_.end();it++){
		if((*it)==pid){
			return;
		}
	}
	pids_.push_back(pid);
}
void ScaleSchedule::UnregisterPath(uint8_t pid){
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



