#ifndef MULTIPATH_FAKEVIDEOGENERATOR_H_
#define MULTIPATH_FAKEVIDEOGENERATOR_H_
#include "ratecontrol.h"
#include "mpcommon.h"
namespace zsy{
//rate_  in bps;
class FakeVideoGenerator :public RateChangeCallback{
public:
	FakeVideoGenerator(uint32_t minbitrate,uint32_t fs);
	~FakeVideoGenerator(){}
	void ChangeRate(uint32_t bitrate) override;
	void RegisterSession(SessionInterface *s);
	void HeartBeat(uint32_t now);
	void SendFrame(uint32_t now);
	void Stop(){stop_=true;}
private:
	uint32_t min_bitrate_;
	uint32_t fs_;
	uint32_t rate_;
	uint32_t first_ts_;
	uint32_t last_send_ts_;
	uint32_t delta_;
	uint32_t frame_id_;
	bool stop_;
	SessionInterface *session_;
};
}



#endif /* MULTIPATH_FAKEVIDEOGENERATOR_H_ */
