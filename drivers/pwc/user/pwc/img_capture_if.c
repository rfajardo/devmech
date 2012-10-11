/*
 * img_capture_if.c
 *
 *  Created on: Apr 15, 2010
 *      Author: rfajardo
 */

#include "img_capture_if.h"
#include "pwc_user.h"

#include "buffers.h"
#include "images.h"

#include "dev_class.h"

#include <stdio.h>		//printf, perror


static int extractFrame(struct images * p_img, struct buffering * buffer);

//
// Delivering Images to the Loopback Device
//
void * fillImageData(void * img_arg)
{
	struct imgArg * p_fill = (struct imgArg *)img_arg;
	int iRet;

	while (!*p_fill->abort)
	{
#ifdef THREAD_INFO
		printf(">>fillImageData\n");
#endif

		if ( p_fill->video_pipe->allow_capture
#ifdef V4L_COMPATIBLE
						&& p_fill->capture )
#else
											)
#endif
		{
			while( extractFrame(p_fill->p_img, p_fill->frames) && !*p_fill->abort && p_fill->video_pipe->allow_capture )
			{
				usleep(50000);		//time "half of a frame" -> 10fps
				if ( *p_fill->abort )
					printf("Immortal process should abort: out of extractFrame\n");
			}

			p_fill->capture = 0;
#ifndef V4L_COMPATIBLE
			iRet = instream_devClass(p_fill->video_pipe, p_fill->p_img->len_per_image);
#endif
#ifdef PIPE_DEV_INFO
			printf("Pushed %d data into pipe\n", iRet);
#endif
#ifdef THREAD_INFO
			printf("<<fillImageData\n");
#endif
		}
		else
		{
			usleep(50000);		//time "half of a frame" -> 10fps
			if ( *p_fill->abort )
				printf("Immortal process should abort: out of abort\n");
		}
	}
	pthread_exit(NULL);
}

//
// Delivering Frames
//
static int extractFrame(struct images * p_img, struct buffering * buffer)
{
	if ( get_read_frame(buffer) )
	{
#ifdef PIPE_DEV_INFO
		printf("No frame ready!\n");
#endif
		return -1;
	}

	pwc_decompress(p_img, buffer->read_frame);

	release_read_frame(buffer);
	return 0;
}
