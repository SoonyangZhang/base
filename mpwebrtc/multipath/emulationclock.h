#ifndef MULTIPATH_EMULATIONCLOCK_H_
#define MULTIPATH_EMULATIONCLOCK_H_
#include "system_wrappers/include/clock.h"
namespace zsy{
class EmulationClock:public webrtc::Clock
{
public:
	EmulationClock(){}
	~EmulationClock(){}
	int64_t TimeInMilliseconds() const override;
	int64_t TimeInMicroseconds() const override;
	webrtc::NtpTime CurrentNtpTime() const override;
	int64_t CurrentNtpInMilliseconds() const override;
};
}



#endif /* MULTIPATH_EMULATIONCLOCK_H_ */
