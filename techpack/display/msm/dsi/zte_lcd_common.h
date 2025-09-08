
#ifndef _ZTE_LCD_COMMON_H_
#define _ZTE_LCD_COMMON_H_
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/sysfs.h>
#include <linux/proc_fs.h>
#include <linux/kobject.h>
#include <linux/mutex.h>
#ifdef CONFIG_ZTE_LCD_HBM_CTRL
#include <video/mipi_display.h>
#include <drm/drm_mipi_dsi.h>
#include "../../../../drivers/gpu/msm/kgsl_device.h"
#endif

#include "dsi_panel.h"

#ifdef CONFIG_ZTE_LCD_REG_DEBUG
#define ZTE_REG_LEN 64
#define REG_MAX_LEN 16 /*one lcd reg display info max length*/
enum {	/* read or write mode */
	REG_WRITE_MODE = 0,
	REG_READ_MODE
};
struct zte_lcd_reg_debug {
	int is_read_mode;  /*if 1 read ,0 write*/
	unsigned char length;
	char rbuf[ZTE_REG_LEN];
	char wbuf[ZTE_REG_LEN];
};
/*WARNING: Single statement macros should not use a do {} while (0) loop*/
#define ZTE_LCD_INFO(fmt, args...) {pr_info("[MSM_LCD][Info]"fmt, ##args); }
#define ZTE_LCD_ERROR(fmt, args...) {pr_err("[MSM_LCD][Error]"fmt, ##args); }
#endif

#ifdef CONFIG_ZTE_LCD_AOD_BRIGHTNESS_CTRL
int panel_set_aod_brightness(struct dsi_panel *panel, u32 level);
#endif
const char *zte_get_lcd_panel_name(void);
int panel_is_in_aod_mode(void);
int panel_is_rm692a4_647_oled(void);
#ifdef CONFIG_ZTE_LCD_HBM_CTRL
void panel_state_send_uevent(int state);
#endif
#endif /* _ZTE_LCD_COMMON_H_ */
