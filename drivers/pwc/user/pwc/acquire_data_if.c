/*
 * acquire_data_if.c
 *
 *  Created on: Apr 15, 2010
 *      Author: rfajardo
 */

#include "acquire_data_if.h"
#include "pwc_user.h"
#include "buffers.h"

#include "frame_constructor_if.h"

#include <stdio.h>		//printf, perror
#include <sys/select.h>

#include <libusb.h>

#include <pwcmech/pwcmech_init.h>
#include <pwcmech/pwcmech.h>


extern struct buffering * frames;

void process_usb(void * acq_arg)
{
	struct acqArg * acq_data = (struct acqArg *)acq_arg;

	const struct libusb_pollfd ** usb_fds;
	int i, nfds;

	fd_set input;
	struct timeval timeout;
	struct timeval zero;

	int sRet;

	zero.tv_sec = 0;
	zero.tv_usec = 0;

	FD_ZERO(&input);
	usb_fds = libusb_get_pollfds(pwc_get_usb_device(acq_data->pwcmech)->usb_context);
	for (i = 0, nfds = 0; usb_fds[i] != NULL; i++)
	{
		FD_SET(usb_fds[i]->fd, &input);
		if ( nfds < usb_fds[i]->fd )
			nfds = usb_fds[i]->fd;
	}

	while (! *acq_data->abort)
	{
		if ( libusb_get_next_timeout(pwc_get_usb_device(acq_data->pwcmech)->usb_context, &timeout) != 1 )	//always returns 0 for Linux > 2.6.27
		{
			timeout.tv_sec = 0;
			timeout.tv_usec = 5000;		//half of the time for one isochronous transfer (10 ms)
		}
		sRet = select(nfds, &input, NULL, NULL, &timeout);
		if ( sRet >= 0 )			//input available or timeout
			libusb_handle_events_timeout(pwc_get_usb_device(acq_data->pwcmech)->usb_context, &zero);
	}

	free(usb_fds);
}

enum cb_ret acquire_frame(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets)
{
	static size_t last_packet_len = 0;

	int i;
	bool end_of_frame = false;
	size_t length_to_end_of_frame;
	size_t internal_len = 0;

	struct pwcmech * pwcmech = context;


#ifdef THREAD_INFO
	printf(">>acquireISOData\n");
#endif


	for (i = 0; i < nr_of_packets; i++)
	{
		internal_len += p_packet_len[i];
		if ( p_packet_len[i] < last_packet_len )
		{
			length_to_end_of_frame = internal_len;
			end_of_frame = true;
		}
		last_packet_len = p_packet_len[i];
	}


	if (end_of_frame)
	{
		finishFrame(frames, buf, length_to_end_of_frame);
		fillFrame(frames->fill_frame, buf+length_to_end_of_frame, len-length_to_end_of_frame);
	}
	else
		fillFrame(frames->fill_frame, buf, len);


#ifdef THREAD_INFO
	printf("<<acquireISOData\n");
#endif

	return REACTIVATE;
}
