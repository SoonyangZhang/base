#ifndef NULLSINK_H_
#define NULLSINK_H_
#include "datasink.h"
#include "videoframe.h"
namespace zsy{
class NullSink:public DataSink<VideoFrame>{
public:
	NullSink(){}
	~NullSink(){}
	void OnIncomingData(VideoFrame *) override;
private:
	bool flag_{true};
	uint32_t first_;
	uint32_t last_;
};
}




#endif /* NULLSINK_H_ */
