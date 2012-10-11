/*
 *    vloopback.c
 *
 *    Copyright Jeroen Vreeken (pe1rxq@amsat.org), 2000
 *    Additional copyright by the contributing authors in the
 *    change history below, 2000-2007
 *
 *    Published under the GNU Public License.
 *
 *    The Video loopback Loopback Device is no longer systematically maintained.
 *    The project is a secondary project for the project "motion" found at
 *    http://motion.sourceforge.net/ and
 *    http://www.lavrsen.dk/twiki/bin/view/Motion/WebHome
 *    and with the vloopback stored at
 *    http://www.lavrsen.dk/twiki/bin/view/Motion/Video loopbackFourLinuxLoopbackDevice
 *
 *    CHANGE HISTORY
 *
 *    UPDATED:    Jeroen Vreeken.
 *            Added locks for smp machines. UNTESTED!
 *            Made the driver much more cpu friendly by using
 *            a wait queue.
 *            Went from vmalloc to rvmalloc (yes, I stole the code
 *            like everybody else) and implemented mmap.
 *            Implemented VIDIOCGUNIT and removed size/palette checks
 *            in VIDIOCSYNC.
 *            Cleaned up a lot of code.
 *            Changed locks to semaphores.
 *            Disabled changing size while somebody is using mmap
 *            Changed mapped check to open check, also don't allow
 *            a open for write while somebody is reading.
 *            Added /proc support
 *            Set dumped count to zero at open.
 *            Modified /proc layout (added vloopbacks entry)
 *
 * 05.10.00 (MTS)    Added Linux 2.2 support
 * 06.10.00 (J Vreeken)    Fixed 2.2 support to make things work under 2.4 again.
 * 17.10.00 (J Vreeken)    Added zero copy mode
 * 19.10.00 (J Vreeken) Added SIGIO on device close.
 * 24.10.00 (J Vreeken) Modified 2.2 stuff and removed spinlock.h
 *            released 0.81
 * 27.10.00 (J Vreeken) Implemented poll
 *            released 0.82
 * 17.01.01 (J Vreeken) support for xawtv
 *             Implemented VIDIOCGFBUF
 *            Additional checks on framebuffer freeing.
 *            released 0.83
 * 31.01.01 (J Vreeken)    Removed need for 'struct ioctl', use _IOC_SIZE() and
 *            IOC_IN instead.
 *            Change the ioctlnr passing to 'unsigned long int'
 *            Instead of just one byte.
 *            THIS BREAKS COMPATIBILITY WITH PREVIOUS VERSIONS!!! 
 * 29.06.01 (J Vreeken)    Added dev_offset module option
 *            Made vloopback_template sane
 *            Added double buffering support
 *            Made vloopback less verbose
 * 20.11.01    (tibit)    Made dev_offset option sane
 *            "Fixed" zerocopy mode by defining the ioctl 
 *            VIDIOCSINVALID. An application which provides data
 *            has to issue it when it encounters an error in
 *            ioctl processing. See dummy.c for examples.
 * 26.11.03    (Kenneth Lavrsen)
 *            released 0.91
 *            0.91 is the combination of the 0.90-tibit by
 *            Tilmann Bitterberg and an update of the Makefile by
 *            Roberto Carvajal.
 * 23.01.05    (W Brack)
 *            (don't know what happened to the comments for 0.92
 *             and 0.93, but I tentatively named this one as 0.99)
 *            enhanced for linux-2.6, with #ifdef to keep it
 *            compatible with linux-2.4.  For linux versions
 *            > 2.5, I changed the memory management
 *            routines to the "more modern" way, most of it
 *            shamelessly copied from other drivers.  I also
 *            added in the code necessary to avoid the "videodev
 *            has no release callback" message when installing.
 *            For versions < 2.5, I updated the routines to be
 *            closer to several other drivers.
 *
 * 04.02.05    (Angel Carpintero)
 *            Fixed version number to 0.93-pre1.
 *            Fixed warning for interruptible_sleep_on() deprecated and added 
 *            wait_event_interruptible compatible with 2.6.x and 2.7.
 *            Fixed memory manager for kernel version > 2.6.9.
 *
 * 07.02.05    (Kenneth Lavrsen)
 *            Changed version to 0.94.
 *            Released as formal released version
 *
 * 20.02.05    (W Brack)
 *            Fixed error with wait_event_interruptible.
 *            Fixed crash when pipe source was stopped before dest.
 *
 * 20.02.05    (Angel Carpintero)     
 *            Added install and uninstall in Makefile.
 *
 *
 * 25.04.05    (Angel Carpintero)
 *            Included Samuel Audet's patch, it checks if the input is already
 *            opened in write mode.
 *
 * 02.05.05    (Kenneth Lavrsen)
 *            Released 0.95-snap2 formerly as 0.95
 *    
 * 10.05.05    (Angel Carpintero)
 *            Added MODULE_VERSION(), fixed create_pipes when video_register_device() returns
 *            -ENFILE . 
 *            Fix warnings about checking return value from copy_to_user() and copy_from_user() functions.
 *
 * 14.11.05    (Angel Carpintero)
 *            Added <linux/version.h> that includes LINUX_VERSION_CODE and KERNEL_VERSION to fix 
 *            compilation agains kernel 2.6.14 , change version to 0.97-snap1
 *
 * 19.12.05    (Angel Carpintero)
 *            Added to example option to choose between rgb24 or yuv420p palettes.
 *
 * 31.12.05    (Angel Carpintero)
 *             Fixed examples, remove perror calls and add support to dummy.c for sysfs.
 *             
 * 04.06.06    (Angel Carpintero)
 *             Add module_param() for kernel > 2.5 because MODULE_PARAM() macro is obsolete.
 *
 * 17.06.06    (Angel Carpintero)
 *            Release version 1.0 with some fixes and code clean up. Added a Jack Bates contribution
 *            to allow build a kernel module in debian way.
 *
 * 26.06.06    (Angel Carpintero)
 *            Added some improvements in Makefile. Fix a problem to compile in Suse.
 *
 *
 * 02.11.06    (Angel Carpintero)
 *            Make compatible with new kernel stable version 2.6.18, Many functions and declarations has
 *            been moved to media/v42l-dev.h and remove from videodev.h/videodev2.h
 *
 * 18.01.07    (Angel Carpintero)    
 *            Change -ENOIOCTLCMD by more appropiate error -ENOTTY.
 *                                 
 * 18.05.08    (Angel Carpintero)
 *            Release 1.1-rc1 as 1.1 stable working with 2.6.24
 *
 * 17.08.08    (Angel Carpintero)
 *            kill_proc() deprecated ,pid API changed , type and owner not available in                             
 *            video_device struct, added param debug.
 *
 * 24.08.08    (Angel Carpintero)
 *            Added compat_iotcl32 init in fopsl, replace tabs by 4 spaces in source code,
 *            add number of buffers as module param.
 *
 * 13.10.08    (Stephan Berberig & Angel Carpintero)
 *            Release to work on 2.6.27 , allow v4l_compat_ioctl32 work in 2.6.27 and a little cleanup 
 *            in Makefile.
 *
 * 22.12.08    (Angel Carpintero)
 *            Allow build with kernel 2.6.28 and 2.6.27.git ( struct video_dev has not priv member anymore).         
 *
 * 17.05.09    (Peter Holik)
 *            Patch to allow work with kernel 2.6.29  
 *
 * 05.08.09   (Angel Carpintero)
 *            Allow to compile with kernel 2.6.30.*
 *
 * 11.09.09   (Angel Carpintero)
 *            Allow to compile with kernel 2.6.31
 *
 * 26.11.09   (Angel Carpintero)
 *            Release 1.3 stable working with 2.6.31
 *
 * 08.12.09   (Manan Tuli)
 *            Allow to compile with 2.6.32
 *
 * 07.01.10   (Angel Carpintero)
 *            Little patch to allow build with real time kernels.
 *
 * 08.09.10   (Darryl Sokoloski)  
 *            Patch for kernel version >= 2.6.34
 *
 * 06.02.11   (PhobosK)  
 *            Patch for kernel version >= 2.6.37
 *            Modified patch for RT kernels.
 */


#define VLOOPBACK_VERSION "1.4-trunk"

/* Include files common to 2.4 and 2.6 versions */
#include <linux/version.h>    /* >= 2.6.14 LINUX_VERSION_CODE */ 
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
#include <linux/sched.h>
#endif

#ifndef CONFIG_VIDEO_V4L1_COMPAT
#error "need CONFIG_VIDEO_V4L1_COMPAT"
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
 #include <media/v4l2-common.h>
#endif

/* v4l_compat_ioctl32 */ 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
 #ifdef __KERNEL__
  #undef __KERNEL__
 #endif
 #include <media/v4l2-ioctl.h>
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,27)
 #define vd_private_data dev.driver_data
 #ifndef __KERNEL__
  #define __KERNEL__
 #endif
#else
 #define vd_private_data priv
#endif

#include <linux/videodev.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>


/* Include files which are unique to versions */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
 #include <asm/ioctl.h>
 #include <asm/page.h> 
 #include <asm/pgtable.h>
 #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
  #ifndef    remap_pfn_range
    #define    remap_pfn_range(a,b,c,d,e) \
     remap_page_range((a),(b),(c)<<PAGE_SHIFT,(d),(e))
  #endif
  #ifndef    vmalloc_to_pfn
    #define    vmalloc_to_pfn(a) page_to_pfn(vmalloc_to_page((a)))
  #endif
 #endif
 #include <asm/uaccess.h>
 #include <linux/init.h>
 #include <linux/device.h>
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
    #include <linux/slab.h>
 #endif
#else
 #include <linux/mm.h>
 #include <linux/slab.h>
 #include <linux/wrapper.h>
 #include <asm/io.h>
#endif

/* kernels <= 2.6.25 have asm only, 2.6.26 has both, and 2.6.27-rc2+ has linux only. */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 27)
#   include <asm/semaphore.h>
#else
#   include <linux/semaphore.h>
#endif
 
#if (defined CONFIG_PREEMPT_RT && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)) || LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)
   #ifndef DECLARE_MUTEX
      #define DECLARE_MUTEX(_m)  DEFINE_SEMAPHORE(_m)
   #endif
   #ifndef init_MUTEX
      #define init_MUTEX(_m) sema_init(_m,1)
   #endif
#endif


#define VIDIOCSINVALID    _IO('v',BASE_VIDIOCPRIVATE+1)

#define verbose(format, arg...) if (printk_ratelimit()) \
        printk(KERN_INFO "[%s] %s: " format "\n" "", \
                __FUNCTION__, __FILE__,  ## arg)

#define info(format, arg...) if (printk_ratelimit()) \
        printk(KERN_INFO "[%s] : " format "\n" "", \
               __FUNCTION__, ## arg)

#define LOG_NODEBUG   0 
#define LOG_FUNCTIONS 1
#define LOG_IOCTL     2
#define LOG_VERBOSE   3

struct vloopback_private {
    int pipenr;
    int in; /* bool , is being feed ? */
};

typedef struct vloopback_private *priv_ptr;

struct vloopback_pipe {
    struct video_device *vloopin;
    struct video_device *vloopout;
    char *buffer;
    unsigned long buflength;
    unsigned int width, height;
    unsigned int palette;
    unsigned long frameswrite;
    unsigned long framesread;
    unsigned long framesdumped;
    unsigned int wopen;
    unsigned int ropen;
    struct semaphore lock;
    wait_queue_head_t wait;
    unsigned int frame;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)    
    unsigned int pid;
#else
    struct pid *pid;    
#endif    
    unsigned int zerocopy;
    unsigned long int ioctlnr;
    unsigned int invalid_ioctl; /* 0 .. none invalid; 1 .. invalid */
    unsigned int ioctllength;
    char *ioctldata;
    char *ioctlretdata;
};

#define MAX_PIPES 16
#define N_BUFFS    2    /* Number of buffers used for pipes */

static struct vloopback_pipe *loops[MAX_PIPES];
static int nr_o_pipes = 0;
static int pipes = -1;
static int spares = 0;
static unsigned int num_buffers = N_BUFFS;
static int pipesused = 0;
static int dev_offset = -1;
static unsigned int debug = LOG_NODEBUG;

/**********************************************************************
 *
 * Memory management - revised for 2.6 kernels
 *
 **********************************************************************/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
/* Here we want the physical address of the memory.
 * This is used when initializing the contents of the
 * area and marking the pages as reserved.
 */
static inline unsigned long kvirt_to_pa(unsigned long adr) 
{
    unsigned long kva;

    kva = (unsigned long)page_address(vmalloc_to_page((void *)adr));
    kva |= adr & (PAGE_SIZE-1); /* restore the offset */
    return __pa(kva);
}
#endif

static void *rvmalloc(unsigned long size)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    struct page *page;
#endif
    void *mem;
    unsigned long adr;

    size = PAGE_ALIGN(size);
    mem = vmalloc_32(size);
    if (!mem)
        return NULL;
    memset(mem, 0, size); /* Clear the ram out, no junk to the user */
    adr = (unsigned long) mem;
    while (size > 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
        page = vmalloc_to_page((void *)adr);
        mem_map_reserve(page);
#else
        SetPageReserved(vmalloc_to_page((void *)adr));
#endif
        adr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    return mem;
}

static void rvfree(void *mem, unsigned long size)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    struct page *page;
#endif
    unsigned long adr;

    if (!mem)
        return;

    adr = (unsigned long) mem;
    while ((long) size > 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
        page = vmalloc_to_page((void *)adr);
        mem_map_unreserve(page);
#else
        ClearPageReserved(vmalloc_to_page((void *)adr));
#endif
        adr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    vfree(mem);
}


static int create_pipe(int nr);

static int fake_ioctl(int nr, unsigned long int cmd, void *arg)
{
    unsigned long fw;

    if (debug > LOG_NODEBUG)
        info("Video loopback %d cmd %lu", nr, cmd);

    loops[nr]->ioctlnr = cmd;
    memcpy(loops[nr]->ioctldata, arg, _IOC_SIZE(cmd));
    loops[nr]->ioctllength = _IOC_SIZE(cmd);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    kill_proc(loops[nr]->pid, SIGIO, 1);    /* Signal the pipe feeder */
#else
    kill_pid(loops[nr]->pid, SIGIO, 1);
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
    fw = loops[nr]->frameswrite;
    wait_event_interruptible(loops[nr]->wait, fw != loops[nr]->frameswrite);
#else
    interruptible_sleep_on(&loops[nr]->wait);
#endif    
    if (cmd & IOC_IN) {
        if (memcmp (arg, loops[nr]->ioctlretdata, _IOC_SIZE(cmd)))
            return 1;
    } else {
        memcpy (arg, loops[nr]->ioctlretdata, _IOC_SIZE(cmd));
    }
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int vloopback_open(struct inode *inod, struct file *f)
#else
static int vloopback_open(struct file *f)
#endif
{    
    struct video_device *loopdev = video_devdata(f);
    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)    
    priv_ptr ptr = (priv_ptr)dev_get_drvdata(&loopdev->dev);
#else
    priv_ptr ptr = (priv_ptr)loopdev->vd_private_data;
#endif    
    int nr = ptr->pipenr;

    if (debug > LOG_NODEBUG)
        info("Video loopback %d", nr);    

    /* Only allow a output to be opened if there is someone feeding
     * the pipe.
     */
    if (!ptr->in) {
        if (loops[nr]->buffer == NULL) 
            return -EINVAL;
        
        loops[nr]->framesread = 0;
        loops[nr]->ropen = 1;
    } else {
        if (loops[nr]->ropen || loops[nr]->wopen) 
            return -EBUSY;

        loops[nr]->framesdumped = 0;
        loops[nr]->frameswrite = 0;
        loops[nr]->wopen = 1;
        loops[nr]->zerocopy = 0;
        loops[nr]->ioctlnr = -1;
        pipesused++;
        if (nr_o_pipes-pipesused<spares) {
            if (!create_pipe(nr_o_pipes)) {
                info("Creating extra spare pipe");
                info("Loopback %d registered, input: video%d, output: video%d",
                    nr_o_pipes,
                    loops[nr_o_pipes]->vloopin->minor,
                    loops[nr_o_pipes]->vloopout->minor
                );
                nr_o_pipes++;
            }
        }
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
        loops[nr]->pid = current->pid;
#elif LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)        
        loops[nr]->pid = find_vpid(current->pid);
#else
        // TODO : Check in stable 2.6.27
        loops[nr]->pid = task_pid(find_task_by_vpid(current->pid));
        //loops[nr]->pid = task_pid(current);
#endif        

        if (debug > LOG_NODEBUG)
            info("Current pid %d", current->pid);
    }
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int vloopback_release(struct inode * inod, struct file *f)
#else
static int vloopback_release(struct file *f)
#endif
{
    struct video_device *loopdev = video_devdata(f);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
    priv_ptr ptr = (priv_ptr)dev_get_drvdata(&loopdev->dev);
#else
    priv_ptr ptr = (priv_ptr)loopdev->vd_private_data;
#endif    
    int nr = ptr->pipenr;

    if (debug > LOG_NODEBUG)
        info("Video loopback %d", nr);

    if (ptr->in) {
        down(&loops[nr]->lock);
        if (loops[nr]->buffer && !loops[nr]->ropen) {
            rvfree(loops[nr]->buffer, loops[nr]->buflength * num_buffers);
            loops[nr]->buffer = NULL;
        }

        up(&loops[nr]->lock);
        loops[nr]->frameswrite++;
        
        if (waitqueue_active(&loops[nr]->wait))
            wake_up(&loops[nr]->wait);

        loops[nr]->width = 0;
        loops[nr]->height = 0;
        loops[nr]->palette = 0;
        loops[nr]->wopen = 0;
        pipesused--;
    } else {
        down(&loops[nr]->lock);

        if (loops[nr]->buffer && !loops[nr]->wopen) {
            rvfree(loops[nr]->buffer, loops[nr]->buflength * num_buffers);
            loops[nr]->buffer = NULL;
        }

        up(&loops[nr]->lock);
        loops[nr]->ropen = 0;

        if (loops[nr]->zerocopy && loops[nr]->buffer) {
            loops[nr]->ioctlnr = 0;
            loops[nr]->ioctllength = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
            kill_proc(loops[nr]->pid, SIGIO, 1);
#else
            kill_pid(loops[nr]->pid, SIGIO, 1);
#endif            
        }
    }

    return 0;
}

static ssize_t vloopback_write(struct file *f, const char *buf,
               size_t count, loff_t *offset)
{
    struct video_device *loopdev = video_devdata(f);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
    priv_ptr ptr = (priv_ptr)dev_get_drvdata(&loopdev->dev);
#else
    priv_ptr ptr = (priv_ptr)loopdev->vd_private_data;
#endif    
    int nr = ptr->pipenr;
    unsigned long realcount = count;

    if (debug > LOG_IOCTL)
        info("Video loopback %d", nr);

    if (!ptr->in)
        return -EINVAL;

    if (loops[nr]->zerocopy)
        return -EINVAL;
    
    if (loops[nr]->buffer == NULL) 
        return -EINVAL;
    
    /* Anybody want some pictures??? */
    if (!waitqueue_active(&loops[nr]->wait)) {
        loops[nr]->framesdumped++;
        return realcount;
    }
    
    down(&loops[nr]->lock);
    if (!loops[nr]->buffer) {
        up(&loops[nr]->lock);
        return -EINVAL;
    }

    if (realcount > loops[nr]->buflength) {
        realcount = loops[nr]->buflength;
        info("Too much data for Video loopback %d ! Only %ld bytes used.", 
             nr, realcount);
    }
    
    if (copy_from_user(loops[nr]->buffer + loops[nr]->frame * loops[nr]->buflength, 
                       buf, realcount)) 
        return -EFAULT;

    loops[nr]->frame = 0;
    up(&loops[nr]->lock);

    loops[nr]->frameswrite++;
    wake_up(&loops[nr]->wait);

    return realcount;
}

static ssize_t vloopback_read(struct file * f, char * buf, size_t count, 
               loff_t *offset)
{
    struct video_device *loopdev = video_devdata(f);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
    priv_ptr ptr = (priv_ptr)dev_get_drvdata(&loopdev->dev);
#else
    priv_ptr ptr = (priv_ptr)loopdev->vd_private_data;
#endif

    int nr = ptr->pipenr;
    unsigned long realcount = count;

    if (debug > LOG_IOCTL)
        info("Video loopback %d", nr);

    if (loops[nr]->zerocopy) {
        if (ptr->in) {
            if (realcount > loops[nr]->ioctllength + sizeof(unsigned long int))
                realcount = loops[nr]->ioctllength + sizeof(unsigned long int);

            if (copy_to_user(buf , &loops[nr]->ioctlnr, sizeof(unsigned long int)))
                return -EFAULT;

            if (copy_to_user(buf + sizeof(unsigned long int), loops[nr]->ioctldata, 
                             realcount - sizeof(unsigned long int)))
                return -EFAULT;    

            if (loops[nr]->ioctlnr == 0)
                loops[nr]->ioctlnr = -1;

            return realcount;
        } else {
            struct video_window vidwin;
            struct video_mmap vidmmap;
            struct video_picture vidpic;
            
            fake_ioctl(nr, VIDIOCGWIN, &vidwin);
            fake_ioctl(nr, VIDIOCGPICT, &vidpic);

            vidmmap.height = vidwin.height;
            vidmmap.width = vidwin.width;
            vidmmap.format = vidpic.palette;
            vidmmap.frame = 0;

            if (fake_ioctl(nr, VIDIOCMCAPTURE, &vidmmap))
                return 0;

            if (fake_ioctl(nr, VIDIOCSYNC, &vidmmap))
                return 0;

            realcount = vidwin.height * vidwin.width * vidpic.depth;
        }
    }

    if (ptr->in)
        return -EINVAL;

    if (realcount > loops[nr]->buflength) {
        realcount = loops[nr]->buflength;
        info("Not so much data in buffer! for Video loopback %d", nr);
    }

    if (!loops[nr]->zerocopy) {
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
        unsigned long fw = loops[nr]->frameswrite;
        
        wait_event_interruptible(loops[nr]->wait, fw != loops[nr]->frameswrite);
#else
        interruptible_sleep_on(&loops[nr]->wait);
#endif
    }

    down(&loops[nr]->lock);
    if (!loops[nr]->buffer) {
        up(&loops[nr]->lock);
        return 0;
    }

    if (copy_to_user(buf, loops[nr]->buffer, realcount))
        return -EFAULT;

    up(&loops[nr]->lock);

    loops[nr]->framesread++;
    return realcount;
}

static int vloopback_mmap(struct file *f, struct vm_area_struct *vma)
{
    struct video_device *loopdev = video_devdata(f);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)    
    priv_ptr ptr = (priv_ptr)dev_get_drvdata(&loopdev->dev);
#else
    priv_ptr ptr = (priv_ptr)loopdev->vd_private_data;
#endif
    int nr = ptr->pipenr;
    unsigned long start = (unsigned long)vma->vm_start;
    long size = vma->vm_end - vma->vm_start;
    unsigned long page, pos;

    if (debug > LOG_NODEBUG)
        info("Video loopback %d", nr);

    down(&loops[nr]->lock);

    if (ptr->in) {
        loops[nr]->zerocopy = 1;

        if (loops[nr]->ropen) {
            info("Can't change size while opened for read in Video loopback %d",
                 nr);
            up(&loops[nr]->lock);
            return -EINVAL;
        }

        if (!size) {
            up(&loops[nr]->lock);
            info("Invalid size Video loopback %d", nr);
            return -EINVAL;
        }

        if (loops[nr]->buffer)
            rvfree(loops[nr]->buffer, loops[nr]->buflength * num_buffers);

        loops[nr]->buflength = size;
        loops[nr]->buffer = rvmalloc(loops[nr]->buflength * num_buffers);
    }

    if (loops[nr]->buffer == NULL) {
        up(&loops[nr]->lock);
        return -EINVAL;
    }

    if (size > (((num_buffers * loops[nr]->buflength) + PAGE_SIZE - 1) 
        & ~(PAGE_SIZE - 1))) {
        up(&loops[nr]->lock);
        return -EINVAL;
    }

    pos = (unsigned long)loops[nr]->buffer;

    while (size > 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
        page = kvirt_to_pa(pos);
        if (remap_page_range(vma,start, page, PAGE_SIZE, PAGE_SHARED)) {
#else
        page = vmalloc_to_pfn((void *)pos);
        if (remap_pfn_range(vma, start, page, PAGE_SIZE, PAGE_SHARED)) {
#endif
            up(&loops[nr]->lock);
            return -EAGAIN;
        }
        start += PAGE_SIZE;
        pos += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    up(&loops[nr]->lock);

    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static int vloopback_ioctl(struct inode *inod, struct file *f, unsigned int cmd, 
           unsigned long arg)
#else
static long vloopback_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
#endif
{
    struct video_device *loopdev = video_devdata(f);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)    
    priv_ptr ptr = (priv_ptr)dev_get_drvdata(&loopdev->dev);
#else
    priv_ptr ptr = (priv_ptr)loopdev->vd_private_data;
#endif

    int nr = ptr->pipenr;
    int i;

    if (debug > LOG_NODEBUG)
        info("Video loopback %d cmd %u", nr, cmd);

    if (loops[nr]->zerocopy) {
        if (!ptr->in) {
            loops[nr]->ioctlnr = cmd;
            loops[nr]->ioctllength = _IOC_SIZE(cmd);
            /* info("DEBUG: vl_ioctl: !loop->in"); */
            /* info("DEBUG: vl_ioctl: cmd %lu", cmd); */
            /* info("DEBUG: vl_ioctl: len %lu", loops[nr]->ioctllength); */
            if (copy_from_user(loops[nr]->ioctldata, (void*)arg, _IOC_SIZE(cmd)))
                return -EFAULT;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
            kill_proc(loops[nr]->pid, SIGIO, 1);
#else
            kill_pid(loops[nr]->pid, SIGIO, 1);
#endif            

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
            wait_event_interruptible(loops[nr]->wait, loops[nr]->ioctlnr == -1);
#else
            interruptible_sleep_on(&loops[nr]->wait);
#endif            
            
            if (loops[nr]->invalid_ioctl) {
                info ("There was an invalid ioctl in Video loopback %d", nr);
                loops[nr]->invalid_ioctl = 0;
                return -ENOTTY;
            }

            if (cmd & IOC_IN && !(cmd & IOC_OUT)) {
                //info("DEBUG: vl_ioctl: cmd & IOC_IN 1"); 
                if (memcmp(loops[nr]->ioctlretdata, loops[nr]->ioctldata, _IOC_SIZE(cmd))) 
                    return -EINVAL;
                
                 //info("DEBUG: vl_ioctl: cmd & IOC_IN 2"); 
                return 0;
            } else {
                if (copy_to_user((void*)arg, loops[nr]->ioctlretdata, _IOC_SIZE(cmd)))
                    return -EFAULT;
                //info("DEBUG: vl_ioctl: !(cmd & IOC_IN) 1");
                return 0;
            }
        } else {
            if ((loops[nr]->ioctlnr != cmd) && (cmd != (VIDIOCSINVALID))) {
                /* wrong ioctl */
                info("Wrong IOCTL %u in Video loopback %d", cmd, nr);
                return 0;
            }

            if (cmd == VIDIOCSINVALID) {
                loops[nr]->invalid_ioctl = 1;
            } else if (copy_from_user(loops[nr]->ioctlretdata, 
                      (void*)arg, loops[nr]->ioctllength)) {
                return -EFAULT;
            }

            loops[nr]->ioctlnr = -1;

            if (waitqueue_active(&loops[nr]->wait))
                wake_up(&loops[nr]->wait);

            return 0;
        }
    }

    switch(cmd)
    {
    /* Get capabilities */
    case VIDIOCGCAP:
    {
        struct video_capability b;
        if (ptr->in) {
            sprintf(b.name, "Video loopback %d input", nr);
            b.type = 0;
        } else {
            sprintf(b.name, "Video loopback %d output", nr);
            b.type = VID_TYPE_CAPTURE;
        }

        b.channels = 1;
        b.audios = 0;
        b.maxwidth = loops[nr]->width;
        b.maxheight = loops[nr]->height;
        b.minwidth = 20;
        b.minheight = 20;

        if (copy_to_user((void*)arg, &b, sizeof(b)))
            return -EFAULT;

        return 0;
    }
    /* Get channel info (sources) */
    case VIDIOCGCHAN:
    {
        struct video_channel v;
        if (copy_from_user(&v, (void*)arg, sizeof(v)))
            return -EFAULT;

        if (v.channel != 0) {
            info("VIDIOCGCHAN: Invalid Channel, was %d", v.channel);
            v.channel = 0;
                //return -EINVAL;
        }

        v.flags = 0;
        v.tuners = 0;
        v.norm = 0;
        v.type = VIDEO_TYPE_CAMERA;
        /*strcpy(v.name, "Loopback"); -- tibit */
        strcpy(v.name, "Composite1");

        if (copy_to_user((void*)arg, &v, sizeof(v)))
            return -EFAULT;

        return 0;
    }
    /* Set channel     */
    case VIDIOCSCHAN:
    {
        int v;

        if (copy_from_user(&v, (void*)arg, sizeof(v)))
            return -EFAULT;

        if (v != 0) {
            info("VIDIOCSCHAN: Invalid Channel, was %d", v);
            return -EINVAL;
        }

        return 0;
    }
    /* Get tuner abilities */
    case VIDIOCGTUNER:
    {
        struct video_tuner v;

        if (copy_from_user(&v, (void*)arg, sizeof(v)) != 0)
            return -EFAULT;

        if (v.tuner) {
            info("VIDIOCGTUNER: Invalid Tuner, was %d", v.tuner);
            return -EINVAL;
        }

        strcpy(v.name, "Format");
        v.rangelow = 0;
        v.rangehigh = 0;
        v.flags = 0;
        v.mode = VIDEO_MODE_AUTO;

        if (copy_to_user((void*)arg,&v, sizeof(v)) != 0)
            return -EFAULT;

        return 0;
    }
    /* Get picture properties */
    case VIDIOCGPICT:
    {
        struct video_picture p;

        p.colour = 0x8000;
        p.hue = 0x8000;
        p.brightness = 0x8000;
        p.contrast = 0x8000;
        p.whiteness = 0x8000;
        p.depth = 0x8000;
        p.palette = loops[nr]->palette;

        if (copy_to_user((void*)arg, &p, sizeof(p)))
            return -EFAULT;

        return 0;
    }
    /* Set picture properties */
    case VIDIOCSPICT:
    {
        struct video_picture p;

        if (copy_from_user(&p, (void*)arg, sizeof(p)))
            return -EFAULT;

        if (!ptr->in) {
            if (p.palette != loops[nr]->palette)
                return -EINVAL;
        } else {
            loops[nr]->palette = p.palette;
        }

        return 0;
    }
    /* Get the video overlay window */
    case VIDIOCGWIN:
    {
        struct video_window vw;

        vw.x = 0;
        vw.y = 0;
        vw.width = loops[nr]->width;
        vw.height = loops[nr]->height;
        vw.chromakey = 0;
        vw.flags = 0;
        vw.clipcount = 0;

        if (copy_to_user((void*)arg, &vw, sizeof(vw)))
            return -EFAULT;

        return 0;
    }
    /* Set the video overlay window - passes clip list for hardware smarts , chromakey etc */
    case VIDIOCSWIN:
    {
        struct video_window vw;
            
        if (copy_from_user(&vw, (void*)arg, sizeof(vw)))
            return -EFAULT;

        if (vw.flags)
            return -EINVAL;

        if (vw.clipcount)
            return -EINVAL;

        if (loops[nr]->height == vw.height &&
            loops[nr]->width == vw.width)
            return 0;

        if (!ptr->in) {
            return -EINVAL;
        } else {
            loops[nr]->height = vw.height;
            loops[nr]->width = vw.width;
            /* Make sure nobody is using the buffer while we
               fool around with it.
               We are also not allowing changes while
               somebody using mmap has the output open.
             */
            down(&loops[nr]->lock);
            if (loops[nr]->ropen) {
                info("Can't change size while opened for read");
                up(&loops[nr]->lock);
                return -EINVAL;
            }

            if (loops[nr]->buffer)
                rvfree(loops[nr]->buffer, loops[nr]->buflength * num_buffers);

            loops[nr]->buflength = vw.width * vw.height * 4;
            loops[nr]->buffer = rvmalloc(loops[nr]->buflength * num_buffers);
            up(&loops[nr]->lock);
        }
        return 0;
    }
    /* Memory map buffer info */
    case VIDIOCGMBUF:
    {
        struct video_mbuf vm;
            
        vm.size = loops[nr]->buflength * num_buffers;
        vm.frames = num_buffers;
        for (i = 0; i < vm.frames; i++)
            vm.offsets[i] = i * loops[nr]->buflength;

        if (copy_to_user((void*)arg, &vm, sizeof(vm)))
            return -EFAULT;

        return 0;
    }
    /* Grab frames */
    case VIDIOCMCAPTURE:
    {
        struct video_mmap vm;

        if (ptr->in)
            return -EINVAL;

        if (!loops[nr]->buffer)
            return -EINVAL;

        if (copy_from_user(&vm, (void*)arg, sizeof(vm)))
            return -EFAULT;

        if (vm.format != loops[nr]->palette)
            return -EINVAL;

        if (vm.frame > num_buffers)
            return -EINVAL;

        return 0;
    }
    /* Sync with mmap grabbing */
    case VIDIOCSYNC:
    {
        int frame;
        unsigned long fw;

        if (copy_from_user((void *)&frame, (void*)arg, sizeof(int)))
            return -EFAULT;

        if (ptr->in)
            return -EINVAL;

        if (!loops[nr]->buffer)
            return -EINVAL;

        /* Ok, everything should be alright since the program
           should have called VIDIOMCAPTURE and we are ready to
           do the 'capturing' */
        //if (frame > 1)
        if (frame > num_buffers-1)
            return -EINVAL;

        loops[nr]->frame = frame;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
        fw = loops[nr]->frameswrite;
        wait_event_interruptible(loops[nr]->wait, fw != loops[nr]->frameswrite);
#else
        interruptible_sleep_on(&loops[nr]->wait);
#endif            
        if (!loops[nr]->buffer)        /* possibly released during sleep */
            return -EINVAL;

        loops[nr]->framesread++;

        return 0;
    }
    /* Get attached units */
    case VIDIOCGUNIT:
    {
        struct video_unit vu;
            
        if (ptr->in)
            vu.video = loops[nr]->vloopout->minor;
        else
            vu.video = loops[nr]->vloopin->minor;

        vu.vbi = VIDEO_NO_UNIT;
        vu.radio = VIDEO_NO_UNIT;
        vu.audio = VIDEO_NO_UNIT;
        vu.teletext = VIDEO_NO_UNIT;

        if (copy_to_user((void*)arg, &vu, sizeof(vu)))
            return -EFAULT;

        return 0;
    }
    /* Get frame buffer */
    case VIDIOCGFBUF:
    {
        struct video_buffer vb;

        memset(&vb, 0, sizeof(vb));
        vb.base = NULL;

        if (copy_to_user((void *)arg, (void *)&vb, sizeof(vb)))
            return -EFAULT;

        return 0;
    }
    /* Start, end capture */
    case VIDIOCCAPTURE:
    {
        int start;

        if (copy_from_user(&start, (void*)arg, sizeof(int)))
            return -EFAULT;

        if (start) { 
            info ("Video loopback %d Capture started", nr);
        } else {
            info ("Video loopback %d Capture stopped", nr);
        }

        return 0;
    }
    case VIDIOCGFREQ:
    case VIDIOCSFREQ:
    case VIDIOCGAUDIO:
    case VIDIOCSAUDIO:
        return -EINVAL;
    case VIDIOCKEY:
        return 0;
    default:
        return -ENOTTY;
        //return -ENOIOCTLCMD;
    }
    return 0;
}

static unsigned int vloopback_poll(struct file *f, struct poll_table_struct *wait)
{
    struct video_device *loopdev = video_devdata(f);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)    
    priv_ptr ptr = (priv_ptr)dev_get_drvdata(&loopdev->dev);
#else
    priv_ptr ptr = (priv_ptr)loopdev->vd_private_data;
#endif
    int nr = ptr->pipenr;

    if (debug > LOG_NODEBUG)
        info("Video loopback %d", nr);

    if (loopdev == NULL)
        return -EFAULT;

    if (!ptr->in)
        return 0;

    if (loops[nr]->ioctlnr != -1) {
        if (loops[nr]->zerocopy) {
            return (POLLIN | POLLPRI | POLLOUT | POLLRDNORM);
        } else {
            return (POLLOUT);
        }
    }
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
static const struct file_operations fileops_template =
#else
static const struct v4l2_file_operations fileops_template =
#endif
{
    owner:        THIS_MODULE,
    open:        vloopback_open,
    release:    vloopback_release,
    read:        vloopback_read,
    write:        vloopback_write,
    poll:        vloopback_poll,
    ioctl:        vloopback_ioctl,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,15) && LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
    compat_ioctl:   v4l_compat_ioctl32,
#endif
    mmap:        vloopback_mmap,
};

static struct video_device vloopback_template =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)    
    owner:      THIS_MODULE,
    type:       VID_TYPE_CAPTURE,
#endif
    minor:      -1,    
    name:        "Video Loopback",
    fops:        &fileops_template,
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
    release:    video_device_release,
#endif
};

static int create_pipe(int nr)
{
    int minor_in, minor_out , ret;

    if (debug > LOG_NODEBUG)
        info("Video loopback %d", nr);

    if (dev_offset == -1) {
        minor_in  = minor_out = -1; /* autoassign */
    } else {
        minor_in  = 2 * nr + dev_offset;
        minor_out = 2 * nr + 1 + dev_offset;
    }
    /* allocate space for this pipe */
    loops[nr]= kmalloc(sizeof(struct vloopback_pipe), GFP_KERNEL);

    if (!loops[nr])
        return -ENOMEM;
    /* set up a new video device plus our private area */
    loops[nr]->vloopin = video_device_alloc();

    if (loops[nr]->vloopin == NULL)
        return -ENOMEM;
    *loops[nr]->vloopin = vloopback_template;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)    
    priv_ptr ptr_in = kmalloc(sizeof(struct vloopback_private), 
                                                  GFP_KERNEL);
    if (ptr_in == NULL) {
        kfree(ptr_in);
#else
    loops[nr]->vloopin->vd_private_data = kmalloc(sizeof(struct vloopback_private),
                                                  GFP_KERNEL);
    if (loops[nr]->vloopin->vd_private_data == NULL) {
        kfree(loops[nr]->vloopin);
#endif    
        return -ENOMEM;
    }
    /* repeat for the output device */
    loops[nr]->vloopout = video_device_alloc();

    if (loops[nr]->vloopout == NULL) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
        kfree(ptr_in);
#else        
        kfree(loops[nr]->vloopin->vd_private_data);
#endif        
        kfree(loops[nr]->vloopin);
        return -ENOMEM;
    }
    *loops[nr]->vloopout = vloopback_template;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)    
    priv_ptr ptr_out = kmalloc(sizeof(struct vloopback_private),
                                                   GFP_KERNEL);
    if (ptr_out == NULL) {
        kfree(ptr_in);
        kfree(ptr_out);
#else
    loops[nr]->vloopout->vd_private_data = kmalloc(sizeof(struct vloopback_private),
                                                   GFP_KERNEL);
    if (loops[nr]->vloopout->vd_private_data == NULL) {
        kfree(loops[nr]->vloopin->vd_private_data);
#endif    
        kfree(loops[nr]->vloopin);
        kfree(loops[nr]->vloopout);
        return -ENOMEM;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
    ptr_in->pipenr = nr;
    ptr_out->pipenr = nr;
#else
    ((priv_ptr)loops[nr]->vloopin->vd_private_data)->pipenr = nr;
    ((priv_ptr)loops[nr]->vloopout->vd_private_data)->pipenr = nr;
#endif    
    loops[nr]->invalid_ioctl = 0; /* tibit */
    loops[nr]->buffer = NULL;
    loops[nr]->width = 0;
    loops[nr]->height = 0;
    loops[nr]->palette = 0;
    loops[nr]->frameswrite = 0;
    loops[nr]->framesread = 0;
    loops[nr]->framesdumped = 0;
    loops[nr]->wopen = 0;
    loops[nr]->ropen = 0;
    loops[nr]->frame = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)    
    ptr_in->in = 1;
    ptr_out->in = 0;
    dev_set_drvdata(&loops[nr]->vloopin->dev, ptr_in);
    dev_set_drvdata(&loops[nr]->vloopout->dev, ptr_out);
#else
    ((priv_ptr)loops[nr]->vloopin->vd_private_data)->in = 1;
    ((priv_ptr)loops[nr]->vloopout->vd_private_data)->in = 0;
#endif

    sprintf(loops[nr]->vloopin->name, "Video loopback %d input", nr);
    sprintf(loops[nr]->vloopout->name, "Video loopback %d output", nr);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
    loops[nr]->vloopin->type = 0;
    loops[nr]->vloopout->type = VID_TYPE_CAPTURE;
#endif
    loops[nr]->vloopout->minor = minor_out;
    loops[nr]->vloopin->minor = minor_in;

    init_waitqueue_head(&loops[nr]->wait);

    init_MUTEX(&loops[nr]->lock);

    ret = video_register_device(loops[nr]->vloopin, VFL_TYPE_GRABBER, minor_in);
    
    if ((ret == -1 ) || ( ret == -23 )) {
        info("error registering device %s", loops[nr]->vloopin->name);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)        
        kfree(ptr_in);
        dev_set_drvdata(&loops[nr]->vloopin->dev, NULL);
#else
        kfree(loops[nr]->vloopin->vd_private_data);
#endif        
        kfree(loops[nr]->vloopin);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)        
        kfree(ptr_out);
        dev_set_drvdata(&loops[nr]->vloopout->dev, NULL);
#else   
        kfree(loops[nr]->vloopout->vd_private_data);
#endif
        kfree(loops[nr]->vloopout);
        kfree(loops[nr]);
        loops[nr] = NULL;
        return ret;
    }
    
    ret = video_register_device(loops[nr]->vloopout, VFL_TYPE_GRABBER, minor_out);
    
    if ((ret ==-1) || (ret == -23)) {
        info("error registering device %s", loops[nr]->vloopout->name);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)        
        kfree(ptr_in);
        dev_set_drvdata(&loops[nr]->vloopin->dev, NULL);
#else
        kfree(loops[nr]->vloopin->vd_private_data);
#endif        
        video_unregister_device(loops[nr]->vloopin);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)        
        kfree(ptr_out);
        dev_set_drvdata(&loops[nr]->vloopout->dev, NULL);
#else   
        kfree(loops[nr]->vloopout->vd_private_data);
#endif        
        kfree(loops[nr]->vloopout);
        kfree(loops[nr]);
        loops[nr] = NULL;
        return ret;
    }
    
    loops[nr]->ioctldata = kmalloc(1024, GFP_KERNEL);
    loops[nr]->ioctlretdata = kmalloc(1024, GFP_KERNEL);
    return 0;
}


/****************************************************************************
 *    init stuff
 ****************************************************************************/


MODULE_AUTHOR("J.B. Vreeken (pe1rxq@amsat.org)");
MODULE_DESCRIPTION("Video4linux loopback device.");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
module_param(pipes, int, 000);
#else
MODULE_PARM(pipes, "i");
#endif

MODULE_PARM_DESC(pipes, " Nr of pipes to create (each pipe uses two video devices)");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
module_param(spares, int, 000);
#else
MODULE_PARM(spares, "i");
#endif

MODULE_PARM_DESC(spares, " Nr of spare pipes that should be created");

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
module_param(num_buffers, int, 000);
#else
MODULE_PARM(num_buffers, "i");
#endif

MODULE_PARM_DESC(num_buffers, " Prefered numbers of internal buffers to map (default 2)");


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
module_param(dev_offset, int, 000);
#else
MODULE_PARM(dev_offset_param, "i");
#endif

MODULE_PARM_DESC(dev_offset, " Prefered offset for video device numbers");


#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
module_param(debug, int, 000);
#else
MODULE_PARM(debug_param, "i");
#endif

MODULE_PARM_DESC(debug, " Enable module debug level 0-3 (by default 0)");

MODULE_LICENSE("GPL");
MODULE_VERSION( VLOOPBACK_VERSION );

static int __init vloopback_init(void)
{
    int i, ret;

    info("video4linux loopback driver v"VLOOPBACK_VERSION);

    if (pipes == -1) 
        pipes = 1;

    if (pipes > MAX_PIPES) {
        pipes = MAX_PIPES;
        info("Nr of pipes is limited to: %d", MAX_PIPES);
    }

    if (num_buffers < N_BUFFS) {
        num_buffers = N_BUFFS;
        info("Nr of buffer set to default value %d", N_BUFFS);
    }

    for (i = 0; i < pipes; i++) {
        
        ret = create_pipe(i);

        if (ret == 0) {
            info("Loopback %d registered, input: video%d,"
                 " output: video%d",
                 i, loops[i]->vloopin->minor,
                 loops[i]->vloopout->minor);
            info("Loopback %d , Using %d buffers", i, num_buffers);
            nr_o_pipes = i + 1;
        } else {
            return ret;
        }
    }
    return 0;
}

static void __exit cleanup_vloopback_module(void)
{
    int i;

    info("Unregistering video4linux loopback devices");

    for (i = 0; i < nr_o_pipes; i++) {
        if (loops[i]) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)            
            priv_ptr ptr_in = (priv_ptr)dev_get_drvdata(&loops[i]->vloopin->dev);
            kfree(ptr_in);
            dev_set_drvdata(&loops[i]->vloopin->dev, NULL);
#else
            kfree(loops[i]->vloopin->vd_private_data);
#endif            
            video_unregister_device(loops[i]->vloopin);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)            
            priv_ptr ptr_out = (priv_ptr)dev_get_drvdata(&loops[i]->vloopout->dev);
            kfree(ptr_out);
            dev_set_drvdata(&loops[i]->vloopout->dev, NULL);
#else
            kfree(loops[i]->vloopout->vd_private_data);
#endif            
            video_unregister_device(loops[i]->vloopout);
            
            if (loops[i]->buffer) 
                rvfree(loops[i]->buffer, loops[i]->buflength * num_buffers);

            kfree(loops[i]->ioctldata);
            kfree(loops[i]->ioctlretdata);
            kfree(loops[i]);
        }
    }        
}

module_init(vloopback_init);
module_exit(cleanup_vloopback_module);
