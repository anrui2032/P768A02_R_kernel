#include <linux/module.h>
#include <linux/crc32.h>
#include <media/cam_sensor.h>
#include "zte_cam_eeprom_dev.h"
#include "zte_cam_eeprom_core.h"
#include "cam_debug_util.h"

#define DEFAULT_EEPROM_SETTING_FILE_NAME "hi846_front_sm6350_default_eeprom_setting.fw"

#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SUNNY		0x01
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_TRULY		0x02
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_A_KERR		0x03
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_LITEARRAY	0x04
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_DARLING	    0x05
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_QTECH		0x06
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_OFLIM		0x07
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_FOXCONN	    0x11
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_IMPORTEK	    0x12
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_ALTEK		0x13
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_ABICO		0x14
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_LITE_ON	    0x15
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_CHICONY	    0x16
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_PRIMAX		0x17
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SHARP		0x21
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_MCNEX		0x31
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_HOLITECH		0x42
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_GOERTEK		0x54
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SHINETECH	0x55
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SUNWIN		0x56
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_JSL  		0x57
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_UNION		0x58
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SEASONS		0x59
#define hi846_front_sm6350_SENSOR_INFO_MODULE_ID_MCNEX_LSC	0xA0

MODULE_Map_Table HI846_FRONT_SM6350_MODULE_MAP[] = {
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SUNNY,
		"sunny_hi846_front_sm6350", "sunny_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_TRULY,
		"truly_hi846_front_sm6350", "truly_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_A_KERR,
		"a_kerr_hi846_front_sm6350", "a_kerr_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_LITEARRAY,
		"litearray_hi846_front_sm6350", "litearray_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_DARLING,
		"darling_hi846_front_sm6350", "darling_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_QTECH,
		"qtech_hi846_front_sm6350", "qtech_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_OFLIM,
		"oflim_hi846_front_sm6350", "oflim_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_FOXCONN,
		"foxconn_hi846_front_sm6350", "foxconn_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_IMPORTEK,
		"importek_hi846_front_sm6350", "importek_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_ALTEK,
		"altek_hi846_front_sm6350", "altek_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_ABICO,
		"abico_hi846_front_sm6350", "abico_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_LITE_ON,
		"lite_on_hi846_front_sm6350", "lite_on_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_CHICONY,
		"chicony_hi846_front_sm6350", "chicony_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_PRIMAX,
		"primax_hi846_front_sm6350", "primax_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SHARP,
		"sharp_hi846_front_sm6350", "sharp_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_MCNEX,
		"mcnex_hi846_front_sm6350", "mcnex_hi846_front_sm6350", NULL},
    { hi846_front_sm6350_SENSOR_INFO_MODULE_ID_HOLITECH,
		"holitech_hi846_front_sm6350", "holitech_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_GOERTEK,
		"goertek_hi846_front_sm6350", "goertek_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SHINETECH,
		"shinetech_hi846_front_sm6350", "shinetech_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SUNWIN,
		"Sunwin_hi846_front_sm6350", "Sunwin_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_JSL,
		"jsl_hi846_front_sm6350", "jsl_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_UNION,
		"union_hi846_front_sm6350", "union_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_SEASONS,
		"seasons_hi846_front_sm6350", "seasons_hi846_front_sm6350", NULL},
	{ hi846_front_sm6350_SENSOR_INFO_MODULE_ID_MCNEX_LSC,
		"mcnex_lsc_hi846_front_sm6350", "mcnex_lsc_hi846_front_sm6350", NULL},
};

#define HI846_FRONT_SM6350_ID_ADDR 0x01

#define FLAG_MODULE_INFO_ADDR 0x00
#define FLAG_AWB_ADDR 0xA5E

#define FLAG_VALID_VALUE 0x01

#define CS_MODULE_INFO_S_ADDR 0x01
#define CS_MODULE_INFO_E_ADDR 0x10
#define CS_MODULE_INFO_ADDR (CS_MODULE_INFO_E_ADDR+1)

#define CS_AWB_S_ADDR 0xA5F
#define CS_AWB_E_ADDR 0xA7B
#define CS_AWB_ADDR (CS_AWB_E_ADDR+1)


void hi846_front_sm6350_parse_module_name(struct cam_eeprom_ctrl_t *e_ctrl)
{
	uint16_t sensor_module_id = e_ctrl->cal_data.mapdata[HI846_FRONT_SM6350_ID_ADDR];
	pr_info("%s :%d:sensor_module_id = %d \n",__func__, __LINE__, sensor_module_id);

	parse_module_name(&(e_ctrl->module_info[0]), HI846_FRONT_SM6350_MODULE_MAP,
		sizeof(HI846_FRONT_SM6350_MODULE_MAP) / sizeof(MODULE_Map_Table),
		sensor_module_id);
}

void hi846_front_sm6350_set_cali_info_valid(struct cam_eeprom_ctrl_t *e_ctrl)
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

int hi846_front_sm6350_validflag_check_eeprom(struct cam_eeprom_ctrl_t *e_ctrl)
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

	pr_info("%s :%d: valid info flag = 0x%x  %s\n",
		__func__, __LINE__, flag, (flag == 0) ? "true" : "false");

	return flag;

}

int hi846_front_sm6350_checksum_eeprom(struct cam_eeprom_ctrl_t *e_ctrl)
{
	int  checksum = 0;
	int j;
	int rc = 0;

	pr_info("%s :%d: E", __func__, __LINE__);

	for (j = CS_MODULE_INFO_S_ADDR; j <= CS_MODULE_INFO_E_ADDR; j++)
		checksum += e_ctrl->cal_data.mapdata[j];

	if ((checksum % 255 + 1) != e_ctrl->cal_data.mapdata[CS_MODULE_INFO_ADDR]) {
		pr_err("%s :%d: module info checksum fail\n", __func__, __LINE__);
		rc  |= EEPROM_MODULE_INFO_CHECKSUM_INVALID;
	}

	checksum = 0;
	for (j = CS_AWB_S_ADDR; j <= CS_AWB_E_ADDR; j++)
		checksum += e_ctrl->cal_data.mapdata[j];

	if ((checksum % 255 + 1) != e_ctrl->cal_data.mapdata[CS_AWB_ADDR]) {
		pr_err("%s :%d: awb info checksum fail\n", __func__, __LINE__);
		rc  |= EEPROM_AWB_INFO_CHECKSUM_INVALID;
	}

	pr_info("%s :%d: cal info checksum rc = 0x%x %s\n",
			__func__, __LINE__, rc, (rc == 0) ? "true" : "false");
	return rc;
}

uint8_t *get_hi846_front_sm6350_default_setting_filename(struct cam_eeprom_ctrl_t *e_ctrl)
{
	return DEFAULT_EEPROM_SETTING_FILE_NAME;
}

#define HI846_WRITE_REG(addr,data) \
			rc = zte_cam_cci_i2c_write(&e_ctrl->io_master_info,addr,data,CAMERA_SENSOR_I2C_TYPE_WORD,CAMERA_SENSOR_I2C_TYPE_WORD); \
			if (rc) {                                                                                                              \
				pr_err("init write failed rc %d",rc);                                                                 \
				return rc;                                                                                                         \
			}

/**
 * hi846_front_sm6350_cam_eeprom_read_memory() - read map data into buffer
 * @e_ctrl:     eeprom control struct
 * @block:      block to be read
 *
 * This function iterates through blocks stored in block->map, reads each
 * region and concatenate them into the pre-allocated block->mapdata
 */
static int hi846_front_sm6350_read_eeprom_memory(struct cam_eeprom_ctrl_t *e_ctrl,
	struct cam_eeprom_memory_block_t *block)
{
	int                                rc = 0;
	int                                j,gc;
	struct cam_sensor_i2c_reg_setting  i2c_reg_settings = {0};
	struct cam_sensor_i2c_reg_array    i2c_reg_array = {0};
	struct cam_eeprom_memory_map_t    *emap = block->map;
	struct cam_eeprom_soc_private     *eb_info = NULL;
	uint8_t                           *memptr = block->mapdata;
	uint8_t                           gc_read = 0;

	if (!e_ctrl) {
		pr_err("e_ctrl is NULL");
		return -EINVAL;
	}

	eb_info = (struct cam_eeprom_soc_private *)e_ctrl->soc_info.soc_private;

	HI846_WRITE_REG(0x2000, 0x987A);
	HI846_WRITE_REG(0x2002, 0x00FF);
	HI846_WRITE_REG(0x2004, 0x0047);
	HI846_WRITE_REG(0x2006, 0x3FFF);
	HI846_WRITE_REG(0x2008, 0x3FFF);
	HI846_WRITE_REG(0x200A, 0xC216);
	HI846_WRITE_REG(0x200C, 0x1292);
	HI846_WRITE_REG(0x200E, 0xC01A);
	HI846_WRITE_REG(0x2010, 0x403D);
	HI846_WRITE_REG(0x2012, 0x000E);
	HI846_WRITE_REG(0x2014, 0x403E);
	HI846_WRITE_REG(0x2016, 0x0B80);
	HI846_WRITE_REG(0x2018, 0x403F);
	HI846_WRITE_REG(0x201A, 0x82AE);
	HI846_WRITE_REG(0x201C, 0x1292);
	HI846_WRITE_REG(0x201E, 0xC00C);
	HI846_WRITE_REG(0x2020, 0x4130);
	HI846_WRITE_REG(0x2022, 0x43E2);
	HI846_WRITE_REG(0x2024, 0x0180);
	HI846_WRITE_REG(0x2026, 0x4130);
	HI846_WRITE_REG(0x2028, 0x7400);
	HI846_WRITE_REG(0x202A, 0x5000);
	HI846_WRITE_REG(0x202C, 0x0253);
	HI846_WRITE_REG(0x202E, 0x0AD1);
	HI846_WRITE_REG(0x2030, 0x2360);
	HI846_WRITE_REG(0x2032, 0x0009);
	HI846_WRITE_REG(0x2034, 0x5020);
	HI846_WRITE_REG(0x2036, 0x000B);
	HI846_WRITE_REG(0x2038, 0x0002);
	HI846_WRITE_REG(0x203A, 0x0044);
	HI846_WRITE_REG(0x203C, 0x0016);
	HI846_WRITE_REG(0x203E, 0x1792);
	HI846_WRITE_REG(0x2040, 0x7002);
	HI846_WRITE_REG(0x2042, 0x154F);
	HI846_WRITE_REG(0x2044, 0x00D5);
	HI846_WRITE_REG(0x2046, 0x000B);
	HI846_WRITE_REG(0x2048, 0x0019);
	HI846_WRITE_REG(0x204A, 0x1698);
	HI846_WRITE_REG(0x204C, 0x000E);
	HI846_WRITE_REG(0x204E, 0x099A);
	HI846_WRITE_REG(0x2050, 0x0058);
	HI846_WRITE_REG(0x2052, 0x7000);
	HI846_WRITE_REG(0x2054, 0x1799);
	HI846_WRITE_REG(0x2056, 0x0310);
	HI846_WRITE_REG(0x2058, 0x03C3);
	HI846_WRITE_REG(0x205A, 0x004C);
	HI846_WRITE_REG(0x205C, 0x064A);
	HI846_WRITE_REG(0x205E, 0x0001);
	HI846_WRITE_REG(0x2060, 0x0007);
	HI846_WRITE_REG(0x2062, 0x0BC7);
	HI846_WRITE_REG(0x2064, 0x0055);
	HI846_WRITE_REG(0x2066, 0x7000);
	HI846_WRITE_REG(0x2068, 0x1550);
	HI846_WRITE_REG(0x206A, 0x158A);
	HI846_WRITE_REG(0x206C, 0x0004);
	HI846_WRITE_REG(0x206E, 0x1488);
	HI846_WRITE_REG(0x2070, 0x7010);
	HI846_WRITE_REG(0x2072, 0x1508);
	HI846_WRITE_REG(0x2074, 0x0004);
	HI846_WRITE_REG(0x2076, 0x0016);
	HI846_WRITE_REG(0x2078, 0x03D5);
	HI846_WRITE_REG(0x207A, 0x0055);
	HI846_WRITE_REG(0x207C, 0x08CA);
	HI846_WRITE_REG(0x207E, 0x2019);
	HI846_WRITE_REG(0x2080, 0x0007);
	HI846_WRITE_REG(0x2082, 0x7057);
	HI846_WRITE_REG(0x2084, 0x0FC7);
	HI846_WRITE_REG(0x2086, 0x5041);
	HI846_WRITE_REG(0x2088, 0x12C8);
	HI846_WRITE_REG(0x208A, 0x5060);
	HI846_WRITE_REG(0x208C, 0x5080);
	HI846_WRITE_REG(0x208E, 0x2084);
	HI846_WRITE_REG(0x2090, 0x12C8);
	HI846_WRITE_REG(0x2092, 0x7800);
	HI846_WRITE_REG(0x2094, 0x0802);
	HI846_WRITE_REG(0x2096, 0x040F);
	HI846_WRITE_REG(0x2098, 0x1007);
	HI846_WRITE_REG(0x209A, 0x0803);
	HI846_WRITE_REG(0x209C, 0x080B);
	HI846_WRITE_REG(0x209E, 0x3803);
	HI846_WRITE_REG(0x20A0, 0x0807);
	HI846_WRITE_REG(0x20A2, 0x0404);
	HI846_WRITE_REG(0x20A4, 0x0400);
	HI846_WRITE_REG(0x20A6, 0xFFFF);
	HI846_WRITE_REG(0x20A8, 0xF0B2);
	HI846_WRITE_REG(0x20AA, 0xFFEF);
	HI846_WRITE_REG(0x20AC, 0x0A84);
	HI846_WRITE_REG(0x20AE, 0x1292);
	HI846_WRITE_REG(0x20B0, 0xC02E);
	HI846_WRITE_REG(0x20B2, 0x4130);
	HI846_WRITE_REG(0x20B4, 0xF0B2);
	HI846_WRITE_REG(0x20B6, 0xFFBF);
	HI846_WRITE_REG(0x20B8, 0x2004);
	HI846_WRITE_REG(0x20BA, 0x403F);
	HI846_WRITE_REG(0x20BC, 0x00C3);
	HI846_WRITE_REG(0x20BE, 0x4FE2);
	HI846_WRITE_REG(0x20C0, 0x8318);
	HI846_WRITE_REG(0x20C2, 0x43CF);
	HI846_WRITE_REG(0x20C4, 0x0000);
	HI846_WRITE_REG(0x20C6, 0x9382);
	HI846_WRITE_REG(0x20C8, 0xC314);
	HI846_WRITE_REG(0x20CA, 0x2003);
	HI846_WRITE_REG(0x20CC, 0x12B0);
	HI846_WRITE_REG(0x20CE, 0xCAB0);
	HI846_WRITE_REG(0x20D0, 0x4130);
	HI846_WRITE_REG(0x20D2, 0x12B0);
	HI846_WRITE_REG(0x20D4, 0xC90A);
	HI846_WRITE_REG(0x20D6, 0x4130);
	HI846_WRITE_REG(0x20D8, 0x42D2);
	HI846_WRITE_REG(0x20DA, 0x8318);
	HI846_WRITE_REG(0x20DC, 0x00C3);
	HI846_WRITE_REG(0x20DE, 0x9382);
	HI846_WRITE_REG(0x20E0, 0xC314);
	HI846_WRITE_REG(0x20E2, 0x2009);
	HI846_WRITE_REG(0x20E4, 0x120B);
	HI846_WRITE_REG(0x20E6, 0x120A);
	HI846_WRITE_REG(0x20E8, 0x1209);
	HI846_WRITE_REG(0x20EA, 0x1208);
	HI846_WRITE_REG(0x20EC, 0x1207);
	HI846_WRITE_REG(0x20EE, 0x1206);
	HI846_WRITE_REG(0x20F0, 0x4030);
	HI846_WRITE_REG(0x20F2, 0xC15E);
	HI846_WRITE_REG(0x20F4, 0x4130);
	HI846_WRITE_REG(0x20F6, 0x1292);
	HI846_WRITE_REG(0x20F8, 0xC008);
	HI846_WRITE_REG(0x20FA, 0x4130);
	HI846_WRITE_REG(0x20FC, 0x42D2);
	HI846_WRITE_REG(0x20FE, 0x82A1);
	HI846_WRITE_REG(0x2100, 0x00C2);
	HI846_WRITE_REG(0x2102, 0x1292);
	HI846_WRITE_REG(0x2104, 0xC040);
	HI846_WRITE_REG(0x2106, 0x4130);
	HI846_WRITE_REG(0x2108, 0x1292);
	HI846_WRITE_REG(0x210A, 0xC006);
	HI846_WRITE_REG(0x210C, 0x42A2);
	HI846_WRITE_REG(0x210E, 0x7324);
	HI846_WRITE_REG(0x2110, 0x9382);
	HI846_WRITE_REG(0x2112, 0xC314);
	HI846_WRITE_REG(0x2114, 0x2011);
	HI846_WRITE_REG(0x2116, 0x425F);
	HI846_WRITE_REG(0x2118, 0x82A1);
	HI846_WRITE_REG(0x211A, 0xF25F);
	HI846_WRITE_REG(0x211C, 0x00C1);
	HI846_WRITE_REG(0x211E, 0xF35F);
	HI846_WRITE_REG(0x2120, 0x2406);
	HI846_WRITE_REG(0x2122, 0x425F);
	HI846_WRITE_REG(0x2124, 0x00C0);
	HI846_WRITE_REG(0x2126, 0xF37F);
	HI846_WRITE_REG(0x2128, 0x522F);
	HI846_WRITE_REG(0x212A, 0x4F82);
	HI846_WRITE_REG(0x212C, 0x7324);
	HI846_WRITE_REG(0x212E, 0x425F);
	HI846_WRITE_REG(0x2130, 0x82D4);
	HI846_WRITE_REG(0x2132, 0xF35F);
	HI846_WRITE_REG(0x2134, 0x4FC2);
	HI846_WRITE_REG(0x2136, 0x01B3);
	HI846_WRITE_REG(0x2138, 0x93C2);
	HI846_WRITE_REG(0x213A, 0x829F);
	HI846_WRITE_REG(0x213C, 0x2421);
	HI846_WRITE_REG(0x213E, 0x403E);
	HI846_WRITE_REG(0x2140, 0xFFFE);
	HI846_WRITE_REG(0x2142, 0x40B2);
	HI846_WRITE_REG(0x2144, 0xEC78);
	HI846_WRITE_REG(0x2146, 0x831C);
	HI846_WRITE_REG(0x2148, 0x40B2);
	HI846_WRITE_REG(0x214A, 0xEC78);
	HI846_WRITE_REG(0x214C, 0x831E);
	HI846_WRITE_REG(0x214E, 0x40B2);
	HI846_WRITE_REG(0x2150, 0xEC78);
	HI846_WRITE_REG(0x2152, 0x8320);
	HI846_WRITE_REG(0x2154, 0xB3D2);
	HI846_WRITE_REG(0x2156, 0x008C);
	HI846_WRITE_REG(0x2158, 0x2405);
	HI846_WRITE_REG(0x215A, 0x4E0F);
	HI846_WRITE_REG(0x215C, 0x503F);
	HI846_WRITE_REG(0x215E, 0xFFD8);
	HI846_WRITE_REG(0x2160, 0x4F82);
	HI846_WRITE_REG(0x2162, 0x831C);
	HI846_WRITE_REG(0x2164, 0x90F2);
	HI846_WRITE_REG(0x2166, 0x0003);
	HI846_WRITE_REG(0x2168, 0x008C);
	HI846_WRITE_REG(0x216A, 0x2401);
	HI846_WRITE_REG(0x216C, 0x4130);
	HI846_WRITE_REG(0x216E, 0x421F);
	HI846_WRITE_REG(0x2170, 0x831C);
	HI846_WRITE_REG(0x2172, 0x5E0F);
	HI846_WRITE_REG(0x2174, 0x4F82);
	HI846_WRITE_REG(0x2176, 0x831E);
	HI846_WRITE_REG(0x2178, 0x5E0F);
	HI846_WRITE_REG(0x217A, 0x4F82);
	HI846_WRITE_REG(0x217C, 0x8320);
	HI846_WRITE_REG(0x217E, 0x3FF6);
	HI846_WRITE_REG(0x2180, 0x432E);
	HI846_WRITE_REG(0x2182, 0x3FDF);
	HI846_WRITE_REG(0x2184, 0x421F);
	HI846_WRITE_REG(0x2186, 0x7100);
	HI846_WRITE_REG(0x2188, 0x4F0E);
	HI846_WRITE_REG(0x218A, 0x503E);
	HI846_WRITE_REG(0x218C, 0xFFD8);
	HI846_WRITE_REG(0x218E, 0x4E82);
	HI846_WRITE_REG(0x2190, 0x7A04);
	HI846_WRITE_REG(0x2192, 0x421E);
	HI846_WRITE_REG(0x2194, 0x831C);
	HI846_WRITE_REG(0x2196, 0x5F0E);
	HI846_WRITE_REG(0x2198, 0x4E82);
	HI846_WRITE_REG(0x219A, 0x7A06);
	HI846_WRITE_REG(0x219C, 0x0B00);
	HI846_WRITE_REG(0x219E, 0x7304);
	HI846_WRITE_REG(0x21A0, 0x0050);
	HI846_WRITE_REG(0x21A2, 0x40B2);
	HI846_WRITE_REG(0x21A4, 0xD081);
	HI846_WRITE_REG(0x21A6, 0x0B88);
	HI846_WRITE_REG(0x21A8, 0x421E);
	HI846_WRITE_REG(0x21AA, 0x831E);
	HI846_WRITE_REG(0x21AC, 0x5F0E);
	HI846_WRITE_REG(0x21AE, 0x4E82);
	HI846_WRITE_REG(0x21B0, 0x7A0E);
	HI846_WRITE_REG(0x21B2, 0x521F);
	HI846_WRITE_REG(0x21B4, 0x8320);
	HI846_WRITE_REG(0x21B6, 0x4F82);
	HI846_WRITE_REG(0x21B8, 0x7A10);
	HI846_WRITE_REG(0x21BA, 0x0B00);
	HI846_WRITE_REG(0x21BC, 0x7304);
	HI846_WRITE_REG(0x21BE, 0x007A);
	HI846_WRITE_REG(0x21C0, 0x40B2);
	HI846_WRITE_REG(0x21C2, 0x0081);
	HI846_WRITE_REG(0x21C4, 0x0B88);
	HI846_WRITE_REG(0x21C6, 0x4392);
	HI846_WRITE_REG(0x21C8, 0x7A0A);
	HI846_WRITE_REG(0x21CA, 0x0800);
	HI846_WRITE_REG(0x21CC, 0x7A0C);
	HI846_WRITE_REG(0x21CE, 0x0B00);
	HI846_WRITE_REG(0x21D0, 0x7304);
	HI846_WRITE_REG(0x21D2, 0x022B);
	HI846_WRITE_REG(0x21D4, 0x40B2);
	HI846_WRITE_REG(0x21D6, 0xD081);
	HI846_WRITE_REG(0x21D8, 0x0B88);
	HI846_WRITE_REG(0x21DA, 0x0B00);
	HI846_WRITE_REG(0x21DC, 0x7304);
	HI846_WRITE_REG(0x21DE, 0x0255);
	HI846_WRITE_REG(0x21E0, 0x40B2);
	HI846_WRITE_REG(0x21E2, 0x0081);
	HI846_WRITE_REG(0x21E4, 0x0B88);
	HI846_WRITE_REG(0x21E6, 0x4130);
	HI846_WRITE_REG(0x23FE, 0xC056);
	HI846_WRITE_REG(0x3232, 0xFC0C);
	HI846_WRITE_REG(0x3236, 0xFC22);
	HI846_WRITE_REG(0x3238, 0xFCFC);
	HI846_WRITE_REG(0x323A, 0xFD84);
	HI846_WRITE_REG(0x323C, 0xFD08);
	HI846_WRITE_REG(0x3246, 0xFCD8);
	HI846_WRITE_REG(0x3248, 0xFCA8);
	HI846_WRITE_REG(0x324E, 0xFCB4);
	HI846_WRITE_REG(0x326A, 0x8302);
	HI846_WRITE_REG(0x326C, 0x830A);
	HI846_WRITE_REG(0x326E, 0x0000);
	HI846_WRITE_REG(0x32CA, 0xFC28);
	HI846_WRITE_REG(0x32CC, 0xC3BC);
	HI846_WRITE_REG(0x32CE, 0xC34C);
	HI846_WRITE_REG(0x32D0, 0xC35A);
	HI846_WRITE_REG(0x32D2, 0xC368);
	HI846_WRITE_REG(0x32D4, 0xC376);
	HI846_WRITE_REG(0x32D6, 0xC3C2);
	HI846_WRITE_REG(0x32D8, 0xC3E6);
	HI846_WRITE_REG(0x32DA, 0x0003);
	HI846_WRITE_REG(0x32DC, 0x0003);
	HI846_WRITE_REG(0x32DE, 0x00C7);
	HI846_WRITE_REG(0x32E0, 0x0031);
	HI846_WRITE_REG(0x32E2, 0x0031);
	HI846_WRITE_REG(0x32E4, 0x0031);
	HI846_WRITE_REG(0x32E6, 0xFC28);
	HI846_WRITE_REG(0x32E8, 0xC3BC);
	HI846_WRITE_REG(0x32EA, 0xC384);
	HI846_WRITE_REG(0x32EC, 0xC392);
	HI846_WRITE_REG(0x32EE, 0xC3A0);
	HI846_WRITE_REG(0x32F0, 0xC3AE);
	HI846_WRITE_REG(0x32F2, 0xC3C4);
	HI846_WRITE_REG(0x32F4, 0xC3E6);
	HI846_WRITE_REG(0x32F6, 0x0003);
	HI846_WRITE_REG(0x32F8, 0x0003);
	HI846_WRITE_REG(0x32FA, 0x00C7);
	HI846_WRITE_REG(0x32FC, 0x0031);
	HI846_WRITE_REG(0x32FE, 0x0031);
	HI846_WRITE_REG(0x3300, 0x0031);
	HI846_WRITE_REG(0x3302, 0x82CA);
	HI846_WRITE_REG(0x3304, 0xC164);
	HI846_WRITE_REG(0x3306, 0x82E6);
	HI846_WRITE_REG(0x3308, 0xC19C);
	HI846_WRITE_REG(0x330A, 0x001F);
	HI846_WRITE_REG(0x330C, 0x001A);
	HI846_WRITE_REG(0x330E, 0x0034);
	HI846_WRITE_REG(0x3310, 0x0000);
	HI846_WRITE_REG(0x3312, 0x0000);
	HI846_WRITE_REG(0x3314, 0xFC94);
	HI846_WRITE_REG(0x3316, 0xC3D8);
	HI846_WRITE_REG(0x0A00, 0x0000);
	HI846_WRITE_REG(0x0E04, 0x0012);
	HI846_WRITE_REG(0x002E, 0x1111);
	HI846_WRITE_REG(0x0032, 0x1111);
	HI846_WRITE_REG(0x0022, 0x0008);
	HI846_WRITE_REG(0x0026, 0x0040);
	HI846_WRITE_REG(0x0028, 0x0017);
	HI846_WRITE_REG(0x002C, 0x09CF);
	HI846_WRITE_REG(0x005C, 0x2101);
	HI846_WRITE_REG(0x0006, 0x09DE);
	HI846_WRITE_REG(0x0008, 0x0ED8);
	HI846_WRITE_REG(0x000E, 0x0100);
	HI846_WRITE_REG(0x000C, 0x0022);
	HI846_WRITE_REG(0x0A22, 0x0000);
	HI846_WRITE_REG(0x0A24, 0x0000);
	HI846_WRITE_REG(0x0804, 0x0000);
	HI846_WRITE_REG(0x0A12, 0x0CC0);
	HI846_WRITE_REG(0x0A14, 0x0990);
	HI846_WRITE_REG(0x0074, 0x09DE);
	HI846_WRITE_REG(0x0076, 0x0000);
	HI846_WRITE_REG(0x051E, 0x0000);
	HI846_WRITE_REG(0x0200, 0x0400);
	HI846_WRITE_REG(0x0A1A, 0x0C00);
	HI846_WRITE_REG(0x0A0C, 0x0010);
	HI846_WRITE_REG(0x0A1E, 0x0CCF);
	HI846_WRITE_REG(0x0402, 0x0110);
	HI846_WRITE_REG(0x0404, 0x00F4);
	HI846_WRITE_REG(0x0408, 0x0000);
	HI846_WRITE_REG(0x0410, 0x008D);
	HI846_WRITE_REG(0x0412, 0x011A);
	HI846_WRITE_REG(0x0414, 0x864C);
	HI846_WRITE_REG(0x021C, 0x0001);
	HI846_WRITE_REG(0x0C00, 0x9950);
	HI846_WRITE_REG(0x0C06, 0x0021);
	HI846_WRITE_REG(0x0C10, 0x0040);
	HI846_WRITE_REG(0x0C12, 0x0040);
	HI846_WRITE_REG(0x0C14, 0x0040);
	HI846_WRITE_REG(0x0C16, 0x0040);
	HI846_WRITE_REG(0x0A02, 0x0100);
	HI846_WRITE_REG(0x0A04, 0x014A);
	HI846_WRITE_REG(0x0418, 0x0000);
	HI846_WRITE_REG(0x012A, 0xFFFF);
	HI846_WRITE_REG(0x0120, 0x0046);
	HI846_WRITE_REG(0x0122, 0x0376);
	HI846_WRITE_REG(0x0746, 0x0050);
	HI846_WRITE_REG(0x0748, 0x01D5);
	HI846_WRITE_REG(0x074A, 0x022B);
	HI846_WRITE_REG(0x074C, 0x03B0);
	HI846_WRITE_REG(0x0756, 0x043F);
	HI846_WRITE_REG(0x0758, 0x3F1D);
	HI846_WRITE_REG(0x0B02, 0xE04D);
	HI846_WRITE_REG(0x0B10, 0x6821);
	HI846_WRITE_REG(0x0B12, 0x0120);
	HI846_WRITE_REG(0x0B14, 0x0001);
	HI846_WRITE_REG(0x2008, 0x38FD);
	HI846_WRITE_REG(0x326E, 0x0000);
	HI846_WRITE_REG(0x0900, 0x0320);
	HI846_WRITE_REG(0x0902, 0xC31A);
	HI846_WRITE_REG(0x0914, 0xC109);
	HI846_WRITE_REG(0x0916, 0x061A);
	HI846_WRITE_REG(0x0918, 0x0306);
	HI846_WRITE_REG(0x091A, 0x0B09);
	HI846_WRITE_REG(0x091C, 0x0C07);
	HI846_WRITE_REG(0x091E, 0x0A00);
	HI846_WRITE_REG(0x090C, 0x042A);
	HI846_WRITE_REG(0x090E, 0x006B);
	HI846_WRITE_REG(0x0954, 0x0089);
	HI846_WRITE_REG(0x0956, 0x0000);
	HI846_WRITE_REG(0x0958, 0xCA00);
	HI846_WRITE_REG(0x095A, 0x924E);
	HI846_WRITE_REG(0x0F08, 0x2F04);
	HI846_WRITE_REG(0x0F30, 0x001F);
	HI846_WRITE_REG(0x0F36, 0x001F);
	HI846_WRITE_REG(0x0F04, 0x3A00);
	HI846_WRITE_REG(0x0F32, 0x025A);
	HI846_WRITE_REG(0x0F38, 0x025A);
	HI846_WRITE_REG(0x0F2A, 0x0024);
	HI846_WRITE_REG(0x006A, 0x0100);
	HI846_WRITE_REG(0x004C, 0x0100);

	for (j = 0; j < block->num_map; j++) {
		if (emap[j].saddr) {
			eb_info->i2c_info.slave_addr = emap[j].saddr;
			rc = zte_cam_eeprom_update_i2c_info(e_ctrl,
				&eb_info->i2c_info);
			if (rc) {
				pr_err("failed: to update i2c info rc %d",rc);
				return rc;
			}
		}

		if (emap[j].page.valid_size) {
			i2c_reg_settings.addr_type = emap[j].page.addr_type;
			i2c_reg_settings.data_type = emap[j].page.data_type;
			i2c_reg_settings.size = 1;
			i2c_reg_settings.delay = emap[j].page.delay;
			i2c_reg_array.reg_addr = emap[j].page.addr;
			i2c_reg_array.reg_data = emap[j].page.data;
			i2c_reg_array.delay = emap[j].page.delay;
			i2c_reg_settings.reg_setting = &i2c_reg_array;

			rc = zte_cam_cci_i2c_write(&e_ctrl->io_master_info,
										emap[j].page.addr,
										emap[j].page.data,
										emap[j].page.addr_type,
										emap[j].page.data_type);
			if (rc) {
				pr_err( "page write failed rc %d",rc);
				return rc;
			}
			if (emap[j].page.delay > 20) {
					msleep(emap[j].page.delay);
			}
			else if (emap[j].page.delay > 1) {
				usleep_range(emap[j].page.delay * 1000, (emap[j].page.delay
						* 1000) + 1000);
			}
			memptr = block->mapdata + emap[j].mem.valid_size;
		}

		if (emap[j].pageen.valid_size) {
			i2c_reg_settings.addr_type = emap[j].pageen.addr_type;
			i2c_reg_settings.data_type = emap[j].pageen.data_type;
			i2c_reg_settings.size = 1;
			i2c_reg_array.reg_addr = emap[j].pageen.addr;
			i2c_reg_array.reg_data = emap[j].pageen.data;
			i2c_reg_array.delay = emap[j].pageen.delay;
			i2c_reg_settings.reg_setting = &i2c_reg_array;
			rc = camera_io_dev_write(&e_ctrl->io_master_info,
				&i2c_reg_settings);
			if (rc) {
				pr_err( "page enable failed rc %d",rc);
				return rc;
			}
		}

		if (emap[j].poll.valid_size) {
			rc = camera_io_dev_poll(&e_ctrl->io_master_info,
				emap[j].poll.addr, emap[j].poll.data,
				0, emap[j].poll.addr_type,
				emap[j].poll.data_type,
				emap[j].poll.delay);
			if (rc) {
				pr_err( "poll failed rc %d",rc);
				return rc;
			}
		}

		if (emap[j].mem.valid_size) {
			for (gc = 0; gc < emap[j].mem.valid_size; gc++) {
				rc = camera_io_dev_read_seq(&e_ctrl->io_master_info,
						emap[j].mem.addr, &gc_read,
						emap[j].mem.addr_type,
						emap[j].mem.data_type,
						1);
				if (rc < 0) {
					pr_err("%s: read failed\n",__func__);
					break;
				}
				*memptr = (uint8_t)gc_read;
				memptr++;
			}
		}

		if (emap[j].pageen.valid_size) {
			i2c_reg_settings.addr_type = emap[j].pageen.addr_type;
			i2c_reg_settings.data_type = emap[j].pageen.data_type;
			i2c_reg_settings.size = 1;
			i2c_reg_array.reg_addr = emap[j].pageen.addr;
			i2c_reg_array.reg_data = 0;
			i2c_reg_array.delay = emap[j].pageen.delay;
			i2c_reg_settings.reg_setting = &i2c_reg_array;
			rc = camera_io_dev_write(&e_ctrl->io_master_info,
				&i2c_reg_settings);
			if (rc) {
				pr_err("page disable failed rc %d",rc);
				return rc;
			}
		}
	}
	return rc;
}

static struct zte_eeprom_fn_t hi846_front_sm6350_eeprom_func_tbl = {
	.read_eeprom_memory = hi846_front_sm6350_read_eeprom_memory,
	.eeprom_match_crc = NULL,
	.eeprom_checksum = hi846_front_sm6350_checksum_eeprom,
	.validflag_check_eeprom = hi846_front_sm6350_validflag_check_eeprom,
	.parse_module_name = hi846_front_sm6350_parse_module_name,
	.set_cali_info_valid = hi846_front_sm6350_set_cali_info_valid,
	.read_id = NULL,
	.get_default_eeprom_setting_filename = get_hi846_front_sm6350_default_setting_filename,
};

static const struct of_device_id hi846_front_sm6350_eeprom_dt_match[] = {
	{ .compatible = "zte,hi846_front_sm6350_eeprom", .data = &hi846_front_sm6350_eeprom_func_tbl},
	{ }
};
MODULE_DEVICE_TABLE(of, hi846_front_sm6350_eeprom_dt_match);

static int hi846_front_sm6350_eeprom_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;

	pr_info("%s:%d %s E", __func__, __LINE__, pdev->name);
	match = of_match_device(hi846_front_sm6350_eeprom_dt_match, &pdev->dev);
	if (match)
		rc = cam_eeprom_platform_driver_probe(pdev, match);
	else {
		pr_err("%s:%d match is null\n", __func__, __LINE__);
		rc = -EINVAL;
	}
	pr_info("%s:%d rc=%d X", __func__, __LINE__, rc);
	return rc;
}

static int hi846_front_sm6350_eeprom_platform_remove(struct platform_device *pdev)
{
	const struct of_device_id *match;
	int32_t rc = 0;

	pr_info("%s:%d E", __func__, __LINE__);
	match = of_match_device(hi846_front_sm6350_eeprom_dt_match, &pdev->dev);
	if (match)
		rc = cam_eeprom_platform_driver_remove(pdev);
	else {
		pr_err("%s:%d match is null\n", __func__, __LINE__);
		rc = -EINVAL;
	}
	pr_info("%s:%d X", __func__, __LINE__);
	return rc;
}

static struct platform_driver hi846_front_sm6350_eeprom_platform_driver = {
	.driver = {
		.name = "zte,hi846_front_sm6350_eeprom",
		.owner = THIS_MODULE,
		.of_match_table = hi846_front_sm6350_eeprom_dt_match,
	},
	.probe = hi846_front_sm6350_eeprom_platform_probe,
	.remove = hi846_front_sm6350_eeprom_platform_remove,
};

static int __init hi846_front_sm6350_eeprom_init_module(void)
{
	int rc = 0;

	rc = platform_driver_register(&hi846_front_sm6350_eeprom_platform_driver);
	pr_info("%s:%d platform rc %d\n", __func__, __LINE__, rc);
	return rc;
}

static void __exit hi846_front_sm6350_eeprom_exit_module(void)
{
	platform_driver_unregister(&hi846_front_sm6350_eeprom_platform_driver);

}

module_init(hi846_front_sm6350_eeprom_init_module);
module_exit(hi846_front_sm6350_eeprom_exit_module);
MODULE_DESCRIPTION("ZTE EEPROM driver");
MODULE_LICENSE("GPL v2");
