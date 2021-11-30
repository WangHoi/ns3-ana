/*-
* Copyright (c) 2017-2018 wenba, Inc.
*	All rights reserved.
*
* See the file LICENSE for redistribution information.
*/

#include "sim_internal.h"
#include <assert.h>

#define CACHE_SIZE 1024
#define INDEX(i)	((i) % CACHE_SIZE)
#define MAX_VIDEO_DELAY_MS 5000

/************************************************���Ż������Ķ���**********************************************************/
static sim_frame_cache_t* open_real_video_cache(sim_session_t* s)
{
	sim_frame_cache_t* cache = calloc(1, sizeof(sim_frame_cache_t));
	cache->wait_timer = s->rtt + 2 * s->rtt_var;
	cache->state = buffer_waiting;
	cache->min_seq = 0;
	cache->frame_timer = 100;
	cache->f = 1.0f;
	cache->size = CACHE_SIZE;
	cache->frames = calloc(cache->size, sizeof(sim_frame_cache_t));
	return cache;
}

static inline void real_video_clean_frame(sim_session_t* session, sim_frame_cache_t* c, sim_frame_t* frame)
{
	int i;
	if (frame->seg_number == 0)
		return;

	for (i = 0; i < frame->seg_number; ++i){
		if (frame->segments[i] != NULL){
			c->min_seq = frame->segments[i]->packet_id - frame->segments[i]->index + frame->segments[i]->total - 1;
			free(frame->segments[i]);
			frame->segments[i] = NULL;
		}
	}

	free(frame->segments);

	c->play_frame_ts = frame->ts;
	frame->ts = 0;
	frame->frame_type = 0;
	frame->seg_number = 0;

	c->min_fid = frame->fid;
	sim_debug("clean frame = %u\n", frame->fid);
}

static void close_real_video_cache(sim_session_t* s, sim_frame_cache_t* cache)
{
	uint32_t i;
	for (i = 0; i < cache->size; ++i)
		real_video_clean_frame(s, cache, &cache->frames[i]);

	free(cache->frames);
	free(cache);
}

static void reset_real_video_cache(sim_session_t* s, sim_frame_cache_t* cache)
{
	uint32_t i;

	for (i = 0; i < cache->size; ++i)
		real_video_clean_frame(s, cache, &cache->frames[i]);

	cache->min_seq = 0;
	cache->min_fid = 0;
	cache->max_fid = 0;
	cache->play_ts = 0;
	cache->frame_ts = 0;
	cache->max_ts = 100;
	cache->frame_timer = 100;
	cache->f = 1.0f;

	cache->state = buffer_waiting;
	cache->wait_timer = SU_MAX(100, s->rtt + 2 * s->rtt_var);
	cache->loss_flag = 0;
}

static void real_video_evict_frame(sim_session_t* s, sim_frame_cache_t* c, uint32_t fid)
{
	uint32_t pos, i;

	for (pos = c->max_fid + 1; pos <= fid; pos++)
		real_video_clean_frame(s, c, &c->frames[INDEX(pos)]);

	if (fid < c->min_fid + c->size)
		return;

	for (pos = c->min_fid + 1; pos < c->max_fid; ++pos){
		if (c->frames[INDEX(pos)].frame_type == 1)
			break;
	}

	for (i = c->min_fid + 1; i < pos; ++i)
		real_video_clean_frame(s, c, &c->frames[INDEX(i)]);
}

static void evict_gop_frame(sim_session_t* s, sim_frame_cache_t* c)
{
	uint32_t i, key_frame_id;
	sim_frame_t* frame = NULL;

	key_frame_id = 0;
	for (i = c->min_fid + 2; i < c->max_fid; ++i){
		frame = &c->frames[INDEX(i)];
		if (frame->frame_type == 1){
			key_frame_id = i;
			break;
		}
	}

	if (key_frame_id == 0)
		return;

	sim_debug("evict_gop_frame, from frame = %u, to frame = %u!! \n", c->min_fid + 1, key_frame_id);
	for (i = c->min_fid + 1; i < key_frame_id; i++){
		c->frame_ts = c->frames[INDEX(i)].ts;
		real_video_clean_frame(s, c, &c->frames[INDEX(i)]);
	}
}

static int real_video_cache_put(sim_session_t* s, sim_frame_cache_t* c, sim_segment_t* seg)
{
	sim_frame_t* frame;
	int ret = -1;

	if (seg->index >= seg->total){
		assert(0);
		return ret;
	}
	else if (seg->fid <= c->min_fid)
		return ret;

	if (seg->fid > c->max_fid){
		if (c->max_fid > 0)
			real_video_evict_frame(s, c, seg->fid);
		else if (c->min_fid == 0){
			c->min_fid = seg->fid - 1;
			c->play_frame_ts = seg->timestamp;
		}

		if (c->max_fid >= 0 && c->max_fid < seg->fid && c->max_ts < seg->timestamp){
			c->frame_timer = (seg->timestamp - c->max_ts) / (seg->fid - c->max_fid);
			if (c->frame_timer < 20)
				c->frame_timer = 20;
			else if (c->frame_timer > 200)
				c->frame_timer = 200;
		}
		c->max_ts = seg->timestamp;
		c->max_fid = seg->fid;
	}

	/*sim_debug("buffer put video frame, frame = %u, packet_id = %u, ts = %u\n", seg->fid, seg->packet_id, seg->timestamp);*/

	frame = &(c->frames[INDEX(seg->fid)]);
	frame->fid = seg->fid;
	frame->frame_type = seg->ftype;
	frame->ts = seg->timestamp;

	if (frame->seg_number == 0){
		frame->seg_number = seg->total;
		frame->segments = calloc(frame->seg_number, sizeof(seg));
		frame->segments[seg->index] = seg;

		ret = 0;
	}
	else{
		if (frame->segments[seg->index] == NULL){
			frame->segments[seg->index] = seg;
			ret = 0;
		}
	}

	return ret;
}

static void real_video_cache_check_playing(sim_session_t* s, sim_frame_cache_t* c)
{
	uint32_t space = SU_MAX(c->wait_timer, c->frame_timer);

	if (c->max_fid > c->min_fid){
		if (c->max_ts >= c->play_frame_ts + space && c->max_fid >= c->min_fid + 1){
			c->state = buffer_playing;

			c->play_ts = GET_SYS_MS();
			c->frame_ts = c->max_ts - c->frame_timer * (c->max_fid - c->min_fid - 1);
			/*sim_debug("buffer playing, frame ts = %u, min_ts = %u, c->frame_timer = %u, max ts = %u\n", c->frame_ts, min_ts, c->frame_timer, max_ts);*/
		}
	}
}

static inline void real_video_cache_check_waiting(sim_session_t* s, sim_frame_cache_t* c)
{
}

static inline int real_video_cache_check_frame_full(sim_session_t* s, sim_frame_t* frame)
{
	int i;
	if (frame->seg_number <= 0)
		return -1;

	for (i = 0; i < frame->seg_number; ++i)
		if (frame->segments[i] == NULL)
			return -1;

	return 0;
}

static inline void real_video_cache_sync_timestamp(sim_session_t* s, sim_frame_cache_t* c)
{
	uint64_t cur_ts = GET_SYS_MS();

	if (cur_ts > c->play_ts){
		c->frame_ts = (uint32_t)((cur_ts - c->play_ts) * c->f) + c->frame_ts;
		c->play_ts = cur_ts;
	}
}

static uint32_t real_video_ready_ms(sim_session_t* s, sim_frame_cache_t* c)
{
	sim_frame_t* frame;
	uint32_t i, min_ready_ts, max_ready_ts, ret;

	ret = 0;
	min_ready_ts = 0;
	max_ready_ts = 0;
	for (i = c->min_fid + 1; i <= c->max_fid; ++i){
		frame = &c->frames[INDEX(i)];
		if (real_video_cache_check_frame_full(s, frame) == 0){
			if (min_ready_ts == 0)
				min_ready_ts = frame->ts;
			max_ready_ts = frame->ts;
		}
		else
			break;
	}

	if (min_ready_ts > 0)
		ret = max_ready_ts - min_ready_ts + c->frame_timer;

	return ret;
}

static int real_video_cache_get(sim_session_t* s, sim_frame_cache_t* c, uint8_t* data, size_t* sizep, uint8_t* payload_type, int loss)
{
	uint32_t pos, space;
	size_t size, buffer_size;
	int ret, i;
	sim_frame_t* frame;
	uint32_t play_ready_ts;
	uint8_t type;

	buffer_size = *sizep;
	*sizep = 0;
	*payload_type = 0;

	type = 0;
	size = 0;
	ret = -1;

	if (c->state == buffer_waiting)
		real_video_cache_check_playing(s, c);
	else
		real_video_cache_check_waiting(s, c);

	if (c->state != buffer_playing || c->min_fid == c->max_fid)
		goto err;

	space = SU_MAX(c->wait_timer, c->frame_timer);

	/*���㲥��ʱ��ͬ��*/
	c->f = 1.0f;
	if (c->play_frame_ts + space > c->max_ts)
		c->f = 0.6f;
	else if (c->play_frame_ts + space * 2 < c->max_ts)
		c->f = 1.6f;
	else if (c->play_frame_ts + space * 3 / 2 < c->max_ts)
		c->f = 1.2f;

	real_video_cache_sync_timestamp(s, c);

	if (c->play_frame_ts + SU_MAX(MAX_VIDEO_DELAY_MS, 4 * c->wait_timer) < c->max_ts){
		evict_gop_frame(s, c);
	}

	pos = INDEX(c->min_fid + 1);
	frame = &c->frames[pos];
	if ((c->min_fid + 1 == frame->fid || frame->frame_type == 1) && real_video_cache_check_frame_full(s, frame) == 0){
		play_ready_ts = real_video_ready_ms(s, c);

		/*���м�Ъ�Կ��*/
		if (frame->ts > c->frame_ts + 2 * space && play_ready_ts > space)
			c->frame_ts = frame->ts - space;
		else if (frame->ts <= c->frame_ts){
			for (i = 0; i < frame->seg_number; ++i){
				if (size + frame->segments[i]->data_size <= buffer_size && frame->segments[i]->data_size <= SIM_VIDEO_SIZE){ /*���ݿ���*/
					memcpy(data + size, frame->segments[i]->data, frame->segments[i]->data_size);
					size += frame->segments[i]->data_size;
					type = frame->segments[i]->payload_type;
				}
				else{
					size = 0;
					break;
				}
			}
			/*�Ͼ��ȴ������ʱ��*/
			if (space * 3 / 2 < play_ready_ts){
				c->frame_ts = frame->ts + c->frame_timer;
			}
			if (space < play_ready_ts)
				c->frame_ts = frame->ts + 2;
			else
				c->frame_ts = frame->ts;

			c->play_frame_ts = frame->ts;

			real_video_clean_frame(s, c, frame);
			ret = 0;
		}
	}
	else{
		size = 0;
		ret = -1;
	}

err:
	*sizep = size;
	*payload_type = type;

	return ret;
}

static uint32_t real_video_cache_get_min_seq(sim_session_t* s, sim_frame_cache_t* c)
{
	return c->min_seq;
}

static uint32_t real_video_cache_delay(sim_session_t* s, sim_frame_cache_t* c)
{
	if (c->max_fid <= c->min_fid)
		return 0;

	return c->max_ts - c->play_frame_ts;
}

/*********************************************��Ƶ���ն˴���*************************************************/
typedef struct
{
	int64_t				ts;					/*��������ʱ�̣�һ��Ҫ��һ�����ڲŽ������������������һ����RTT�ı�����ϵ*/
	int					count;				/*��������ʱ��*/
}sim_loss_t;

static void loss_free(skiplist_item_t key, skiplist_item_t val, void* args)
{
	if (val.ptr != NULL)
		free(val.ptr);
}

/*���ն˽���feedback�� ��feedback���͵����Ͷ�*/
static void send_sim_feedback(void* handler, const uint8_t* payload, int payload_size)
{
	sim_header_t header;
	sim_feedback_t feedback;
	sim_receiver_t* r = (sim_receiver_t*)handler;
	sim_session_t* s = r->s;

	if (payload_size > SIM_FEEDBACK_SIZE){
		/*sim_error("feedback size > SIM_FEEDBACK_SIZE\n");*/
		return;
	}

	INIT_SIM_HEADER(header, SIM_FEEDBACK, s->uid);

	feedback.base_packet_id = r->base_seq;

	feedback.feedback_size = payload_size;
	memcpy(feedback.feedback, payload, payload_size);

	sim_encode_msg(&s->sstrm, &header, &feedback);
	sim_session_network_send(s, &s->sstrm);

	sim_debug("sim send SIM_FEEDBACK, feedback size = %u\n", payload_size);
}

sim_receiver_t* sim_receiver_create(sim_session_t* s)
{
	sim_receiver_t* r = calloc(1, sizeof(sim_receiver_t));

	r->loss = skiplist_create(idu32_compare, loss_free, NULL);
	r->cache = open_real_video_cache(s);
	r->cache_ts = GET_SYS_MS();
	r->s = s;

	/*����һ�����ն˵�ӵ�����ƶ���*/
	r->cc = razor_receiver_create(MIN_BITRATE, MAX_BITRATE, SIM_SEGMENT_HEADER_SIZE, r, send_sim_feedback);

	return r;
}

void sim_receiver_destroy(sim_session_t* s, sim_receiver_t* r)
{
	if (r == NULL)
		return;

	assert(r->cache && r->loss);
	skiplist_destroy(r->loss);
	close_real_video_cache(s, r->cache);

	/*���ٽ��ն˵�ӵ�����ƶ���*/
	if (r->cc != NULL){
		razor_receiver_destroy(r->cc);
		r->cc = NULL;
	}

	free(r);
}

void sim_receiver_reset(sim_session_t* s, sim_receiver_t* r)
{
	reset_real_video_cache(s, r->cache);
	skiplist_clear(r->loss);

	r->base_uid = 0;
	r->base_seq = 0;
	r->actived = 0;
	r->max_seq = 0;
	r->ack_ts = GET_SYS_MS();
	r->active_ts = r->ack_ts;
	r->loss_count = 0;

	/*���´���һ��CC����*/
	if (r->cc != NULL){
		razor_receiver_destroy(r->cc);
		r->cc = NULL;
	}
	r->cc = razor_receiver_create(MIN_BITRATE, MAX_BITRATE, SIM_SEGMENT_HEADER_SIZE, r, send_sim_feedback);
}

int sim_receiver_active(sim_session_t* s, sim_receiver_t* r, uint32_t uid)
{
	if (r->actived == 1)
		return -1;

	r->actived = 1;
	r->cache->frame_timer = 50;		/*��дһ��Ĭ�ϵĲ���֡���*/

	r->base_uid = uid;
	r->active_ts = GET_SYS_MS();
	return 0;
}

static void sim_receiver_update_loss(sim_session_t* s, sim_receiver_t* r, uint32_t seq)
{
	uint32_t i;
	skiplist_item_t key, val;
	skiplist_iter_t* iter;
	int64_t now_ts;

	now_ts = GET_SYS_MS();
	key.u32 = seq;
	skiplist_remove(r->loss, key);

	for (i = r->max_seq + 1; i < seq; ++i){
		key.u32 = i;
		iter = skiplist_search(r->loss, key);
		if (iter == NULL){
			sim_loss_t* l = calloc(1, sizeof(sim_loss_t));
			l->ts = now_ts - s->rtt/ 2;						/*������һ�������ش���ʱ��*/
			l->count = 0;
			val.ptr = l;

			skiplist_insert(r->loss, key, val);
		}
	}
}

static inline void sim_receiver_send_ack(sim_session_t* s, sim_segment_ack_t* ack)
{
	sim_header_t header;
	INIT_SIM_HEADER(header, SIM_SEG_ACK, s->uid);

	sim_encode_msg(&s->sstrm, &header, ack);
	sim_session_network_send(s, &s->sstrm);
	/*sim_debug("send SEG_ACK, base = %u, ack_id = %u\n", ack->base_packet_id, ack->acked_packet_id);*/
}

/*����ack��nackȷ�ϣ������㻺�����ĵȴ�ʱ��*/
#define ACK_REAL_TIME	20
#define ACK_HB_TIME		200
static void video_real_ack(sim_session_t* s, sim_receiver_t* r, int hb, uint32_t seq)
{
	uint64_t cur_ts;
	sim_segment_ack_t ack;
	skiplist_iter_t* iter;
	skiplist_item_t key;
	sim_loss_t* l;
	uint32_t min_seq, delay, space_factor;
	int max_count = 0;

	cur_ts = GET_SYS_MS();
	/*�������������*/
	if ((hb == 0 && r->ack_ts + ACK_REAL_TIME < cur_ts) || (r->ack_ts + ACK_HB_TIME < cur_ts)){
		ack.acked_packet_id = seq;
		/**/
		min_seq = real_video_cache_get_min_seq(s, r->cache);
		if (min_seq > r->base_seq){
			for (key.u32 = r->base_seq + 1; key.u32 <= min_seq; ++key.u32)
				skiplist_remove(r->loss, key);
			r->base_seq = min_seq;
		}

		ack.base_packet_id = r->base_seq;
		ack.nack_num = 0;
		SKIPLIST_FOREACH(r->loss, iter){
			l = (sim_loss_t*)iter->val.ptr;

			if (iter->key.u32 <= r->base_seq)
				continue;

			space_factor = (SU_MIN(2, l->count / 2 + 1)) * (s->rtt + s->rtt_var); /*���ڼ򵥵�ӵ����������ֹGET��ˮ*/
			if (l->ts + space_factor <= cur_ts && l->count < 30 && ack.nack_num < NACK_NUM){
				ack.nack[ack.nack_num++] = iter->key.u32 - r->base_seq;
				l->ts = cur_ts;

				r->loss_count++;
				l->count++;
			}

			if (l->count > max_count)
				max_count = l->count;
		}
		
		sim_receiver_send_ack(s, &ack);

		r->ack_ts = cur_ts;
	}

	/*���ݶ����ش�ȷ����Ƶ��������Ҫ�Ļ���ʱ��*/
	if (max_count > 1){
		delay = (max_count + 8) * (s->rtt + s->rtt_var) / 8;
		if (delay > r->cache->wait_timer)
			r->cache->wait_timer = SU_MIN(delay, 3000);

		r->cache->wait_timer = SU_MAX(r->cache->frame_timer, r->cache->wait_timer);
	}
}

int sim_receiver_put(sim_session_t* s, sim_receiver_t* r, sim_segment_t* seg)
{
	uint32_t seq;

	/*ӵ������*/
	if (r->cc != NULL)
		r->cc->on_received(r->cc, seg->transport_seq, seg->timestamp + seg->send_ts, seg->data_size + SIM_SEGMENT_HEADER_SIZE, seg->remb);

	if (r->max_seq == 0 && seg->ftype == 0)
		return -1;

	seq = seg->packet_id;
	if (r->actived == 0 || seg->packet_id <= r->base_seq)
		return -1;

	if (r->max_seq == 0 && seg->packet_id > seg->index){
		r->max_seq = seg->packet_id - seg->index - 1;
		r->base_seq = seg->packet_id - seg->index - 1;
	}

	sim_receiver_update_loss(s, r, seq);
	if (real_video_cache_put(s, r->cache, seg) != 0)
		return -1;

	if (seq == r->base_seq + 1)
		r->base_seq = seq;

	r->max_seq = SU_MAX(r->max_seq, seq);

	video_real_ack(s, r, 0, seg->packet_id);

	return 0;
}

/*��ȡ��Ƶ֡����*/
int sim_receiver_get(sim_session_t* s, sim_receiver_t* r, uint8_t* data, size_t* sizep, uint8_t* payload_type)
{
	if (r == NULL || r->actived == 0)
		return -1;

	return real_video_cache_get(s, r->cache, data, sizep, payload_type, skiplist_size(r->loss));
}

void sim_receiver_timer(sim_session_t* s, sim_receiver_t* r, int64_t now_ts)
{
	video_real_ack(s, r, 1, 0);

	/*ÿ1�볢����һ�λ������ȴ�ʱ��*/
	if (r->cache_ts + SU_MAX(s->rtt + s->rtt_var, 1000) < now_ts){
		if (r->loss_count == 0)
			r->cache->wait_timer = SU_MAX(r->cache->wait_timer * 7 / 8, r->cache->frame_timer);
		else if (r->cache->wait_timer > 2 * (s->rtt + s->rtt_var))
			r->cache->wait_timer = SU_MAX(r->cache->wait_timer * 15 / 16, r->cache->frame_timer);

		r->cache_ts = GET_SYS_MS();
		r->loss_count = 0;
	}

	/*ӵ����������*/
	if (r->cc != NULL)
		r->cc->heartbeat(r->cc);
}

void sim_receiver_update_rtt(sim_session_t* s, sim_receiver_t* r)
{
	if (r->cc != NULL)
		r->cc->update_rtt(r->cc, s->rtt + s->rtt_var);
}

uint32_t sim_receiver_cache_delay(sim_session_t* s, sim_receiver_t* r)
{
	return real_video_cache_delay(s, r->cache);
}


