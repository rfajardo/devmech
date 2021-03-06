/* (C) 1999-2003 Nemosoft Unv.
   (C) 2004-2006 Luc Saillard (luc@saillard.org)

   NOTE: this version of pwc is an unofficial (modified) release of pwc & pcwx
   driver and thus may have bugs that are not present in the original version.
   Please send bug reports and support requests to <luc@saillard.org>.
   The decompression routines have been implemented by reverse-engineering the
   Nemosoft binary pwcx module. Caveat emptor.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef PWC_H
#define PWC_H

#include <linux/module.h>
#include <linux/usb.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/errno.h>
#include <linux/videodev2.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/videobuf2-vmalloc.h>
#ifdef CONFIG_USB_PWC_INPUT_EVDEV
#include <linux/input.h>
#endif

#include <media/pwc-ioctl.h>

/* Version block */
#define PWC_VERSION	"10.0.15"
#define PWC_NAME 	"pwc"
#define PFX		PWC_NAME ": "


/* Trace certain actions in the driver */
#define PWC_DEBUG_LEVEL_MODULE	(1<<0)
#define PWC_DEBUG_LEVEL_PROBE	(1<<1)
#define PWC_DEBUG_LEVEL_OPEN	(1<<2)
#define PWC_DEBUG_LEVEL_READ	(1<<3)
#define PWC_DEBUG_LEVEL_MEMORY	(1<<4)
#define PWC_DEBUG_LEVEL_FLOW	(1<<5)
#define PWC_DEBUG_LEVEL_SIZE	(1<<6)
#define PWC_DEBUG_LEVEL_IOCTL	(1<<7)
#define PWC_DEBUG_LEVEL_TRACE	(1<<8)

#define PWC_DEBUG_MODULE(fmt, args...) PWC_DEBUG(MODULE, fmt, ##args)
#define PWC_DEBUG_PROBE(fmt, args...) PWC_DEBUG(PROBE, fmt, ##args)
#define PWC_DEBUG_OPEN(fmt, args...) PWC_DEBUG(OPEN, fmt, ##args)
#define PWC_DEBUG_READ(fmt, args...) PWC_DEBUG(READ, fmt, ##args)
#define PWC_DEBUG_MEMORY(fmt, args...) PWC_DEBUG(MEMORY, fmt, ##args)
#define PWC_DEBUG_FLOW(fmt, args...) PWC_DEBUG(FLOW, fmt, ##args)
#define PWC_DEBUG_SIZE(fmt, args...) PWC_DEBUG(SIZE, fmt, ##args)
#define PWC_DEBUG_IOCTL(fmt, args...) PWC_DEBUG(IOCTL, fmt, ##args)
#define PWC_DEBUG_TRACE(fmt, args...) PWC_DEBUG(TRACE, fmt, ##args)


#ifdef CONFIG_USB_PWC_DEBUG

#define PWC_DEBUG_LEVEL	(PWC_DEBUG_LEVEL_MODULE)

#define PWC_DEBUG(level, fmt, args...) do {\
	if ((PWC_DEBUG_LEVEL_ ##level) & pwc_trace) \
		printk(KERN_DEBUG PFX fmt, ##args); \
	} while (0)

#define PWC_ERROR(fmt, args...) printk(KERN_ERR PFX fmt, ##args)
#define PWC_WARNING(fmt, args...) printk(KERN_WARNING PFX fmt, ##args)
#define PWC_INFO(fmt, args...) printk(KERN_INFO PFX fmt, ##args)
#define PWC_TRACE(fmt, args...) PWC_DEBUG(TRACE, fmt, ##args)

#else /* if ! CONFIG_USB_PWC_DEBUG */

#define PWC_ERROR(fmt, args...) printk(KERN_ERR PFX fmt, ##args)
#define PWC_WARNING(fmt, args...) printk(KERN_WARNING PFX fmt, ##args)
#define PWC_INFO(fmt, args...) printk(KERN_INFO PFX fmt, ##args)
#define PWC_TRACE(fmt, args...) do { } while(0)
#define PWC_DEBUG(level, fmt, args...) do { } while(0)

#define pwc_trace 0

#endif

/* Defines for ToUCam cameras */
#define TOUCAM_HEADER_SIZE		8
#define TOUCAM_TRAILER_SIZE		4

#define FEATURE_MOTOR_PANTILT		0x0001
#define FEATURE_CODEC1			0x0002
#define FEATURE_CODEC2			0x0004

/* Ignore errors in the first N frames, to allow for startup delays */
#define FRAME_LOWMARK 5

/* Size and number of buffers for the ISO pipe. */
#define MAX_ISO_BUFS		3
#define ISO_FRAMES_PER_DESC	10
#define ISO_MAX_FRAME_SIZE	960
#define ISO_BUFFER_SIZE 	(ISO_FRAMES_PER_DESC * ISO_MAX_FRAME_SIZE)

/* Maximum size after decompression is 640x480 YUV data, 1.5 * 640 * 480 */
#define PWC_FRAME_SIZE 		(460800 + TOUCAM_HEADER_SIZE + TOUCAM_TRAILER_SIZE)

/* Absolute minimum and maximum number of buffers available for mmap() */
#define MIN_FRAMES		2
#define MAX_FRAMES		16

/* Some macros to quickly find the type of a webcam */
#define DEVICE_USE_CODEC1(x) ((x)<675)
#define DEVICE_USE_CODEC2(x) ((x)>=675 && (x)<700)
#define DEVICE_USE_CODEC3(x) ((x)>=700)
#define DEVICE_USE_CODEC23(x) ((x)>=675)

/* from pwc-dec.h */
#define PWCX_FLAG_PLANAR        0x0001



/* Formatters for the motorized pan & tilt [GS]ET_MPT_CTL */
#define PT_RELATIVE_CONTROL_FORMATTER		0x01
#define PT_RESET_CONTROL_FORMATTER		0x02
#define PT_STATUS_FORMATTER			0x03

/* intermediate buffers with raw data from the USB cam */
struct pwc_frame_buf
{
	struct vb2_buffer vb;	/* common v4l buffer stuff -- must be first */
	struct list_head list;
	void *data;
	int filled;		/* number of bytes filled */
};

/* The following structures were based on cpia.h. Why reinvent the wheel? :-) */
struct pwc_iso_buf
{
	void *data;
	int  length;
	int  read;
	struct urb *urb;
};

struct pwc_device
{
	struct pwcmech * pwcmech;
	struct video_device vdev;
	struct mutex modlock;

	/* Pointer to our usb_device, may be NULL after unplug */
	struct usb_device *udev;
	/* Protects the setting of udev to NULL by our disconnect handler */
	struct mutex udevlock;

	/* type of cam (645, 646, 675, 680, 690, 720, 730, 740, 750) */
	int type;
	int release;		/* release number */
	int features;		/* feature bits */
	char serial[30];	/* serial number (string) */

	/*** Video data ***/
	struct file *capt_file;	/* file doing video capture */
	int vendpoint;		/* video isoc endpoint */
	int vcinterface;	/* video control interface */
	int valternate;		/* alternate interface needed */
	int vframes, vsize;	/* frames-per-second & size (see PSZ_*) */
	int pixfmt;		/* pixelformat: V4L2_PIX_FMT_YUV420 or _PWCX */
	int vframe_count;	/* received frames */
	int vmax_packet_size;	/* USB maxpacket size */
	int vlast_packet_size;	/* for frame synchronisation */
	int visoc_errors;	/* number of contiguous ISOC errors */
	int vcompression;	/* desired compression factor */
	int vbandlength;	/* compressed band length; 0 is uncompressed */
	char vsnapshot;		/* snapshot mode */
	char vsync;		/* used by isoc handler */
	char vmirror;		/* for ToUCaM series */
	char power_save;	/* Do powersaving for this cam */

	int cmd_len;
	unsigned char cmd_buf[13];

	struct pwc_iso_buf sbuf[MAX_ISO_BUFS];

	struct urb *urbs[MAX_ISO_BUFS];
	char iso_init;

	/* videobuf2 queue and queued buffers list */
	struct vb2_queue vb_queue;
	struct list_head queued_bufs;
	struct mutex queued_bufs_lock;

	/*
	 * Frame currently being filled, this only gets touched by the
	 * isoc urb complete handler, and by stream start / stop since
	 * start / stop touch it before / after starting / killing the urbs
	 * no locking is needed around this
	 */
	struct pwc_frame_buf *fill_buf;

	int frame_header_size, frame_trailer_size;
	int frame_size;
	int frame_total_size;	/* including header & trailer */
	int drop_frames;

	void *decompress_data;	/* private data for decompression engine */

	/*
	 * We have an 'image' and a 'view', where 'image' is the fixed-size img
	 * as delivered by the camera, and 'view' is the size requested by the
	 * program. The camera image is centered in this viewport, laced with
	 * a gray or black border. view_min <= image <= view <= view_max;
	 */
	int image_mask;				/* supported sizes */
	struct pwc_coord view_min, view_max;	/* minimum and maximum view */
	struct pwc_coord abs_max;		/* maximum supported size */
	struct pwc_coord image, view;		/* image and viewport size */
	struct pwc_coord offset;		/* offset of the viewport */

	/*** motorized pan/tilt feature */
	struct pwc_mpt_range angle_range;
	int pan_angle;			/* in degrees * 100 */
	int tilt_angle;			/* absolute angle; 0,0 is home */

	/*
	 * Set to 1 when the user push the button, reset to 0
	 * when this value is read from sysfs.
	 */
	int snapshot_button_status;
#ifdef CONFIG_USB_PWC_INPUT_EVDEV
	struct input_dev *button_dev;	/* webcam snapshot button input */
	char button_phys[64];
#endif

	/* controls */
	struct v4l2_ctrl_handler	ctrl_handler;
	u16				saturation_fmt;
	struct v4l2_ctrl		*brightness;
	struct v4l2_ctrl		*contrast;
	struct v4l2_ctrl		*saturation;
	struct v4l2_ctrl		*gamma;
	struct {
		/* awb / red-blue balance cluster */
		struct v4l2_ctrl	*auto_white_balance;
		struct v4l2_ctrl	*red_balance;
		struct v4l2_ctrl	*blue_balance;
		/* usb ctrl transfers are slow, so we cache things */
		int			color_bal_valid;
		unsigned long		last_color_bal_update; /* In jiffies */
		s32			last_red_balance;
		s32			last_blue_balance;
	};
	struct {
		/* autogain / gain cluster */
		struct v4l2_ctrl	*autogain;
		struct v4l2_ctrl	*gain;
		int			gain_valid;
		unsigned long		last_gain_update; /* In jiffies */
		s32			last_gain;
	};
	struct {
		/* exposure_auto / exposure cluster */
		struct v4l2_ctrl	*exposure_auto;
		struct v4l2_ctrl	*exposure;
		int			exposure_valid;
		unsigned long		last_exposure_update; /* In jiffies */
		s32			last_exposure;
	};
	struct v4l2_ctrl		*colorfx;
	struct {
		/* autocontour/contour cluster */
		struct v4l2_ctrl	*autocontour;
		struct v4l2_ctrl	*contour;
	};
	struct v4l2_ctrl		*backlight;
	struct v4l2_ctrl		*flicker;
	struct v4l2_ctrl		*noise_reduction;
	struct v4l2_ctrl		*save_user;
	struct v4l2_ctrl		*restore_user;
	struct v4l2_ctrl		*restore_factory;
	struct {
		/* motor control cluster */
		struct v4l2_ctrl	*motor_pan;
		struct v4l2_ctrl	*motor_tilt;
		struct v4l2_ctrl	*motor_pan_reset;
		struct v4l2_ctrl	*motor_tilt_reset;
	};
	/* CODEC3 models have both gain and exposure controlled by autogain */
	struct v4l2_ctrl		*autogain_expo_cluster[3];
};

/* Global variables */
#ifdef CONFIG_USB_PWC_DEBUG
extern int pwc_trace;
#endif

/** Functions in pwc-misc.c */
/* sizes in pixels */
extern const struct pwc_coord pwc_image_sizes[PSZ_MAX];

int pwc_decode_size(struct pwc_device *pdev, int width, int height);
void pwc_construct(struct pwc_device *pdev);

/** Functions in pwc-ctrl.c */
/* Request a certain video mode. Returns < 0 if not possible */
extern int pwc_set_video_mode(struct pwc_device *pdev, int width, int height, int frames, int compression, int snapshot);
extern unsigned int pwc_get_fps(struct pwc_device *pdev, unsigned int index, unsigned int size);

int pwc_init_controls(struct pwc_device *pdev);

/* Private ioctl()s; see pwc-ioctl.h */
extern long pwc_ioctl(struct pwc_device *pdev, unsigned int cmd, void *arg);

extern const struct v4l2_ioctl_ops pwc_ioctl_ops;

/** pwc-uncompress.c */
/* Expand frame to image, possibly including decompression. Uses read_frame and fill_image */
int pwc_decompress(struct pwc_device *pdev, struct pwc_frame_buf *fbuf);

/*pwcmech wrapped functions*/
extern int pwc_mpt_reset(struct pwc_device *pdev, int flags);
extern int pwc_mpt_set_angle(struct pwc_device *pdev, int pan, int tilt);
extern int pwc_set_leds(struct pwc_device *pdev, int on_value, int off_value);
extern int pwc_get_cmos_sensor(struct pwc_device *pdev, int *sensor);
extern void pwc_camera_power(struct pwc_device *pdev, int power);

#endif
