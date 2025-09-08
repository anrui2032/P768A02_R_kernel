
#include "zte_lcd_common.h"

struct dsi_panel *g_zte_ctrl_pdata;
#ifdef CONFIG_ZTE_LCD_REG_DEBUG
extern void zte_lcd_reg_debug_func(void);
#endif
#ifdef CONFIG_ZTE_LCD_AOD_BRIGHTNESS_CTRL
#define AOD_BL_LEVEL_LOW  0
#define AOD_BL_LEVEL_MID  1
#define AOD_BL_LEVEL_HIGH  2
#define AOD_BL_LEVEL_OFF  3
#endif
const char *zte_get_lcd_panel_name(void)
{
	if (g_zte_ctrl_pdata == NULL || g_zte_ctrl_pdata->zte_lcd_ctrl == NULL)
		return NULL;
	else
		return g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_panel_name;
}

#ifdef CONFIG_ZTE_LCD_AOD_BRIGHTNESS_CTRL
/*************************************************************
 * Function: panel_is_in_aod_mode
 * Helper function to check whether the panel power mode that we set is in lp.
 * lp - 1
 * OFF - 5
 * ON - 0
*************************************************************/
int panel_is_in_aod_mode(void)
{
	if (g_zte_ctrl_pdata->zte_panel_state == 1)
		return 1;
	else
		return 0;
}

/*************************************************************
 * Function: panel_is_rm692a4_647_oled
 * Helper function to check whether the panel is A4.
*************************************************************/
int panel_is_rm692a4_647_oled(void)
{
	if ((!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692A4-otpt3-1080-2340-6P5Inch")) ||
		(!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692A4-nootp240nit-1080-2340-6P5Inch")))
		return 1;
	else
		return 0;
}
#endif

/*************************************************************
 * read lcm hardware info begin
 * file path: proc/driver/lcd_id/ or proc/msm_lcd
*************************************************************/
static int zte_lcd_proc_info_show(struct seq_file *m, void *v)
{
	if (!g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_init_code_version) {
		seq_printf(m, "panel_name=%s\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_panel_name);
	} else {
		seq_printf(m, "panel_name=%s,version=%s\n",
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_panel_name,
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_init_code_version);
	}
	return 0;
}
static int zte_lcd_proc_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, zte_lcd_proc_info_show, NULL);
}
static const struct file_operations zte_lcd_common_func_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= zte_lcd_proc_info_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release		= single_release,
};
static int zte_lcd_proc_info_display(struct device_node *node)
{
	proc_create("driver/lcd_id", 0664, NULL, &zte_lcd_common_func_proc_fops);

	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_panel_name = of_get_property(node,
		"qcom,mdss-dsi-panel-name", NULL);
	if (!g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_panel_name) {
		pr_info("%s:%d, panel name not found!\n", __func__, __LINE__);
		return -ENODEV;
	}

	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_init_code_version = of_get_property(node,
		"zte,lcd-init-code-version", NULL);

	pr_info("[MSM_LCD]%s: Panel Name = %s,init code version=%s\n", __func__,
		g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_panel_name,
		g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_init_code_version);

	return 0;
}

/*************************************************************
* HBM MODE
* file path: proc/driver/lcd_hbm/
*************************************************************/
#ifdef CONFIG_ZTE_LCD_HBM_CTRL

struct kgsl_device *ksgl_uevent_device = NULL;
#define PARTIAL_HBM_SOURCE_EM_THRESHOLD     (0x155 + 1)
#define HBM_ENABLE_CMD_ADDR     0X83

void panel_state_send_uevent(int state)
{
	char *envp[3];
	if (state == 0)
		envp[0] = "LCD_STATUS=OFF";
	else if (state == 1)
		envp[0] = "LCD_STATUS=ON";
	else if (state == 2)
		envp[0] = "LCD_STATUS=AOD";

	envp[1] = NULL;
	envp[2] = NULL;

	if (ksgl_uevent_device)
		kobject_uevent_env(&ksgl_uevent_device->dev->kobj, KOBJ_CHANGE, envp);
	else
		pr_info("[MSM_LCD]HBM: ksgl_uevent_device is NULL, LCD state send faild\n");
}

int zte_hbm_ctrl_display(struct dsi_panel *panel, u32 setHbm)
{
	int err = 0;
	struct mipi_dsi_device *dsi;
	struct dsi_display_mode_priv_info *priv_info;
	struct dsi_cmd_desc *cmds = NULL;
	u8 *tx_buf;
	u32 count;
	bool hbmon_to_aod;
	hbmon_to_aod = false;

	dsi = &panel->mipi_device;
	if (!dsi) {
		pr_info("HBM: No device");
		return -ENOMEM;
	}

	mutex_lock(&panel->panel_lock);
	if (!panel->panel_initialized) {
		err = -EPERM;
		pr_err("HBM: panel is off or not initialized1\n");
		goto error;
	}

	if (panel->cur_mode)
		priv_info = panel->cur_mode->priv_info;
	else
		priv_info = NULL;

	if (setHbm == 0) {
		if (!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692E1-1080-2400-6P67Inch")) {
			if (panel_is_in_aod_mode()) {
				err = zte_dsi_panel_tx_cmd_set(panel, dsi_panel_get_zte_dfps_aod_switch_index());
				if (err < 0) {
					pr_info("[MSM_LCD]HBM: enter LP again error\n");
					goto error;
				}
				switch (g_zte_ctrl_pdata->zte_lcd_aod_brightness) {
					case AOD_BL_LEVEL_LOW:
						err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_LOW);
						break;
					case AOD_BL_LEVEL_MID:
						err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_MID);
						break;
					case AOD_BL_LEVEL_HIGH:
						err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_HIGH);
						break;
					default:
						pr_info("[MSM_LCD]HBM: please check AOD Brightness level\n");
						break;
				}
				if (err < 0) {
					pr_info("[MSM_LCD]HBM: set LP brightness error\n");
					goto error;
				}
				pr_info("[MSM_LCD]HBM: enter LP again\n");
				hbmon_to_aod = true;
			}
		}
		if (!hbmon_to_aod) {
			if ((!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692A4-otpt3-1080-2340-6P5Inch")) ||
					(!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692A4-1080-2340-6P5Inch"))) {
			if (panel->bl_config.real_bl_level_to_panel < PARTIAL_HBM_SOURCE_EM_THRESHOLD)
            	err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_HBM_OFF_FOR_OTPT3_PANEL);	
				if (err)
					goto error;
			}
			if (!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692E1-1080-2400-6P67Inch")) {
				if (panel->zte_lcd_ctrl->hbm_brightness != 0x0 && priv_info) {
					pr_info("MSM_LCD set HBM Brightness : %d\n",panel->zte_lcd_ctrl->hbm_brightness);
					count = priv_info->cmd_sets[DSI_CMD_SET_ZTE_HBM_OFF].count;
					cmds = priv_info->cmd_sets[DSI_CMD_SET_ZTE_HBM_OFF].cmds;
					if (cmds && count > 1) {
						tx_buf = (u8 *)cmds[g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_off_reg51_index].msg.tx_buf;
						if (tx_buf && tx_buf[0] == 0x51) {
							tx_buf[1] = panel->zte_lcd_ctrl->hbm_brightness >> 8;
							tx_buf[2] = panel->zte_lcd_ctrl->hbm_brightness & 0xff;
							pr_info("MSM_LCD HBM DSI_CMD_SET_ZTE_HBM_OFF 0x%02X = 0x%02X 0x%02X\n",tx_buf[0], tx_buf[1], tx_buf[2]);
						}
					}
				} 
			} 
			err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_HBM_OFF);
			if (err) {
				pr_err(" HBM: failed to send DSI_CMD_SET_ZTE_HBM_OFF cmd, rc=%d\n", err);
				goto error;
			} else {
				pr_info("HBM: success to send DSI_CMD_SET_ZTE_HBM_OFF cmd");
			}
			panel->zte_lcd_ctrl->hbm_off_to_dim_on = true;
		}
	} else {
		if (!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692E1-1080-2400-6P67Inch")) {
			if (panel_is_in_aod_mode()) {
				err = zte_dsi_panel_tx_cmd_set(panel,DSI_CMD_SET_NOLP);
				if (err < 0) {
					pr_info("[MSM_LCD]HBM: exit from LP error\n");
					goto error;
				}
				pr_info("[MSM_LCD]HBM: exit from LP first\n");
			}
		}
		if ((!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692A4-otpt3-1080-2340-6P5Inch")) ||
					(!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692A4-1080-2340-6P5Inch"))) {
			if (panel->bl_config.real_bl_level_to_panel < PARTIAL_HBM_SOURCE_EM_THRESHOLD)
            	err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_HBM_ON_FOR_OTPT3_PANEL);	
				if (err)
					goto error;
		}
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_HBM_ON);
		if (err) {
			pr_err(" MSM_LCD HBM: failed to send DSI_CMD_SET_ZTE_HBM_ON cmd, rc=%d\n", err);
			goto error;
		} else {
			pr_info("MSM_LCD HBM: success to send DSI_CMD_SET_ZTE_HBM_ON cmd");
		}
	}
	if (!panel->panel_initialized) {
		err = -EPERM;
		pr_err("HBM: panel is off or not initialized\n");
		goto error;
	}
	mutex_unlock(&panel->panel_lock);

	return err;

error:
	mutex_unlock(&panel->panel_lock);
	pr_err("HBM: send cmds failed");
	return err;

}

static int zte_lcd_proc_hbm_show(struct seq_file *m, void *v)
{

	seq_printf(m, "%d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode);
	pr_info("HBM: read value = %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode);

	return 0;
}

static ssize_t zte_lcd_proc_hbm_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count+1), GFP_KERNEL);
	u32 mode;
	int ret = 0;
	char *envp[3];

	if (!tmp)
		return -ENOMEM;

	if (!g_zte_ctrl_pdata->panel_initialized) {
		pr_info("HBM: Panel not initialized\n");
		kfree(tmp);
		return -ENOMEM;
	}

	if (copy_from_user(tmp, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	mode = *tmp - '0';

	if (mode >1) {
		if (mode == 2) {
			g_zte_ctrl_pdata->zte_hbm_flag = true;
		} else {
			g_zte_ctrl_pdata->zte_hbm_flag = false;
		}
		pr_info("MSM_LCD panel hbm_flag is %d\n",g_zte_ctrl_pdata->zte_hbm_flag);
		kfree(tmp);
		return count;
	}

	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode != mode) {
			pr_info("HBM: Send HBM cmds START\n");
			ret = zte_hbm_ctrl_display(g_zte_ctrl_pdata, mode);
			if (ret == 0) {
				g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode = mode;
				pr_info("HBM: Set new mode successful, new mode = %d\n",
						g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode);
			}
		pr_info("HBM PanelName: %s\n", g_zte_ctrl_pdata->name);

		if (ret == 0) {
			if (mode == 0)
				envp[0] = "HBM_STATUS=OFF";
			else
				envp[0] = "HBM_STATUS=ON";
			envp[1] = "HBM_SET_RESULT=SUCCESSFUL";
		} else {
			if (mode == 0)
				envp[0] = "HBM_STATUS=ON";
			else
				envp[0] = "HBM_STATUS=OFF";
			envp[1] = "HBM_SET_RESULT=FAILED";
		}
		envp[2] = NULL;

		if (ksgl_uevent_device)
			kobject_uevent_env(&ksgl_uevent_device->dev->kobj, KOBJ_CHANGE, envp);
		else
			pr_info("HBM: ksgl_uevent_device is NULL\n");
	} else
		pr_info("HBM: New mode is same as old ,do nothing! mode = %d\n",
				g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode);

	kfree(tmp);
	return count;
}

static int zte_lcd_proc_hbm_open(struct inode *inode, struct file *file)
{
	return single_open(file, zte_lcd_proc_hbm_show, NULL);
}

static const struct file_operations zte_lcd_hbm_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= zte_lcd_proc_hbm_open,
	.read		= seq_read,
	.write		= zte_lcd_proc_hbm_write,
	.llseek		= seq_lseek,
	.release		= single_release,
};

static int zte_lcd_hbm_ctrl(struct device_node *node)
{
	proc_create("driver/lcd_hbm", 0664, NULL, &zte_lcd_hbm_proc_fops);

	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode = 0;
	pr_info("HBM:  = %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_mode);

	return 0;
}

/*************************************************************
 * Function: panel_global_hbm_ctrl_RM692C9display
 * Helper to ctrl global hbm 
 * FOR C9 panel
*************************************************************/
int panel_global_hbm_ctrl_RM692C9display(struct dsi_panel *panel, u32 setHbm){
	char *envp[3];
	int err = 0;

	struct mipi_dsi_device *dsi;

	dsi = &panel->mipi_device;
	if (!dsi) {
		pr_info("HDR: No device\n");
		return -ENOMEM;
	}

	/*Do not use panel_lock while called in dsi_panel_set_backlight*/
	if (!panel->panel_initialized) {
		pr_err("HDR: panel is off or not initialized1\n");
		return -EPERM;
	}

	if (setHbm == 1) {
		envp[0] = "GLOBAL_HBM_STATUS=ON";
	} else {
		envp[0] = "GLOBAL_HBM_STATUS=OFF";
	}
	envp[1] = "GLOBAL_HBM_SET_RESULT=FAILED";
	envp[2] = NULL;

	if (setHbm == 1) {
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_GLOBAL_HBM_ON);
		if (err)
			goto error;
		pr_info("HDR: Send enable HDR cmds to panel\n");
	} else {
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_GLOBAL_HBM_OFF);
		if (err)
			goto error;
		pr_info("HDR: Send disable HDR cmds to panel\n");
	}
	usleep_range(5000, 5100);
	envp[1] = "GLOBAL_HBM_SET_RESULT=SUCCESSFUL";

error:
	if (ksgl_uevent_device)
		kobject_uevent_env(&ksgl_uevent_device->dev->kobj, KOBJ_CHANGE, envp);
	else
		pr_info("HDR: Global HBM ksgl_uevent_device is NULL\n");
	if (err < 0)
		pr_err("HDR: send cmds failed\n");
	return err;
}

/*************************************************************
 * HDR MODE
 * file path: proc/driver/lcd_hbm/
 * setHdr flag for LCD brightness:
 * HDR OFF - 0
 * HDR ON - 1
 * AUTO SENSOR ON - 2
 * AUTO SENSOR OFF - 3
 * These should be same as the defination in sensor file, do not change.
*************************************************************/
void panel_set_hdr_flag(struct dsi_panel *panel, u32 setHdr)
{
	u32 bl_lvl_bak = panel->bl_config.bl_level;
	char cal_value = g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on;

	switch (setHdr) {
	case 0:
		cal_value &= 0xfe;/* clear bit(0) */
		break;
	case 1:
		cal_value |= 0x01;/* set bit(0) */
		break;
	case 2:
		cal_value |= 0x02;/* set bit(1) */
		break;
	case 3:
		cal_value &= 0xfd;/* clear bit(1) */
		break;
	default:
		pr_err("HDR invalid flag:  %d\n", setHdr);
	break;
	}

	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on != cal_value) {
		g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on = cal_value;
		if (bl_lvl_bak != 0)
			dsi_panel_set_backlight(panel, bl_lvl_bak);
		pr_info("HDR flag:  new value: %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on);
	} else {
		pr_info("HDR flag is same as old: %d, do nothing\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on);
	}

}
static ssize_t zte_lcd_proc_hdr_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count+1), GFP_KERNEL);
	u32 mode;

	if (!tmp)
		return -ENOMEM;

	if (!g_zte_ctrl_pdata->panel_initialized) {
		pr_info("HDR: Panel not initialized\n");
		kfree(tmp);
		return -ENOMEM;
	}

	if (copy_from_user(tmp, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	mode = *tmp - '0';

	panel_set_hdr_flag(g_zte_ctrl_pdata, mode);

	kfree(tmp);
	return count;
}
static int zte_lcd_proc_hdr_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on);
	pr_info("HDR flag:  %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on);
	return 0;
}
static int zte_lcd_proc_hdr_open(struct inode *inode, struct file *file)
{
	return single_open(file, zte_lcd_proc_hdr_show, NULL);
}
static const struct file_operations zte_lcd_hdr_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= zte_lcd_proc_hdr_open,
	.read		= seq_read,
	.write		= zte_lcd_proc_hdr_write,
	.llseek		= seq_lseek,
	.release		= single_release,
};
static int zte_lcd_set_hdr_flag(struct device_node *node)
{
	proc_create("driver/lcd_hdr", 0664, NULL, &zte_lcd_hdr_proc_fops);

	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on = 0;
	pr_info("HDR flag:  = %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hdr_on);

	return 0;
}
#endif

/*************************************************************
 * Second panel
 * file path: proc/driver/lcd_sec_panel/
 * FOR C9 panel control second panel.
*************************************************************/
#ifdef CONFIG_ZTE_LCD_SEC_PANEL_CTRL
#define SEC_PANEL_OFF_CMDS_LEN	7
#define SEC_PANEL_ON_CMDS_LEN	11
int panel_sec_panel_set_state(struct dsi_panel *panel, u32 state){
	int i, err = 0;
	u8 sec_panel_off[SEC_PANEL_OFF_CMDS_LEN][2] = {
								{0xfe, 0x74},
								{0xbc, 0x0e},
								{0xc1, 0x0e},
								{0xc2, 0x0e},
								{0xc3, 0x0e},
								{0xc5, 0x0e},
								{0xfe, 0x00}
							};
	u8 sec_panel_on[SEC_PANEL_ON_CMDS_LEN][2] = {
								{0xfe, 0x74},
								{0x10, 0x05},
								{0x15, 0x05},
								{0x16, 0x01},
								{0x17, 0x01},
								{0xbc, 0x09},
								{0xc1, 0x09},
								{0xc2, 0x0d},
								{0xc3, 0x0d},
								{0xc5, 0x0d},
								{0xfe, 0x00}
							};

	struct mipi_dsi_device *dsi;

	dsi = &panel->mipi_device;
	if (!dsi) {
		pr_info("Sec panel: No device\n");
		return -ENOMEM;
	}

	mutex_lock(&panel->panel_lock);
	if (!panel->panel_initialized) {
		err = -EPERM;
		pr_err("Sec panel: panel is off or not initialized1\n");
		goto error;
	}

	if (state == 1) {
		for (i = 0; i < SEC_PANEL_ON_CMDS_LEN; i++) {
			err = mipi_dsi_dcs_write(dsi, sec_panel_on[i][0], &sec_panel_on[i][1], sizeof(sec_panel_on[i]) - 1);
			if (err < 0)
				goto error;
		}
		pr_info("Sec panel: Send sec panel on cmds\n");
	} else {
		for (i = 0; i < SEC_PANEL_OFF_CMDS_LEN; i++) {
			err = mipi_dsi_dcs_write(dsi, sec_panel_off[i][0], &sec_panel_off[i][1], sizeof(sec_panel_off[i]) - 1);
			if (err < 0)
				goto error;
		}
		pr_info("Sec panel: Send sec panel off cmds\n");
	}
	usleep_range(5000, 6000);
	mutex_unlock(&panel->panel_lock);

	return err;

error:
	mutex_unlock(&panel->panel_lock);
	pr_err("Sec panel: send cmds failed\n");
	return err;
}

static ssize_t zte_lcd_proc_sec_panel_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count+1), GFP_KERNEL);
	u32 state;
	int ret = 0, retry_times = 3;

	if (!tmp)
		return -ENOMEM;

	if (!g_zte_ctrl_pdata->panel_initialized) {
		pr_info("Sec panel: Panel not initialized\n");
		kfree(tmp);
		return -ENOMEM;
	}

	if (copy_from_user(tmp, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	pr_info("Sec panel: Panel Name is %s\n", g_zte_ctrl_pdata->name);

	state = *tmp - '0';
	if ((!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692C9-1080-2460-6P9Inch")) ||
			(!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692C9-1080-2460-6P9Inch-10bit")) ||
			(!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692C9-1080-2460-6P9Inch-10bit-90hz"))) {
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state != state) {
			while (retry_times--) {
				ret = panel_sec_panel_set_state(g_zte_ctrl_pdata, state);
				if (ret == 0) {
					g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state = state;
					pr_info("Sec panel: Set new state successful, new state = %d\n",
						g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state);
					break;
				}
				pr_info("Sec panel: Set new state failed, state = %d, retry_times = %d\n",
						g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state, retry_times);
			}

		} else {
			pr_info("Sec panel: New state is same as old ,do nothing! state = %d\n",
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state);
		}
	}
	kfree(tmp);
	return count;
}
static int zte_lcd_proc_sec_panel_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state);
	pr_info("Sec panel: state = %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state);
	return 0;
}
static int zte_lcd_proc_sec_panel_open(struct inode *inode, struct file *file)
{
	return single_open(file, zte_lcd_proc_sec_panel_show, NULL);
}

static const struct file_operations zte_lcd_sec_panel_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= zte_lcd_proc_sec_panel_open,
	.read		= seq_read,
	.write		= zte_lcd_proc_sec_panel_write,
	.llseek		= seq_lseek,
	.release		= single_release,
};

static int zte_lcd_sec_panel_ctrl(struct device_node *node)
{
	proc_create("driver/lcd_sec_panel", 0664, NULL, &zte_lcd_sec_panel_proc_fops);

	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state = 1;
	pr_info("Sec panel: state = %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_sec_panel_state);

	return 0;
}
#endif

/*************************************************************
 * Color Mode
 * file path: proc/driver/lcd_color_gamut/
 * COLOR_GAMUT_ORIGINA - 0
 * COLOR_GAMUT_SRGB - 1
 * COLOR_GAMUT_P3 - 2
 * FOR C9 panel
*************************************************************/
#ifdef CONFIG_ZTE_LCD_COLOR_GAMUT_CTRL
int panel_color_gamut_set(struct dsi_panel *panel, u32 index){
	int err = 0;

	struct mipi_dsi_device *dsi;

	dsi = &panel->mipi_device;
	if (!dsi) {
		pr_info("MSM_LCD panel: No device\n");
		return -ENOMEM;
	}

	mutex_lock(&panel->panel_lock);
	if (!panel->panel_initialized) {
		err = -EPERM;
		pr_err("MSM_LCD panel: panel is off or not initialized1\n");
		goto error;
	}
    if (!strcmp(g_zte_ctrl_pdata->name, "Visionox-RM692E1-1080-2400-6P67Inch") && panel->zte_lcd_ctrl->lcd_id < 2) {
		if (index == COLOR_GAMUT_SRGB) {
			index = COLOR_GAMUT_ORIGINAL;
		}
	}
	switch (index) {
	case COLOR_GAMUT_ORIGINAL:
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_COLOR_ORIGINAL);
		if (err) {
			pr_err("MSM_LCD Color gamut: send DSI_CMD_SET_ZTE_COLOR_ORIGINAL failed");
			goto error;
		}
		pr_info("MSM_LCD Color gamut: Send original gamut cmds ok\n");
		break;
	case COLOR_GAMUT_SRGB:
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_COLOR_SRGB);
		if (err) {
			pr_err("MSM_LCD Color gamut: send DSI_CMD_SET_ZTE_COLOR_SRGB failed");
			goto error;
		}
		pr_info("MSM_LCD Color gamut: Send srgb gamut cmds ok\n");
		break;
	case COLOR_GAMUT_P3:
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_COLOR_P3);
		if (err) {
			pr_err("MSM_LCD Color gamut: send DSI_CMD_SET_ZTE_COLOR_P3 failed");
			goto error;
		}
		pr_info("MSM_LCD Color gamut: Send dcip3 gamut cmds ok\n");
		break;
	default:
		pr_err("MSM_LCD Color gamut index %d  not supported\n", index);
		break;
	}
	usleep_range(5000, 5100);
	mutex_unlock(&panel->panel_lock);

	return err;

error:
	mutex_unlock(&panel->panel_lock);
	pr_err("MSM_LCD Color gamut: send cmds failed\n");
	return err;
}

static ssize_t zte_lcd_proc_color_gamut_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count+1), GFP_KERNEL);
	u32 index;
	int ret = 0;

	if (!tmp)
		return -ENOMEM;

	if (!g_zte_ctrl_pdata->panel_initialized) {
		pr_info("Color gamut: Panel not initialized\n");
		kfree(tmp);
		return -ENOMEM;
	}

	if (copy_from_user(tmp, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	pr_info("Color gamut: Panel Name is %s\n", g_zte_ctrl_pdata->name);

	index = *tmp - '0';
	if (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index != index) {
		ret = panel_color_gamut_set(g_zte_ctrl_pdata, index);
		if (ret == 0) {
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index = index;
			pr_info("Color gamut: Set new gamut successful, new index = %d\n",
				g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index);
		}	
	} else {
		pr_info("Color gamut: New color gamut index is same as old ,do nothing! index = %d\n",
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index);
	}
	
	kfree(tmp);
	return count;
}
static int zte_lcd_proc_color_gamut_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index);
	pr_info("Color gamut: state = %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index);
	return 0;
}
static int zte_lcd_proc_color_gamut_open(struct inode *inode, struct file *file)
{
	return single_open(file, zte_lcd_proc_color_gamut_show, NULL);
}

static const struct file_operations zte_lcd_color_gamut_fops = {
	.owner		= THIS_MODULE,
	.open		= zte_lcd_proc_color_gamut_open,
	.read		= seq_read,
	.write		= zte_lcd_proc_color_gamut_write,
	.llseek		= seq_lseek,
	.release		= single_release,
};

static int zte_lcd_color_gamut_ctrl(struct device_node *node)
{
	proc_create("driver/lcd_color_gamut", 0664, NULL, &zte_lcd_color_gamut_fops);

	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index = COLOR_GAMUT_P3;
	pr_info("Color gamut: state = %d\n", g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_color_gamut_index);

	return 0;
}
#endif

/*************************************************************
 * AOD BRIGHTNESS Level Ctrl
 * file path: proc/driver/lcd_aod_bl/
*************************************************************/
#ifdef CONFIG_ZTE_LCD_AOD_BRIGHTNESS_CTRL
int panel_set_aod_brightness(struct dsi_panel *panel, u32 level)
{
	int err = 0;

	struct mipi_dsi_device *dsi;

	dsi = &panel->mipi_device;
	if (!dsi) {
		pr_info("AOD: No device");
		return -ENOMEM;
	}
	if (!panel->panel_initialized) {
		pr_info("AOD: Panel not initialized\n");
		return -ENOMEM;
	}
	mutex_lock(&panel->panel_lock);
	if (level == 0) {
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_LOW);	
			if (err) {
				pr_err("AOD: send DSI_CMD_SET_ZTE_AOD_LOW failed");
				goto error;
			}
	} else if (level == 1) {
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_MID);
			if (err) {
				pr_err("AOD: send DSI_CMD_SET_ZTE_AOD_MID failed");
				goto error;
			}
	} else {
		err = zte_dsi_panel_tx_cmd_set(panel, DSI_CMD_SET_ZTE_AOD_HIGH);
			if (err) {
				pr_err("AOD: send DSI_CMD_SET_ZTE_AOD_HIGH failed");
				goto error;
			}
	}
	
	mutex_unlock(&panel->panel_lock);

	pr_info("AOD: set brightness successful, new level  = %d\n",
			g_zte_ctrl_pdata->zte_lcd_aod_brightness);

	return err;

error:
	mutex_unlock(&panel->panel_lock);
	pr_err("AOD: send cmds failed");
	return err;

}

static ssize_t zte_lcd_proc_aod_bl_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count+1), GFP_KERNEL);
	u32 level;

	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp, buffer, count)) {
		kfree(tmp);
		return -EFAULT;
	}

	level = *tmp - '0';
	g_zte_ctrl_pdata->zte_lcd_aod_brightness = level;
	pr_info("AOD set new bl level: %d\n", level);

	if (g_zte_ctrl_pdata->zte_panel_state == 1) /* set brightness here if already in aod mode*/
		panel_set_aod_brightness(g_zte_ctrl_pdata, level);
	else
		pr_info("AOD save new bl level, not in aod mode now.\n");

	kfree(tmp);
	return count;
}
static int zte_lcd_proc_aod_bl_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", g_zte_ctrl_pdata->zte_lcd_aod_brightness);
	pr_info("AOD brightness:  = %d\n", g_zte_ctrl_pdata->zte_lcd_aod_brightness);
	return 0;
}
static int zte_lcd_proc_aod_bl_open(struct inode *inode, struct file *file)
{
	return single_open(file, zte_lcd_proc_aod_bl_show, NULL);
}
static const struct file_operations zte_lcd_aod_bl_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= zte_lcd_proc_aod_bl_open,
	.read		= seq_read,
	.write		= zte_lcd_proc_aod_bl_write,
	.llseek		= seq_lseek,
	.release		= single_release,
};
static int zte_lcd_set_aod_brightness(struct device_node *node)
{
	proc_create("driver/lcd_aod_bl", 0664, NULL, &zte_lcd_aod_bl_proc_fops);

	/* 0 - low level, 1 - middle level(default), 2 - high level*/
	g_zte_ctrl_pdata->zte_lcd_aod_brightness = 1;

	pr_info("AOD brightness level:  = %d\n", g_zte_ctrl_pdata->zte_lcd_aod_brightness);

	return 0;
}
#endif

/*************************************************************
 * lcd backlight level curve
 * CURVE_MATRIX_MAX_350_LUX - 1
 * CURVE_MATRIX_MAX_350_LUX - 2
 * CURVE_MATRIX_MAX_350_LUX - 3
 * CURVE_MATRIX_MAX_RM692C9_LUX -4
*************************************************************/
#ifdef CONFIG_ZTE_LCD_BACKLIGHT_LEVEL_CURVE
enum {	/* lcd curve mode */
	CURVE_MATRIX_MAX_350_LUX = 1,
	CURVE_MATRIX_MAX_400_LUX,
	CURVE_MATRIX_MAX_450_LUX,
	CURVE_MATRIX_MAX_RM692C9_LUX,
};

int zte_backlight_curve_matrix_max_350_lux[256] = {
0, 1, 2, 3, 3, 3, 4, 5, 6, 6, 7, 7, 8, 8, 9, 9,
10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 16, 16, 17, 17, 18,
18, 19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27,
28, 28, 29, 29, 30, 31, 31, 32, 33, 33, 34, 35, 35, 36, 36, 37,
38, 38, 39, 40, 40, 41, 42, 43, 43, 44, 45, 45, 46, 47, 47, 48,
49, 50, 50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 58, 58, 59, 60,
61, 61, 62, 63, 64, 64, 65, 66, 67, 68, 68, 69, 70, 71, 72, 73,
74, 75, 76, 77, 77, 78, 79, 80, 81, 82, 82, 83, 84, 85, 86, 87,
88, 88, 89, 90, 91, 92, 93, 94, 95, 96, 96, 97, 98, 99, 100, 101,
102, 103, 104, 105, 106, 107, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 146, 147,
148, 149, 151, 152, 153, 155, 156, 157, 159, 160, 162, 163, 164, 166, 167, 169,
170, 172, 173, 175, 176, 178, 179, 181, 183, 184, 186, 187, 189, 191, 192, 194,
196, 197, 199, 201, 203, 204, 206, 208, 210, 212, 214, 215, 217, 219, 221, 223,
225, 227, 229, 231, 233, 235, 237, 239, 241, 243, 245, 248, 250, 252, 254, 255
};

int zte_backlight_curve_matrix_max_400_lux[256] = {
0, 1, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8,
8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16,
16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24,
25, 25, 26, 26, 27, 27, 28, 28, 29, 30, 30, 31, 31, 32, 32, 33,
34, 34, 35, 35, 36, 37, 37, 38, 38, 39, 40, 40, 41, 42, 42, 43,
43, 44, 45, 45, 46, 47, 47, 48, 49, 49, 50, 51, 51, 52, 53, 53,
54, 55, 55, 56, 57, 57, 58, 59, 59, 60, 61, 61, 62, 63, 64, 64,
65, 66, 66, 67, 68, 69, 69, 70, 71, 72, 72, 73, 74, 74, 75, 76,
77, 78, 78, 79, 80, 81, 81, 82, 83, 84, 84, 85, 86, 87, 88, 88,
89, 90, 91, 92, 92, 93, 94, 95, 96, 96, 97, 98, 99, 100, 101, 101,
102, 103, 104, 105, 106, 107, 107, 108, 109, 110, 111, 112, 113, 113, 114, 115,
116, 117, 118, 119, 120, 121, 121, 122, 123, 124, 125, 126, 127, 128, 129, 129,
130, 132, 133, 134, 136, 137, 139, 140, 142, 143, 145, 147, 148, 150, 151, 153,
155, 156, 158, 160, 162, 163, 165, 167, 169, 170, 172, 174, 176, 178, 180, 182,
184, 186, 188, 190, 192, 194, 196, 198, 200, 203, 205, 207, 209, 212, 214, 216,
219, 221, 223, 226, 228, 231, 233, 236, 238, 241, 243, 246, 249, 252, 254, 255
};

int zte_backlight_curve_matrix_max_450_lux[256] = {
0, 1, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7,
8, 8, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 13, 14,
14, 15, 15, 16, 16, 17, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21,
22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29,
30, 30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 36, 36, 37, 37, 38,
38, 39, 39, 40, 41, 41, 42, 42, 43, 43, 44, 45, 45, 46, 46, 47,
48, 48, 49, 49, 50, 51, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
57, 58, 59, 59, 60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67, 67,
68, 69, 69, 70, 71, 71, 72, 73, 73, 74, 75, 75, 76, 77, 78, 78,
79, 80, 80, 81, 82, 83, 83, 84, 85, 85, 86, 87, 88, 88, 89, 90,
91, 91, 92, 93, 94, 94, 95, 96, 97, 97, 98, 99, 100, 101, 101, 102,
103, 104, 105, 105, 106, 107, 108, 109, 109, 110, 111, 112, 112, 112, 113, 113,
114, 116, 117, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 136, 138,
140, 142, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169,
171, 173, 176, 178, 180, 183, 185, 187, 190, 192, 194, 197, 199, 202, 205, 207,
210, 213, 215, 218, 221, 224, 226, 229, 232, 235, 238, 241, 244, 248, 251, 255
};

int zte_backlight_curve_matrix_max_rm692c9_lux[256] = {
0, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12,
13, 13, 13, 14, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17,
17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22,
23, 23, 23, 24, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29,
30, 30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 36, 36, 37, 37, 38,
38, 39, 39, 40, 41, 41, 42, 42, 43, 43, 44, 45, 45, 46, 46, 47,
48, 48, 49, 49, 50, 51, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
57, 58, 59, 59, 60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67, 67,
68, 69, 69, 70, 71, 71, 72, 73, 73, 74, 75, 75, 76, 77, 78, 78,
79, 80, 80, 81, 82, 83, 83, 84, 85, 85, 86, 87, 88, 88, 89, 90,
91, 91, 92, 93, 94, 94, 95, 96, 97, 97, 98, 99, 100, 101, 101, 102,
103, 104, 105, 105, 106, 107, 108, 109, 109, 110, 111, 112, 112, 112, 113, 113,
114, 116, 117, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 136, 138,
140, 142, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167, 169,
171, 173, 176, 178, 180, 183, 185, 187, 190, 192, 194, 197, 199, 202, 205, 207,
210, 213, 215, 218, 221, 224, 226, 229, 232, 235, 238, 241, 244, 248, 251, 255
};

static int zte_convert_backlevel_function(int level, u32 bl_max)
{
	int bl, convert_level;

	if (level == 0)
		return 0;

	if ((bl_max != 4095) && (bl_max != 1023) && (bl_max != 255)) {
		bl = level * 255 / bl_max;
	} else if (bl_max > 1023) {
		bl = level>>4;
	} else if (bl_max > 255) {
		bl = level >> 2;
	} else {
		bl = level;
	}

	if (!bl && level)
		bl = 1;/*ensure greater than 0 and less than 16 equal to 1*/

	switch (g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode) {
	case CURVE_MATRIX_MAX_350_LUX:
		convert_level = zte_backlight_curve_matrix_max_350_lux[bl];
		break;
	case CURVE_MATRIX_MAX_400_LUX:
		convert_level = zte_backlight_curve_matrix_max_400_lux[bl];
		break;
	case CURVE_MATRIX_MAX_450_LUX:
		convert_level = zte_backlight_curve_matrix_max_450_lux[bl];
		break;
	case CURVE_MATRIX_MAX_RM692C9_LUX:
		convert_level = zte_backlight_curve_matrix_max_rm692c9_lux[bl];
		break;
	default:
		convert_level = zte_backlight_curve_matrix_max_450_lux[bl];
		break;
	}

	if ((bl_max != 4095) && (bl_max != 1023) && (bl_max != 255)) {
		convert_level = convert_level * bl_max / 255;
	} else if (bl_max > 1023) {
		convert_level = (convert_level >= 255) ? 4095 : (convert_level<<4);
	} else if (bl_max > 255) {
		convert_level = (convert_level >= 255) ? 1023 : (convert_level<<2);
	}

	return convert_level;
}
#endif

/*************************************************************
 * lcd gpio power ctrl
 * Helper function to control the added GPIO
*************************************************************/
#ifdef CONFIG_ZTE_LCD_GPIO_CTRL_POWER
static int zte_gpio_ctrl_lcd_power_enable(int enable)
{
	pr_info("[MSM_LCD] %s:%s\n", __func__, enable ? "enable":"disable");
	if (enable) {
		usleep_range(5000, 5100);
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio, 1);
			usleep_range(5000, 5100);
		}
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio, 1);
			usleep_range(5000, 5100);
		}
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio, 1);
			usleep_range(5000, 5100);
		}
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio, 1);
			usleep_range(5000, 5100);
		}
	} else {
		usleep_range(1000, 1100);
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio, 0);
			usleep_range(5000, 5100);
		}
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio, 0);
			usleep_range(5000, 5100);
		}
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio, 0);
			usleep_range(5000, 5100);
		}
		if (g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio > 0) {
			gpio_set_value(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio, 0);
			usleep_range(5000, 5100);
		}
	}
	return 0;
}
static int zte_gpio_ctrl_lcd_power_gpio_dt(struct device_node *node)
{
	g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio = of_get_named_gpio(node,
		"zte,disp_avdd_en_gpio", 0);
	if (!gpio_is_valid(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio)) {
		pr_info("[MSM_LCD]%s:%d, zte,disp_avdd_en_gpio not specified\n", __func__, __LINE__);
	} else {
		if (gpio_request(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio, "disp_avdd_en_gpio")) {
			pr_info("request disp_avdd_en_gpio failed\n");
		} else {
			gpio_direction_output(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_avdd_en_gpio, 1);
			pr_info("%s:request disp_avdd_en_gpio success\n", __func__);
		}
	}
	g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio = of_get_named_gpio(node,
		"zte,disp_iovdd_en_gpio", 0);
	if (!gpio_is_valid(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio)) {
		pr_info("[MSM_LCD]%s:%d, zte,disp_iovdd_en_gpio not specified\n", __func__, __LINE__);
	} else {
		if (gpio_request(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio, "disp_iovdd_en_gpio")) {
			pr_info("request disp_iovdd_en_gpio failed %d\n",
				g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio);
		} else {
			gpio_direction_output(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_iovdd_en_gpio, 1);
			pr_info("%s:request disp_iovdd_en_gpio success\n", __func__);
		}
	}

	g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio = of_get_named_gpio(node,
		"zte,disp_vsp_en_gpio", 0);
	if (!gpio_is_valid(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio)) {
		pr_info("[MSM_LCD]%s:%d, zte,disp_vsp_en_gpio not specified\n", __func__, __LINE__);
	} else {
		if (gpio_request(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio, "disp_vsp_en_gpio")) {
			pr_info("[MSM_LCD]request disp_vsp_en_gpio failed\n");
		} else {
			gpio_direction_output(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsp_en_gpio, 1);
			pr_info("[MSM_LCD]%s:request disp_vsp_en_gpio success\n", __func__);
		}
	}
	g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio = of_get_named_gpio(node,
		"zte,disp_vsn_en_gpio", 0);
	if (!gpio_is_valid(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio)) {
		pr_info("[MSM_LCD]%s:%d, zte,disp_vsn_en_gpio not specified\n", __func__, __LINE__);
	} else {
		if (gpio_request(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio, "disp_vsn_en_gpio")) {
			pr_info("[MSM_LCD]request disp_vsn_en_gpio failed %d\n",
				g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio);
		} else {
			gpio_direction_output(g_zte_ctrl_pdata->zte_lcd_ctrl->disp_vsn_en_gpio, 1);
			pr_info("[MSM_LCD]%s:request disp_vsn_en_gpio success\n", __func__);
		}
	}

	return 0;
}
#endif

/*************************************************************
 * lcd common function
*************************************************************/
static void zte_lcd_panel_parse_dt(struct device_node *node)
{
	int rc;
#ifdef CONFIG_ZTE_LCD_BACKLIGHT_LEVEL_CURVE
	const char *data;
#endif
#ifdef CONFIG_ZTE_LCD_VSP_VSN_VALUE_BY_I2C
	u32 vsp_vsn_value;
#endif

	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_reset_high_sleeping = of_property_read_bool(node,
										"zte,lcm_reset_pin_keep_high_sleeping");

	rc = of_property_read_u32(node, "zte,lcd_dimreg_value",
			&g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_dimreg_value);
	if (rc) {
		g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_dimreg_value = 0x0;
	}
	pr_info("[MSM_LCD]%s lcd_dimreg_value = %x ", __func__,g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_dimreg_value);
	
	rc = of_property_read_u32(node, "zte,lcd_aod_hbm_reg51_ctrl",
			&g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_aod_hbm_reg_ctrl);
	if (rc) {
		g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_aod_hbm_reg_ctrl = 0;
		pr_info("[MSM_LCD]%s hbm and aod reg ctrl is not 51.", __func__);
	} else {
		rc = of_property_read_u32(node, "zte,lcd_hbm_off_reg51_index",
			&g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_off_reg51_index);
		if (!rc) {
			pr_info("[MSM_LCD]%s hbm and aod reg51 index is %d.", __func__,g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_off_reg51_index);
		}
	}
	pr_info("[MSM_LCD]%s lcd_aod_hbm_reg_ctrl = %d ", __func__,g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_aod_hbm_reg_ctrl);
#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	rc = of_property_read_u32(node, "zte,lcd_hbm_max_bl",
			&g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_max_bl);
	if (!rc) {
		pr_info("[MSM_LCD]%s HDR and HBM max bl_lvl is %d.", __func__,g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_hbm_max_bl);
	}
#endif	
#ifdef CONFIG_ZTE_LCD_GPIO_CTRL_POWER
	zte_gpio_ctrl_lcd_power_gpio_dt(node);
#endif

#ifdef CONFIG_ZTE_LCD_BACKLIGHT_LEVEL_CURVE
	data = of_get_property(node, "zte,lcm_backlight_curve_mode", NULL);
	if (data) {
		if (!strcmp(data, "lcd_brightness_max_350_lux"))
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode = CURVE_MATRIX_MAX_350_LUX;
		else if (!strcmp(data, "lcd_brightness_max_400_lux"))
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode = CURVE_MATRIX_MAX_400_LUX;
		else if (!strcmp(data, "lcd_brightness_max_450_lux"))
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode = CURVE_MATRIX_MAX_450_LUX;
		else if (!strcmp(data, "lcd_brightness_max_rm692c9_lux"))
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode = CURVE_MATRIX_MAX_RM692C9_LUX;
		else
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode = CURVE_MATRIX_MAX_450_LUX;
	} else
		g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode = CURVE_MATRIX_MAX_450_LUX;

	pr_info("[MSM_LCD]%s:dtsi_mode=%s matrix_mode=%d\n", __func__, data,
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_bl_curve_mode);
#endif

#ifdef CONFIG_ZTE_LCD_VSP_VSN_VALUE_BY_I2C
	rc = of_property_read_u32(node, "zte,lcd_vsp_vsn_voltage", &vsp_vsn_value);
	g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_vsp_vsn_voltage = (!rc ? vsp_vsn_value : 0xf);
	pr_info("[MSM_LCD]%s rc=%d,lcd_vsp_vsn_voltage = %d\n", __func__, rc,
			g_zte_ctrl_pdata->zte_lcd_ctrl->lcd_vsp_vsn_voltage);
#endif

}
void zte_lcd_common_func(struct dsi_panel *panel, struct device_node *node)
{
	g_zte_ctrl_pdata = panel;
	/*kzalloc zte_lcd_ctrl,must use always in whole life,don't need to free zte_lcd_ctrl*/
	g_zte_ctrl_pdata->zte_lcd_ctrl = kzalloc(sizeof(struct zte_lcd_ctrl_data), GFP_KERNEL);

	if (!g_zte_ctrl_pdata->zte_lcd_ctrl) {
		pr_err("%s:kzalloc memory failed\n", __func__);
		return;
	}

	zte_lcd_panel_parse_dt(node);
	zte_lcd_proc_info_display(node);
#ifdef CONFIG_ZTE_LCD_HBM_CTRL
	zte_lcd_hbm_ctrl(node);
	zte_lcd_set_hdr_flag(node);
#endif
#ifdef CONFIG_ZTE_LCD_SEC_PANEL_CTRL
	zte_lcd_sec_panel_ctrl(node);
#endif
#ifdef CONFIG_ZTE_LCD_COLOR_GAMUT_CTRL
	zte_lcd_color_gamut_ctrl(node);
#endif
#ifdef CONFIG_ZTE_LCD_AOD_BRIGHTNESS_CTRL
	zte_lcd_set_aod_brightness(node);
#endif
#ifdef CONFIG_ZTE_LCD_GPIO_CTRL_POWER
	g_zte_ctrl_pdata->zte_lcd_ctrl->gpio_enable_lcd_power = zte_gpio_ctrl_lcd_power_enable;
#endif
#ifdef CONFIG_ZTE_LCD_BACKLIGHT_LEVEL_CURVE
	g_zte_ctrl_pdata->zte_lcd_ctrl->zte_convert_brightness = zte_convert_backlevel_function;
#endif
#ifdef CONFIG_ZTE_LCD_REG_DEBUG
	zte_lcd_reg_debug_func();
#endif
}


