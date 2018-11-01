#ifndef MULTIPATH_FAKEVIDEOCONSUMER_H_
#define MULTIPATH_FAKEVIDEOCONSUMER_H_
#include "mpcommon.h"
namespace zsy{
class FakeVideoConsumer:public NetworkDataConsumer{
public:
	void ForwardUp(uint32_t fid,uint8_t*data,
			uint32_t len,uint32_t recv,uint32_t total) override;
};
}




#endif /* MULTIPATH_FAKEVIDEOCONSUMER_H_ */
