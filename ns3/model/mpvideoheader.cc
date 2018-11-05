#include "mpvideoheader.h"
#include "ns3/log.h"
namespace ns3{
NS_LOG_COMPONENT_DEFINE("MpvideoHeader");
TypeId MpvideoHeader::GetTypeId ()
{
    static TypeId tid = TypeId ("MpvideoHeader")
      .SetParent<Header> ()
      .AddConstructor<MpvideoHeader> ()
    ;
    return tid;
}
ns3::TypeId MpvideoHeader::GetInstanceTypeId() const
{
	return GetTypeId();
}
uint32_t MpvideoHeader::GetSerializedSize() const{
	return sizeof(uint8_t);
}
void MpvideoHeader::Serialize (Buffer::Iterator start) const{
	NS_LOG_FUNCTION (this << &start);
	Buffer::Iterator i = start;
	i.WriteU8(pt_);
}
uint32_t MpvideoHeader::Deserialize (Buffer::Iterator start){
    Buffer::Iterator i = start;
	pt_=i.ReadU8();
	return GetSerializedSize();
}
void MpvideoHeader::Print(std::ostream &os) const{
	switch(pt_){
	case PayloadType::razor:{
		os<<"razor";
		break;
	}
	case PayloadType::webrtc_fb:{
		os<<"webrtc";
		break;
	}
	default:{
		os<<"unknown";
		break;
	}
	}

}
}



