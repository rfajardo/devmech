/*
 * dev_class.h
 *
 *  Created on: Mar 26, 2010
 *      Author: rfajardo
 */

#ifndef DEV_CLASS_H_
#define DEV_CLASS_H_

#include <linux/videodev.h>

struct images;

struct devClass
{
	int pipe;
	void * video_data;
	char command[280];

	struct video_capability capabilities;
	struct video_window window;
	struct video_picture picture;

	int allow_capture;
};


struct devClass * alloc_devClass();
void free_devClass(struct devClass * dev_class);

void * init_devClass(struct devClass * dev_class, const unsigned char * device_name,
		struct images * imgs);
int close_devClass(struct devClass * dev_class);

int instream_devClass(struct devClass * dev_class, unsigned int len);

void devClass_ioctl(struct devClass * dev_class, int * capture);


#endif /* DEV_CLASS_H_ */
