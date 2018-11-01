#ifndef MULTIPATH_MINCOSTSCHEDULE_H_
#define MULTIPATH_MINCOSTSCHEDULE_H_
#include <list>
#include "schedule.h"
namespace zsy{
class MinCostSchedule:public Schedule{
public:
MinCostSchedule(){}
~MinCostSchedule(){}
void IncomingPackets(std::map<uint32_t,uint32_t>&packets) override;
void RetransPackets(std::map<uint32_t,uint32_t>&packets) override;
void RegisterPath(uint8_t) override;
void UnregisterPath(uint8_t) override;
private:
	std::list<uint8_t> pids_;
};
}




#endif /* MULTIPATH_MINCOSTSCHEDULE_H_ */
