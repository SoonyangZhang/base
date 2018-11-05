#ifndef NS3_MPVIDEO_SCALESCHEDULE_H_
#define NS3_MPVIDEO_SCALESCHEDULE_H_
#include "schedule.h"
namespace ns3{
class ScaleSchedule:public Schedule{
public:
	ScaleSchedule(){}
	~ScaleSchedule(){}
	void IncomingPackets(std::map<uint32_t,uint32_t>&packets) override;
	void RetransPackets(std::map<uint32_t,uint32_t>&packets) override;
	void RoundRobin(std::map<uint32_t,uint32_t>&packets);
	void RegisterPath(uint8_t pid) override;
	void UnregisterPath(uint8_t pid) override;
private:
	std::vector<uint8_t> pids_;
};
}




#endif /* NS3_MPVIDEO_SCALESCHEDULE_H_ */
