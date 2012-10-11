/*
 * pwc_user.c
 *
 *  Created on: Mar 12, 2010
 *      Author: rfajardo
 */

#include "pwc_user.h"

#include "dev_class.h"

#include "buffers.h"
#include "images.h"

#include "acquire_data_if.h"
#include "img_capture_if.h"

#include "devregs.h"

#include <stdio.h>		//printf, perror
#include <stdlib.h>		//exit

#include <signal.h>

#include <pthread.h>
#include <unistd.h>

#include <pwcmech/pwcmech.h>
#include <pwcmech/pwcmech_init.h>

//this shouldn't be necessary
#include <usbif/usbif.h>
#include <pwcmech/usbcom.h>


struct buffering * frames;
//
// Signaling to abort operation
//
int op_abort = 0;
int thread_abort = 0;
int ioctl_call = 0;
void sig_handler(int sig);
void ioctl_callback(int sig);

//
// Threads for device acquisition and data delivery
//
#define NUM_THREADS 		2

int main(int argc, char *argv[])
{
	int i, iRet = 0;
	struct pwcmech * pwcmech;

	//
	// Signal Handling
	//
	sigset_t block_no_signals;
	sigemptyset(&block_no_signals);


	struct sigaction pwc_sig_handler;
	struct sigaction standard_sig_handler;
	struct sigaction video_io_sig_handler;

	pwc_sig_handler.sa_flags = 0;
	pwc_sig_handler.sa_handler = &sig_handler;

	sigaction(SIGINT, &pwc_sig_handler, &standard_sig_handler);
	sigaction(SIGTERM, &pwc_sig_handler, &standard_sig_handler);

	video_io_sig_handler.sa_flags = 0;
	video_io_sig_handler.sa_handler = &ioctl_callback;
	sigaction(SIGIO, &video_io_sig_handler, &standard_sig_handler);


	//
	// Userspace Driver Interfaces
	//
	struct devClass * video_dev;
	struct images * p_img;


	//
	// Buffering Initialization
	//
	printf("Allocating buffers...");
	frames = alloc_buffers();
	if ( frames == NULL )
	{
		printf("failed?!\n");
		goto finish;
	}
	init_data_buffer(frames);
	printf("done!\n");


	//
	// Device Initialization
	//
	printf("Create camera device...");
	pwcmech = pwc_devmech_start();
	if ( pwcmech_register_driver(pwcmech) )
	{
		printf("failed?!\n");
		goto release_buffers;
	}
	printf("done!\n");

	printf("Initialize camera device...");
	if ( pwcmech_register_handler(pwcmech) )
	{
		perror("Failed opening camera device");
		printf("failed?!\n");
		goto stop_usb;
	}
	printf("done!\n");

	printf("Set camera's video mode...");
	//if_claim should work seamslessly somehow
	iRet = usb_if_claim(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.cameraStreamConf);
	if ( iRet )
	{
		perror("Failed setting camera's video mode");
		printf("failed?!\n");
		goto close_camera;
	}
	iRet = setVideoMode(pwcmech, 9);	//still old way, pre-setting instead of from V4L
	if ( iRet )
	{
		perror("Failed setting camera's video mode");
		printf("failed?!\n");
		goto close_camera;
	}
	set_ctrl0_timeout(pwcmech->com, 1000);
	setLeds(pwcmech, 0,0);	//don't bother with the light
	iRet = sendVideoCommand(pwcmech, video_mode_string, VIDEO_MODE_STRING_LEN);
	if ( iRet )
	{
		perror("Failed setting camera's video mode");
		printf("failed?!\n");
		goto close_camera;
	}
	printf("done!\n");


	//
	// Decompression
	//
	printf("Initializing decompression system...");
	p_img = alloc_images();
	if ( p_img == NULL )
	{
		printf("failed?!\n");
		goto close_camera;
	}
	if ( ( iRet = init_images(p_img) ) )
	{
		printf("failed?!\n");
		goto free_p_img;
	}
	printf("done!\n");

#ifndef NO_IMG
	//
	// Open video pipe
	//
	printf("Opening the video pipe (vl4 device emulation)...");
	video_dev = alloc_devClass();
	if ( video_dev == NULL )
	{
		printf("failed?!\n");
		goto free_p_img;
	}
	p_img->img_buf = init_devClass(video_dev, argv[1], p_img);
	if ( p_img->img_buf == NULL )
	{
		perror("Initializing device class failed");
		printf("failed?!\n");
		printf("Did you input the device path, \"/dev/video0\"?\"\n\n\n");
		goto free_video_dev;
	}
	printf("done!\n");
#endif

	//
	//	ISO transfers
	//
	printf("Create USB isochronous transfers...");
	for (i = 0; i < ISO_BUFFERS_NR; i++)
	{
		iRet = assignVideoBuffer(pwcmech, frames->iso_buffer[i], ISO_BUFFER_SIZE);
		if (iRet)
		{
			perror("Failed creating image transfer");
			printf("failed?!\n");
			goto unregister_buffers;
		}
	}
	printf("done!\n");


	//
	// Threads
	//
	printf("Set-up threads...");
	int active_threads = 0;
	pthread_t threads[NUM_THREADS];

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	struct acqArg * acq_arg;
	acq_arg = malloc(sizeof(struct acqArg));
	if ( acq_arg == NULL )
	{
		printf("failed?!\n");
		goto close_threads;
	}
	acq_arg->abort = &thread_abort;
	acq_arg->pwcmech = pwcmech;

	struct imgArg * img_arg;
	img_arg = malloc(sizeof(struct imgArg));
	if ( img_arg == NULL )
	{
		printf("failed?!\n");
		goto close_threads;
	}
	img_arg->frames = frames;
	img_arg->p_img = p_img;
	img_arg->video_pipe = video_dev;
	img_arg->capture = 0;
	img_arg->abort = &thread_abort;
	printf("done!\n");


	//
	//	MAIN
	//
	printf("Registering callback function for Isochronous transfer...");
	setPower(pwcmech, 0);		//enable camera (our state machine discovered that this was missing)
	registerVideoCallback(pwcmech, acquire_frame, pwcmech);
	if ( acknowledgeVideoCallback(pwcmech) )
		goto close_threads;
	printf("done!\n");

#ifndef NO_COM
	printf("Creating the data acquisition thread...");
	if ( ( iRet = pthread_create(&threads[active_threads], &attr, process_usb, (void *)acq_arg) ) )
	{
		printf("failed?!\n");
		goto stop_camera;
	}
	active_threads++;
	printf("done!\n");
#endif

#ifndef NO_IMG
	printf("Creating the image fill thread...");
	if ( ( iRet = pthread_create(&threads[active_threads], &attr, fillImageData, (void *)img_arg) ) )
	{
		printf("failed?!\n");
		goto stop_camera;
	}
	active_threads++;
	printf("done!\n");
#endif

	while (!op_abort)
	{
//		sigsuspend(&block_no_signals);
		if ( ioctl_call )
		{
			devClass_ioctl(video_dev, &img_arg->capture);
			ioctl_call = 0;
		}
	}


	//
	//	~MAIN
	//

stop_camera:
	printf("Releasing video callback...");
	releaseVideoCallback(pwcmech);
	printf("done!\n");
	printf("Unregistering buffers...");
	unassignVideoBuffers(pwcmech);
	printf("done!\n");

close_threads:
	printf("Closing all threads...");
	thread_abort = 1;
	for ( i = 0; i < active_threads; i++)
		pthread_join(threads[i], NULL);

	free(acq_arg);
	free(img_arg);

	pthread_attr_destroy(&attr);
	printf("done!\n");

unregister_buffers:
	printf("Unregistering buffers if not done before...");
	unassignVideoBuffers(pwcmech);
	printf("done!\n");

free_video_dev:
#ifndef NO_IMG
	printf("Closing video pipe...");
	close_devClass(video_dev);
	free_devClass(video_dev);
	printf("done!\n");
#endif

free_p_img:
	printf("Releasing decompression system...");
	free_images(p_img);
	printf("done!\n");
	usb_release_interface(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.cameraStreamConf);

close_camera:
	printf("Close camera device...");
	pwcmech_deregister_handler(pwcmech);
	printf("done!\n");

stop_usb:
	printf("Stopping USB interface...");
	pwcmech_deregister_driver(pwcmech);
	printf("done!\n");

release_buffers:
	pwc_devmech_stop(pwcmech);
	//release buffers, they are linked to the camera device
	//so only release them after releasing the camera
	printf("Releasing buffers...");
	free_buffers(frames);
	printf("done!\n");

finish:
	return 0;
}

void sig_handler(int sig)
{
	if ( sig == SIGINT || sig == SIGTERM )
	{
		printf("Aborting operation...\n");
		op_abort = 1;
	}
}

void ioctl_callback(int sig)
{
	if ( sig == SIGIO )
		ioctl_call = 1;
}

