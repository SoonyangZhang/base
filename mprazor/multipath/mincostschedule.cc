#include "mincostschedule.h"
#include "path.h"
namespace zsy{
void MinCostSchedule::IncomingPackets(std::map<uint32_t,uint32_t>&packets){

}
void MinCostSchedule::RetransPackets(std::map<uint32_t,uint32_t>&packets){

}
void MinCostSchedule::RegisterPath(uint8_t pid){
	pids_.push_back(pid);
}
void MinCostSchedule::UnregisterPath(uint8_t pid){
	pids_.remove(pid);
}
}




