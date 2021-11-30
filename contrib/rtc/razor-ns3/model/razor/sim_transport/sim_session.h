/*-
* Copyright (c) 2017-2018 wenba, Inc.
*	All rights reserved.
*
* See the file LICENSE for redistribution information.
*/

struct __sim_session
{
	su_socket		s;
	su_addr			peer;				

	uint32_t		cid;
	uint32_t		uid;				/*�����û�ID*/

	uint32_t		rtt;				/*rttֵ*/
	uint32_t		rtt_var;			/*rtt�������ֵ*/
	uint8_t			loss_fraction;		/*������, 0 ~ 255֮�������100% = 255*/

	int				state;				/*״̬*/
	int				interrupt;			/*�ж�*/

	volatile int	run;				/*run�̱߳�ʾ */
	su_mutex		mutex;				/*�����ϲ���̲߳����ı�����*/
	su_thread		thr;				/*�߳�ID*/

	sim_sender_t*	sender;				/*��Ƶ������*/
	sim_receiver_t*	receiver;			/*��Ƶ������*/

	int				resend;				/*�ط�����*/
	int64_t			commad_ts;			/*����ʱ���*/
	int64_t			stat_ts;

	uint32_t		rbandwidth;			/*���մ���*/
	uint32_t		sbandwidth;			/*���ʹ���*/
	uint32_t		rcount;				/*���յı�������*/
	uint32_t		scount;				/*���͵ı�������*/
	uint32_t		video_bytes;
	uint32_t		max_frame_size;		/*���������֡*/

	int				min_bitrate;		/*���õ���С���ʣ���λ��bps*/
	int				max_bitrate;		/*���õ�������ʣ���λ: bps*/
	int				start_bitrate;		/*���õ���ʼ���ʣ���λ: bps*/

	sim_notify_fn	notify_cb;
	sim_change_bitrate_fn change_bitrate_cb;
	sim_state_fn	state_cb;
	void*			event;

	bin_stream_t	sstrm;
};

