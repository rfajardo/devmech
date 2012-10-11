/*
 * dev_class.c
 *
 *  Created on: Mar 26, 2010
 *      Author: rfajardo
 */

#include "dev_class.h"

#include "pwc_user.h"
#include "pwc_debug.h"

#include "images.h"

#include <stdlib.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/videodev.h>

#include <sys/mman.h>

#include <string.h>


#define VIDIOCSINVALID    _IO('v',BASE_VIDIOCPRIVATE+1)

struct devClass * alloc_devClass()
{
	struct devClass * dev_class = malloc(sizeof(struct devClass));
	return dev_class;
}

void free_devClass(struct devClass * dev_class)
{
	free(dev_class);
}


void * init_devClass(struct devClass * dev_class, const unsigned char * device_name,
		struct images * imgs)
{
	dev_class->pipe = open(device_name, O_RDWR);
	if ( dev_class->pipe == -1 )
		return NULL;

#ifdef V4L_COMPATIBLE
	dev_class->video_data = mmap(NULL, IMAGE_SIZE, PROT_WRITE, MAP_SHARED, dev_class->pipe, 0);

	if ( dev_class->video_data == MAP_FAILED )
		return NULL;

	strcpy(dev_class->capabilities.name, "Philips webcam");
	dev_class->capabilities.type = VID_TYPE_CAPTURE;
	dev_class->capabilities.channels = 1;
	dev_class->capabilities.audios = 0;

	dev_class->capabilities.minwidth = imgs->view_min.x;
	dev_class->capabilities.maxwidth = imgs->view_max.x;
	dev_class->capabilities.minheight = imgs->view_min.y;
	dev_class->capabilities.maxheight= imgs->view_max.y;

	dev_class->picture.colour = 0x8000;
	dev_class->picture.hue = 0x8000;
	dev_class->picture.brightness = 0x8000;
	dev_class->picture.contrast = 0x8000;
	dev_class->picture.whiteness = 0x8000;
	dev_class->picture.depth = 0x8000;
	dev_class->picture.palette = imgs->palette;

	dev_class->window.x = 0;
	dev_class->window.y = 0;
	dev_class->window.chromakey = 0;
	dev_class->window.flags = 0;
	dev_class->window.clipcount = 0;
	dev_class->window.width = imgs->view.x;
	dev_class->window.height = imgs->view.y;

	dev_class->allow_capture = 0;
#else
	dev_class->video_data = malloc(IMAGE_SIZE);

	if ( ioctl(dev_class->pipe, VIDIOCGCAP, &dev_class->capabilities) == -1)
		return -1;

	if ( ioctl(dev_class->pipe, VIDIOCGPICT, &dev_class->picture) == -1 )
		return -1;

	if ( ioctl(dev_class->pipe, VIDIOCGWIN, &dev_class->window) == -1 )
		return -1;

	dev_class->picture.palette = imgs->palette;

	dev_class->window.width = imgs->view.x;
	dev_class->window.height = imgs->view.y;

	if ( ioctl(dev_class->pipe, VIDIOCSPICT, &dev_class->picture) == -1 )
		return -1;

	if ( ioctl(dev_class->pipe, VIDIOCSWIN, &dev_class->window) == -1 )
		return -1;

	dev_class->allow_capture = 1;
#endif

	return dev_class->video_data;
}

int close_devClass(struct devClass * dev_class)
{
#ifdef V4L_COMPATIBLE
	munmap(dev_class->video_data, IMAGE_SIZE);
#endif
	return close(dev_class->pipe);
}

int instream_devClass(struct devClass * dev_class, unsigned int len)
{
	int written_len;
	written_len = write(dev_class->pipe, dev_class->video_data, len);

	if ( written_len != len )
		return written_len;

	return len;
}

void devClass_ioctl(struct devClass * dev_class, int * capture)
{
	char buf[1024];
	int len;
	len = read(dev_class->pipe, buf, 1024);
	if ( len <= 0 )
		INFO("vloopback error\n");

	unsigned long int * ioctl_cmd = (unsigned long int *)buf;

	char * ioctl_data = &buf[sizeof(*ioctl_cmd)];

	switch ( *ioctl_cmd )
	{
	case 0:
		INFO("Interface released from the other side\n");
		break;
	case VIDIOCGCAP:
		INFO("Processing VIDIOCGCAP\n");
		ioctl(dev_class->pipe, VIDIOCGCAP, &dev_class->capabilities);
		break;
	case VIDIOCGCHAN:
		INFO("Processing VIDIOCGCHAN\n");
		;struct video_channel v;	//workaround for a gcc bug
		v.channel = 0;
		v.flags = 0;
		v.tuners = 0;
		v.norm = 0;
		v.type = VIDEO_TYPE_CAMERA;
		strcpy(v.name, "Composite1");
		ioctl(dev_class->pipe, VIDIOCGCHAN, &v);
		break;
	case VIDIOCSCHAN:
		INFO("Processing VIDIOCSCHAN\n");
		;struct video_channel * vs = (struct video_channel *)ioctl_data;		//workaround for a gcc bug
		if ( vs->channel != 0 )
		{
			printf("VIDIOCSCHAN: Invalid channel %d, we only accept channel 0->return error\n", vs->channel);
			ioctl(dev_class->pipe, VIDIOCSINVALID, NULL);
			break;
		}
		ioctl(dev_class->pipe, VIDIOCSCHAN, ioctl_data);		//report setting was ok
		break;

	case VIDIOCGFBUF:
		;struct video_buffer vb;
		INFO("Processing VIDIOCGFBUF\n");
		memset(&vb, 0, sizeof(vb));
		vb.base = NULL;
		ioctl(dev_class->pipe, VIDIOCGFBUF, &vb);
		break;

	case VIDIOCGPICT:
		INFO("Processing VIDIOCGPICT\n");
		ioctl(dev_class->pipe, VIDIOCGPICT, &dev_class->picture);
		break;

	case VIDIOCSPICT:
		;struct video_picture * vpic = (struct video_picture *)ioctl_data;
		INFO("Processing VIDIOCSPICT\n");
		if ( vpic->palette != dev_class->picture.palette )
		{
			INFO("Trying to set palette to: %d, only accepts 15->return error\n", vpic->palette);
			ioctl(dev_class->pipe, VIDIOCSINVALID, NULL);
			break;
		}
		ioctl(dev_class->pipe, VIDIOCSPICT, vpic); 	//return OK
		break;

	case VIDIOCGWIN:
		INFO("Processing VIDIOCGWIN\n");
		ioctl(dev_class->pipe, VIDIOCGWIN, &dev_class->window);
		break;
	case VIDIOCMCAPTURE:
		INFO_TRANSFER("Processing VIDIOCMCAPTURE\n");
		;struct video_mmap * vm = (struct video_mmap *)ioctl_data;
		if ( vm->format != dev_class->picture.palette )
		{
			INFO("Trying to capture in wrong format %d:->return error\n", vm->format);
			ioctl(dev_class->pipe, VIDIOCSINVALID, NULL);
			break;
		}
		*capture = 1;
		dev_class->allow_capture = 1;
		ioctl(dev_class->pipe, VIDIOCMCAPTURE, ioctl_data);
		break;
	case VIDIOCSYNC:
		INFO_TRANSFER("Processing VIDIOCSYNC\n");
		while (*capture);
		dev_class->allow_capture = 0;
		ioctl(dev_class->pipe, VIDIOCSYNC, ioctl_data);
		INFO_TRANSFER("Processed VIDIOCSYNC\n");
		break;
	case VIDIOCGMBUF:
		;struct video_mbuf mbuf;
		INFO("Processing VIDIOCGMBUF\n");
		mbuf.size = IMAGE_SIZE;
		mbuf.frames = 1;
		mbuf.offsets[0] = 0;
		ioctl(dev_class->pipe, VIDIOCGMBUF, &mbuf);
		break;
	default:
		INFO("Invalid IOCTL: %x\n", *ioctl_cmd);
		ioctl(dev_class->pipe, VIDIOCSINVALID, NULL);
		break;
	}

}
