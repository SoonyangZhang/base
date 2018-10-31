/*-
* Copyright (c) 2017-2018 wenba, Inc.
*	All rights reserved.
*
* See the file LICENSE for redistribution information.
*/

/*һ��nack������Ĵ���������*/
typedef struct
{
	uint32_t*		buckets;
	uint32_t		index;
	int64_t			oldest_ts;

	int				wnd_size;			/*ͳ�Ƶ�ʱ�䴰*/
	uint32_t		wnd_bytes;
	uint32_t		threshold;			/*�����Է��͵��ֽ���*/
}sim_sender_limiter_t;

struct __sim_sender
{
	int							actived;
	uint32_t					base_packet_id;			/*���ܶ˱����Ѿ����յ������������ID*/

	uint32_t					packet_id_seed;
	uint32_t					frame_id_seed;
	uint32_t					transport_seq_seed;

	int64_t						first_ts;			/*��һ֡��ʼʱ���*/

	skiplist_t*					cache;				/*���ʹ���*/

	razor_sender_t*				cc;					/*ӵ�����ƶ���*/

	sim_session_t*				s;

	sim_sender_limiter_t		limiter;
};




