#include "sim_proto.h"
#include "cf_stream.h"
#include <iostream>
#include "sim_proto.h"
void sim_encode_header(bin_stream_t* strm, sim_header_t* header)
{
	mach_uint8_write(strm, header->ver);
	mach_uint8_write(strm, header->mid);
	mach_uint32_write(strm, header->uid);
}
void sim_connect_encode(bin_stream_t* strm, sim_connect_t* body)
{
	mach_uint32_write(strm, body->cid);
	mach_data_write(strm, body->token, body->token_size);
	mach_int8_write(strm, body->cc_type);
}
void test_encode_msg(bin_stream_t* strm, sim_header_t* header, void* body){
    bin_stream_rewind(strm, 1);
    sim_encode_header(strm, header);
    std::cout<<strm->used<<std::endl;
    sim_connect_encode(strm, (sim_connect_t*)body);
    std::cout<<strm->used<<std::endl;
}

int main()
{
    bin_stream_t	strm;
    bin_stream_init(&strm);   
	sim_header_t header;
	sim_connect_t body;

	INIT_SIM_HEADER(header, SIM_CONNECT, 1234);
	body.cid = 1234;
	body.token_size = 0;
	body.cc_type = 0;
	//test_encode_msg(&strm,&header,&body);
    sim_encode_msg(&strm,&header,&body);
    std::cout<<"fuck "<<strm.used<<std::endl;
    bin_stream_destroy(&strm);  
    return 0;
}
