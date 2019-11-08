#pragma once
#include <stdint.h>
#include <string>
#include "random.h"
class Camera{
public:
	Camera(int width,int height,int fps,
			std::string &name);
	~Camera();
	bool getNewFrame(int now,uint8_t *buffer,
			int bufSize,int &outSize);
	int getFrames() const {return frames_;}
	int getFrameCount() const {return frame_count_;}
private:
	int FillBuffer(uint8_t *buffer,int bufSize);
	void Reset();
	int frame_gap_{0};
	int frames_{0};
	int frame_count_{0};
	int frame_size_{0};
	int nextFrametick_{0};
	FILE *yuv_{nullptr};
	basic::Random random_;
};
