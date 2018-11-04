#include "fakevideoconsumer.h"
#include <string>
#include "ns3/log.h"
#include <stdio.h>
namespace ns3{
NS_LOG_COMPONENT_DEFINE ("FakeVideoConsumer");
void FakeVideoConsumer::ForwardUp(uint32_t fid,uint8_t*data,
			uint32_t len,uint32_t recv,uint32_t total){
	NS_LOG_INFO("recv video "<<std::to_string(fid)<< " total "<<std::to_string(total)
                <<" recv "<<std::to_string(recv));
}
}




