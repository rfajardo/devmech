/*
 * img_capture_if.h
 *
 *  Created on: Apr 15, 2010
 *      Author: rfajardo
 */

#ifndef IMG_CAPTURE_IF_H_
#define IMG_CAPTURE_IF_H_


struct buffering;
struct images;
struct devClass;


struct imgArg
{
	struct buffering * frames;
	struct images * p_img;
	struct devClass * video_pipe;
	int capture;
	int * abort;
};

void * fillImageData(void * img_arg);


#endif /* IMG_CAPTURE_IF_H_ */
