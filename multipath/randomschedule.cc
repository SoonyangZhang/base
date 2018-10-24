#include "randomschedule.h"
#include "path.h"
namespace zsy{
RandomSchedule::RandomSchedule(){

}
void RandomSchedule::IncomingPackets(std::map<uint32_t,uint32_t>&packets){

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




