#ifndef MULTIPATH_RANDOMSCHEDULE_H_
#define MULTIPATH_RANDOMSCHEDULE_H_
#include <vector>
#include "schedule.h"
namespace zsy{
class RandomSchedule:public Schedule{
public:
	RandomSchedule();
	~RandomSchedule(){}
void IncomingPackets(std::map<uint32_t,uint32_t>&packets) override;
void RetransPackets(std::map<uint32_t,uint32_t>&packets) override;
void RegisterPath(uint8_t) override;
void UnregisterPath(uint8_t) override;
private:
	std::vector<uint8_t> pids_;
};
}




#endif /* MULTIPATH_RANDOMSCHEDULE_H_ */
