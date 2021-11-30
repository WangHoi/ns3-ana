/*-
* Copyright (c) 2017-2018 wenba, Inc.
*	All rights reserved.
*
* See the file LICENSE for redistribution information.
*/

enum{
	buffer_waiting = 0,
	buffer_playing
};

typedef struct
{
	uint32_t			fid;
	uint32_t			last_seq;
	uint32_t			ts;
	int					frame_type;

	int					seg_number;
	sim_segment_t**		segments;
}sim_frame_t;

typedef struct
{
	uint32_t			size;

	uint32_t			min_seq;

	uint32_t			min_fid;
	uint32_t			max_fid;
	uint32_t			play_frame_ts;
	uint32_t			max_ts;			/*������������ts*/

	uint32_t			frame_ts;		/*�Ѿ����ŵ������Ƶʱ���*/
	uint64_t			play_ts;		/*��ǰϵͳʱ�ӵ�ʱ���*/

	uint32_t			frame_timer;	/*֡���ʱ��*/
	uint32_t			wait_timer;		/*cache����ʱ�䣬�Ժ���Ϊ��λ�����cache��ʱ�䳤��Ӧ����> rtt + 2 * rtt_val,�����ط����ĵĴ���������*/

	int					state;
	int					loss_flag;

	float				f;

	sim_frame_t*		frames;
}sim_frame_cache_t;

struct __sim_receiver
{
	uint32_t			base_uid;
	uint32_t			base_seq;
	uint32_t			max_seq;

	skiplist_t*			loss;
	int					loss_count;				/*��λʱ���ڳ��ֶ����Ĵ���*/

	sim_frame_cache_t*	cache;

	uint64_t			ack_ts;
	uint64_t			cache_ts;
	uint64_t			active_ts;

	int					actived;

	razor_receiver_t*	cc;
	sim_session_t*		s;
};


