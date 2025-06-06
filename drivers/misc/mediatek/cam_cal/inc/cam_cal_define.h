/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef _CAM_CAL_DATA_H
#define _CAM_CAL_DATA_H

#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

//+bug577281 chenbocheng.wt, add, 2020/9/9, three camera otp porting
struct stCAM_CAL_DATAINFO_STRUCT{
	u32 sensorID; // Sensor ID
	u32 deviceID; // MAIN = 0x01, SUB  = 0x02, MAIN_2 = 0x04 SUB_2 = 0x08
	u32 dataLength; //Data len
	u32 sensorVendorid; // Module ID | Pos ID | Len ID | Vcm ID
	u8  vendorByte[4]; // Module ID offset, Pos ID offset, Len ID offset,  Vcm ID offset
	u8  *dataBuffer; //It's need malloc dataLength cache
};

typedef enum{
	MODULE_ITEM = 0,
	AWB_5100K_ITEM,
	AWB_3000K_ITEM,
	SEGMENT_ITEM,
	AF_ITEM,
	LSC_ITEM,
	PDAF_ITEM,
	DUALCAM_ITEM,
	TOTAL_ITEM,
	MAX_ITEM,
}stCAM_CAL_CHECKSUM_ITEM;

struct stCAM_CAL_CHECKSUM_STRUCT{
	stCAM_CAL_CHECKSUM_ITEM item;
	u32 flagAdrees;
	u32 startAdress;
	u32 endAdress;
	u32 checksumAdress;
	u8  validFlag;
};
//-bug577281 chenbocheng.wt, add, 2020/9/9, camera otp porting

struct stCAM_CAL_INFO_STRUCT {
	u32 u4Offset;
	u32 u4Length;
	u32 sensorID;
	/*
	 * MAIN = 0x01,
	 * SUB  = 0x02,
	 * MAIN_2 = 0x04,
	 * SUB_2 = 0x08,
	 * MAIN_3 = 0x10,
	 */
	u32 deviceID;
	u8 *pu1Params;
};

#ifdef CONFIG_COMPAT

struct COMPAT_stCAM_CAL_INFO_STRUCT {
	u32 u4Offset;
	u32 u4Length;
	u32 sensorID;
	u32 deviceID;
	compat_uptr_t pu1Params;
};
#endif

#endif/*_CAM_CAL_DATA_H*/
