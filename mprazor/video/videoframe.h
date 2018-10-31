#ifndef VIDEOFRAME_H_
#define VIDEOFRAME_H_
#include <stdint.h>
#include <memory>
namespace zsy{
class VideoFrame{
public:
	VideoFrame();
	~VideoFrame(){}
	void ini(uint32_t len);
	void set_time(uint64_t time){capture_ts_=time;}
	int size(){ return size_;};
	uint8_t* get_buf(){return buf_.get();}
	void fill_buf(uint8_t *data,uint32_t len);
	uint64_t get_time(){return capture_ts_;}
	void Clear(){empty_=true;}
	bool Empty(){return empty_;}
private:
	std::unique_ptr<uint8_t []> buf_;
	uint64_t capture_ts_;
	bool empty_;
	uint32_t size_;
	uint32_t capacity_;
};
}




#endif /* VIDEOFRAME_H_ */
