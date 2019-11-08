#pragma once
#include <string>
#include <fstream>
#include <stdint.h>
class BandwidthReader{
public:
	BandwidthReader(std::string &trace,int column);
	~BandwidthReader();
	bool IsEnd() const {return offset_>=lines_;}
	bool ShouldUpdateBandwidth(int now) const;
	void Update(int now);
	double getBandwidth() const {return bw_;};
	int getLines() const {return lines_;}
private:
	void CountLines();
	void UpdateBandwidth();
	std::fstream bwTrace_;
	int column_{0};
	double bw_{0.0};
	int offset_{0};
	int lines_{0};
	int nextFrametick_{0};
};
