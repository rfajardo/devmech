/*
 * images.h
 *
 *  Created on: Mar 15, 2010
 *      Author: rfajardo
 */

#ifndef IMAGES_H_
#define IMAGES_H_


#define MAX_IMAGES 				10

#define IMAGE_BUFFERS			2

#define X_SIZE					640
#define Y_SIZE					480
#define IMAGE_SIZE				( X_SIZE * Y_SIZE * 3 / 2 )


struct pwc_dec23_private;
struct frameBuf;

struct pwc_coord
{
	int x,y;
};

struct images
{
	   struct pwc_coord view_min, view_max;	/* minimum and maximum viewable sizes */
	   struct pwc_coord abs_max;            /* maximum supported size with compression */

	   struct pwc_coord image, view;	/* image and viewport size */
	   struct pwc_coord offset;		/* offset within the viewport */

	   void * img_buf;/* ...several images... */
	   int len_per_image;			/* length per image */
	   int vbandlength;

	   int palette;

	   struct pwc_dec23_private * pdec;
};

struct images * alloc_images();
void free_images(struct images * p_img);

int init_images(struct images * p_img);
void pwc_decompress(struct images * p_img, struct frameBuf * fbuf);


#endif /* IMAGES_H_ */
