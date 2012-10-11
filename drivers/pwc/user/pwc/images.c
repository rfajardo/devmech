/*
 * decompressing.c
 *
 *  Created on: Mar 24, 2010
 *      Author: rfajardo
 */

#include "images.h"
#include "buffers.h"
#include "pwc-dec23.h"

#include "devregs.h"		//video_mode_string

#include <stdlib.h>	//NULL pointer, malloc

#include <linux/videodev.h>	//palette


struct images * alloc_images()
{
	struct images * p_img;
	p_img = malloc(sizeof(struct images));
	if ( p_img == NULL )
		return NULL;

	p_img->pdec = pwc_dec23_alloc();
	if ( p_img->pdec == NULL )
		goto error_buffer_alloc;

	return p_img;

error_buffer_alloc:
	free(p_img);
	return NULL;
}


void free_images(struct images * p_img)
{
	pwc_dec32_free(p_img->pdec);
	free(p_img);
}


int init_images(struct images * p_img)
{
	if ( p_img == NULL )
		return -1;

	int iRet;

	p_img->view_min.x = 160;
	p_img->view_min.y = 120;

	p_img->view_max.x = 640;
	p_img->view_max.y = 480;

	p_img->abs_max.x = 640;
	p_img->abs_max.y = 480;

	p_img->image.x = X_SIZE;
	p_img->image.y = Y_SIZE;

	p_img->view.x = X_SIZE;
	p_img->view.y = Y_SIZE;

	p_img->offset.x = (p_img->view.x - p_img->image.x)/2 & 0xFFFC;
	p_img->offset.y = (p_img->view.y - p_img->image.y)/2 & 0xFFFE;

	p_img->len_per_image = IMAGE_SIZE;

	p_img->vbandlength = 788;

	p_img->palette = VIDEO_PALETTE_YUV420P;
//	p_img->palette = 7;		//7 mto similar 9
	if ( iRet = pwc_dec23_init(p_img->pdec, 740/*type(unused)*/, video_mode_string) )
		return -1;

	p_img->img_buf = NULL;

	return 0;
}

void pwc_decompress(struct images * p_img, struct frameBuf * fbuf)
{
	int n, line, col, stride;
	void *yuv, *image;
	unsigned short *src;
	unsigned short *dsty, *dstu, *dstv;

	if (fbuf == NULL)
		return;

	image = p_img->img_buf;

	yuv = fbuf->data + HEADER_SIZE;  /* Skip header */

	pwc_dec23_decompress(p_img->pdec, p_img, yuv, image, PWCX_FLAG_PLANAR);
}
