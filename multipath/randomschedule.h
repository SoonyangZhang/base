#ifndef MULTIPATH_RANDOMSCHEDULE_H_
#define MULTIPATH_RANDOMSCHEDULE_H_
#include <vector>
#include "schedule.h"
#include "random-variable-stream.h"
namespace zsy{
// implement in round robin way
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
	ns3::UniformRandomVariable uniform_; 
    uint32_t last_index_;
};
}




#endif /* MULTIPATH_RANDOMSCHEDULE_H_ */
