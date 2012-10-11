/* Linux driver for Philips webcam
   USB and Video4Linux interface part.
   (C) 1999-2004 Nemosoft Unv.
   (C) 2004-2006 Luc Saillard (luc@saillard.org)
   (C) 2011 Hans de Goede <hdegoede@redhat.com>

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

/*
   This code forms the interface between the USB layers and the Philips
   specific stuff. Some adanved stuff of the driver falls under an
   NDA, signed between me and Philips B.V., Eindhoven, the Netherlands, and
   is thus not distributed in source form. The binary pwcx.o module
   contains the code that falls under the NDA.

   In case you're wondering: 'pwc' stands for "Philips WebCam", but
   I really didn't want to type 'philips_web_cam' every time (I'm lazy as
   any Linux kernel hacker, but I don't like uncomprehensible abbreviations
   without explanation).

   Oh yes, convention: to disctinguish between all the various pointers to
   device-structures, I use these names for the pointer variables:
   udev: struct usb_device *
   vdev: struct video_device (member of pwc_dev)
   pdev: struct pwc_devive *
*/

/* Contributors:
   - Alvarado: adding whitebalance code
   - Alistar Moire: QuickCam 3000 Pro device/product ID
   - Tony Hoyle: Creative Labs Webcam 5 device/product ID
   - Mark Burazin: solving hang in VIDIOCSYNC when camera gets unplugged
   - Jk Fang: Sotec Afina Eye ID
   - Xavier Roche: QuickCam Pro 4000 ID
   - Jens Knudsen: QuickCam Zoom ID
   - J. Debert: QuickCam for Notebooks ID
   - Pham Thanh Nam: webcam snapshot button as an event input device
*/

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#ifdef CONFIG_USB_PWC_INPUT_EVDEV
#include <linux/usb/input.h>
#endif
#include <linux/vmalloc.h>
#include <asm/io.h>
#include <linux/kernel.h>		/* simple_strtol() */

#include "pwc.h"
#include "pwc-kiara.h"
#include "pwc-timon.h"
#include "pwc-dec23.h"
#include "pwc-dec1.h"

#include <pwcmech/pwcmech_init.h>
#include <pwcmech/pwcmech.h>
//#include <pwcmech/pwcmechwrap.h>
#include <usbif/usbif.h>

/* Function prototypes and driver templates */

/* hotplug device table support */
static const struct usb_device_id pwc_device_table [] = {
	{ USB_DEVICE(0x0471, 0x0302) }, /* Philips models */
	{ USB_DEVICE(0x0471, 0x0303) },
	{ USB_DEVICE(0x0471, 0x0304) },
	{ USB_DEVICE(0x0471, 0x0307) },
	{ USB_DEVICE(0x0471, 0x0308) },
	{ USB_DEVICE(0x0471, 0x030C) },
	{ USB_DEVICE(0x0471, 0x0310) },
	{ USB_DEVICE(0x0471, 0x0311) }, /* Philips ToUcam PRO II */
	{ USB_DEVICE(0x0471, 0x0312) },
	{ USB_DEVICE(0x0471, 0x0313) }, /* the 'new' 720K */
	{ USB_DEVICE(0x0471, 0x0329) }, /* Philips SPC 900NC PC Camera */
	{ USB_DEVICE(0x069A, 0x0001) }, /* Askey */
	{ USB_DEVICE(0x046D, 0x08B0) }, /* Logitech QuickCam Pro 3000 */
	{ USB_DEVICE(0x046D, 0x08B1) }, /* Logitech QuickCam Notebook Pro */
	{ USB_DEVICE(0x046D, 0x08B2) }, /* Logitech QuickCam Pro 4000 */
	{ USB_DEVICE(0x046D, 0x08B3) }, /* Logitech QuickCam Zoom (old model) */
	{ USB_DEVICE(0x046D, 0x08B4) }, /* Logitech QuickCam Zoom (new model) */
	{ USB_DEVICE(0x046D, 0x08B5) }, /* Logitech QuickCam Orbit/Sphere */
	{ USB_DEVICE(0x046D, 0x08B6) }, /* Cisco VT Camera */
	{ USB_DEVICE(0x046D, 0x08B7) }, /* Logitech ViewPort AV 100 */
	{ USB_DEVICE(0x046D, 0x08B8) }, /* Logitech (reserved) */
	{ USB_DEVICE(0x055D, 0x9000) }, /* Samsung MPC-C10 */
	{ USB_DEVICE(0x055D, 0x9001) }, /* Samsung MPC-C30 */
	{ USB_DEVICE(0x055D, 0x9002) },	/* Samsung SNC-35E (Ver3.0) */
	{ USB_DEVICE(0x041E, 0x400C) }, /* Creative Webcam 5 */
	{ USB_DEVICE(0x041E, 0x4011) }, /* Creative Webcam Pro Ex */
	{ USB_DEVICE(0x04CC, 0x8116) }, /* Afina Eye */
	{ USB_DEVICE(0x06BE, 0x8116) }, /* new Afina Eye */
	{ USB_DEVICE(0x0d81, 0x1910) }, /* Visionite */
	{ USB_DEVICE(0x0d81, 0x1900) },
	{ }
};
MODULE_DEVICE_TABLE(usb, pwc_device_table);

static int usb_pwc_probe(struct usb_interface *intf, const struct usb_device_id *id);
static void usb_pwc_disconnect(struct usb_interface *intf);
static void pwc_isoc_cleanup(struct pwc_device *pdev);

static struct usb_driver pwc_driver = {
	.name =			"Philips webcam",	/* name */
	.id_table =		pwc_device_table,
	.probe =		usb_pwc_probe,		/* probe() */
	.disconnect =		usb_pwc_disconnect,	/* disconnect() */
};

#define MAX_DEV_HINTS	20
#define MAX_ISOC_ERRORS	20

static int default_fps = 10;
#ifdef CONFIG_USB_PWC_DEBUG
	int pwc_trace = PWC_DEBUG_LEVEL;
#endif
static int power_save = -1;
static int led_on = 100, led_off; /* defaults to LED that is on while in use */
static int pwc_preferred_compression = 1; /* 0..3 = uncompressed..high */
static struct {
	int type;
	char serial_number[30];
	int device_node;
	struct pwc_device *pdev;
} device_hint[MAX_DEV_HINTS];

/***/

static int pwc_video_open(struct file *file);
static int pwc_video_close(struct file *file);
static ssize_t pwc_video_read(struct file *file, char __user *buf,
			  size_t count, loff_t *ppos);
static unsigned int pwc_video_poll(struct file *file, poll_table *wait);
static int  pwc_video_mmap(struct file *file, struct vm_area_struct *vma);
static void pwc_video_release(struct video_device *vfd);

static const struct v4l2_file_operations pwc_fops = {
	.owner =	THIS_MODULE,
	.open =		pwc_video_open,
	.release =     	pwc_video_close,
	.read =		pwc_video_read,
	.poll =		pwc_video_poll,
	.mmap =		pwc_video_mmap,
	.unlocked_ioctl = video_ioctl2,
};
static struct video_device pwc_template = {
	.name =		"Philips Webcam",	/* Filled in later */
	.release =	pwc_video_release,
	.fops =         &pwc_fops,
	.ioctl_ops =	&pwc_ioctl_ops,
};

/***************************************************************************/
/* Private functions */

struct pwc_frame_buf *pwc_get_next_fill_buf(struct pwc_device *pdev)
{
	struct pwc_frame_buf *buf = NULL;

	mutex_lock(&pdev->queued_bufs_lock);
	if (list_empty(&pdev->queued_bufs))
		goto leave;

	buf = list_entry(pdev->queued_bufs.next, struct pwc_frame_buf, list);
	list_del(&buf->list);
leave:
	mutex_unlock(&pdev->queued_bufs_lock);
	return buf;
}


static void fillFrame(struct pwc_device * pdev, const unsigned char * p_packet, unsigned int len)
{
	unsigned char * dst_buf;

	if ( pdev->fill_buf == NULL )
		return;

	if ( len > 0 )
	{
		if ( pdev->fill_buf->filled + len <= pdev->frame_total_size )		//avoid overflow
		{
			dst_buf = pdev->fill_buf->data + pdev->fill_buf->filled;
			memcpy(dst_buf, p_packet, len);
			pdev->fill_buf->filled += len;
		}
	}
}

static void finishFrame(struct pwc_device * pdev, const unsigned char * p_packet, unsigned int len)
{
	unsigned char * dst_buf;
	struct pwc_frame_buf * frame_buffer = pdev->fill_buf;

	if ( frame_buffer == NULL )
		return;

	if ( frame_buffer->filled + len == pdev->frame_total_size ) //frame complete, store and get new frame ptr
	{
		dst_buf = frame_buffer->data + frame_buffer->filled;
		memcpy(dst_buf, p_packet, len);
		frame_buffer->filled += len;

		frame_buffer->vb.v4l2_buf.field = V4L2_FIELD_NONE;
		frame_buffer->vb.v4l2_buf.sequence = pdev->vframe_count;
		vb2_buffer_done(&frame_buffer->vb, VB2_BUF_STATE_DONE);
		pdev->vsync = 0;

		pdev->fill_buf = pwc_get_next_fill_buf(pdev);
		if ( pdev->fill_buf )
		{
			frame_buffer = pdev->fill_buf;
			frame_buffer->filled = 0;
		}
		else
			PWC_ERROR("Failed to acquire a new frame buffer\n");
		pdev->vframe_count++;
	}
	else //frame incomplete or buffer overflow
	{
		PWC_DEBUG_FLOW("Frame incomplete, dropping\n");
		PWC_DEBUG_FLOW("Expected frame size: %d\n", pdev->frame_total_size);
		PWC_DEBUG_FLOW("Received frame: %d\n", frame_buffer->filled + len);
		frame_buffer->filled = 0;
	}
}


enum cb_ret acquire_frame(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets)
{
	int i;
	bool end_of_frame = false;
	size_t length_to_end_of_frame = 0;
	size_t internal_len = 0;
	size_t last_packet_len;
	struct pwc_device * pdev;

	pdev = context;
	last_packet_len = pdev->vlast_packet_size;

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

	pdev->vlast_packet_size = last_packet_len;

	if (pdev->fill_buf == NULL)
		pdev->fill_buf = pwc_get_next_fill_buf(pdev);

	if (end_of_frame)
	{
		finishFrame(pdev, buf, length_to_end_of_frame);
		fillFrame(pdev, buf+length_to_end_of_frame, len-length_to_end_of_frame);
		PWC_DEBUG_FLOW("Frame possibly received\n");
	}
	else
		fillFrame(pdev, buf, len);

	return REACTIVATE;
}


static int pwc_isoc_init(struct pwc_device *pdev)
{
	int i, ret;
	void *kbuf;

	if (pdev == NULL)
		return -EFAULT;
	if (pdev->iso_init)
		return 0;

	pdev->vsync = 0;
	pdev->vlast_packet_size = 0;
	pdev->fill_buf = NULL;
	pdev->vframe_count = 0;
	pdev->visoc_errors = 0;

	/* Set alternate interface */
	ret = 0;
	PWC_DEBUG_OPEN("Setting alternate interface %d\n", pdev->valternate);
	ret = setVideoMode(pdev->pwcmech, pdev->valternate);
	if (ret)
		return ret;

	for (i = 0; i < MAX_ISO_BUFS; i++) {
		if (pdev->sbuf[i].data == NULL) {
			kbuf = kzalloc(ISO_BUFFER_SIZE, GFP_KERNEL);
			if (kbuf == NULL) {
				PWC_ERROR("Failed to allocate iso buffer %d.\n", i);
				pdev->iso_init = 1;
				pwc_isoc_cleanup(pdev);
				return -ENOMEM;
			}
			PWC_DEBUG_MEMORY("Allocated iso buffer at %p.\n", kbuf);
			pdev->sbuf[i].data = kbuf;
		}
		ret = assignVideoBuffer(pdev->pwcmech, pdev->sbuf[i].data, ISO_BUFFER_SIZE);
		if (ret)
		{
			PWC_ERROR("Failed to assign video buffer %d\n", i);
			pdev->iso_init = 1;
			pwc_isoc_cleanup(pdev);
			return ret;
		}
	}

	if ( registerVideoCallback(pdev->pwcmech, acquire_frame, pdev) )
	{
		pdev->iso_init = 1;
		pwc_isoc_cleanup(pdev);
		return ret;
	}
	if ( ( ret = acknowledgeVideoCallback(pdev->pwcmech) ) )
	{
		pdev->iso_init = 1;
		pwc_isoc_cleanup(pdev);
		return ret;
	}

	/* All is done... */
	pdev->iso_init = 1;
	PWC_DEBUG_OPEN("<< pwc_isoc_init()\n");
	return 0;
}

static void pwc_isoc_cleanup(struct pwc_device *pdev)
{
	int i;
	PWC_DEBUG_OPEN(">> pwc_isoc_cleanup()\n");

	if (pdev->iso_init == 0)
		return;

	releaseVideoCallback(pdev->pwcmech);
	unassignVideoBuffers(pdev->pwcmech);

	/* Release Iso-pipe buffers */
	for (i = 0; i < MAX_ISO_BUFS; i++)
		if (pdev->sbuf[i].data != NULL) {
			PWC_DEBUG_MEMORY("Freeing ISO buffer at %p.\n", pdev->sbuf[i].data);
			kfree(pdev->sbuf[i].data);
			pdev->sbuf[i].data = NULL;
		}

	pdev->iso_init = 0;
	PWC_DEBUG_OPEN("<< pwc_isoc_cleanup()\n");
}

/*
 * Release all queued buffers, no need to take queued_bufs_lock, since all
 * iso urbs have been killed when we're called so pwc_isoc_handler won't run.
 */
static void pwc_cleanup_queued_bufs(struct pwc_device *pdev)
{
	while (!list_empty(&pdev->queued_bufs)) {
		struct pwc_frame_buf *buf;

		buf = list_entry(pdev->queued_bufs.next, struct pwc_frame_buf,
				 list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
}

/*********
 * sysfs
 *********/
static struct pwc_device *cd_to_pwc(struct device *cd)
{
	struct video_device *vdev = to_video_device(cd);
	return video_get_drvdata(vdev);
}

static ssize_t show_pan_tilt(struct device *class_dev,
			     struct device_attribute *attr, char *buf)
{
	struct pwc_device *pdev = cd_to_pwc(class_dev);
	return sprintf(buf, "%d %d\n", pdev->pan_angle, pdev->tilt_angle);
}

static ssize_t store_pan_tilt(struct device *class_dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct pwc_device *pdev = cd_to_pwc(class_dev);
	int pan, tilt;
	int ret = -EINVAL;

	if (strncmp(buf, "reset", 5) == 0)
		ret = pwc_mpt_reset(pdev, 0x3);

	else if (sscanf(buf, "%d %d", &pan, &tilt) > 0)
		ret = pwc_mpt_set_angle(pdev, pan, tilt);

	if (ret < 0)
		return ret;
	return strlen(buf);
}
static DEVICE_ATTR(pan_tilt, S_IRUGO | S_IWUSR, show_pan_tilt,
		   store_pan_tilt);

static ssize_t show_snapshot_button_status(struct device *class_dev,
					   struct device_attribute *attr, char *buf)
{
	struct pwc_device *pdev = cd_to_pwc(class_dev);
	int status = pdev->snapshot_button_status;
	pdev->snapshot_button_status = 0;
	return sprintf(buf, "%d\n", status);
}

static DEVICE_ATTR(button, S_IRUGO | S_IWUSR, show_snapshot_button_status,
		   NULL);

static int pwc_create_sysfs_files(struct pwc_device *pdev)
{
	int rc;

	rc = device_create_file(&pdev->vdev.dev, &dev_attr_button);
	if (rc)
		goto err;
	if (pdev->features & FEATURE_MOTOR_PANTILT) {
		rc = device_create_file(&pdev->vdev.dev, &dev_attr_pan_tilt);
		if (rc)
			goto err_button;
	}

	return 0;

err_button:
	device_remove_file(&pdev->vdev.dev, &dev_attr_button);
err:
	PWC_ERROR("Could not create sysfs files.\n");
	return rc;
}

static void pwc_remove_sysfs_files(struct pwc_device *pdev)
{
	if (pdev->features & FEATURE_MOTOR_PANTILT)
		device_remove_file(&pdev->vdev.dev, &dev_attr_pan_tilt);
	device_remove_file(&pdev->vdev.dev, &dev_attr_button);
}

#ifdef CONFIG_USB_PWC_DEBUG
static const char *pwc_sensor_type_to_string(unsigned int sensor_type)
{
	switch(sensor_type) {
		case 0x00:
			return "Hyundai CMOS sensor";
		case 0x20:
			return "Sony CCD sensor + TDA8787";
		case 0x2E:
			return "Sony CCD sensor + Exas 98L59";
		case 0x2F:
			return "Sony CCD sensor + ADI 9804";
		case 0x30:
			return "Sharp CCD sensor + TDA8787";
		case 0x3E:
			return "Sharp CCD sensor + Exas 98L59";
		case 0x3F:
			return "Sharp CCD sensor + ADI 9804";
		case 0x40:
			return "UPA 1021 sensor";
		case 0x100:
			return "VGA sensor";
		case 0x101:
			return "PAL MR sensor";
		default:
			return "unknown type of sensor";
	}
}
#endif

/***************************************************************************/
/* Video4Linux functions */

static int pwc_video_open(struct file *file)
{
	struct video_device *vdev = video_devdata(file);
	struct pwc_device *pdev;

	PWC_DEBUG_OPEN(">> video_open called(vdev = 0x%p).\n", vdev);

	pdev = video_get_drvdata(vdev);
	if (!pdev->udev)
		return -ENODEV;

	file->private_data = vdev;
	PWC_DEBUG_OPEN("<< video_open() returns 0.\n");
	return 0;
}

static void pwc_video_release(struct video_device *vfd)
{
	struct pwc_device *pdev = container_of(vfd, struct pwc_device, vdev);
	int hint;

	/* search device_hint[] table if we occupy a slot, by any chance */
	for (hint = 0; hint < MAX_DEV_HINTS; hint++)
		if (device_hint[hint].pdev == pdev)
			device_hint[hint].pdev = NULL;

	/* Free intermediate decompression buffer & tables */
	if (pdev->decompress_data != NULL) {
		PWC_DEBUG_MEMORY("Freeing decompression buffer at %p.\n",
				 pdev->decompress_data);
		kfree(pdev->decompress_data);
		pdev->decompress_data = NULL;
	}

	v4l2_ctrl_handler_free(&pdev->ctrl_handler);

	kfree(pdev);
}

static int pwc_video_close(struct file *file)
{
	struct video_device *vdev = file->private_data;
	struct pwc_device *pdev;

	PWC_DEBUG_OPEN(">> video_close called(vdev = 0x%p).\n", vdev);

	pdev = video_get_drvdata(vdev);
	if (pdev->capt_file == file) {
		vb2_queue_release(&pdev->vb_queue);
		pdev->capt_file = NULL;
	}

	PWC_DEBUG_OPEN("<< video_close()\n");
	return 0;
}

static ssize_t pwc_video_read(struct file *file, char __user *buf,
			      size_t count, loff_t *ppos)
{
	struct video_device *vdev = file->private_data;
	struct pwc_device *pdev = video_get_drvdata(vdev);

	if (!pdev->udev)
		return -ENODEV;

	if (pdev->capt_file != NULL &&
	    pdev->capt_file != file)
		return -EBUSY;

	pdev->capt_file = file;

	return vb2_read(&pdev->vb_queue, buf, count, ppos,
			file->f_flags & O_NONBLOCK);
}

static unsigned int pwc_video_poll(struct file *file, poll_table *wait)
{
	struct video_device *vdev = file->private_data;
	struct pwc_device *pdev = video_get_drvdata(vdev);

	if (!pdev->udev)
		return POLL_ERR;

	return vb2_poll(&pdev->vb_queue, file, wait);
}

static int pwc_video_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct video_device *vdev = file->private_data;
	struct pwc_device *pdev = video_get_drvdata(vdev);

	if (pdev->capt_file != file)
		return -EBUSY;

	return vb2_mmap(&pdev->vb_queue, vma);
}

/***************************************************************************/
/* Videobuf2 operations */

static int queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
				unsigned int *nplanes, unsigned long sizes[],
				void *alloc_ctxs[])
{
	struct pwc_device *pdev = vb2_get_drv_priv(vq);

	if (*nbuffers < MIN_FRAMES)
		*nbuffers = MIN_FRAMES;
	else if (*nbuffers > MAX_FRAMES)
		*nbuffers = MAX_FRAMES;

	*nplanes = 1;

	sizes[0] = PAGE_ALIGN((pdev->abs_max.x * pdev->abs_max.y * 3) / 2);

	return 0;
}

static int buffer_init(struct vb2_buffer *vb)
{
	struct pwc_frame_buf *buf = container_of(vb, struct pwc_frame_buf, vb);

	/* need vmalloc since frame buffer > 128K */
	buf->data = vzalloc(PWC_FRAME_SIZE);
	if (buf->data == NULL)
		return -ENOMEM;

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct pwc_device *pdev = vb2_get_drv_priv(vb->vb2_queue);

	/* Don't allow queing new buffers after device disconnection */
	if (!pdev->udev)
		return -ENODEV;

	return 0;
}

static int buffer_finish(struct vb2_buffer *vb)
{
	struct pwc_device *pdev = vb2_get_drv_priv(vb->vb2_queue);
	struct pwc_frame_buf *buf = container_of(vb, struct pwc_frame_buf, vb);

	/*
	 * Application has called dqbuf and is getting back a buffer we've
	 * filled, take the pwc data we've stored in buf->data and decompress
	 * it into a usable format, storing the result in the vb2_buffer
	 */
	return pwc_decompress(pdev, buf);
}

static void buffer_cleanup(struct vb2_buffer *vb)
{
	struct pwc_frame_buf *buf = container_of(vb, struct pwc_frame_buf, vb);

	vfree(buf->data);
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct pwc_device *pdev = vb2_get_drv_priv(vb->vb2_queue);
	struct pwc_frame_buf *buf = container_of(vb, struct pwc_frame_buf, vb);

	mutex_lock(&pdev->queued_bufs_lock);
	list_add_tail(&buf->list, &pdev->queued_bufs);
	mutex_unlock(&pdev->queued_bufs_lock);
}

static int start_streaming(struct vb2_queue *vq)
{
	struct pwc_device *pdev = vb2_get_drv_priv(vq);

	if (!pdev->udev)
		return -ENODEV;

	/* Turn on camera and set LEDS on */
	pwc_camera_power(pdev, 1);
	if (pdev->power_save) {
		/* Restore video mode */
		pwc_set_video_mode(pdev, pdev->view.x, pdev->view.y,
				   pdev->vframes, pdev->vcompression,
				   pdev->vsnapshot);
	}
	pwc_set_leds(pdev, led_on, led_off);

	return pwc_isoc_init(pdev);
}

static int stop_streaming(struct vb2_queue *vq)
{
	struct pwc_device *pdev = vb2_get_drv_priv(vq);

	if (pdev->udev) {
		pwc_set_leds(pdev, 0, 0);
		pwc_isoc_cleanup(pdev);
		pwc_camera_power(pdev, 0);
	}
	pwc_cleanup_queued_bufs(pdev);

	return 0;
}

static void pwc_lock(struct vb2_queue *vq)
{
	struct pwc_device *pdev = vb2_get_drv_priv(vq);
	mutex_lock(&pdev->modlock);
}

static void pwc_unlock(struct vb2_queue *vq)
{
	struct pwc_device *pdev = vb2_get_drv_priv(vq);
	mutex_unlock(&pdev->modlock);
}

static struct vb2_ops pwc_vb_queue_ops = {
	.queue_setup		= queue_setup,
	.buf_init		= buffer_init,
	.buf_prepare		= buffer_prepare,
	.buf_finish		= buffer_finish,
	.buf_cleanup		= buffer_cleanup,
	.buf_queue		= buffer_queue,
	.start_streaming	= start_streaming,
	.stop_streaming		= stop_streaming,
	.wait_prepare		= pwc_unlock,
	.wait_finish		= pwc_lock,
};

/***************************************************************************/
/* USB functions */

/* This function gets called when a new device is plugged in or the usb core
 * is loaded.
 */

static int usb_pwc_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct pwc_device *pdev = NULL;
	int vendor_id, product_id, type_id;
	int hint, rc;
	int features = 0;
	int video_nr = -1; /* default: use next available device */
	int my_power_save = power_save;
	char serial_number[30], *name;

	vendor_id = le16_to_cpu(udev->descriptor.idVendor);
	product_id = le16_to_cpu(udev->descriptor.idProduct);

	/* Check if we can handle this device */
	PWC_DEBUG_PROBE("probe() called [%04X %04X], if %d\n",
		vendor_id, product_id,
		intf->altsetting->desc.bInterfaceNumber);

	/* the interfaces are probed one by one. We are only interested in the
	   video interface (0) now.
	   Interface 1 is the Audio Control, and interface 2 Audio itself.
	 */
	if (intf->altsetting->desc.bInterfaceNumber > 0)
		return -ENODEV;

	if (vendor_id == 0x0471) {
		switch (product_id) {
		case 0x0302:
			PWC_INFO("Philips PCA645VC USB webcam detected.\n");
			name = "Philips 645 webcam";
			type_id = 645;
			break;
		case 0x0303:
			PWC_INFO("Philips PCA646VC USB webcam detected.\n");
			name = "Philips 646 webcam";
			type_id = 646;
			break;
		case 0x0304:
			PWC_INFO("Askey VC010 type 2 USB webcam detected.\n");
			name = "Askey VC010 webcam";
			type_id = 646;
			break;
		case 0x0307:
			PWC_INFO("Philips PCVC675K (Vesta) USB webcam detected.\n");
			name = "Philips 675 webcam";
			type_id = 675;
			break;
		case 0x0308:
			PWC_INFO("Philips PCVC680K (Vesta Pro) USB webcam detected.\n");
			name = "Philips 680 webcam";
			type_id = 680;
			break;
		case 0x030C:
			PWC_INFO("Philips PCVC690K (Vesta Pro Scan) USB webcam detected.\n");
			name = "Philips 690 webcam";
			type_id = 690;
			break;
		case 0x0310:
			PWC_INFO("Philips PCVC730K (ToUCam Fun)/PCVC830 (ToUCam II) USB webcam detected.\n");
			name = "Philips 730 webcam";
			type_id = 730;
			break;
		case 0x0311:
			PWC_INFO("Philips PCVC740K (ToUCam Pro)/PCVC840 (ToUCam II) USB webcam detected.\n");
			name = "Philips 740 webcam";
			type_id = 740;
			break;
		case 0x0312:
			PWC_INFO("Philips PCVC750K (ToUCam Pro Scan) USB webcam detected.\n");
			name = "Philips 750 webcam";
			type_id = 750;
			break;
		case 0x0313:
			PWC_INFO("Philips PCVC720K/40 (ToUCam XS) USB webcam detected.\n");
			name = "Philips 720K/40 webcam";
			type_id = 720;
			break;
		case 0x0329:
			PWC_INFO("Philips SPC 900NC USB webcam detected.\n");
			name = "Philips SPC 900NC webcam";
			type_id = 740;
			break;
		default:
			return -ENODEV;
			break;
		}
	}
	else if (vendor_id == 0x069A) {
		switch(product_id) {
		case 0x0001:
			PWC_INFO("Askey VC010 type 1 USB webcam detected.\n");
			name = "Askey VC010 webcam";
			type_id = 645;
			break;
		default:
			return -ENODEV;
			break;
		}
	}
	else if (vendor_id == 0x046d) {
		switch(product_id) {
		case 0x08b0:
			PWC_INFO("Logitech QuickCam Pro 3000 USB webcam detected.\n");
			name = "Logitech QuickCam Pro 3000";
			type_id = 740; /* CCD sensor */
			break;
		case 0x08b1:
			PWC_INFO("Logitech QuickCam Notebook Pro USB webcam detected.\n");
			name = "Logitech QuickCam Notebook Pro";
			type_id = 740; /* CCD sensor */
			break;
		case 0x08b2:
			PWC_INFO("Logitech QuickCam 4000 Pro USB webcam detected.\n");
			name = "Logitech QuickCam Pro 4000";
			type_id = 740; /* CCD sensor */
			if (my_power_save == -1)
				my_power_save = 1;
			break;
		case 0x08b3:
			PWC_INFO("Logitech QuickCam Zoom USB webcam detected.\n");
			name = "Logitech QuickCam Zoom";
			type_id = 740; /* CCD sensor */
			break;
		case 0x08B4:
			PWC_INFO("Logitech QuickCam Zoom (new model) USB webcam detected.\n");
			name = "Logitech QuickCam Zoom";
			type_id = 740; /* CCD sensor */
			if (my_power_save == -1)
				my_power_save = 1;
			break;
		case 0x08b5:
			PWC_INFO("Logitech QuickCam Orbit/Sphere USB webcam detected.\n");
			name = "Logitech QuickCam Orbit";
			type_id = 740; /* CCD sensor */
			if (my_power_save == -1)
				my_power_save = 1;
			features |= FEATURE_MOTOR_PANTILT;
			break;
		case 0x08b6:
			PWC_INFO("Logitech/Cisco VT Camera webcam detected.\n");
			name = "Cisco VT Camera";
			type_id = 740; /* CCD sensor */
			break;
		case 0x08b7:
			PWC_INFO("Logitech ViewPort AV 100 webcam detected.\n");
			name = "Logitech ViewPort AV 100";
			type_id = 740; /* CCD sensor */
			break;
		case 0x08b8: /* Where this released? */
			PWC_INFO("Logitech QuickCam detected (reserved ID).\n");
			name = "Logitech QuickCam (res.)";
			type_id = 730; /* Assuming CMOS */
			break;
		default:
			return -ENODEV;
			break;
		}
	}
	else if (vendor_id == 0x055d) {
		/* I don't know the difference between the C10 and the C30;
		   I suppose the difference is the sensor, but both cameras
		   work equally well with a type_id of 675
		 */
		switch(product_id) {
		case 0x9000:
			PWC_INFO("Samsung MPC-C10 USB webcam detected.\n");
			name = "Samsung MPC-C10";
			type_id = 675;
			break;
		case 0x9001:
			PWC_INFO("Samsung MPC-C30 USB webcam detected.\n");
			name = "Samsung MPC-C30";
			type_id = 675;
			break;
		case 0x9002:
			PWC_INFO("Samsung SNC-35E (v3.0) USB webcam detected.\n");
			name = "Samsung MPC-C30";
			type_id = 740;
			break;
		default:
			return -ENODEV;
			break;
		}
	}
	else if (vendor_id == 0x041e) {
		switch(product_id) {
		case 0x400c:
			PWC_INFO("Creative Labs Webcam 5 detected.\n");
			name = "Creative Labs Webcam 5";
			type_id = 730;
			if (my_power_save == -1)
				my_power_save = 1;
			break;
		case 0x4011:
			PWC_INFO("Creative Labs Webcam Pro Ex detected.\n");
			name = "Creative Labs Webcam Pro Ex";
			type_id = 740;
			break;
		default:
			return -ENODEV;
			break;
		}
	}
	else if (vendor_id == 0x04cc) {
		switch(product_id) {
		case 0x8116:
			PWC_INFO("Sotec Afina Eye USB webcam detected.\n");
			name = "Sotec Afina Eye";
			type_id = 730;
			break;
		default:
			return -ENODEV;
			break;
		}
	}
	else if (vendor_id == 0x06be) {
		switch(product_id) {
		case 0x8116:
			/* This is essentially the same cam as the Sotec Afina Eye */
			PWC_INFO("AME Co. Afina Eye USB webcam detected.\n");
			name = "AME Co. Afina Eye";
			type_id = 750;
			break;
		default:
			return -ENODEV;
			break;
		}

	}
	else if (vendor_id == 0x0d81) {
		switch(product_id) {
		case 0x1900:
			PWC_INFO("Visionite VCS-UC300 USB webcam detected.\n");
			name = "Visionite VCS-UC300";
			type_id = 740; /* CCD sensor */
			break;
		case 0x1910:
			PWC_INFO("Visionite VCS-UM100 USB webcam detected.\n");
			name = "Visionite VCS-UM100";
			type_id = 730; /* CMOS sensor */
			break;
		default:
			return -ENODEV;
			break;
		}
	}
	else
		return -ENODEV; /* Not any of the know types; but the list keeps growing. */

	if (my_power_save == -1)
		my_power_save = 0;

	memset(serial_number, 0, 30);
	usb_string(udev, udev->descriptor.iSerialNumber, serial_number, 29);
	PWC_DEBUG_PROBE("Device serial number is %s\n", serial_number);

	if (udev->descriptor.bNumConfigurations > 1)
		PWC_WARNING("Warning: more than 1 configuration available.\n");

	/* Allocate structure, initialize pointers, mutexes, etc. and link it to the usb_device */
	pdev = kzalloc(sizeof(struct pwc_device), GFP_KERNEL);
	if (pdev == NULL) {
		PWC_ERROR("Oops, could not allocate memory for pwc_device.\n");
		return -ENOMEM;
	}

	pdev->pwcmech = pwc_devmech_start();
	pwcmech_register_driver(pdev->pwcmech, &pwc_driver);
	pwcmech_register_handler(pdev->pwcmech, udev);
	set_ctrl0_timeout(pdev->pwcmech->com, 1000);

	pdev->type = type_id;
	pdev->vframes = default_fps;
	strcpy(pdev->serial, serial_number);
	pdev->features = features;
	if (vendor_id == 0x046D && product_id == 0x08B5) {
		/* Logitech QuickCam Orbit
		   The ranges have been determined experimentally; they may differ from cam to cam.
		   Also, the exact ranges left-right and up-down are different for my cam
		  */
		pdev->angle_range.pan_min  = -7000;
		pdev->angle_range.pan_max  =  7000;
		pdev->angle_range.tilt_min = -3000;
		pdev->angle_range.tilt_max =  2500;
	}
	pwc_construct(pdev); /* set min/max sizes correct */

	mutex_init(&pdev->modlock);
	mutex_init(&pdev->udevlock);
	mutex_init(&pdev->queued_bufs_lock);
	INIT_LIST_HEAD(&pdev->queued_bufs);

	pdev->udev = udev;
	pdev->vcompression = pwc_preferred_compression;
	pdev->power_save = my_power_save;

	/* Init videobuf2 queue structure */
	memset(&pdev->vb_queue, 0, sizeof(pdev->vb_queue));
	pdev->vb_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	pdev->vb_queue.io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
	pdev->vb_queue.drv_priv = pdev;
	pdev->vb_queue.buf_struct_size = sizeof(struct pwc_frame_buf);
	pdev->vb_queue.ops = &pwc_vb_queue_ops;
	pdev->vb_queue.mem_ops = &vb2_vmalloc_memops;
	vb2_queue_init(&pdev->vb_queue);

	/* Init video_device structure */
	memcpy(&pdev->vdev, &pwc_template, sizeof(pwc_template));
	pdev->vdev.parent = &intf->dev;
	pdev->vdev.lock = &pdev->modlock;
	strcpy(pdev->vdev.name, name);
	video_set_drvdata(&pdev->vdev, pdev);

	pdev->release = le16_to_cpu(udev->descriptor.bcdDevice);
	PWC_DEBUG_PROBE("Release: %04x\n", pdev->release);

	/* Now search device_hint[] table for a match, so we can hint a node number. */
	for (hint = 0; hint < MAX_DEV_HINTS; hint++) {
		if (((device_hint[hint].type == -1) || (device_hint[hint].type == pdev->type)) &&
		     (device_hint[hint].pdev == NULL)) {
			/* so far, so good... try serial number */
			if ((device_hint[hint].serial_number[0] == '*') || !strcmp(device_hint[hint].serial_number, serial_number)) {
				/* match! */
				video_nr = device_hint[hint].device_node;
				PWC_DEBUG_PROBE("Found hint, will try to register as /dev/video%d\n", video_nr);
				break;
			}
		}
	}

	/* occupy slot */
	if (hint < MAX_DEV_HINTS)
		device_hint[hint].pdev = pdev;

	PWC_DEBUG_PROBE("probe() function returning struct at 0x%p.\n", pdev);
	usb_set_intfdata(intf, pdev);

#ifdef CONFIG_USB_PWC_DEBUG
	/* Query sensor type */
	if (pwc_get_cmos_sensor(pdev, &rc) >= 0) {
		PWC_DEBUG_OPEN("This %s camera is equipped with a %s (%d).\n",
				pdev->vdev.name,
				pwc_sensor_type_to_string(rc), rc);
	}
#endif

	/* Set the leds off */
	pwc_set_leds(pdev, 0, 0);

	/* Setup intial videomode */
	rc = pwc_set_video_mode(pdev, pdev->view_max.x, pdev->view_max.y,
				pdev->vframes, pdev->vcompression, 0);
	if (rc)
		goto err_free_mem;

	/* Register controls (and read default values from camera */
	rc = pwc_init_controls(pdev);
	if (rc) {
		PWC_ERROR("Failed to register v4l2 controls (%d).\n", rc);
		goto err_free_mem;
	}

	pdev->vdev.ctrl_handler = &pdev->ctrl_handler;

	/* And powerdown the camera until streaming starts */
	pwc_camera_power(pdev, 0);

	rc = video_register_device(&pdev->vdev, VFL_TYPE_GRABBER, video_nr);
	if (rc < 0) {
		PWC_ERROR("Failed to register as video device (%d).\n", rc);
		goto err_free_controls;
	}
	rc = pwc_create_sysfs_files(pdev);
	if (rc)
		goto err_video_unreg;

	PWC_INFO("Registered as %s.\n", video_device_node_name(&pdev->vdev));

#ifdef CONFIG_USB_PWC_INPUT_EVDEV
	/* register webcam snapshot button input device */
	pdev->button_dev = input_allocate_device();
	if (!pdev->button_dev) {
		PWC_ERROR("Err, insufficient memory for webcam snapshot button device.");
		rc = -ENOMEM;
		pwc_remove_sysfs_files(pdev);
		goto err_video_unreg;
	}

	usb_make_path(udev, pdev->button_phys, sizeof(pdev->button_phys));
	strlcat(pdev->button_phys, "/input0", sizeof(pdev->button_phys));

	pdev->button_dev->name = "PWC snapshot button";
	pdev->button_dev->phys = pdev->button_phys;
	usb_to_input_id(pdev->udev, &pdev->button_dev->id);
	pdev->button_dev->dev.parent = &pdev->udev->dev;
	pdev->button_dev->evbit[0] = BIT_MASK(EV_KEY);
	pdev->button_dev->keybit[BIT_WORD(KEY_CAMERA)] = BIT_MASK(KEY_CAMERA);

	rc = input_register_device(pdev->button_dev);
	if (rc) {
		input_free_device(pdev->button_dev);
		pdev->button_dev = NULL;
		pwc_remove_sysfs_files(pdev);
		goto err_video_unreg;
	}
#endif

	return 0;

err_video_unreg:
	if (hint < MAX_DEV_HINTS)
		device_hint[hint].pdev = NULL;
	video_unregister_device(&pdev->vdev);
err_free_controls:
	v4l2_ctrl_handler_free(&pdev->ctrl_handler);
err_free_mem:
	usb_set_intfdata(intf, NULL);
	kfree(pdev);
	return rc;
}

/* The user yanked out the cable... */
static void usb_pwc_disconnect(struct usb_interface *intf)
{
	struct pwc_device *pdev  = usb_get_intfdata(intf);

	mutex_lock(&pdev->udevlock);
	mutex_lock(&pdev->modlock);

	pwcmech_deregister_driver(pdev->pwcmech);
	pwcmech_deregister_handler(pdev->pwcmech);
	pwc_devmech_stop(pdev->pwcmech);

	usb_set_intfdata(intf, NULL);
	/* No need to keep the urbs around after disconnection */
	pwc_isoc_cleanup(pdev);
	pwc_cleanup_queued_bufs(pdev);
	pdev->udev = NULL;

	mutex_unlock(&pdev->modlock);
	mutex_unlock(&pdev->udevlock);

	pwc_remove_sysfs_files(pdev);
	video_unregister_device(&pdev->vdev);

#ifdef CONFIG_USB_PWC_INPUT_EVDEV
	if (pdev->button_dev)
		input_unregister_device(pdev->button_dev);
#endif
}


/*
 * Initialization code & module stuff
 */

static int fps;
static int compression = -1;
static int leds[2] = { -1, -1 };
static unsigned int leds_nargs;
static char *dev_hint[MAX_DEV_HINTS];
static unsigned int dev_hint_nargs;

module_param(fps, int, 0444);
#ifdef CONFIG_USB_PWC_DEBUG
module_param_named(trace, pwc_trace, int, 0644);
#endif
module_param(power_save, int, 0644);
module_param(compression, int, 0444);
module_param_array(leds, int, &leds_nargs, 0444);
module_param_array(dev_hint, charp, &dev_hint_nargs, 0444);

MODULE_PARM_DESC(fps, "Initial frames per second. Varies with model, useful range 5-30");
#ifdef CONFIG_USB_PWC_DEBUG
MODULE_PARM_DESC(trace, "For debugging purposes");
#endif
MODULE_PARM_DESC(power_save, "Turn power saving for new cameras on or off");
MODULE_PARM_DESC(compression, "Preferred compression quality. Range 0 (uncompressed) to 3 (high compression)");
MODULE_PARM_DESC(leds, "LED on,off time in milliseconds");
MODULE_PARM_DESC(dev_hint, "Device node hints");

MODULE_DESCRIPTION("Philips & OEM USB webcam driver");
MODULE_AUTHOR("Luc Saillard <luc@saillard.org>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("pwcx");
MODULE_VERSION( PWC_VERSION );

static int __init usb_pwc_init(void)
{
	int i;

#ifdef CONFIG_USB_PWC_DEBUG
	PWC_INFO("Philips webcam module version " PWC_VERSION " loaded.\n");
	PWC_INFO("Supports Philips PCA645/646, PCVC675/680/690, PCVC720[40]/730/740/750 & PCVC830/840.\n");
	PWC_INFO("Also supports the Askey VC010, various Logitech Quickcams, Samsung MPC-C10 and MPC-C30,\n");
	PWC_INFO("the Creative WebCam 5 & Pro Ex, SOTEC Afina Eye and Visionite VCS-UC300 and VCS-UM100.\n");

	if (pwc_trace >= 0) {
		PWC_DEBUG_MODULE("Trace options: 0x%04x\n", pwc_trace);
	}
#endif

	if (fps) {
		if (fps < 4 || fps > 30) {
			PWC_ERROR("Framerate out of bounds (4-30).\n");
			return -EINVAL;
		}
		default_fps = fps;
		PWC_DEBUG_MODULE("Default framerate set to %d.\n", default_fps);
	}

	if (compression >= 0) {
		if (compression > 3) {
			PWC_ERROR("Invalid compression setting; use a number between 0 (uncompressed) and 3 (high).\n");
			return -EINVAL;
		}
		pwc_preferred_compression = compression;
		PWC_DEBUG_MODULE("Preferred compression set to %d.\n", pwc_preferred_compression);
	}
	if (leds[0] >= 0)
		led_on = leds[0];
	if (leds[1] >= 0)
		led_off = leds[1];

	/* Big device node whoopla. Basically, it allows you to assign a
	   device node (/dev/videoX) to a camera, based on its type
	   & serial number. The format is [type[.serialnumber]:]node.

	   Any camera that isn't matched by these rules gets the next
	   available free device node.
	 */
	for (i = 0; i < MAX_DEV_HINTS; i++) {
		char *s, *colon, *dot;

		/* This loop also initializes the array */
		device_hint[i].pdev = NULL;
		s = dev_hint[i];
		if (s != NULL && *s != '\0') {
			device_hint[i].type = -1; /* wildcard */
			strcpy(device_hint[i].serial_number, "*");

			/* parse string: chop at ':' & '/' */
			colon = dot = s;
			while (*colon != '\0' && *colon != ':')
				colon++;
			while (*dot != '\0' && *dot != '.')
				dot++;
			/* Few sanity checks */
			if (*dot != '\0' && dot > colon) {
				PWC_ERROR("Malformed camera hint: the colon must be after the dot.\n");
				return -EINVAL;
			}

			if (*colon == '\0') {
				/* No colon */
				if (*dot != '\0') {
					PWC_ERROR("Malformed camera hint: no colon + device node given.\n");
					return -EINVAL;
				}
				else {
					/* No type or serial number specified, just a number. */
					device_hint[i].device_node =
						simple_strtol(s, NULL, 10);
				}
			}
			else {
				/* There's a colon, so we have at least a type and a device node */
				device_hint[i].type =
					simple_strtol(s, NULL, 10);
				device_hint[i].device_node =
					simple_strtol(colon + 1, NULL, 10);
				if (*dot != '\0') {
					/* There's a serial number as well */
					int k;

					dot++;
					k = 0;
					while (*dot != ':' && k < 29) {
						device_hint[i].serial_number[k++] = *dot;
						dot++;
					}
					device_hint[i].serial_number[k] = '\0';
				}
			}
			PWC_TRACE("device_hint[%d]:\n", i);
			PWC_TRACE("  type    : %d\n", device_hint[i].type);
			PWC_TRACE("  serial# : %s\n", device_hint[i].serial_number);
			PWC_TRACE("  node    : %d\n", device_hint[i].device_node);
		}
		else
			device_hint[i].type = 0; /* not filled */
	} /* ..for MAX_DEV_HINTS */

	PWC_DEBUG_PROBE("Registering driver at address 0x%p.\n", &pwc_driver);
	return usb_register(&pwc_driver);
}

static void __exit usb_pwc_exit(void)
{
	PWC_DEBUG_MODULE("Deregistering driver.\n");
	usb_deregister(&pwc_driver);
	PWC_INFO("Philips webcam module removed.\n");
}

module_init(usb_pwc_init);
module_exit(usb_pwc_exit);

/* vim: set cino= formatoptions=croql cindent shiftwidth=8 tabstop=8: */