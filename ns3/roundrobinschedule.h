#ifndef NS3_MPVIDEO_ROUNDROBINSCHEDULE_H_
#define NS3_MPVIDEO_ROUNDROBINSCHEDULE_H_
#include <vector>
#include "schedule.h"
namespace ns3{
// implement in round robin way
class RoundRobinSchedule:public Schedule{
public:
	RoundRobinSchedule();
	~RoundRobinSchedule(){}
void IncomingPackets(std::map<uint32_t,uint32_t>&packets) override;
void RetransPackets(std::map<uint32_t,uint32_t>&packets) override;
void RegisterPath(uint8_t) override;
void UnregisterPath(uint8_t) override;
private:
	std::vector<uint8_t> pids_;
    uint32_t last_index_;
};

}

#endif /* NS3_MPVIDEO_ROUNDROBINSCHEDULE_H_ */
