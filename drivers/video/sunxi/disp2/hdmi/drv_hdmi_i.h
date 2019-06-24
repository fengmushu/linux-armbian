
#ifndef  _DRV_HDMI_I_H_
#define  _DRV_HDMI_I_H_
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/memory.h>
#include <asm/unistd.h>
#include "asm-generic/int-ll64.h"
#include "linux/kernel.h"
#include "linux/mm.h"
#include "linux/semaphore.h"
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()??kthread_run()
#include <linux/err.h> //IS_ERR()??PTR_ERR()
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/module.h>
#include <mach/sys_config.h>
#include <linux/switch.h>
#include <mach/platform.h>

#include <video/sunxi_display2.h>
#include <video/drv_hdmi.h>

#define HDMI_LOG_INFO (0x1<<0)
#define HDMI_LOG_ERRO (0x1<<1)
#define HDMI_LOG_WARN (0x1<<2)
#define HDMI_LOG_DBGX (0x1<<3)
#define HDMI_LOG_TRAC (0x1<<4)

extern __u32 hdmi_print;
extern __u32 rgb_only;
extern __u32 hdmi_hpd_mask;//0x10: force unplug; 0x11: force plug

#define HDMI_LOG_DEFAULT (HDMI_LOG_INFO | HDMI_LOG_ERRO | HDMI_LOG_WARN | HDMI_LOG_TRAC)

#define OSAL_PRINTF(msg...) do{printk(KERN_WARNING "[HDMI] ");printk(msg);}while(0)
#define __inf(msg...)       do{if(hdmi_print & HDMI_LOG_INFO){printk(KERN_WARNING "[HDMI] %s,%d: ", __func__, __LINE__);printk(msg);}}while(0)
#define __msg(msg...)       do{if(hdmi_print & HDMI_LOG_ERRO){printk(KERN_WARNING "[HDMI] %s,line:%d: ", __FILE__, __LINE__);printk(msg);}}while(0)
#define __wrn(msg...)       do{if(hdmi_print & HDMI_LOG_DBGX){printk(KERN_WARNING "[HDMI WRN] %s,line:%d: ",__FILE__,__LINE__);printk(msg);}}while(0)
#define __here__            do{if(hdmi_print & HDMI_LOG_TRAC){printk(KERN_WARNING "[HDMI TRC] %s,%s,line:%d\n",__FILE__, __func__, __LINE__);}}while(0)


__s32 Hdmi_init(void);
__s32 Hdmi_exit(void);
__u32 IsHdcpDriver(void);
__u32 Hdmi_hdcp_enable(__u32 hdcp_en);
__s32 Hdmi_open(void);
__s32 Hdmi_close(void);
__s32 Hdmi_set_display_mode(disp_tv_mode mode);
__s32 Hdmi_mode_support(disp_tv_mode mode);
__s32 Hdmi_get_HPD_status(void);
__s32 Hdmi_Audio_Enable(__u8 mode, __u8 channel);
__s32 Hdmi_Set_Audio_Para(hdmi_audio_t * audio_para);
__s32 Hdmi_Is_Playback(void);
__s32 Hdmi_set_pll(__u32 pll, __u32 clk);
__s32 Hdmi_dvi_enable(__u32 mode);
__s32 Hdmi_dvi_support(void);
__s32 Hdmi_get_hdcp_enable(void);
__s32 Hdmi_get_video_timming_info(disp_video_timings **video_info);
int Hdmi_get_video_info_index(u32 mode_id);
__s32 Hdmi_get_input_csc(void);

extern __s32 hdmi_i2c_add_driver(void);
extern __s32 hdmi_i2c_del_driver(void);

#define sys_get_wvalue(n)   (*((volatile __u32 *)(n)))          /* word input */
#define sys_put_wvalue(n,c) (*((volatile __u32 *)(n))  = (c))   /* word output */

struct disp_hdmi_mode
{
	disp_tv_mode mode;
	int hdmi_mode;//vic
};

#endif
