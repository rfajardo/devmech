/*
 * buffers.h
 *
 *  Created on: Mar 15, 2010
 *      Author: rfajardo
 */

#ifndef BUFFERS_H_
#define BUFFERS_H_

#include <pthread.h>


#define ISO_FRAMES_PER_DESC 	10
#define ISO_MAX_FRAME_SIZE 		960
#define ISO_BUFFER_SIZE 		(ISO_FRAMES_PER_DESC * ISO_MAX_FRAME_SIZE)
#define ISO_BUFFERS_NR			2

#define HEADER_SIZE				8
#define TRAILER_SIZE			4
#define FRAME_SIZE				(94560 + HEADER_SIZE + TRAILER_SIZE) //94560 results from: (vbandlength * image.y)/4
#define	FRAME_BUFFERS_NR		3


enum buf_errors
{
	BUF_READ_SET = 100,
	BUF_FULL_NULL = 120,
};


struct frameBuf
{
	void * data;
	volatile int filled;		/* number of bytes filled */
	struct frameBuf * next;		/* list */
};

struct buffering
{
	pthread_mutex_t mutex;

	unsigned char * iso_buffer[ISO_BUFFERS_NR];
	unsigned char * data_buffer[FRAME_BUFFERS_NR];

	struct frameBuf * frame_buffer[FRAME_BUFFERS_NR];
	struct frameBuf * empty_frames, * empty_frames_tail;
	struct frameBuf * full_frames, * full_frames_tail;
	struct frameBuf * fill_frame;
	struct frameBuf * read_frame;
};

struct buffering * alloc_buffers();
void free_buffers(struct buffering * buffer);

void init_data_buffer(struct buffering * buffer);
void next_fill_data_buffer(struct buffering * buffer);

int get_read_frame(struct buffering * buffer);
void release_read_frame(struct buffering * buffer);


#endif /* BUFFERS_H_ */
