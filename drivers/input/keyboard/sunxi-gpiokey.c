#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h> 
#include <linux/clk.h>
#include <mach/sys_config.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>

static volatile u32 key_val;
static struct gpio_config	power_key_io, power_key_ctl;
static struct mutex		gk_mutex;
static struct input_dev *sunxigk_dev;
static int suspend_flg = 0, powerkey_is_down = 0;
static unsigned long poweroff_delay = 0;

#ifdef CONFIG_PM
static struct dev_pm_domain keyboard_pm_domain;
#endif

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 255;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);

static u32 default_delay = (35 * HZ); /* MS to delay */
module_param_named(default_delay, default_delay, int, 0644);

static u32 force_off_delay = (5 * HZ); /* MS to delay force poweroff */
module_param_named(force_off_delay, force_off_delay, int, 0644);

static void fn_timer_poweroff(unsigned long);
static DEFINE_TIMER(timer_poweroff, fn_timer_poweroff, 0, 0);

static void do_poweroff(void)
{
	__gpio_set_value(power_key_ctl.gpio, 0);
}

static void fn_timer_poweroff(unsigned long data)
{
	/* keep pressing down > 5s */
	if(powerkey_is_down && 
		time_after(jiffies, powerkey_is_down + force_off_delay)) 
	{
		pr_info("Force sucide NOW...\n");
		do_poweroff();
	}
	/* power-key released */
	if(time_before(jiffies, poweroff_delay)) {
		pr_info(".");
		mod_timer(&timer_poweroff, jiffies + HZ);
	} else {
		pr_info("On timer sucide poweroff...\n");
		do_poweroff();
	}
}

static void trigger_poweroff_timer(int delay)
{
	poweroff_delay = jiffies + delay;
	pr_info("start sucide timer[%d]...\n", delay);
	mod_timer(&timer_poweroff, jiffies + HZ);
}

static int poweroff_set(const char *val, const struct kernel_param *kp)
{
	int n = 0, ret;

	ret = kstrtoint(val, 10, &n);
	if (ret != 0 || n < 1 || n > 120) {
		pr_err("%s: err fmt: %s; <1-120>\n", __func__, val);
		return -EINVAL;
	}

	trigger_poweroff_timer((n * HZ));

	return param_set_int(val, kp);
}

static const struct kernel_param_ops param_ops = {
	.set	= poweroff_set,
	.get	= param_get_int,
};

static int poweroff_now;
module_param_cb(poweroff, &param_ops, &poweroff_now, 0664);

static irqreturn_t sunxi_isr_gk(int irq, void *dummy)
{
	dprintk(DEBUG_INT, "sunxi gpio key int \n");
	mutex_lock(&gk_mutex);
	if(__gpio_get_value(power_key_io.gpio)){
		if (suspend_flg) {
			input_report_key(sunxigk_dev, KEY_POWER, 1);
			input_sync(sunxigk_dev);
			dprintk(DEBUG_INT, "report data: power key down \n");
			suspend_flg = 0;
		}
		input_report_key(sunxigk_dev, KEY_POWER, 0);
		input_sync(sunxigk_dev);
		dprintk(DEBUG_INT, "report data: power key up \n");
		/* setup sucide timer */
		if(powerkey_is_down) {
			trigger_poweroff_timer(default_delay);
		}
		powerkey_is_down = 0;
	}else{
		input_report_key(sunxigk_dev, KEY_POWER, 1);
		input_sync(sunxigk_dev);
		dprintk(DEBUG_INT, "report data: power key down \n");
		suspend_flg = 0;
		if(!powerkey_is_down) {
			powerkey_is_down = jiffies;
		}
	}
	mutex_unlock(&gk_mutex);
	return IRQ_HANDLED;
}

static int sunxi_keyboard_suspend(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "sunxi gpio key suspend \n");
	mutex_lock(&gk_mutex);
	suspend_flg = 1;
	mutex_unlock(&gk_mutex);
	return 0;
}
static int sunxi_keyboard_resume(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "sunxi gpio key resume \n");
	return 0;
}

static int sunxigk_script_init(void)
{
	int re;
	script_item_u	val;
	script_item_value_type_e  type;

	type = script_get_item("gpio_power_key", "key_used", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: key para not found, used default para. \n", __func__);
		return -1;
	}

	if(1 == val.val){
		type = script_get_item("gpio_power_key", "key_io", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){
			pr_err("%s: get key io err! \n", __func__);
			return -1;
		}
		power_key_io = val.gpio;
		if(gpio_request(power_key_io.gpio, "powerkey") == 0) {
			dprintk(DEBUG_INT,"%s power key io number = %d.\n", __func__, power_key_io.gpio);
		} else {
			pr_err("%s power key io req %d failed.\n", __func__, power_key_io.gpio);
			return -1;
		}

		/* get the power control GPIO: PL10(H3V1) */
		type = script_get_item("gpio_power_key", "key_ctl", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){
			pr_err("%s: get key io err! \n", __func__);
			return -1;
		}
		power_key_ctl = val.gpio;
		if(gpio_request(power_key_ctl.gpio, "powerctl") == 0) {
			dprintk(DEBUG_INT,"%s power control io number = %d.\n", __func__, power_key_ctl.gpio);
		} else {
			pr_err("%s power control io %d req failed\n", __func__, power_key_ctl.gpio);
			gpio_free(power_key_io.gpio);
			return -1;
		}
	}else{
		dprintk(DEBUG_INT,"sunxi io power key no used.\n");
		return -1;
	}

	return 0;
}

static int __init sunxigk_init(void)
{
	int err =0;
	int irq_number = 0;
	
	dprintk(DEBUG_INIT, "sunxigk_init \n");
	if(sunxigk_script_init()){
		err = -EFAULT;
		goto fail0;
	}
	
	sunxigk_dev = input_allocate_device();
	if (!sunxigk_dev) {
		pr_err( "sunxigk: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxigk_dev->name = "sunxi-gpiokey";  
	sunxigk_dev->phys = "sunxikbd/input0"; 
	sunxigk_dev->id.bustype = BUS_HOST;      
	sunxigk_dev->id.vendor = 0x0001;
	sunxigk_dev->id.product = 0x0001;
	sunxigk_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	sunxigk_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	printk(KERN_DEBUG "REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	sunxigk_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif
	set_bit(KEY_POWER, sunxigk_dev->keybit);
	mutex_init(&gk_mutex);

	irq_number = gpio_to_irq(power_key_io.gpio);

	err = input_register_device(sunxigk_dev);
	if (err)
		goto fail2;
#ifdef CONFIG_PM
	keyboard_pm_domain.ops.suspend = sunxi_keyboard_suspend;
	keyboard_pm_domain.ops.resume = sunxi_keyboard_resume;
	sunxigk_dev->dev.pm_domain = &keyboard_pm_domain;	
#endif
	if (devm_request_irq(&sunxigk_dev->dev, irq_number, sunxi_isr_gk, 
			       IRQF_TRIGGER_FALLING |IRQF_TRIGGER_RISING |IRQF_NO_SUSPEND, "gk_EINT", NULL)) {
		err = -EBUSY;
		pr_err(KERN_DEBUG "request irq failure. \n");
		goto fail3;
	}
	dprintk(DEBUG_INIT, "sunxikbd_init end\n");

	return 0;

fail3:	
	input_unregister_device(sunxigk_dev);
fail2:	
	input_free_device(sunxigk_dev);
fail1:
	gpio_free(power_key_io.gpio);
	gpio_free(power_key_ctl.gpio);
fail0:
	printk(KERN_DEBUG "sunxikbd_init failed. \n");
	return err;
}

static void __exit sunxigk_exit(void)
{	
	del_timer_sync(&timer_poweroff);
	devm_free_irq(&sunxigk_dev->dev, gpio_to_irq(power_key_io.gpio), NULL);
	gpio_free(power_key_io.gpio);
	gpio_free(power_key_ctl.gpio);
	input_unregister_device(sunxigk_dev);
}

module_init(sunxigk_init);
module_exit(sunxigk_exit);

MODULE_AUTHOR(" <@>");
MODULE_DESCRIPTION("sunxi gpio power key driver");
MODULE_LICENSE("GPL");


