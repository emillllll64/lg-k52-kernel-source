/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

/* --------------------------------------------------------------------------- */
/* Header */
/* --------------------------------------------------------------------------- */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/spinlock.h>

#include "upmu_common.h"

#include <linux/lcd_power_mode.h>
#include <linux/lge_panel_notify.h>
#include <linux/input/lge_touch_notify.h>
#include <soc/mediatek/lge/board_lge.h>

#include "lcm_drv.h"
#include "ddp_hal.h"
#include "ddp_path.h"
#include "disp_recovery.h"
#include "disp_dts_gpio.h"
#include "primary_display.h"
#ifdef CONFIG_MT6370_PMU_DSV
#include "mt6370_pmu_dsv.h"
#endif

#include "sm5109.h"

/* --------------------------------------------------------------------------- */
/* Local Constants */
/* --------------------------------------------------------------------------- */

/* pixel */
#define FRAME_WIDTH       (720)
#define FRAME_HEIGHT      (1600)

/* physical dimension */
#define PHYSICAL_WIDTH    (66)
#define PHYSICAL_HEIGHT   (152)

#define ENABLE            (1)
#define DISABLE           (0)

#define DELAY             (0)

#define USE_DEEP_SLEEP    (0)

#define UDELAY(n)         (lcm_util.udelay(n))
#define MDELAY(n)         (lcm_util.mdelay(n))
#define SET_RESET_PIN(v)  (lcm_util.set_reset_pin((v)))

/* --------------------------------------------------------------------------- */
/* Local Variables */
/* --------------------------------------------------------------------------- */

struct LCM_setting_table {
  unsigned char cmd;
  unsigned char count;
  unsigned char para_list[64];
};

typedef enum {
  LCM_SUSPEND = 0,
  LCM_RESUME,
  LCM_DEEP_SLEEP,
  LCM_SHUTDOWN
} LCM_CURRENT_STAUTS;

static unsigned int lcm_status = LCM_RESUME;
static struct LCM_UTIL_FUNCS lcm_util = { 0 };

static char* cur_lcm_status[4] = {
  "suspend","resume","deep sleep","shutdown"
};
static int dsv_ctrl[2][2] = {
  {DSV_VNEG_EN,DSV_VPOS_EN},
  {DSV_VPOS_EN,DSV_VNEG_EN},
};
//static DEFINE_SPINLOCK(lcm_status_lock);

/* --------------------------------------------------------------------------- */
/* Local Functions */
/* --------------------------------------------------------------------------- */

#define dsi_set_cmdq_V3(para_tbl, size, force_update)\
lcm_util.dsi_set_cmdq_V3(para_tbl, size, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)\
lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)\
lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)\
lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)\
lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)\
lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)\
lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

#define LCM_PRINT (printk)

/* --------------------------------------------------------------------------- */
/* External Functions */
/* --------------------------------------------------------------------------- */

extern int ddp_path_top_clock_on(void);
extern int ddp_path_top_clock_off(void);
extern int ddp_dsi_power_on(enum DISP_MODULE_ENUM module, void *cmdq_handle);
extern int ddp_dsi_power_off(enum DISP_MODULE_ENUM module, void *cmdq_handle);

/* --------------------------------------------------------------------------- */
/* GPIO Set */
/* --------------------------------------------------------------------------- */

//dir out : mode 00 dir in : mode 01

#if 0
#define GPIO_IO_LDO_EN             (GPIO177 | 0x80000000)
#define GPIO_IO_LDO_EN_MODE        (GPIO_MODE_00)

#define GPIO_LCD_RESET             (GPIO45 | 0x80000000)
#define GPIO_LCD_RESET_MODE        (GPIO_MODE_00)

#define GPIO_TOUCH_RESET           (GPIO164 | 0x80000000)
#define GPIO_TOUCH_RESET_MODE      (GPIO_MODE_00)

#define GPIO_DSI_TE_PIN            (GPIO44 | 0x80000000)
#define GPIO_DSI_TE_PIN_MODE       (GPIO_MODE_01)
#endif

/* --------------------------------------------------------------------------- */
/* Initial Code */
/* --------------------------------------------------------------------------- */

static struct LCM_setting_table_V3 lcm_initial_command_1[] = {
  {0x15,  0xFF,   1,  {0x24}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x60,   1,  {0x60}},
  {0x15,  0x61,   1,  {0x40}},
  {0x15,  0x91,   1,  {0x40}},
  {0x15,  0x92,   1,  {0xCF}},
  {0x15,  0x93,   1,  {0x12}},
  {0x15,  0x94,   1,  {0x1F}},
  {0x15,  0xC2,   1,  {0x86}},
  {0x15,  0xFF,   1,  {0x25}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x0A,   1,  {0x82}},
  {0x15,  0x0B,   1,  {0x75}},
  {0x15,  0xC6,   1,  {0x00}},
  {0x15,  0xFF,   1,  {0x26}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x00,   1,  {0xA0}},
  {0x15,  0x04,   1,  {0x24}},
  {0x15,  0x06,   1,  {0x80}},
  {0x15,  0x0A,   1,  {0x02}},
  {0x15,  0x0C,   1,  {0x09}},
  {0x15,  0x0D,   1,  {0x00}},
  {0x15,  0x0E,   1,  {0x01}},
  {0x15,  0x0F,   1,  {0x05}},
  {0x15,  0x10,   1,  {0x06}},
  {0x15,  0x12,   1,  {0x50}},
  {0x15,  0x13,   1,  {0x9C}},
  {0x15,  0x19,   1,  {0x00}},
  {0x15,  0x1A,   1,  {0x01}},
  {0x15,  0x1B,   1,  {0x11}},
  {0x15,  0x1C,   1,  {0xF2}},
  {0x15,  0x1D,   1,  {0x00}},
  {0x15,  0x1E,   1,  {0xCF}},
  {0x15,  0x1F,   1,  {0xCF}},
  {0x15,  0x22,   1,  {0x00}},
  {0x15,  0x23,   1,  {0x00}},
  {0x15,  0x24,   1,  {0x01}},
  {0x15,  0x25,   1,  {0x32}},
  {0x15,  0x2A,   1,  {0x12}},
  {0x15,  0x2B,   1,  {0x7C}},
  {0x15,  0x2F,   1,  {0x00}},
  {0x15,  0x30,   1,  {0xCF}},
  {0x15,  0x31,   1,  {0x11}},
  {0x15,  0x32,   1,  {0x11}},
  {0x15,  0x33,   1,  {0x00}},
  {0x15,  0x34,   1,  {0x04}},
  {0x15,  0x35,   1,  {0xCF}},
  {0x15,  0x36,   1,  {0x89}},
  {0x15,  0x37,   1,  {0x67}},
  {0x15,  0x38,   1,  {0x11}},
  {0x15,  0x39,   1,  {0x01}},
  {0x15,  0x3A,   1,  {0x00}},
  {0x15,  0x3B,   1,  {0x00}},
  {0x15,  0x3F,   1,  {0x09}},
  {0x15,  0x40,   1,  {0xB3}},
  {0x15,  0x58,   1,  {0xE7}},
  {0x15,  0x59,   1,  {0xE7}},
  {0x15,  0x5A,   1,  {0xE7}},
  {0x15,  0x5B,   1,  {0xDF}},
  {0x15,  0x5C,   1,  {0x00}},
  {0x15,  0x5D,   1,  {0x00}},
  {0x15,  0x5E,   1,  {0x09}},
  {0x15,  0x5F,   1,  {0x80}},
  {0x15,  0x8C,   1,  {0x1B}},
  {0x15,  0x92,   1,  {0x50}},
  {0x15,  0x93,   1,  {0x36}},
  {0x15,  0x99,   1,  {0x00}},
  {0x15,  0x9A,   1,  {0xDC}},
  {0x15,  0x9B,   1,  {0x11}},
  {0x15,  0x9C,   1,  {0xF2}},
  {0x15,  0x9D,   1,  {0x12}},
  {0x15,  0x9E,   1,  {0x7C}},
  {0x15,  0x9F,   1,  {0x00}},
  {0x15,  0xA0,   1,  {0x00}},
  {0x15,  0xA9,   1,  {0xB0}},
  {0x15,  0xAA,   1,  {0xA0}},
  {0x15,  0xAB,   1,  {0xB4}},
  {0x15,  0xAC,   1,  {0xA4}},
  {0x15,  0xAD,   1,  {0xAC}},
  {0x15,  0xAE,   1,  {0xB8}},
  {0x15,  0xAF,   1,  {0xA8}},
  {0x15,  0xC8,   1,  {0x01}},
  {0x15,  0xC9,   1,  {0x28}},
  {0x15,  0xCA,   1,  {0x4E}},
  {0x15,  0xCB,   1,  {0x00}},
  {0x15,  0xFF,   1,  {0x27}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x58,   1,  {0x80}},
  {0x15,  0x59,   1,  {0x11}},
  {0x15,  0x5A,   1,  {0x00}},
  {0x15,  0x5B,   1,  {0xC9}},
  {0x15,  0x5C,   1,  {0x00}},
  {0x15,  0x5D,   1,  {0x00}},
  {0x15,  0x5E,   1,  {0xB0}},
  {0x15,  0x5F,   1,  {0x10}},
  {0x15,  0x60,   1,  {0x00}},
  {0x15,  0x61,   1,  {0x00}},
  {0x15,  0x62,   1,  {0x00}},
  {0x15,  0x63,   1,  {0x00}},
  {0x15,  0x64,   1,  {0x00}},
  {0x15,  0x65,   1,  {0x00}},
  {0x15,  0x66,   1,  {0x00}},
  {0x15,  0x67,   1,  {0x00}},
  {0x15,  0x68,   1,  {0x00}},
  {0x15,  0x78,   1,  {0x80}},
  {0x15,  0x79,   1,  {0x00}},
  {0x15,  0x7A,   1,  {0x00}},
  {0x15,  0x7B,   1,  {0x00}},
  {0x15,  0x7C,   1,  {0x00}},
  {0x15,  0x7D,   1,  {0x00}},
  {0x15,  0x7E,   1,  {0x00}},
  {0x15,  0x7F,   1,  {0x00}},
  {0x15,  0x80,   1,  {0x00}},
  {0x15,  0x81,   1,  {0x00}},
  {0x15,  0x82,   1,  {0x00}},
  {0x15,  0x83,   1,  {0x00}},
  {0x15,  0x84,   1,  {0x00}},
  {0x15,  0x85,   1,  {0x00}},
  {0x15,  0x86,   1,  {0x00}},
  {0x15,  0x87,   1,  {0x00}},
  {0x15,  0x88,   1,  {0x00}},
  {0x15,  0xC0,   1,  {0x18}},
  {0x15,  0xC1,   1,  {0xB0}},
  {0x15,  0xFF,   1,  {0x2A}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x04,   1,  {0x0E}},
  {0x15,  0x05,   1,  {0x22}},
  {0x15,  0x06,   1,  {0x64}},
  {0x15,  0x07,   1,  {0x64}},
  {0x15,  0x08,   1,  {0x40}},
  {0x15,  0x09,   1,  {0x2C}},
  {0x15,  0x0A,   1,  {0x03}},
  {0x15,  0x0B,   1,  {0x66}},
  {0x15,  0x0C,   1,  {0x66}},
  {0x15,  0x0E,   1,  {0x03}},
  {0x15,  0x0F,   1,  {0x22}},
  {0x15,  0x10,   1,  {0x64}},
  {0x15,  0x11,   1,  {0x64}},
  {0x15,  0x12,   1,  {0x66}},
  {0x15,  0x13,   1,  {0x66}},
  {0x15,  0x14,   1,  {0x03}},
  {0x15,  0x15,   1,  {0x22}},
  {0x15,  0x16,   1,  {0x64}},
  {0x15,  0x17,   1,  {0x64}},
  {0x15,  0x18,   1,  {0x66}},
  {0x15,  0x19,   1,  {0x66}},
  {0x15,  0x1A,   1,  {0x03}},
  {0x15,  0x1B,   1,  {0x22}},
  {0x15,  0x1C,   1,  {0x15}},
  {0x15,  0x1D,   1,  {0x16}},
  {0x15,  0x1E,   1,  {0x17}},
  {0x15,  0x1F,   1,  {0x18}},
  {0x15,  0x20,   1,  {0x03}},
  {0x15,  0x21,   1,  {0x22}},
  {0x15,  0x22,   1,  {0x64}},
  {0x15,  0x23,   1,  {0x64}},
  {0x15,  0x24,   1,  {0x66}},
  {0x15,  0x25,   1,  {0x66}},
  {0x15,  0x26,   1,  {0x03}},
  {0x15,  0x27,   1,  {0x22}},
  {0x15,  0x28,   1,  {0x64}},
  {0x15,  0x29,   1,  {0x64}},
  {0x15,  0x2A,   1,  {0x66}},
  {0x15,  0x2B,   1,  {0x66}},
  {0x15,  0x2F,   1,  {0x03}},
  {0x15,  0x30,   1,  {0x22}},
  {0x15,  0x31,   1,  {0x64}},
  {0x15,  0x32,   1,  {0x64}},
  {0x15,  0x33,   1,  {0x66}},
  {0x15,  0x34,   1,  {0x66}},
  {0x15,  0x35,   1,  {0x03}},
  {0x15,  0x36,   1,  {0x22}},
  {0x15,  0x37,   1,  {0x64}},
  {0x15,  0x38,   1,  {0x64}},
  {0x15,  0x39,   1,  {0x66}},
  {0x15,  0x3A,   1,  {0x66}},
  {0x15,  0x3F,   1,  {0x03}},
  {0x15,  0x40,   1,  {0x58}},
  {0x15,  0x41,   1,  {0x58}},
  {0x15,  0x47,   1,  {0xC0}},
  {0x15,  0x48,   1,  {0x64}},
  {0x15,  0x49,   1,  {0x64}},
  {0x15,  0x4B,   1,  {0x3C}},
  {0x15,  0x4C,   1,  {0x04}},
  {0x15,  0x4D,   1,  {0x70}},
  {0x15,  0x4E,   1,  {0x74}},
  {0x15,  0x50,   1,  {0x04}},
  {0x15,  0x51,   1,  {0xC0}},
  {0x15,  0x52,   1,  {0x64}},
  {0x15,  0x53,   1,  {0x64}},
  {0x15,  0x54,   1,  {0x70}},
  {0x15,  0x55,   1,  {0x74}},
  {0x15,  0x58,   1,  {0x04}},
  {0x15,  0x59,   1,  {0xC0}},
  {0x15,  0x5A,   1,  {0x64}},
  {0x15,  0x5B,   1,  {0x64}},
  {0x15,  0x5C,   1,  {0x70}},
  {0x15,  0x5D,   1,  {0x74}},
  {0x15,  0x5E,   1,  {0x06}},
  {0x15,  0x5F,   1,  {0x6A}},
  {0x15,  0x60,   1,  {0x70}},
  {0x15,  0xFF,   1,  {0x22}},
};

static struct LCM_setting_table_V3 lcm_initial_command_2[] = {
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0xA2,   1,  {0x80}},
  {0x15,  0xFF,   1,  {0x20}},
};

static struct LCM_setting_table_V3 lcm_initial_command_3[] = {
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x6D,   1,  {0xAA}},
  {0x15,  0x1F,   1,  {0x03}},
  {0x15,  0x6A,   1,  {0x22}},
  {0x15,  0x6B,   1,  {0x80}},
  {0x15,  0xFF,   1,  {0x24}},
};

static struct LCM_setting_table_V3 lcm_initial_command_4[] = {
  {0x15,  0xFB,   1,  {0x01}},
  {0x39,  0x5C,   2,  {0x90, 0x10}},
  {0x39,  0x5D,   2,  {0x06, 0x00}},
  {0x15,  0xFF,   1,  {0x2A}},
};

static struct LCM_setting_table_V3 lcm_initial_command_5[] = {
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0xC6,   1,  {0x05}},
  {0x39,  0xB0,  16,  {0x0A, 0x08, 0x08, 0x08, 0x07, 0x07, 0x05, 0x05, 0x04, 0x03, 0x03, 0x02, 0x01, 0x01, 0x01, 0x00}},
  {0x39,  0xB1,  16,  {0x0A, 0x08, 0x08, 0x08, 0x07, 0x07, 0x05, 0x05, 0x04, 0x03, 0x03, 0x02, 0x01, 0x01, 0x01, 0x00}},
  {0x39,  0xB2,  16,  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {0x39,  0xB3,  16,  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {0x39,  0xB4,  16,  {0x8E, 0xA0, 0xA6, 0xAE, 0xB6, 0xBF, 0xE6, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
  {0x39,  0xB5,  16,  {0x8E, 0xA0, 0xA6, 0xAE, 0xB6, 0xBF, 0xE6, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
  {0x39,  0xB6,   8,  {0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44}},
  {0x15,  0xAF,   1,  {0x70}},
  {0x15,  0xD0,   1,  {0x32}},
  {0x15,  0xD2,   1,  {0xAA}},
  {0x15,  0xD4,   1,  {0xAA}},
  {0x15,  0xFF,   1,  {0x20}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x05,   1,  {0x91}},
  {0x15,  0x07,   1,  {0x5F}},
  {0x15,  0x08,   1,  {0x5F}},
  {0x15,  0x0E,   1,  {0x7D}},
  {0x15,  0x0F,   1,  {0x7D}},
  {0x15,  0x88,   1,  {0xFF}},
  /* vcom */
  {0x15,  0x89,   1,  {0x86}},
  {0x15,  0x94,   1,  {0xFF}},
  {0x15,  0x95,   1,  {0x09}},
  {0x15,  0x96,   1,  {0x09}},
  {0x15,  0x69,   1,  {0xA0}},
  {0x15,  0xFF,   1,  {0x24}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x00,   1,  {0xAB}},
  {0x15,  0x01,   1,  {0x04}},
  {0x15,  0x02,   1,  {0x00}},
  {0x15,  0x03,   1,  {0x06}},
  {0x15,  0x04,   1,  {0x07}},
  {0x15,  0x05,   1,  {0x2B}},
  {0x15,  0x06,   1,  {0x21}},
  {0x15,  0x07,   1,  {0x20}},
  {0x15,  0x08,   1,  {0x11}},
  {0x15,  0x09,   1,  {0x10}},
  {0x15,  0x0A,   1,  {0x1D}},
  {0x15,  0x0B,   1,  {0x1C}},
  {0x15,  0x0C,   1,  {0x0D}},
  {0x15,  0x0D,   1,  {0x0C}},
  {0x15,  0x0E,   1,  {0x00}},
  {0x15,  0x0F,   1,  {0x00}},
  {0x15,  0x10,   1,  {0x00}},
  {0x15,  0x11,   1,  {0x00}},
  {0x15,  0x12,   1,  {0x00}},
  {0x15,  0x13,   1,  {0x00}},
  {0x15,  0x14,   1,  {0x00}},
  {0x15,  0x15,   1,  {0x00}},
  {0x15,  0x16,   1,  {0xAB}},
  {0x15,  0x17,   1,  {0x05}},
  {0x15,  0x18,   1,  {0x00}},
  {0x15,  0x19,   1,  {0x06}},
  {0x15,  0x1A,   1,  {0x07}},
  {0x15,  0x1B,   1,  {0x2B}},
  {0x15,  0x1C,   1,  {0x23}},
  {0x15,  0x1D,   1,  {0x22}},
  {0x15,  0x1E,   1,  {0x13}},
  {0x15,  0x1F,   1,  {0x12}},
  {0x15,  0x20,   1,  {0x1F}},
  {0x15,  0x21,   1,  {0x1E}},
  {0x15,  0x22,   1,  {0x0F}},
  {0x15,  0x23,   1,  {0x0E}},
  {0x15,  0x24,   1,  {0x00}},
  {0x15,  0x25,   1,  {0x00}},
  {0x15,  0x26,   1,  {0x00}},
  {0x15,  0x27,   1,  {0x00}},
  {0x15,  0x28,   1,  {0x00}},
  {0x15,  0x29,   1,  {0x00}},
  {0x15,  0x2A,   1,  {0x00}},
  {0x15,  0x2B,   1,  {0x00}},
  {0x15,  0x2D,   1,  {0x03}},
  {0x15,  0x2F,   1,  {0x1F}},
  {0x15,  0x30,   1,  {0x20}},
  {0x15,  0x31,   1,  {0x15}},
  {0x15,  0x32,   1,  {0x11}},
  {0x15,  0x33,   1,  {0x1F}},
  {0x15,  0x34,   1,  {0x20}},
  {0x15,  0x35,   1,  {0x15}},
  {0x15,  0x36,   1,  {0x11}},
  {0x15,  0x37,   1,  {0x00}},
  {0x15,  0x38,   1,  {0x00}},
  {0x15,  0x39,   1,  {0x00}},
  {0x15,  0x3A,   1,  {0x01}},
  {0x15,  0x3B,   1,  {0x4E}},
  {0x15,  0x3D,   1,  {0x14}},
  {0x15,  0xAB,   1,  {0x0A}},
  {0x15,  0xAC,   1,  {0x0A}},
  {0x15,  0xAD,   1,  {0x0A}},
  {0x15,  0xAE,   1,  {0x0A}},
  {0x15,  0xAF,   1,  {0x00}},
  {0x15,  0x3F,   1,  {0x88}},
  {0x15,  0x40,   1,  {0x88}},
  {0x15,  0x43,   1,  {0x88}},
  {0x15,  0x44,   1,  {0x88}},
  {0x15,  0x47,   1,  {0x00}},
  {0x15,  0x48,   1,  {0x00}},
  {0x15,  0x49,   1,  {0x00}},
  {0x15,  0x4A,   1,  {0x01}},
  {0x15,  0x4B,   1,  {0x4E}},
  {0x15,  0x4C,   1,  {0x02}},
  {0x15,  0xB0,   1,  {0x0A}},
  {0x15,  0xB1,   1,  {0x0A}},
  {0x15,  0xB2,   1,  {0x0A}},
  {0x15,  0xB3,   1,  {0x0A}},
  {0x15,  0xB4,   1,  {0x00}},
  {0x15,  0x4D,   1,  {0xCC}},
  {0x15,  0x4E,   1,  {0x00}},
  {0x15,  0x4F,   1,  {0x33}},
  {0x15,  0x50,   1,  {0x00}},
  {0x15,  0x51,   1,  {0x33}},
  {0x15,  0x52,   1,  {0x00}},
  {0x15,  0x53,   1,  {0xCC}},
  {0x15,  0x54,   1,  {0x00}},
  {0x15,  0x55,   1,  {0x0F}},
  {0x15,  0x56,   1,  {0x68}},
  {0x15,  0x59,   1,  {0x40}},
  {0x15,  0x5A,   1,  {0x59}},
  {0x15,  0x5B,   1,  {0x52}},
  {0x39,  0x5E,   3,  {0x00, 0x08, 0x08}},
  {0x15,  0xD3,   1,  {0x65}},
  {0x15,  0xD4,   1,  {0x87}},
  {0x15,  0xD5,   1,  {0x21}},
  {0x15,  0xD6,   1,  {0x43}},
  {0x15,  0xD7,   1,  {0x11}},
  {0x15,  0xD8,   1,  {0x11}},
  {0x15,  0xD9,   1,  {0x11}},
  {0x15,  0xDA,   1,  {0x11}},
  {0x15,  0xDB,   1,  {0x00}},
  {0x15,  0xDC,   1,  {0x00}},
  {0x15,  0xDD,   1,  {0x34}},
  {0x15,  0xDE,   1,  {0x12}},
  {0x15,  0xDF,   1,  {0x78}},
  {0x15,  0xE0,   1,  {0x56}},
  {0x15,  0xE1,   1,  {0x11}},
  {0x15,  0xE2,   1,  {0x11}},
  {0x15,  0xE3,   1,  {0x11}},
  {0x15,  0xE4,   1,  {0x11}},
  {0x15,  0xE5,   1,  {0x00}},
  {0x15,  0xE6,   1,  {0x00}},
  {0x15,  0xE7,   1,  {0x11}},
  {0x15,  0xE8,   1,  {0x11}},
  {0x15,  0xE9,   1,  {0x11}},
  {0x15,  0xEA,   1,  {0x11}},
  {0x15,  0xEB,   1,  {0x00}},
  {0x15,  0xEE,   1,  {0x00}},
  {0x15,  0xEF,   1,  {0x00}},
  {0x15,  0xF0,   1,  {0x00}},
  {0x15,  0xF2,   1,  {0x22}},
  {0x15,  0xF3,   1,  {0x22}},
  {0x15,  0xF4,   1,  {0x22}},
  {0x15,  0xF5,   1,  {0x22}},
  {0x15,  0xF6,   1,  {0x00}},
  {0x15,  0xF7,   1,  {0x00}},
  {0x15,  0xF8,   1,  {0x00}},
  {0x15,  0xF9,   1,  {0x00}},
  {0x15,  0xFF,   1,  {0x24}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0xC3,   1,  {0x44}},
  {0x15,  0xC4,   1,  {0x44}},
  {0x15,  0xC5,   1,  {0x44}},
  {0x15,  0xC6,   1,  {0x44}},
  {0x15,  0xC7,   1,  {0x65}},
  {0x15,  0xC8,   1,  {0x87}},
  {0x15,  0xC9,   1,  {0x21}},
  {0x15,  0xCA,   1,  {0x43}},
  {0x15,  0xCB,   1,  {0x65}},
  {0x15,  0xCC,   1,  {0x87}},
  {0x15,  0xCD,   1,  {0x21}},
  {0x15,  0xCE,   1,  {0x43}},
  {0x15,  0x80,   1,  {0x18}},
  {0x15,  0x81,   1,  {0x08}},
  {0x15,  0x82,   1,  {0x18}},
  {0x15,  0x83,   1,  {0x08}},
  {0x15,  0x84,   1,  {0x00}},
  {0x15,  0x97,   1,  {0x28}},
  {0x15,  0xFF,   1,  {0x25}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x67,   1,  {0x08}},
  {0x15,  0x68,   1,  {0x00}},
  {0x15,  0x1F,   1,  {0x59}},
  {0x15,  0x20,   1,  {0x52}},
  {0x15,  0x21,   1,  {0x59}},
  {0x15,  0x22,   1,  {0x52}},
  {0x15,  0x26,   1,  {0x59}},
  {0x15,  0x27,   1,  {0x52}},
  {0x15,  0x28,   1,  {0x59}},
  {0x15,  0x29,   1,  {0x52}},
  {0x15,  0x33,   1,  {0x59}},
  {0x15,  0x34,   1,  {0x52}},
  {0x15,  0x35,   1,  {0x59}},
  {0x15,  0x36,   1,  {0x52}},
  {0x15,  0x3F,   1,  {0x08}},
  {0x15,  0x41,   1,  {0x80}},
  {0x15,  0x42,   1,  {0x59}},
  {0x15,  0x43,   1,  {0x52}},
  {0x15,  0x44,   1,  {0x59}},
  {0x15,  0x45,   1,  {0x52}},
  {0x15,  0x4F,   1,  {0x59}},
  {0x15,  0x50,   1,  {0x52}},
  {0x15,  0x51,   1,  {0x59}},
  {0x15,  0x52,   1,  {0x52}},
  {0x15,  0x5D,   1,  {0x01}},
  {0x15,  0x5E,   1,  {0x4E}},
  {0x15,  0x5F,   1,  {0x01}},
  {0x15,  0x60,   1,  {0x4E}},
  {0x15,  0x61,   1,  {0x59}},
  {0x15,  0x62,   1,  {0x52}},
  {0x15,  0xFF,   1,  {0x2A}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x8E,   1,  {0x00}},
  {0x15,  0x8F,   1,  {0x00}},
  {0x15,  0xC8,   1,  {0x00}},
  {0x15,  0xC9,   1,  {0x00}},
  {0x15,  0xCA,   1,  {0x00}},
  {0x15,  0xCB,   1,  {0x00}},
  {0x15,  0xCC,   1,  {0x00}},
  {0x15,  0xCD,   1,  {0x00}},
  {0x15,  0xCE,   1,  {0x00}},
  {0x15,  0xCF,   1,  {0x00}},
  {0x15,  0xDA,   1,  {0x1B}},
  {0x15,  0xDB,   1,  {0x1B}},
  {0x15,  0xDC,   1,  {0x2D}},
  {0x15,  0xDD,   1,  {0x2D}},
  {0x15,  0xDE,   1,  {0x00}},
  {0x15,  0xDF,   1,  {0x00}},
  {0x15,  0xE0,   1,  {0x00}},
  {0x15,  0xE1,   1,  {0x00}},
  {0x15,  0xE2,   1,  {0x00}},
  {0x15,  0xE3,   1,  {0x00}},
  {0x15,  0xE4,   1,  {0x00}},
  {0x15,  0xE5,   1,  {0x00}},
  {0x15,  0xE6,   1,  {0x00}},
  {0x15,  0xE7,   1,  {0x00}},
  {0x15,  0xE8,   1,  {0x00}},
  {0x15,  0xE9,   1,  {0x00}},
  {0x15,  0xEA,   1,  {0x12}},
  {0x15,  0xEB,   1,  {0x12}},
  {0x15,  0xD6,   1,  {0x00}},
  {0x15,  0xD7,   1,  {0x20}},
  {0x15,  0xD8,   1,  {0x00}},
  {0x15,  0xD9,   1,  {0x12}},
  {0x15,  0xC0,   1,  {0x00}},
  {0x15,  0xC1,   1,  {0x12}},
  {0x15,  0xC7,   1,  {0x00}},
  {0x15,  0xFF,   1,  {0x2A}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0xC2,   1,  {0x55}},
  {0x15,  0xC3,   1,  {0x10}},
  {0x15,  0xC4,   1,  {0x22}},
  {0x15,  0xC5,   1,  {0x0F}},
  {0x15,  0xFF,   1,  {0x24}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x9B,   1,  {0x00}},
  {0x15,  0x9C,   1,  {0x00}},
  {0x15,  0xFF,   1,  {0x2A}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0xB9,   1,  {0x01}},
  {0x15,  0x80,   1,  {0x00}},
  {0x15,  0x81,   1,  {0x03}},
  {0x15,  0x82,   1,  {0x55}},
  {0x15,  0x83,   1,  {0x77}},
  {0x15,  0x84,   1,  {0x00}},
  {0x15,  0x85,   1,  {0x00}},
  {0x15,  0x86,   1,  {0x10}},
  {0x15,  0x87,   1,  {0x32}},
  {0x15,  0x88,   1,  {0x00}},
  {0x15,  0x89,   1,  {0x00}},
  {0x15,  0x8A,   1,  {0x10}},
  {0x15,  0x8B,   1,  {0x32}},
  {0x15,  0x8C,   1,  {0x00}},
  {0x15,  0x8D,   1,  {0x00}},
  {0x15,  0xFF,   1,  {0x26}},
  {0x15,  0xFB,   1,  {0x01}},
  {0x15,  0x4B,   1,  {0x00}},
  {0x15,  0x4C,   1,  {0x00}},
  {0x15,  0x4D,   1,  {0x00}},
  {0x15,  0x4E,   1,  {0x00}},
  {0x15,  0x4F,   1,  {0x00}},
  {0x15,  0x50,   1,  {0x00}},
  {0x15,  0x51,   1,  {0x00}},
  {0x15,  0x52,   1,  {0x00}},
  {0x15,  0x53,   1,  {0x00}},
  {0x15,  0x54,   1,  {0x00}},
  {0x15,  0x55,   1,  {0x00}},
  {0x15,  0x56,   1,  {0x00}},
  /* 2.2 gamma start - PG */
  /* CMD2_Page0 */
  {0x15,  0xFF,   1,  {0x20}},
  {0x15,  0xFB,   1,  {0x01}},
  /* R(+) */
  {0x39,  0xB0,  16,  {0x00 , 0x08 , 0x00 , 0x14 , 0x00 , 0x28 , 0x00 , 0x3B , 0x00 , 0x4C , 0x00 , 0x5B , 0x00 , 0x6A , 0x00 , 0x77}},
  {0x39,  0xB1,  16,  {0x00 , 0x84 , 0x00 , 0xB3 , 0x00 , 0xD8 , 0x01 , 0x17 , 0x01 , 0x47 , 0x01 , 0x97 , 0x01 , 0xD5 , 0x01 , 0xD7}},
  {0x39,  0xB2,  16,  {0x02 , 0x19 , 0x02 , 0x61 , 0x02 , 0x94 , 0x02 , 0xD1 , 0x02 , 0xFD , 0x03 , 0x2F , 0x03 , 0x41 , 0x03 , 0x52}},
  {0x39,  0xB3,  12,  {0x03 , 0x66 , 0x03 , 0x7D , 0x03 , 0x99 , 0x03 , 0xB8 , 0x03 , 0xCF , 0x03 , 0xD2}},
  /* G(+) */
  {0x39,  0xB4,  16,  {0x00 , 0x08 , 0x00 , 0x14 , 0x00 , 0x28 , 0x00 , 0x3B , 0x00 , 0x4C , 0x00 , 0x5B , 0x00 , 0x6A , 0x00 , 0x77}},
  {0x39,  0xB5,  16,  {0x00 , 0x84 , 0x00 , 0xB3 , 0x00 , 0xD8 , 0x01 , 0x17 , 0x01 , 0x47 , 0x01 , 0x97 , 0x01 , 0xD5 , 0x01 , 0xD7}},
  {0x39,  0xB6,  16,  {0x02 , 0x19 , 0x02 , 0x61 , 0x02 , 0x94 , 0x02 , 0xD1 , 0x02 , 0xFD , 0x03 , 0x2F , 0x03 , 0x41 , 0x03 , 0x52}},
  {0x39,  0xB7,  12,  {0x03 , 0x66 , 0x03 , 0x7D , 0x03 , 0x99 , 0x03 , 0xB8 , 0x03 , 0xCF , 0x03 , 0xD2}},
  /* B(+) */
  {0x39,  0xB8,  16,  {0x00 , 0x08 , 0x00 , 0x14 , 0x00 , 0x28 , 0x00 , 0x3B , 0x00 , 0x4C , 0x00 , 0x5B , 0x00 , 0x6A , 0x00 , 0x77}},
  {0x39,  0xB9,  16,  {0x00 , 0x84 , 0x00 , 0xB3 , 0x00 , 0xD8 , 0x01 , 0x17 , 0x01 , 0x47 , 0x01 , 0x97 , 0x01 , 0xD5 , 0x01 , 0xD7}},
  {0x39,  0xBA,  16,  {0x02 , 0x19 , 0x02 , 0x61 , 0x02 , 0x94 , 0x02 , 0xD1 , 0x02 , 0xFD , 0x03 , 0x2F , 0x03 , 0x41 , 0x03 , 0x52}},
  {0x39,  0xBB,  12,  {0x03 , 0x66 , 0x03 , 0x7D , 0x03 , 0x99 , 0x03 , 0xB8 , 0x03 , 0xCF , 0x03 , 0xD2}},
  /* CMD2_Page1 */
  {0x15,  0xFF,   1,  {0x21}},
  {0x15,  0xFB,   1,  {0x01}},
  /* R(-) */
  {0x39,  0xB0,  16,  {0x00 , 0x00 , 0x00 , 0x0C , 0x00 , 0x20 , 0x00 , 0x33 , 0x00 , 0x44 , 0x00 , 0x53 , 0x00 , 0x62 , 0x00 , 0x6F}},
  {0x39,  0xB1,  16,  {0x00 , 0x7C , 0x00 , 0xAB , 0x00 , 0xD0 , 0x01 , 0x0F , 0x01 , 0x3F , 0x01 , 0x8F , 0x01 , 0xCD , 0x01 , 0xCF}},
  {0x39,  0xB2,  16,  {0x02 , 0x11 , 0x02 , 0x59 , 0x02 , 0x8C , 0x02 , 0xC9 , 0x02 , 0xF5 , 0x03 , 0x27 , 0x03 , 0x39 , 0x03 , 0x4A}},
  {0x39,  0xB3,  12,  {0x03 , 0x5E , 0x03 , 0x75 , 0x03 , 0x91 , 0x03 , 0xB0 , 0x03 , 0xC7 , 0x03 , 0xCA}},
  /* G(-) */
  {0x39,  0xB4,  16,  {0x00 , 0x00 , 0x00 , 0x0C , 0x00 , 0x20 , 0x00 , 0x33 , 0x00 , 0x44 , 0x00 , 0x53 , 0x00 , 0x62 , 0x00 , 0x6F}},
  {0x39,  0xB5,  16,  {0x00 , 0x7C , 0x00 , 0xAB , 0x00 , 0xD0 , 0x01 , 0x0F , 0x01 , 0x3F , 0x01 , 0x8F , 0x01 , 0xCD , 0x01 , 0xCF}},
  {0x39,  0xB6,  16,  {0x02 , 0x11 , 0x02 , 0x59 , 0x02 , 0x8C , 0x02 , 0xC9 , 0x02 , 0xF5 , 0x03 , 0x27 , 0x03 , 0x39 , 0x03 , 0x4A}},
  {0x39,  0xB7,  12,  {0x03 , 0x5E , 0x03 , 0x75 , 0x03 , 0x91 , 0x03 , 0xB0 , 0x03 , 0xC7 , 0x03 , 0xCA}},
  /* B(-) */
  {0x39,  0xB8,  16,  {0x00 , 0x00 , 0x00 , 0x0C , 0x00 , 0x20 , 0x00 , 0x33 , 0x00 , 0x44 , 0x00 , 0x53 , 0x00 , 0x62 , 0x00 , 0x6F}},
  {0x39,  0xB9,  16,  {0x00 , 0x7C , 0x00 , 0xAB , 0x00 , 0xD0 , 0x01 , 0x0F , 0x01 , 0x3F , 0x01 , 0x8F , 0x01 , 0xCD , 0x01 , 0xCF}},
  {0x39,  0xBA,  16,  {0x02 , 0x11 , 0x02 , 0x59 , 0x02 , 0x8C , 0x02 , 0xC9 , 0x02 , 0xF5 , 0x03 , 0x27 , 0x03 , 0x39 , 0x03 , 0x4A}},
  {0x39,  0xBB,  12,  {0x03 , 0x5E , 0x03 , 0x75 , 0x03 , 0x91 , 0x03 , 0xB0 , 0x03 , 0xC7 , 0x03 , 0xCA}},
  /* gamma end */
  {0x15,  0xFF,   1,  {0x10}},
  {0x15,  0xFB,   1,  {0x01}},
};

/* set mipi data lane */
static struct LCM_setting_table_V3 lcm_initial_command_6[] = {
  {0x15,  0xBA,  1,  {0x03}},
};

/* Sleep Out Set */
static struct LCM_setting_table_V3 lcm_initial_sleep_out[] = {
  {0x05, 0x11,  1, {0x00}},
};

/* Display On Set */
static struct LCM_setting_table_V3 lcm_initial_disp_on[] = {
  {0x05, 0x29,  1, {0x00}},
};

/* Sleep In Set */
static struct LCM_setting_table_V3 lcm_initial_sleep_in[] = {
  {0x05, 0x10,  1, {0x00}},
};

/* Display Off Set */
static struct LCM_setting_table_V3 lcm_initial_display_off[] = {
  {0x05, 0x28,  1, {0x00}},
};

#if 0
/* TE Signal on Set */
static struct LCM_setting_table_V3 lcm_initial_te_on[] = {
  {0x15, 0x35,  1, {0x00}},
};
#endif

/* --------------------------------------------------------------------------- */
/* LCM Driver Implementations */
/* --------------------------------------------------------------------------- */

static unsigned int get_lcm_status(void)
{
  return lcm_status;
}

static void set_lcm_status(unsigned int mode)
{
  lcm_status = mode;

  LCM_PRINT("[LCD] %s : %s\n",__func__,cur_lcm_status[lcm_status]);
}

static unsigned int check_lcm_status(unsigned int mode)
{
  int ret = 0;

  if(get_lcm_status() == mode)
    ret = 1;

  LCM_PRINT("[LCD] current lcm status : %s, ret = %d\n",cur_lcm_status[get_lcm_status()],ret);

  return ret;
}

static void lcm_init_registers(void)
{
  dsi_set_cmdq_V3(lcm_initial_command_1, sizeof(lcm_initial_command_1) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(1);

  dsi_set_cmdq_V3(lcm_initial_command_2, sizeof(lcm_initial_command_2) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(1);

  dsi_set_cmdq_V3(lcm_initial_command_3, sizeof(lcm_initial_command_3) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(1);

  dsi_set_cmdq_V3(lcm_initial_command_4, sizeof(lcm_initial_command_4) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(1);

  dsi_set_cmdq_V3(lcm_initial_command_5, sizeof(lcm_initial_command_5) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(1);

  LCM_PRINT("[LCD] %s\n",__func__);
}

static void lcm_init_registers_sleep_in(void)
{
  dsi_set_cmdq_V3(lcm_initial_display_off, sizeof(lcm_initial_display_off) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(20);

  dsi_set_cmdq_V3(lcm_initial_sleep_in, sizeof(lcm_initial_sleep_in) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(60);

  LCM_PRINT("[LCD] %s\n",__func__);
}

static void lcm_init_registers_sleep_out(void)
{
  lcm_init_registers();

  dsi_set_cmdq_V3(lcm_initial_sleep_out, sizeof(lcm_initial_sleep_out) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(100);

  dsi_set_cmdq_V3(lcm_initial_disp_on,sizeof(lcm_initial_disp_on) / sizeof(struct LCM_setting_table_V3), 1);
  MDELAY(10);

  dsi_set_cmdq_V3(lcm_initial_command_6, sizeof(lcm_initial_command_6) / sizeof(struct LCM_setting_table_V3), 1);

  LCM_PRINT("[LCD] %s\n",__func__);
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
  memcpy((void *)&lcm_util, (void *)util, sizeof(struct LCM_UTIL_FUNCS));
}

#if defined(CONFIG_LGE_MULTI_FRAME_RATE)
/*DynFPS*/
static void lcm_dfps_int(struct LCM_DSI_PARAMS *dsi)
{
        struct dfps_info *dfps_params = dsi->dfps_params;

        dsi->dfps_enable = 1;
        dsi->dfps_default_fps = 6000;/*real fps * 100, to support float*/
        dsi->dfps_def_vact_tim_fps = 6000;/*real vact timing fps * 100*/

        /*traversing array must less than DFPS_LEVELS*/
        /*DPFS_LEVEL0*/
        dfps_params[0].level = DFPS_LEVEL0;
        dfps_params[0].fps = 6000;/*real fps * 100, to support float*/
        dfps_params[0].vact_timing_fps = 6000;/*real vact timing fps * 100*/
        /*if mipi clock solution*/
        /*dfps_params[0].PLL_CLOCK = xx;*/
        /*dfps_params[0].data_rate = xx; */
        /*if HFP solution*/
        /*dfps_params[0].horizontal_frontporch = xx;*/
        dfps_params[0].vertical_frontporch = 18;
        //dfps_params[1].vertical_frontporch_for_low_power = 540;

        /*if need mipi hopping params add here*/
        /*dfps_params[0].PLL_CLOCK_dyn =xx;
         *dfps_params[0].horizontal_frontporch_dyn =xx ;
         * dfps_params[0].vertical_frontporch_dyn = 1291;
         */

        /*DPFS_LEVEL1*/
        dfps_params[1].level = DFPS_LEVEL1;
        dfps_params[1].fps = 3000;/*real fps * 100, to support float*/
        dfps_params[1].vact_timing_fps = 3000;/*real vact timing fps * 100*/
        /*if mipi clock solution*/
        /*dfps_params[1].PLL_CLOCK = xx;*/
        /*dfps_params[1].data_rate = xx; */
        /*if HFP solution*/
        /*dfps_params[1].horizontal_frontporch = xx;*/
        dfps_params[1].vertical_frontporch = 18;
        //dfps_params[0].vertical_frontporch_for_low_power = 540;

        /*if need mipi hopping params add here*/
        /*dfps_params[1].PLL_CLOCK_dyn =xx;
         *dfps_params[1].horizontal_frontporch_dyn =xx ;
         * dfps_params[1].vertical_frontporch_dyn= 54;
         * dfps_params[1].vertical_frontporch_for_low_power_dyn =xx;
         */

        dsi->dfps_num = 2;
}
#endif

static void lcm_get_params(struct LCM_PARAMS *params)
{
  memset(params, 0, sizeof(struct LCM_PARAMS));

  params->type = LCM_TYPE_DSI;

  params->width = FRAME_WIDTH;
  params->height = FRAME_HEIGHT;

  /* physical size */
  params->physical_width = PHYSICAL_WIDTH;
  params->physical_height = PHYSICAL_HEIGHT;

  params->dsi.mode = SYNC_PULSE_VDO_MODE;
  /* non-continuous clock */
  params->dsi.cont_clock = 0;
  params->dsi.LANE_NUM = LCM_FOUR_LANE;

  /* The following defined the fomat for data coming from LCD engine. */
  params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
  params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
  params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
  params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

  /* Highly depends on LCD driver capability. */
  params->dsi.packet_size = 256;
  params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

  /* porch value is not necessary in command mode. */
  params->dsi.vertical_sync_active = 2;
  params->dsi.vertical_backporch = 207;
  params->dsi.vertical_frontporch = 18;
  params->dsi.vertical_active_line = FRAME_HEIGHT;

  params->dsi.horizontal_sync_active = 8;
  params->dsi.horizontal_backporch = 76;
  params->dsi.horizontal_frontporch = 168;
  params->dsi.horizontal_active_pixel = FRAME_WIDTH;

  params->dsi.HS_TRAIL = 6;
  params->dsi.HS_PRPR = 6;

  params->dsi.PLL_CLOCK = 338;
  params->dsi.ssc_disable = 1;

  params->esd_powerctrl_support = false;
  params->lcm_seq_resume = LCM_MIPI_READY_VIDEO_FRAME_RESUME;
  params->lcm_seq_suspend = LCM_MIPI_VIDEO_FRAME_DONE_SUSPEND;
  params->lcm_seq_shutdown = LCM_SHUTDOWN_AFTER_DSI_OFF;

#ifdef CONFIG_LGE_DISPLAY_ESD_RECOVERY
  params->dsi.esd_check_enable = 1;
  params->dsi.customization_esd_check_enable = 1;

  params->dsi.lcm_esd_check_table[0].cmd  = 0x0A;
  params->dsi.lcm_esd_check_table[0].count  = 1;
  params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;

  set_disp_esd_check_lcm(true);
#endif

#if defined(CONFIG_LGE_MULTI_FRAME_RATE)
  lcm_dfps_int(&(params->dsi));
#endif

  LCM_PRINT("[LCD] %s\n",__func__);
}

#if 0
static void lcm_io_ldo_ctrl(unsigned int mode)
{
  disp_set_gpio_ctrl(LCD_LDO_EN, mode);

  LCM_PRINT("[LCD] %s : %s\n",__func__,(mode)? "enable":"disable");
}
# endif

#if USE_DEEP_SLEEP
static void lcm_mipi_lane_ctrl(unsigned int mode)
{
  if(mode) {
    ddp_path_top_clock_on();
    ddp_dsi_power_on(DISP_MODULE_DSI0, NULL);
  } else {
    ddp_dsi_power_off(DISP_MODULE_DSI0, NULL);
    ddp_path_top_clock_off();
  }

  LCM_PRINT("[LCD] %s : %s\n",__func__,(mode)? "enable":"disable");
}
#endif

static void lcm_dsv_ctrl(unsigned int enable, unsigned int delay)
{
  disp_set_gpio_ctrl(dsv_ctrl[enable][0], enable);

  if(enable)
    sm5109_set_vspn();

  if(delay)
    MDELAY(delay);

  disp_set_gpio_ctrl(dsv_ctrl[enable][1], enable);

  LCM_PRINT("[LCD] %s : %s\n",__func__,(enable)? "enable":"disable");
}

static void lcd_reset_pin(unsigned int mode)
{
  disp_set_gpio_ctrl(LCM_RST, mode);

  LCM_PRINT("[LCD] %s : %s\n",__func__,(mode)? "high":"low");
}

#if USE_DEEP_SLEEP
static void touch_reset_pin(unsigned int mode)
{
  disp_set_gpio_ctrl(TOUCH_RST, mode);

  LCM_PRINT("[LCD] %s : %s\n",__func__,(mode)? "high":"low");
}
#endif

static void lcm_reset_ctrl(unsigned int mode, unsigned int delay)
{
  if(mode)
    lcd_reset_pin(mode);
  else
    touch_notifier_call_chain(LCD_EVENT_TOUCH_RESET_START, NULL);

  if(delay)
    MDELAY(delay);

  if(mode)
    touch_notifier_call_chain(LCD_EVENT_TOUCH_RESET_END, NULL);
  else
    lcd_reset_pin(mode);

  LCM_PRINT("[LCD] %s : %s\n",__func__,(mode)? "enable":"disable");
}

#if USE_DEEP_SLEEP
static void lcm_enter_deep_sleep(void)
{
  if(!check_lcm_status(LCM_SUSPEND)){
    LCM_PRINT("[LCD] %s : skip\n",__func__);
    return;
  }

  /* */

  set_lcm_status(LCM_DEEP_SLEEP);

  LCM_PRINT("[LCD] %s : end\n",__func__);
}

static void lcm_exit_deep_sleep(void)
{
  if(!check_lcm_status(LCM_DEEP_SLEEP)) {
    LCM_PRINT("[LCD] %s : skip\n",__func__);
    return;
  }

  /* lcd power control */

  lcm_mipi_lane_ctrl(ENABLE);

  /* reset pin control */

  lcm_mipi_lane_ctrl(DISABLE);

  set_lcm_status(LCM_SUSPEND);

  LCM_PRINT("[LCD] %s : end\n",__func__);
}

static void lcm_set_deep_sleep(unsigned int mode)
{
  switch(mode) {
    case DEEP_SLEEP_ENTER:
      lcm_enter_deep_sleep();
      break;
    case DEEP_SLEEP_EXIT:
      lcm_exit_deep_sleep();
      break;
    default:
      LCM_PRINT("[LCD] do nothing from touch event\n");
      break;
  }
}
#endif

static void lcm_suspend_mfts(void)
{
  LCM_PRINT("[LCD] %s : do nothing \n",__func__);
}

static void lcm_suspend(void)
{
  if(!check_lcm_status(LCM_RESUME)){
    LCM_PRINT("[LCD] %s : skip\n",__func__);
    return;
  }

  lcm_init_registers_sleep_in();

  if(primary_get_shutdown_scenario() != NORMAL_SUSPEND) {
    lcm_reset_ctrl(DISABLE,DELAY);
    MDELAY(1);
  }

  set_lcm_status(LCM_SUSPEND);

  LCM_PRINT("[LCD] %s : end\n",__func__);
}

static void lcm_resume_mfts(void)
{
  lcm_reset_ctrl(DISABLE,DELAY);

  LCM_PRINT("[LCD] %s : end\n",__func__);
}

static void lcm_init_power(void)
{
  if(check_lcm_status(LCM_SHUTDOWN)) {
    set_lcm_status(LCM_SUSPEND);
    MDELAY(1);

    lcm_dsv_ctrl(ENABLE,1);
    MDELAY(1);

    goto end;
  }

#if USE_DEEP_SLEEP
  if(check_lcm_status(LCM_DEEP_SLEEP)) {
    LCM_PRINT("[LCD] %s : exit deep sleep\n",__func__);

    lcm_set_deep_sleep(DEEP_SLEEP_EXIT);
  }
#endif

end:
  lcm_reset_ctrl(DISABLE,DELAY);
  MDELAY(5);

  LCM_PRINT("[LCD] %s : end\n",__func__);
}

static void lcm_resume(void)
{
  if(!check_lcm_status(LCM_SUSPEND)) {
    LCM_PRINT("[LCD] %s : skip\n",__func__);
    return;
  }

  lcm_reset_ctrl(ENABLE,DELAY);
  MDELAY(5);
  lcm_reset_ctrl(DISABLE,DELAY);
  MDELAY(5);
  lcm_reset_ctrl(ENABLE,DELAY);
  MDELAY(10);

  lcm_init_registers_sleep_out();

  set_lcm_status(LCM_RESUME);

  LCM_PRINT("[LCD] %s : end\n",__func__);
}

static void lcm_shutdown(void)
{
  lcm_dsv_ctrl(DISABLE,1);
  MDELAY(1);

  set_lcm_status(LCM_SHUTDOWN);

  LCM_PRINT("[LCD] %s : end\n",__func__);
}

#ifdef CONFIG_LGE_INIT_CMD_TUNING
static struct LCM_setting_table_V3* lcm_get_lcm_init_cmd_str(void)
{
  struct LCM_setting_table_V3 * p_str = lcm_initial_command;
  return p_str;
}

static int lcm_get_init_cmd_str_size(void)
{
  return (sizeof(lcm_initial_command)/sizeof(struct LCM_setting_table_V3));
}
#endif

/* --------------------------------------------------------------------------- */
/* Get LCM Driver Hooks */
/* --------------------------------------------------------------------------- */

struct LCM_DRIVER nt36526_txd_hdplus_dsi_vdo_drv = {
  .name = "TXD-NT36526",
  .set_util_funcs = lcm_set_util_funcs,
  .get_params = lcm_get_params,
  .init_power = lcm_init_power,
  .suspend = lcm_suspend,
  .resume = lcm_resume,
  .suspend_mfts = lcm_suspend_mfts,
  .resume_mfts = lcm_resume_mfts,
  .shutdown = lcm_shutdown,
#if USE_DEEP_SLEEP
  .set_deep_sleep = lcm_set_deep_sleep
#endif
#ifdef CONFIG_LGE_INIT_CMD_TUNING
  .get_lcm_init_cmd_str = lcm_get_lcm_init_cmd_str,
  .get_init_cmd_str_size = lcm_get_init_cmd_str_size,
#endif
};
