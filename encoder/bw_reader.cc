#include <vector>
#include "split.h"
#include "bw_reader.h"
#include "logging.h"
const double kUpdateInterval=500;
BandwidthReader::BandwidthReader(std::string &trace,int column){
	bwTrace_.open(trace);
	if(!bwTrace_){
		LOG(INFO)<<"open BWTrace Failed";
		exit(0);
	}
	column_=column;
	CountLines();
	UpdateBandwidth();
}
BandwidthReader::~BandwidthReader(){
	if(bwTrace_.is_open()){
		bwTrace_.close();
	}

}
bool BandwidthReader::ShouldUpdateBandwidth(int now) const{
	bool ret=false;
	if(nextFrametick_==0){
		ret=true;
	}else{
		if(now>=nextFrametick_){
			ret=true;
		}
	}
	return ret;
}
void BandwidthReader::Update(int now){
	if(now>=nextFrametick_){
		nextFrametick_=now+kUpdateInterval;
		UpdateBandwidth();
	}
}
void BandwidthReader::CountLines(){
	if(!bwTrace_.is_open()){
		return;
	}
	bwTrace_.clear();
	bwTrace_.seekg(0, std::ios::beg);
	while(!bwTrace_.eof()){
		std::string line;
		getline(bwTrace_,line);
		std::vector<std::string> str_arry=split(line," ");
		int component=str_arry.size();
		if(component>0){
			lines_++;
		}
	}
	//https://blog.csdn.net/lixiaoguang20/article/details/78193419
	//without clear, it crashes.
	//when end of file, seek has no effect, clear first,
	bwTrace_.clear();
	bwTrace_.seekg(0, std::ios::beg);
}
void BandwidthReader::UpdateBandwidth(){
	if(!bwTrace_.eof()){
		std::string line;
		getline(bwTrace_,line);
		std::vector<std::string> str_arry=split(line," ");
		int component=str_arry.size();
		CHECK(component>0);
		double bw=std::stod(str_arry[column_-1]);
		bw=bw*100;
		int cast=bw;
		bw_=(double)cast/100;
	}
	offset_++;
}
