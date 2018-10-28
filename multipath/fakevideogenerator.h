#ifndef MULTIPATH_FAKEVIDEOGENERATOR_H_
#define MULTIPATH_FAKEVIDEOGENERATOR_H_
#include "ratecontrol.h"
namespace zsy{
class FakeVideoGenerator :public RateChangeCallback{
public:
	FakeVideoGenerator(uint32_t minbitrate,uint32_t fs);
	~FakeVideoGenerator(){}
	void ChangeRate(uint32_t bitrate) override;
private:
	uint32_t min_bitrate_;
	uint32_t fs_;
	uint32_t rate_;
	uint32_t first_ts_;
};
}



#endif /* MULTIPATH_FAKEVIDEOGENERATOR_H_ */
