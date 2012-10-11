/*
 * buffers.c
 *
 *  Created on: Mar 17, 2010
 *      Author: rfajardo
 */

#include "buffers.h"

#include <stdio.h>
#include <stdlib.h>	//NULL pointer, malloc

#include <pthread.h>


struct buffering * alloc_buffers()
{
	struct buffering * buffer = malloc( sizeof(struct buffering) );
	if ( buffer == NULL )
		return NULL;

	int i;
	for ( i = 0; i < ISO_BUFFERS_NR; i++)
	{
		buffer->iso_buffer[i] = malloc(ISO_BUFFER_SIZE);
		if ( buffer->iso_buffer[i] == NULL )
			goto error_alloc_isoBuf;
	}

	for ( i = 0; i < FRAME_BUFFERS_NR; i++)
	{
		buffer->data_buffer[i] = malloc(FRAME_SIZE);
		if ( buffer->data_buffer[i] == NULL )
			goto error_alloc_dataBuf;
	}

	for ( i = 0; i < FRAME_BUFFERS_NR; i++)
	{
		buffer->frame_buffer[i] = malloc(sizeof(struct frameBuf));
		if ( buffer->frame_buffer[i] == NULL )
			goto error_alloc_frameBuf;
	}

	if ( pthread_mutex_init(&buffer->mutex, NULL) )
		goto error_alloc_frameBuf;

	return buffer;

error_alloc_frameBuf:
	for ( i = 0; i < FRAME_BUFFERS_NR; i++)
		free(buffer->frame_buffer[i]);
error_alloc_dataBuf:
	for ( i = 0; i < FRAME_BUFFERS_NR; i++)
		free(buffer->data_buffer[i]);
error_alloc_isoBuf:
	for ( i = 0; i < ISO_BUFFERS_NR; i++)
		free(buffer->iso_buffer[i]);

	free(buffer);
	return NULL;
}

void free_buffers(struct buffering * buffer)
{
	pthread_mutex_destroy(&buffer->mutex);
	int i;
	for ( i = 0; i < ISO_BUFFERS_NR; i++)
		free(buffer->iso_buffer[i]);
	for ( i = 0; i < FRAME_BUFFERS_NR; i++)
		free(buffer->data_buffer[i]);
	for ( i = 0; i < FRAME_BUFFERS_NR; i++)
		free(buffer->frame_buffer[i]);

	free(buffer);
}

void init_data_buffer(struct buffering * buffer)
{
	int i;

	for ( i = 0; i < FRAME_BUFFERS_NR; i++)
	{
		buffer->frame_buffer[i]->filled = 0;
		buffer->frame_buffer[i]->data = buffer->data_buffer[i];
		if ( i > 0 )
			buffer->frame_buffer[i]->next = buffer->frame_buffer[i-1];
		else
			buffer->frame_buffer[0]->next = NULL;
	}

	buffer->empty_frames = buffer->frame_buffer[FRAME_BUFFERS_NR-1];
	buffer->empty_frames_tail = buffer->frame_buffer[0];

	buffer->fill_frame = buffer->empty_frames;
	buffer->empty_frames = buffer->empty_frames->next;

	buffer->read_frame = NULL;

	buffer->full_frames = NULL;
	buffer->full_frames_tail = NULL;
}

void next_fill_data_buffer(struct buffering * buffer)
{
	pthread_mutex_lock(&buffer->mutex);
	if ( buffer->fill_frame != NULL )
	{
		/* append to 'full' list */
		if ( buffer->full_frames == NULL)
		{
			buffer->full_frames = buffer->fill_frame;
			buffer->full_frames_tail = buffer->full_frames;
		}
		else
		{
			buffer->full_frames_tail->next = buffer->fill_frame;
			buffer->full_frames_tail = buffer->fill_frame;
		}
	}
	if ( buffer->empty_frames != NULL )
	{
		/* We have empty frames available. That's easy */
		buffer->fill_frame = buffer->empty_frames;
		buffer->empty_frames = buffer->empty_frames->next;
	}
	else
	{
		/* sanity check */
		if ( buffer->full_frames == NULL )
		{
			printf("Error, neither empty nor full frames available!\n");
		}
		/* Hmm. Take it from the full list */
		buffer->fill_frame = buffer->full_frames;
		buffer->full_frames = buffer->full_frames->next;
	}
	pthread_mutex_unlock(&buffer->mutex);
}

int get_read_frame(struct buffering * buffer)
{
	if ( buffer->read_frame != NULL )
		return -BUF_READ_SET;
	if ( buffer->full_frames == NULL )
		return -BUF_FULL_NULL;
	if ( buffer->full_frames == buffer->full_frames_tail ) //check if this frame has been read already?
			return -BUF_FULL_NULL;

	pthread_mutex_lock(&buffer->mutex);
	buffer->read_frame = buffer->full_frames;
	buffer->full_frames = buffer->full_frames->next;
	buffer->read_frame->next = NULL;
	pthread_mutex_unlock(&buffer->mutex);

	return 0;
}

void release_read_frame(struct buffering * buffer)
{
	pthread_mutex_lock(&buffer->mutex);
	if ( buffer->empty_frames == NULL )
	{
		buffer->empty_frames = buffer->read_frame;
		buffer->empty_frames_tail = buffer->read_frame;
	}
	else
	{
		buffer->empty_frames_tail->next = buffer->read_frame;
		buffer->empty_frames_tail = buffer->read_frame;
	}
	buffer->read_frame = NULL;
	pthread_mutex_unlock(&buffer->mutex);
}
