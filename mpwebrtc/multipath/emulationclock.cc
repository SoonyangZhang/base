#include "rtc_base/timeutils.h"
#include "emulationclock.h"
#include "log.h"
namespace zsy{
NS_LOG_COMPONENT_DEFINE("EmulationClock");
int64_t EmulationClock::TimeInMilliseconds() const
{
	return rtc::TimeMillis();
}
int64_t EmulationClock::TimeInMicroseconds() const
{
	return rtc::TimeMicros();
}
webrtc::NtpTime EmulationClock::CurrentNtpTime() const
{
	NS_LOG_FUNCTION("this should not happen");
	int64_t now_ms =rtc::TimeMillis();
	uint32_t seconds = (now_ms / 1000) + webrtc::kNtpJan1970;
	uint32_t fractions =
    	static_cast<uint32_t>((now_ms % 1000) * webrtc::kMagicNtpFractionalUnit / 1000);
	return webrtc::NtpTime(seconds, fractions);
}
int64_t EmulationClock::CurrentNtpInMilliseconds() const
{
	NS_LOG_FUNCTION("this should not happen");
	return rtc::TimeMillis() + 1000 * static_cast<int64_t>(webrtc::kNtpJan1970);
}
}




