#include <linux/module.h>
#include <linux/crc32.h>
#include <media/cam_sensor.h>
#include "zte_cam_eeprom_dev.h"
#include "zte_cam_eeprom_core.h"
#include "cam_debug_util.h"

#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_SUNNY		0x01
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_TRULY		0x02
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_A_KERR		0x03
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_LITEARRAY	0x04
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_DARLING	    0x05
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_QTECH		0x06
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_OFLIM		0x07
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_FOXCONN	    0x11
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_IMPORTEK	0x12
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_ALTEK		0x13
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_ABICO		0x14
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_LITE_ON 	0x15
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_CHICONY	    0x16
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_PRIMAX		0x17
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_SHARP		0x21
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_LITE_ON_N	0x22
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_MCNEX		0x31
#define OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_MCNEX_LSC	0xA0

MODULE_Map_Table OV32A1Q_FRONT_MODULE_MAP[] = {
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_SUNNY,
		"sunny_ov32a1q_front", "sunny_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_TRULY,
		"truly_ov32a1q_front", "truly_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_A_KERR,
		"a_kerr_ov32a1q_front", "a_kerr_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_LITEARRAY,
		"litearray_ov32a1q_front", "litearray_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_DARLING,
		"darling_ov32a1q_front", "darling_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_QTECH,
		"qtech_ov32a1q_front", "qtech_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_OFLIM,
		"oflim_ov32a1q_front", "oflim_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_FOXCONN,
		"foxconn_ov32a1q_front", "foxconn_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_IMPORTEK,
		"importek_ov32a1q_front", "importek_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_ALTEK,
		"altek_ov32a1q_front", "altek_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_ABICO,
		"abico_ov32a1q_front", "abico_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_LITE_ON,
		"lite_on_ov32a1q_front", "lite_on_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_CHICONY,
		"chicony_ov32a1q_front", "chicony_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_PRIMAX,
		"primax_ov32a1q_front", "primax_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_SHARP,
		"sharp_ov32a1q_front", "sharp_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_LITE_ON_N,
		"lite_on_new_ov32a1q_front", "lite_on_new_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_MCNEX,
		"mcnex_ov32a1q_front", "mcnex_ov32a1q_front", NULL},
	{ OV32A1Q_FRONT_SENSOR_INFO_MODULE_ID_MCNEX_LSC,
		"mcnex_lsc_ov32a1q_front", "mcnex_lsc_ov32a1q_front", NULL},
};

#define OV32A1Q_FRONT_ID_ADDR 0x1

#define FLAG_MODULE_INFO_ADDR 0x0
#define FLAG_AWB_ADDR 0x1A
#define FLAG_LSC_ADDR 0x28

#define FLAG_VALID_VALUE 0x01

#define CS_MODULE_INFO_S_ADDR 0x01
#define CS_MODULE_INFO_E_ADDR 0x08
#define CS_MODULE_INFO_ADDR 0x09

#define CS_AWB_S_ADDR 0x1B
#define CS_AWB_E_ADDR 0x26
#define CS_AWB_ADDR (CS_AWB_E_ADDR+1)

#define CS_LSC_S_ADDR 0x29
#define CS_LSC_E_ADDR 0x710
#define CS_LSC_ADDR (CS_LSC_E_ADDR+1)

void ov32a1q_front_parse_module_name(struct cam_eeprom_ctrl_t *e_ctrl)
{
	uint16_t sensor_module_id = e_ctrl->cal_data.mapdata[OV32A1Q_FRONT_ID_ADDR];

	parse_module_name(&(e_ctrl->module_info[0]), OV32A1Q_FRONT_MODULE_MAP,
		sizeof(OV32A1Q_FRONT_MODULE_MAP) / sizeof(MODULE_Map_Table),
		sensor_module_id);
}

void ov32a1q_front_set_cali_info_valid(struct cam_eeprom_ctrl_t *e_ctrl)
{
	uint8_t *peepromdata = &(e_ctrl->cal_data.mapdata[0]);
	uint32_t num = e_ctrl->cal_data.num_data;

	pr_info("%s :%d:num = %d valid_flag=0x%x  checksum=0x%x\n",
		__func__, __LINE__, num, e_ctrl->valid_flag, e_ctrl->checksum);
	if ((e_ctrl->checksum & EEPROM_REMOSAIC_INFO_CHECKSUM_INVALID) ||
		(e_ctrl->valid_flag & EEPROM_REMOSAIC_INFO_INVALID)) {
		pr_info("%s :%d: REMOSAIC info invalid\n", __func__, __LINE__);
		peepromdata[num - 1] = 1;
	} else {
		peepromdata[num - 1] = 0;
	}

	if ((e_ctrl->checksum & EEPROM_LSC_INFO_CHECKSUM_INVALID) ||
		(e_ctrl->valid_flag & EEPROM_LSC_INFO_INVALID)) {
		pr_info("%s :%d: LSC info invalid\n", __func__, __LINE__);
		peepromdata[num - 2] = 1;
	} else {
		peepromdata[num - 2] = 0;
	}

	if ((e_ctrl->checksum & EEPROM_AWB_INFO_CHECKSUM_INVALID) ||
		(e_ctrl->valid_flag & EEPROM_AWB_INFO_INVALID)) {
		peepromdata[num - 3] = 1;
		pr_info("%s :%d: AWB info invalid\n", __func__, __LINE__);
	} else {
		peepromdata[num - 3] = 0;
	}
}

int ov32a1q_front_validflag_check_eeprom(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int  flag = 0;

	if (e_ctrl->cal_data.mapdata[FLAG_MODULE_INFO_ADDR] != FLAG_VALID_VALUE) {
		pr_err("%s :%d: module info flag invalid 0x%x\n",
			__func__, __LINE__, e_ctrl->cal_data.mapdata[FLAG_MODULE_INFO_ADDR]);
		flag |= EEPROM_MODULE_INFO_INVALID;
	}

	if (e_ctrl->cal_data.mapdata[FLAG_AWB_ADDR] != FLAG_VALID_VALUE) {
		pr_err("%s :%d: AWB flag invalid 0x%x\n",
			__func__, __LINE__, e_ctrl->cal_data.mapdata[FLAG_AWB_ADDR]);
		flag |= EEPROM_AWB_INFO_INVALID;
	}

	if (e_ctrl->cal_data.mapdata[FLAG_LSC_ADDR] != FLAG_VALID_VALUE) {
		pr_err("%s :%d: LSC flag invalid 0x%x\n",
			__func__, __LINE__, e_ctrl->cal_data.mapdata[FLAG_LSC_ADDR]);
		flag |= EEPROM_LSC_INFO_INVALID;
	}

	pr_info("%s :%d: valid info flag = 0x%x  %s\n",
		__func__, __LINE__, flag, (flag == 0) ? "true" : "false");

	return flag;

}

int ov32a1q_front_checksum_eeprom(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int  checksum = 0;
	int j;
	int rc = 0;

	pr_info("%s :%d: E", __func__, __LINE__);

	for (j = CS_MODULE_INFO_S_ADDR; j <= CS_MODULE_INFO_E_ADDR; j++)
		checksum += e_ctrl->cal_data.mapdata[j];

	if ((checksum % 256) != e_ctrl->cal_data.mapdata[CS_MODULE_INFO_ADDR]) {
		pr_err("%s :%d: module info checksum fail\n", __func__, __LINE__);
		rc  |= EEPROM_MODULE_INFO_CHECKSUM_INVALID;
	}

	checksum = 0;
	for (j = CS_AWB_S_ADDR; j <= CS_AWB_E_ADDR; j++)
		checksum += e_ctrl->cal_data.mapdata[j];

	if ((checksum % 256) != e_ctrl->cal_data.mapdata[CS_AWB_ADDR]) {
		pr_err("%s :%d: awb info checksum fail\n", __func__, __LINE__);
		rc  |= EEPROM_AWB_INFO_CHECKSUM_INVALID;
	}

	checksum = 0;
	for (j = CS_LSC_S_ADDR; j <= CS_LSC_E_ADDR; j++)
		checksum += e_ctrl->cal_data.mapdata[j];

	if ((checksum % 256) != e_ctrl->cal_data.mapdata[CS_LSC_ADDR]) {
		pr_err("%s :%d: lsc info checksum fail\n", __func__, __LINE__);
		rc  |= EEPROM_LSC_INFO_CHECKSUM_INVALID;
	}

	pr_info("%s :%d: cal info checksum rc = 0x%x %s\n",
			__func__, __LINE__, rc, (rc == 0) ? "true" : "false");
	return rc;
}

static struct zte_eeprom_fn_t ov32a1q_front_eeprom_func_tbl = {
	.read_eeprom_memory = NULL,
	.eeprom_match_crc = NULL,
	.eeprom_checksum = ov32a1q_front_checksum_eeprom,
	.validflag_check_eeprom = ov32a1q_front_validflag_check_eeprom,
	.parse_module_name = ov32a1q_front_parse_module_name,
	.set_cali_info_valid = ov32a1q_front_set_cali_info_valid,
	.read_id = NULL,
	.get_default_eeprom_setting_filename = NULL,
};

static const struct of_device_id ov32a1q_front_eeprom_dt_match[] = {
	{ .compatible = "zte,ov32a1q_front_eeprom", .data = &ov32a1q_front_eeprom_func_tbl},
	{ }
};
MODULE_DEVICE_TABLE(of, ov32a1q_front_eeprom_dt_match);

static int ov32a1q_front_eeprom_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;

	pr_info("%s:%d %s E", __func__, __LINE__, pdev->name);
	match = of_match_device(ov32a1q_front_eeprom_dt_match, &pdev->dev);
	if (match)
		rc = cam_eeprom_platform_driver_probe(pdev, match);
	else {
		pr_err("%s:%d match is null\n", __func__, __LINE__);
		rc = -EINVAL;
	}
	pr_info("%s:%d rc=%d X", __func__, __LINE__, rc);
	return rc;
}

static int ov32a1q_front_eeprom_platform_remove(struct platform_device *pdev)
{
	const struct of_device_id *match;
	int32_t rc = 0;

	pr_info("%s:%d E", __func__, __LINE__);
	match = of_match_device(ov32a1q_front_eeprom_dt_match, &pdev->dev);
	if (match)
		rc = cam_eeprom_platform_driver_remove(pdev);
	else {
		pr_err("%s:%d match is null\n", __func__, __LINE__);
		rc = -EINVAL;
	}
	pr_info("%s:%d X", __func__, __LINE__);
	return rc;
}

static struct platform_driver ov32a1q_front_eeprom_platform_driver = {
	.driver = {
		.name = "zte,ov32a1q_front_eeprom",
		.owner = THIS_MODULE,
		.of_match_table = ov32a1q_front_eeprom_dt_match,
	},
	.probe = ov32a1q_front_eeprom_platform_probe,
	.remove = ov32a1q_front_eeprom_platform_remove,
};

static int __init ov32a1q_front_eeprom_init_module(void)
{
	int rc = 0;

	rc = platform_driver_register(&ov32a1q_front_eeprom_platform_driver);
	pr_info("%s:%d platform rc %d\n", __func__, __LINE__, rc);
	return rc;
}

static void __exit ov32a1q_front_eeprom_exit_module(void)
{
	platform_driver_unregister(&ov32a1q_front_eeprom_platform_driver);

}

module_init(ov32a1q_front_eeprom_init_module);
module_exit(ov32a1q_front_eeprom_exit_module);
MODULE_DESCRIPTION("ZTE EEPROM driver");
MODULE_LICENSE("GPL v2");
