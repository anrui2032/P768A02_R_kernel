/*
 * TEE driver for goodix fingerprint sensor
 * Copyright (C) 2016 Goodix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#define pr_fmt(fmt)		KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>

#if defined(CONFIG_DRM_PANEL_NOTIFIER) || defined(CONFIG_FB)
#include <drm/drm_panel.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include <linux/pm_wakeup.h>
#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

#define VER_MAJOR   1
#define VER_MINOR   2
#define PATCH_LEVEL 11

#define WAKELOCK_HOLD_TIME 500 /* in ms */

#define GF_SPIDEV_NAME     "goodix,fingerprint"
/*device name after register in character*/
#define GF_DEV_NAME            "goodix_fp"
#define	GF_INPUT_NAME	    "qwerty"	/*"goodix_fp" */

#define	CHRD_DRIVER_NAME	"goodix_fp_spi"
#define	CLASS_NAME		    "goodix_fp"


#define USER_BUF_SIZE 4
#define N_SPI_MINORS		32	/* ... up to 256 */
static int SPIDEV_MAJOR;

extern int is_goodix_fp_gf3658(void);
#define TEMP_SET_FINGERID
#ifdef TEMP_SET_FINGERID
/*temp return 1*/
int is_goodix_fp_gf3658(void)
{
	pr_debug("[%s][%d] temp to set the fingerpritnid gf3658\n", __func__, __LINE__);
	return 1;
}
#endif
static DECLARE_BITMAP(minors, N_SPI_MINORS);
static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);
static struct wakeup_source fp_wakelock;
static struct gf_dev gf;
static struct proc_dir_entry *dir;
static struct proc_dir_entry *refresh;

static struct gf_key_map maps[] = {
	{ EV_KEY, GF_KEY_INPUT_HOME },
	{ EV_KEY, GF_KEY_INPUT_MENU },
	{ EV_KEY, GF_KEY_INPUT_BACK },
	{ EV_KEY, GF_KEY_INPUT_POWER },
#if defined(SUPPORT_NAV_EVENT)
	{ EV_KEY, GF_NAV_INPUT_UP },
	{ EV_KEY, GF_NAV_INPUT_DOWN },
	{ EV_KEY, GF_NAV_INPUT_RIGHT },
	{ EV_KEY, GF_NAV_INPUT_LEFT },
	{ EV_KEY, GF_KEY_INPUT_CAMERA },
	{ EV_KEY, GF_NAV_INPUT_CLICK },
	{ EV_KEY, GF_NAV_INPUT_DOUBLE_CLICK },
	{ EV_KEY, GF_NAV_INPUT_LONG_PRESS },
	{ EV_KEY, GF_NAV_INPUT_HEAVY },
#endif
};

#ifdef CONFIG_DRM_PANEL_NOTIFIER
static struct drm_panel *active_panel = NULL;
static int goodix_drm_register(struct device *dev)
{
	int retval = -1;
	int i;
	int count;
	struct device_node *node;
	struct drm_panel *panel;

	count = of_count_phandle_with_args(dev->of_node, "panel", NULL);
	if (count <= 0)
		return -ENODEV;

	for (i = 0; i < count; i++) {
		node = of_parse_phandle(dev->of_node, "panel", i);
		if (node != NULL)
			pr_err("node = %s\n", node->name);

		panel = of_drm_find_panel(node);
		if (!IS_ERR(panel)) {
			of_node_put(node);
			active_panel = panel;
			return 0;
		}

		if (PTR_ERR(panel) == -ENODEV) {
			pr_err("no device!\n");
			retval = -ENODEV;
		} else if (PTR_ERR(panel) == -EPROBE_DEFER) {
			pr_err("device has not been probed yet\n");
			retval = -EPROBE_DEFER;
		}

		of_node_put(node);
	}

	return retval;
}
#endif

static void gf_enable_irq(struct gf_dev *gf_dev)
{
	if (gf_dev->irq_enabled) {
		pr_warn("IRQ has been enabled.\n");
	} else {
		enable_irq(gf_dev->irq);
		gf_dev->irq_enabled = 1;
	}
}

static void gf_disable_irq(struct gf_dev *gf_dev)
{
	if (gf_dev->irq_enabled) {
		gf_dev->irq_enabled = 0;
		disable_irq(gf_dev->irq);
	} else {
		pr_warn("IRQ has been disabled.\n");
	}
}

#ifdef AP_CONTROL_CLK
static long spi_clk_max_rate(struct clk *clk, unsigned long rate)
{
	long lowest_available, nearest_low, step_size, cur;
	long step_direction = -1;
	long guess = rate;
	int max_steps = 10;

	cur = clk_round_rate(clk, rate);
	if (cur == rate)
		return rate;

	/* if we got here then: cur > rate */
	lowest_available = clk_round_rate(clk, 0);
	if (lowest_available > rate)
		return -EINVAL;

	step_size = (rate - lowest_available) >> 1;
	nearest_low = lowest_available;

	while (max_steps-- && step_size) {
		guess += step_size * step_direction;
		cur = clk_round_rate(clk, guess);

		if ((cur < rate) && (cur > nearest_low))
			nearest_low = cur;
		/*
		 * if we stepped too far, then start stepping in the other
		 * direction with half the step size
		 */
		if (((cur > rate) && (step_direction > 0))
				|| ((cur < rate) && (step_direction < 0))) {
			step_direction = -step_direction;
			step_size >>= 1;
		}
	}
	return nearest_low;
}

static void spi_clock_set(struct gf_dev *gf_dev, int speed)
{
	long rate;
	int rc;

	rate = spi_clk_max_rate(gf_dev->core_clk, speed);
	if (rate < 0) {
		pr_info("%s: no match found for requested clock frequency:%d",
				__func__, speed);
		return;
	}

	rc = clk_set_rate(gf_dev->core_clk, rate);
}

static int gfspi_ioctl_clk_init(struct gf_dev *data)
{
	pr_debug("%s: enter\n", __func__);

	data->clk_enabled = 0;
	data->core_clk = clk_get(&data->spi->dev, "core_clk");
	if (IS_ERR_OR_NULL(data->core_clk)) {
		pr_err("%s: fail to get core_clk\n", __func__);
		return -EPERM;
	}
	data->iface_clk = clk_get(&data->spi->dev, "iface_clk");
	if (IS_ERR_OR_NULL(data->iface_clk)) {
		pr_err("%s: fail to get iface_clk\n", __func__);
		clk_put(data->core_clk);
		data->core_clk = NULL;
		return -ENOENT;
	}
	return 0;
}

static int gfspi_ioctl_clk_enable(struct gf_dev *data)
{
	int err;

	pr_debug("%s: enter\n", __func__);

	if (data->clk_enabled)
		return 0;

	err = clk_prepare_enable(data->core_clk);
	if (err) {
		pr_err("%s: fail to enable core_clk\n", __func__);
		return -EPERM;
	}

	err = clk_prepare_enable(data->iface_clk);
	if (err) {
		pr_err("%s: fail to enable iface_clk\n", __func__);
		clk_disable_unprepare(data->core_clk);
		return -ENOENT;
	}

	data->clk_enabled = 1;

	return 0;
}

static int gfspi_ioctl_clk_disable(struct gf_dev *data)
{
	pr_debug("%s: enter\n", __func__);

	if (!data->clk_enabled)
		return 0;

	clk_disable_unprepare(data->core_clk);
	clk_disable_unprepare(data->iface_clk);
	data->clk_enabled = 0;

	return 0;
}

static int gfspi_ioctl_clk_uninit(struct gf_dev *data)
{
	pr_debug("%s: enter\n", __func__);

	if (data->clk_enabled)
		gfspi_ioctl_clk_disable(data);

	if (!IS_ERR_OR_NULL(data->core_clk)) {
		clk_put(data->core_clk);
		data->core_clk = NULL;
	}

	if (!IS_ERR_OR_NULL(data->iface_clk)) {
		clk_put(data->iface_clk);
		data->iface_clk = NULL;
	}

	return 0;
}
#endif

static struct platform_device *gf_platform_device = NULL;
static void gf_report_uevent(struct gf_dev *gf_dev, char *str)
{

	char *envp[2];

	pr_info("%s enter!\n", __func__);

	envp[0] = str;
	envp[1] = NULL;

	if (gf_platform_device) {
		kobject_uevent_env(&(gf_platform_device->dev.kobj), KOBJ_CHANGE, envp);
	} else {
		pr_info("%s gf_platform_device is null!\n", __func__);
	}
}

static void nav_event_input(struct gf_dev *gf_dev, gf_nav_event_t nav_event)
{
	uint32_t nav_input = 0;

	pr_info("%s nav_event = %d\n", __func__, nav_event);
	switch (nav_event) {
	case GF_NAV_FINGER_DOWN:
		pr_info("%s nav finger down\n", __func__);
		break;

	case GF_NAV_FINGER_UP:
		pr_info("%s nav finger up\n", __func__);
		break;

	case GF_NAV_DOWN:
		nav_input = GF_NAV_INPUT_DOWN;
		gf_report_uevent(gf_dev, FP_NAV_DOWN);
		pr_info("%s nav down\n", __func__);
		break;

	case GF_NAV_UP:
		nav_input = GF_NAV_INPUT_UP;
		gf_report_uevent(gf_dev, FP_NAV_UP);
		pr_info("%s nav up\n", __func__);
		break;

	case GF_NAV_LEFT:
		nav_input = GF_NAV_INPUT_LEFT;
		gf_report_uevent(gf_dev, FP_NAV_LEFT);
		pr_info("%s nav left\n", __func__);
		break;

	case GF_NAV_RIGHT:
		nav_input = GF_NAV_INPUT_RIGHT;
		gf_report_uevent(gf_dev, FP_NAV_RIGHT);
		pr_info("%s nav right\n", __func__);
		break;

	case GF_NAV_CLICK:
		nav_input = GF_NAV_INPUT_CLICK;
		pr_info("%s nav click\n", __func__);
		break;

	case GF_NAV_HEAVY:
		nav_input = GF_NAV_INPUT_HEAVY;
		pr_info("%s nav heavy\n", __func__);
		break;

	case GF_NAV_LONG_PRESS:
		nav_input = GF_NAV_INPUT_LONG_PRESS;
		pr_info("%s nav long press\n", __func__);
		break;

	case GF_NAV_DOUBLE_CLICK:
		nav_input = GF_NAV_INPUT_DOUBLE_CLICK;
		pr_info("%s nav double click\n", __func__);
		break;

	default:
		pr_warn("%s unknown nav event: %d\n", __func__, nav_event);
		break;
	}
	/*if ((nav_event != GF_NAV_FINGER_DOWN) &&
			(nav_event != GF_NAV_FINGER_UP)) {
		input_report_key(gf_dev->input, nav_input, 1);
		input_sync(gf_dev->input);
		input_report_key(gf_dev->input, nav_input, 0);
		input_sync(gf_dev->input);
	}*/
}

static ssize_t zte_fp_nav_event(struct file *file,
		const char __user *buffer, size_t count, loff_t *pos)
{
	int ret;
	unsigned int keycode;
	char data_buf[USER_BUF_SIZE] = {0};
	struct gf_dev *gf_dev = &gf;

	pr_info("%s: enter\n", __func__);
	ret = copy_from_user(data_buf, buffer, count);
	if (ret) {
		pr_err("%s: fail to copy buf\n", __func__);
		return -EINVAL;
	}

	keycode = data_buf[0];
	pr_info("%s: keycode=%d\n", __func__, keycode);
	nav_event_input(gf_dev, keycode);
	return count;
}

static const struct file_operations proc_fp_nav_event = {
	.owner = THIS_MODULE,
	.write = zte_fp_nav_event,
};

int gf_uevent_init(void)
{
	int ret = 0;

	pr_info("%s", __func__);
	dir = proc_mkdir("fingerprint", NULL);
	refresh = proc_create("goodix_nav_event", 0664, dir, &proc_fp_nav_event);
	if (refresh == NULL)
		pr_err("proc_create fpc_nav_event failed!\n");

	gf_platform_device = platform_device_alloc("zte_fingerprint", -1);
	if (!gf_platform_device) {
		pr_err("%s failed to allocate platform device", __func__);
		ret = -ENOMEM;
		goto alloc_failed;
	}

	ret = platform_device_add(gf_platform_device);
	if (ret < 0) {
		pr_err("%s failed to add platform device ret=%d", __func__, ret);
		goto register_failed;
	}

	return 0;

register_failed:
	gf_platform_device->dev.release(&(gf_platform_device->dev));
alloc_failed:
	remove_proc_entry("goodix_nav_event", dir);
	remove_proc_entry("fingerprint", NULL);
	return ret;
}

void gf_uevent_exit(void)
{
	remove_proc_entry("goodix_nav_event", dir);
	remove_proc_entry("fingerprint", NULL);
	gf_platform_device->dev.release(&(gf_platform_device->dev));
	platform_device_unregister(gf_platform_device);
}

static irqreturn_t gf_irq(int irq, void *handle)
{
#if defined(GF_NETLINK_ENABLE)
	char msg = GF_NET_EVENT_IRQ;

	__pm_wakeup_event(&fp_wakelock, msecs_to_jiffies(WAKELOCK_HOLD_TIME));
	pr_info("gf_kernel, sendnlmsg\n");
	sendnlmsg(&msg);
#elif defined(GF_FASYNC)
	struct gf_dev *gf_dev = &gf;

	if (gf_dev->async)
		kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
#endif

	return IRQ_HANDLED;
}

static int irq_setup(struct gf_dev *gf_dev)
{
	int status;

	gf_dev->irq = gf_irq_num(gf_dev);
	if (gf_dev->irq  > 0) {
		pr_err("the irq(%d) has been registered, so free it\n",  gf_dev->irq);
		free_irq(gf_dev->irq,  gf_dev);
	}
	status = request_threaded_irq(gf_dev->irq, NULL, gf_irq,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT,
			"gf", gf_dev);

	if (status) {
		pr_err("failed to request IRQ:%d\n", gf_dev->irq);
		return status;
	}
	enable_irq_wake(gf_dev->irq);
	gf_dev->irq_enabled = 1;

	return status;
}

static void irq_cleanup(struct gf_dev *gf_dev)
{
	gf_dev->irq_enabled = 0;
	disable_irq(gf_dev->irq);
	disable_irq_wake(gf_dev->irq);
	free_irq(gf_dev->irq, gf_dev);
}

static void gf_kernel_key_input(struct gf_dev *gf_dev, struct gf_key *gf_key)
{
	uint32_t key_input = 0;

	if (gf_key->key == GF_KEY_HOME) {
		key_input = GF_KEY_INPUT_HOME;
	} else if (gf_key->key == GF_KEY_POWER) {
		key_input = GF_KEY_INPUT_POWER;
	} else if (gf_key->key == GF_KEY_CAMERA) {
		key_input = GF_KEY_INPUT_CAMERA;
	} else {
		/* add special key define */
		key_input = gf_key->key;
	}
	pr_info("%s: received key event[%d], key=%d, value=%d\n",
			__func__, key_input, gf_key->key, gf_key->value);

	if ((GF_KEY_POWER == gf_key->key || GF_KEY_CAMERA == gf_key->key)
			&& (gf_key->value == 1)) {
		input_report_key(gf_dev->input, key_input, 1);
		input_sync(gf_dev->input);
		input_report_key(gf_dev->input, key_input, 0);
		input_sync(gf_dev->input);
	}

	if (gf_key->key == GF_KEY_HOME) {
		input_report_key(gf_dev->input, key_input, gf_key->value);
		input_sync(gf_dev->input);
	}
}

static long gf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct gf_dev *gf_dev = &gf;
	struct gf_key gf_key;
#if defined(SUPPORT_NAV_EVENT)
	gf_nav_event_t nav_event = GF_NAV_NONE;
#endif
	int retval = 0;
	u8 netlink_route = NETLINK_TEST;
	struct gf_ioc_chip_info info;

	if (_IOC_TYPE(cmd) != GF_IOC_MAGIC)
		return -ENODEV;

	if (_IOC_DIR(cmd) & _IOC_READ)
		retval = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		retval = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (retval)
		return -EFAULT;

	switch (cmd) {
	case GF_IOC_INIT:
		pr_debug("%s GF_IOC_INIT\n", __func__);
		if (copy_to_user((void __user *)arg, (void *)&netlink_route, sizeof(u8))) {
			pr_err("GF_IOC_INIT failed\n");
			retval = -EFAULT;
			break;
		}
		break;

	case GF_IOC_EXIT:
		pr_debug("%s GF_IOC_EXIT\n", __func__);
		break;

	case GF_IOC_DISABLE_IRQ:
		pr_debug("%s GF_IOC_DISABEL_IRQ\n", __func__);
		gf_disable_irq(gf_dev);
		break;

	case GF_IOC_ENABLE_IRQ:
		pr_debug("%s GF_IOC_ENABLE_IRQ\n", __func__);
		gf_enable_irq(gf_dev);
		break;

	case GF_IOC_RESET:
		pr_debug("%s GF_IOC_RESET\n", __func__);
		gf_hw_reset(gf_dev, 3);
		break;

	case GF_IOC_INPUT_KEY_EVENT:
		if (copy_from_user(&gf_key, (void __user *)arg, sizeof(struct gf_key))) {
			pr_err("failed to copy input key event from user to kernel\n");
			retval = -EFAULT;
			break;
		}

		gf_kernel_key_input(gf_dev, &gf_key);
		break;

#if defined(SUPPORT_NAV_EVENT)
	case GF_IOC_NAV_EVENT:
		pr_debug("%s GF_IOC_NAV_EVENT\n", __func__);
		if (copy_from_user(&nav_event, (void __user *)arg, sizeof(gf_nav_event_t))) {
			pr_err("failed to copy nav event from user to kernel\n");
			retval = -EFAULT;
			break;
		}

		nav_event_input(gf_dev, nav_event);
		break;
#endif

	case GF_IOC_ENABLE_SPI_CLK:
		pr_debug("%s GF_IOC_ENABLE_SPI_CLK\n", __func__);
#ifdef AP_CONTROL_CLK
		gfspi_ioctl_clk_enable(gf_dev);
#else
		pr_debug("Doesn't support control clock.\n");
#endif
		break;

	case GF_IOC_DISABLE_SPI_CLK:
		pr_debug("%s GF_IOC_DISABLE_SPI_CLK\n", __func__);
#ifdef AP_CONTROL_CLK
		gfspi_ioctl_clk_disable(gf_dev);
#else
		pr_debug("Doesn't support control clock\n");
#endif
		break;

	case GF_IOC_ENABLE_POWER:
		pr_debug("%s GF_IOC_ENABLE_POWER\n", __func__);
		gf_power_on(gf_dev);
		break;

	case GF_IOC_DISABLE_POWER:
		pr_debug("%s GF_IOC_DISABLE_POWER\n", __func__);
		if (gpio_is_valid(gf_dev->reset_gpio)) {
			gpio_set_value(gf_dev->reset_gpio, 0);
			pr_info("before GF_IOC_DISABLE_POWER reset gpio set low success!\n");
		}
		msleep(20);
		gf_power_off(gf_dev);
		break;

	case GF_IOC_ENTER_SLEEP_MODE:
		pr_debug("%s GF_IOC_ENTER_SLEEP_MODE\n", __func__);
		break;

	case GF_IOC_GET_FW_INFO:
		pr_debug("%s GF_IOC_GET_FW_INFO\n", __func__);
		break;

	case GF_IOC_REMOVE:
		pr_debug("%s GF_IOC_REMOVE\n", __func__);
		irq_cleanup(gf_dev);
		gf_cleanup(gf_dev);
		break;

	case GF_IOC_CHIP_INFO:
		pr_debug("%s GF_IOC_CHIP_INFO\n", __func__);
		if (copy_from_user(&info, (void __user *)arg, sizeof(struct gf_ioc_chip_info))) {
			retval = -EFAULT;
			break;
		}
		pr_info("vendor_id : 0x%x\n", info.vendor_id);
		pr_info("mode : 0x%x\n", info.mode);
		pr_info("operation: 0x%x\n", info.operation);
		break;

	default:
		pr_warn("unsupport cmd:0x%x\n", cmd);
		break;
	}

	return retval;
}

#ifdef CONFIG_COMPAT
static long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return gf_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif /*CONFIG_COMPAT*/


static int gf_open(struct inode *inode, struct file *filp)
{
	struct gf_dev *gf_dev = &gf;
	int status = -ENXIO;

	mutex_lock(&device_list_lock);

	list_for_each_entry(gf_dev, &device_list, device_entry) {
		if (gf_dev->devt == inode->i_rdev) {
			pr_info("gf_dev Found\n");
			status = 0;
			break;
		}
	}

	if (status == 0) {
		if (status == 0) {
			gf_dev->users++;
			filp->private_data = gf_dev;
			nonseekable_open(inode, filp);
			pr_info("Succeed to open gf device. irq = %d\n",
					gf_dev->irq);
			if (gf_dev->users == 1) {
				status = gf_parse_dts(gf_dev);
				if (status)
					goto err_parse_dt;

				status = irq_setup(gf_dev);
				if (status)
					goto err_irq;
			}
		/*	gf_hw_reset(gf_dev, 3);  */
			gf_dev->device_available = 1;
		}
	} else {
		pr_info("No device for minor %d\n", iminor(inode));
	}
	mutex_unlock(&device_list_lock);

	return status;
err_irq:
	gf_cleanup(gf_dev);
err_parse_dt:
	mutex_unlock(&device_list_lock);
	return status;
}

#ifdef GF_FASYNC
static int gf_fasync(int fd, struct file *filp, int mode)
{
	struct gf_dev *gf_dev = filp->private_data;
	int ret;

	ret = fasync_helper(fd, filp, mode, &gf_dev->async);
	pr_info("ret = %d\n", ret);
	return ret;
}
#endif

static int gf_release(struct inode *inode, struct file *filp)
{
	struct gf_dev *gf_dev = &gf;
	int status = 0;

	mutex_lock(&device_list_lock);
	gf_dev = filp->private_data;
	filp->private_data = NULL;

	/*last close?? */
	gf_dev->users--;
	if (!gf_dev->users) {

		pr_info("disble_irq. irq = %d\n", gf_dev->irq);
		gf_disable_irq(gf_dev);
		if (gpio_is_valid(gf_dev->reset_gpio)) {
			gpio_set_value(gf_dev->reset_gpio, 0);
			pr_info("gf_release reset gpio set low success!\n");
		}
		msleep(20);
		/*power off the sensor*/
		gf_dev->device_available = 0;
		gf_power_off(gf_dev);
		gf_cleanup(gf_dev);
	}
	mutex_unlock(&device_list_lock);
	return status;
}

static const struct file_operations gf_fops = {
	.owner = THIS_MODULE,
	/* REVISIT switch to aio primitives, so that userspace
	 * gets more complete API coverage.  It'll simplify things
	 * too, except for the locking.
	 */
	.unlocked_ioctl = gf_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = gf_compat_ioctl,
#endif /*CONFIG_COMPAT*/
	.open = gf_open,
	.release = gf_release,
#ifdef GF_FASYNC
	.fasync = gf_fasync,
#endif
};

#if defined(CONFIG_DRM_PANEL_NOTIFIER)
static int goodix_drm_panel_state_chg_callback(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct gf_dev *gf_dev;
	struct fb_event *fb_event = data;
	unsigned int blank;
	char msg = 0;
	pr_info("val(%lu) need process\n", val);
	gf_dev = container_of(nb, struct gf_dev, notifier);
	if (fb_event && fb_event->data && gf_dev) {
		if (val == DRM_PANEL_EARLY_EVENT_BLANK) {
			/* before fb blank */
		} else if (val == DRM_PANEL_EVENT_BLANK) {
				blank = *(int *)(fb_event->data);
				switch (blank) {
				case DRM_PANEL_BLANK_POWERDOWN:
					if (gf_dev->device_available == 1) {
						gf_dev->fb_black = 1;
#if defined(GF_NETLINK_ENABLE)
						msg = GF_NET_EVENT_FB_BLACK;
						pr_info("%s send to goodixfp, LCD OFF msg=%d\n", __func__, msg);
						sendnlmsg(&msg);
#elif defined(GF_FASYNC)
						if (gf_dev->async)
							kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
#endif
					}
					break;
				case DRM_PANEL_BLANK_UNBLANK:
					if (gf_dev->device_available == 1) {
						gf_dev->fb_black = 0;
#if defined(GF_NETLINK_ENABLE)
						msg = GF_NET_EVENT_FB_UNBLACK;
						pr_info("%s send to goodixfp, LCD ON msg=%d\n", __func__, msg);
						sendnlmsg(&msg);
#elif defined(GF_FASYNC)
						if (gf_dev->async)
							kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
#endif
					}
					break;
				default:
					pr_info("%s defalut\n", __func__);
					break;
				}
		}
	}
	return NOTIFY_OK;
}

#elif defined(CONFIG_FB)
static int goodix_fb_state_chg_callback(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct gf_dev *gf_dev;
	struct fb_event *evdata = data;
	unsigned int blank;
	char msg = 0;

	if (val != FB_EARLY_EVENT_BLANK)
		return 0;
	pr_info("[info] %s go to the goodix_fb_state_chg_callback value = %d\n",
			__func__, (int)val);
	gf_dev = container_of(nb, struct gf_dev, notifier);
	if (evdata && evdata->data && val == FB_EARLY_EVENT_BLANK && gf_dev) {
		blank = *(int *)(evdata->data);
		switch (blank) {
		case FB_BLANK_POWERDOWN:
			if (gf_dev->device_available == 1) {
				gf_dev->fb_black = 1;
#if defined(GF_NETLINK_ENABLE)
				msg = GF_NET_EVENT_FB_BLACK;
				sendnlmsg(&msg);
#elif defined(GF_FASYNC)
				if (gf_dev->async)
					kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
#endif
			}
			break;
		case FB_BLANK_UNBLANK:
			if (gf_dev->device_available == 1) {
				gf_dev->fb_black = 0;
#if defined(GF_NETLINK_ENABLE)
				msg = GF_NET_EVENT_FB_UNBLACK;
				sendnlmsg(&msg);
#elif defined(GF_FASYNC)
				if (gf_dev->async)
					kill_fasync(&gf_dev->async, SIGIO, POLL_IN);
#endif
			}
			break;
		default:
			pr_info("%s defalut\n", __func__);
			break;
		}
	}
	return NOTIFY_OK;
}
#endif

static struct notifier_block goodix_noti_block = {
	.notifier_call = goodix_drm_panel_state_chg_callback,
};

static struct class *gf_class;
#if defined(USE_SPI_BUS)
static int gf_probe(struct spi_device *spi)
#elif defined(USE_PLATFORM_BUS)
static int gf_probe(struct platform_device *pdev)
#endif
{
	struct gf_dev *gf_dev = &gf;
	int status = -EINVAL;
	unsigned long minor;
	int i;

	pr_info("enter gf_probe\n");
	/* Initialize the driver data */
	INIT_LIST_HEAD(&gf_dev->device_entry);
#if defined(USE_SPI_BUS)
	gf_dev->spi = spi;
#elif defined(USE_PLATFORM_BUS)
	gf_dev->spi = pdev;
#endif
	gf_dev->irq_gpio = -EINVAL;
	gf_dev->reset_gpio = -EINVAL;
	gf_dev->pwr_gpio = -EINVAL;
	gf_dev->device_available = 0;
	gf_dev->fb_black = 0;
	gf_dev->irq = 0;

	/* If we can allocate a minor number, hook up this device.
	 * Reusing minors is fine so long as udev or mdev is working.
	 */
	mutex_lock(&device_list_lock);
	minor = find_first_zero_bit(minors, N_SPI_MINORS);
	if (minor < N_SPI_MINORS) {
		struct device *dev;

		gf_dev->devt = MKDEV(SPIDEV_MAJOR, minor);
		dev = device_create(gf_class, &gf_dev->spi->dev, gf_dev->devt,
				gf_dev, GF_DEV_NAME);
		status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
	} else {
		dev_dbg(&gf_dev->spi->dev, "no minor number available!\n");
		status = -ENODEV;
		mutex_unlock(&device_list_lock);
		goto error_hw;
	}

	if (status == 0) {
		set_bit(minor, minors);
		list_add(&gf_dev->device_entry, &device_list);
	} else {
		gf_dev->devt = 0;
		goto error_hw;
	}
	mutex_unlock(&device_list_lock);

	gf_dev->input = input_allocate_device();
	if (gf_dev->input == NULL) {
		pr_err("%s, failed to allocate input device\n", __func__);
		status = -ENOMEM;
		goto error_dev;
	}
	for (i = 0; i < ARRAY_SIZE(maps); i++)
		input_set_capability(gf_dev->input, maps[i].type, maps[i].code);

	gf_dev->input->name = GF_INPUT_NAME;
	status = input_register_device(gf_dev->input);
	if (status) {
		pr_err("failed to register input device\n");
		goto error_input;
	}

#ifdef AP_CONTROL_CLK
	pr_info("Get the clk resource.\n");
	/* Enable spi clock */
	if (gfspi_ioctl_clk_init(gf_dev))
		goto gfspi_probe_clk_init_failed;

	if (gfspi_ioctl_clk_enable(gf_dev))
		goto gfspi_probe_clk_enable_failed;

	spi_clock_set(gf_dev, 1000000);
#endif

	gf_dev->notifier = goodix_noti_block;
#ifdef CONFIG_DRM_PANEL_NOTIFIER
	pr_info("CONFIG_DRM_PANEL_NOTIFIER");
	if (goodix_drm_register(&gf_dev->spi->dev) == 0) {
		if (drm_panel_notifier_register(active_panel, &gf_dev->notifier) < 0)
			pr_err("Failed to register FB notifier client\n");
	}
#elif defined(CONFIG_FB)
	pr_info("CONFIG_FB");
	if (fb_register_client(&gf_dev->notifier))
		pr_err("Failed to register fb notifier client:%d", r);
#endif
	wakeup_source_init(&fp_wakelock, "fp_wakelock");

	pr_info("version V%d.%d.%02d\n", VER_MAJOR, VER_MINOR, PATCH_LEVEL);

	return status;

#ifdef AP_CONTROL_CLK
gfspi_probe_clk_enable_failed:
	gfspi_ioctl_clk_uninit(gf_dev);
gfspi_probe_clk_init_failed:
#endif

error_input:
	if (gf_dev->input != NULL)
		input_free_device(gf_dev->input);
error_dev:
	if (gf_dev->devt != 0) {
		pr_info("Err: status = %d\n", status);
		mutex_lock(&device_list_lock);
		list_del(&gf_dev->device_entry);
		device_destroy(gf_class, gf_dev->devt);
		clear_bit(MINOR(gf_dev->devt), minors);
		mutex_unlock(&device_list_lock);
	}
error_hw:
	gf_dev->device_available = 0;

	return status;
}

#if defined(USE_SPI_BUS)
static int gf_remove(struct spi_device *spi)
#elif defined(USE_PLATFORM_BUS)
static int gf_remove(struct platform_device *pdev)
#endif
{
	struct gf_dev *gf_dev = &gf;

	wakeup_source_trash(&fp_wakelock);

	if (gf_dev->input)
		input_unregister_device(gf_dev->input);
	input_free_device(gf_dev->input);

	/* prevent new opens */
	mutex_lock(&device_list_lock);
	list_del(&gf_dev->device_entry);
	device_destroy(gf_class, gf_dev->devt);
	clear_bit(MINOR(gf_dev->devt), minors);
	mutex_unlock(&device_list_lock);

	return 0;
}

static const struct of_device_id gx_match_table[] = {
	{ .compatible = GF_SPIDEV_NAME },
	{},
};

#if defined(USE_SPI_BUS)
static struct spi_driver gf_driver = {
#elif defined(USE_PLATFORM_BUS)
static struct platform_driver gf_driver = {
#endif
	.driver = {
		.name = GF_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = gx_match_table,
	},
	.probe = gf_probe,
	.remove = gf_remove,
};

static int __init gf_init(void)
{
	int status;

	/* Claim our 256 reserved device numbers.  Then register a class
	 * that will key udev/mdev to add/remove /dev nodes.  Last, register
	 * the driver which manages those device numbers.
	 */
	pr_info("enter gf_init\n");
	if (is_goodix_fp_gf3658()) {
		pr_info("fingerprint is goodix series.\n");
	} else {
		pr_info("fingerprint is not goodix series, exit\n");
		return 0;
	}
	BUILD_BUG_ON(N_SPI_MINORS > 256);
	status = register_chrdev(SPIDEV_MAJOR, CHRD_DRIVER_NAME, &gf_fops);
	if (status < 0) {
		pr_warn("Failed to register char device!\n");
		return status;
	}
	SPIDEV_MAJOR = status;
	gf_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(gf_class)) {
		unregister_chrdev(SPIDEV_MAJOR, gf_driver.driver.name);
		pr_warn("Failed to create class.\n");
		return PTR_ERR(gf_class);
	}
#if defined(USE_PLATFORM_BUS)
	status = platform_driver_register(&gf_driver);
#elif defined(USE_SPI_BUS)
	status = spi_register_driver(&gf_driver);
#endif
	if (status < 0) {
		class_destroy(gf_class);
		unregister_chrdev(SPIDEV_MAJOR, gf_driver.driver.name);
		pr_warn("Failed to register SPI driver.\n");
	}

#ifdef GF_NETLINK_ENABLE
	netlink_init();
#endif

	gf_uevent_init();

	pr_info("%s end status = 0x%x\n", __func__, status);
	return 0;
}

#ifdef CONFIG_DRM_PANEL_NOTIFIER
late_initcall(gf_init);
#else
module_init(gf_init);
#endif

static void __exit gf_exit(void)
{
#ifdef GF_NETLINK_ENABLE
	netlink_exit();
#endif
#if defined(USE_PLATFORM_BUS)
	platform_driver_unregister(&gf_driver);
#elif defined(USE_SPI_BUS)
	spi_unregister_driver(&gf_driver);
#endif
	class_destroy(gf_class);
	unregister_chrdev(SPIDEV_MAJOR, gf_driver.driver.name);

	gf_uevent_exit();
}
module_exit(gf_exit);

MODULE_AUTHOR("Jiangtao Yi, <yijiangtao@goodix.com>");
MODULE_AUTHOR("Jandy Gou, <gouqingsong@goodix.com>");
MODULE_DESCRIPTION("goodix fingerprint sensor device driver");
MODULE_LICENSE("GPL");
