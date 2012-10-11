/*
 * frame_constructor_if.c
 *
 *  Created on: Apr 15, 2010
 *      Author: rfajardo
 */

#include "frame_constructor_if.h"
#include "pwc_user.h"
#include "buffers.h"

#include <libusb-1.0/libusb.h>

#include <stdio.h>		//printf, perror
#include <string.h>		// memcpy, memmove

//
// Dealing with Communication
//
void fillFrame(struct frameBuf * frame_buffer, const unsigned char * p_packet, unsigned int len)
{
#ifdef ACQUIRE_INFO
			printf(">>fillFrame\n");
#endif
	static unsigned int frame_nr = 0;

	unsigned char * dst_buf;

	if ( len > 0 )
	{
		if ( frame_buffer->filled + len <= FRAME_SIZE )					//avoid overflow
		{
			dst_buf = frame_buffer->data + frame_buffer->filled;
			memcpy(dst_buf, p_packet, len);
			frame_buffer->filled += len;
		}
	}
#ifdef ACQUIRE_INFO
			printf("<<fillFrame\n");
#endif
}

void finishFrame(struct buffering * fbufs, const unsigned char * p_packet, unsigned int len)
{
#ifdef ACQUIRE_INFO
			printf(">>finishFrame\n");
#endif
	static unsigned int frame_nr = 0;

	unsigned char * dst_buf;
	struct frameBuf * frame_buffer = fbufs->fill_frame;

	if ( frame_buffer->filled + len == FRAME_SIZE ) //frame complete, store and get new frame ptr
	{
		dst_buf = frame_buffer->data + frame_buffer->filled;
		memcpy(dst_buf, p_packet, len);
		frame_buffer->filled += len;

		next_fill_data_buffer(fbufs);
		frame_buffer = fbufs->fill_frame;
		frame_buffer->filled = 0;
#ifdef COM_DEV_INFO
		printf("Frame nr: %d\n", frame_nr++);
#endif
	}
	else //frame incomplete or buffer overflow
	{
		frame_buffer->filled = 0;
#ifdef COM_DEV_INFO
		printf("Dropping frame\n");
#endif
	}
#ifdef ACQUIRE_INFO
			printf("<<finishFrame\n");
#endif
}
//
// ~Dealing with Communication
//
