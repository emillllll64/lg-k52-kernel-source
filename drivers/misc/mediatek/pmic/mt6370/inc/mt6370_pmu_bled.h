/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __LINUX_MT6370_PMU_BLED_H
#define __LINUX_MT6370_PMU_BLED_H

#ifdef CONFIG_LGE_DISPLAY_COMMON
#include <linux/semaphore.h>
#endif

struct mt6370_pmu_bled_platdata {
	uint8_t ext_en_pin:1;
	uint8_t chan_en:4;
	uint8_t map_linear:1;
	uint8_t bl_ovp_level:2;
	uint8_t bl_ocp_level:2;
	uint8_t use_pwm:1;
	uint8_t pwm_fsample:2;
	uint8_t pwm_deglitch:2;
	uint8_t pwm_hys_en:1;
	uint8_t pwm_hys:2;
	uint8_t pwm_avg_cycle:3;
	uint8_t bled_ramptime:4;
	uint8_t bled_flash_ramp:3;
	uint8_t bled_curr_scale:2;
	uint8_t pwm_lpf_coef:2;
	uint8_t pwm_lpf_en:1;
	uint8_t bled_curr_mode:1;
	uint32_t max_bled_brightness;
	const char *bled_name;
};

/* MT6370_PMU_REG_BLEN : 0xA0 */
#define MT6370_BLED_EN	(0x40)
#define MT6370_BLED_EXTEN (0x80)
#ifdef CONFIG_MT6370_PMU_BLED_3CH
#define MT6370_BLED_CHAN_EN (0x38) // bled 3 channel
#else
#define MT6370_BLED_CHAN_EN (0x3C) // bled 4 channel
#endif
#define MT6370_BLED_CHANENSHFT 2
#define MT6370_BLED_MAPLINEAR (0x02)

/* MT6370_PMU_REG_BLBSTCTRL : 0xA1 */
#define MT6370_BLED_OVOCSHDNDIS (0x88)
#define MT6370_BLED_OVPSHFT (5)
#define MT6370_BLED_OCPSHFT (1)

/* MT6370_PMU_REG_BLPWM : 0xA2 */
#define MT6370_BLED_PWMSHIFT (7)
#define MT6370_BLED_PWMDSHFT (5)
#define MT6370_BLED_PWMFSHFT (3)
#define MT6370_BLED_PWMHESHFT (2)
#define MT6370_BLED_PWMHSHFT (0)

/* MT6370_PMU_REG_BLCTRL : 0xA3 */
#define MT6370_BLED_RAMPTSHFT (4)

/* MT6370_PMU_REG_BLDIM2 : 0xA6 */
#define MT6370_DIM2_MASK (0x07)

/* MT6370_PMU_REG_BLDIM1 : 0xA5 */
#define MT6370_DIM_MASK	(0xFF)

/* MT6370_PMU_REG_BLFL : 0xA7 */
#define MT6370_BLFLMODE_MASK (0xC0)
#define MT6370_BLFLMODE_SHFT (6)
#define MT6370_BLFLRAMP_SHFT (3)

/* MT6370_PMU_REG_BLFLTO : 0xA8 */
#define MT6370_BLSTRB_TOMASK (0x7F)

/* MT6370_PMU_REG_BLMODECTRL : 0xAD */
#define MT6370_BLED_CURR_SCALESHFT	(4)
#define MT6370_PWM_LPF_COEFSHFT		(2)
#define MT6370_PWM_LPF_ENSHFT		(1)
#define MT6370_BLED_CURR_MODESHFT	(0)

#ifdef CONFIG_LGE_DISPLAY_COMMON
#define MT6370_BLED_REG_NUM  11

#define MT6370_BLED_OVPMASK (0x60)
#define MT6370_BLED_OCPMASK (0x06)
#define MT6370_BLED_PWMMASK (0x80)
#define MT6370_BLED_PWMDMASK (0x60)
#define MT6370_BLED_PWMFMASK (0x18)
#define MT6370_BLED_RAMPTMASK (0xF0)

extern unsigned int* blmap_arr;
extern int down_interruptible(struct semaphore *sem);
extern void up(struct semaphore *sem);
int chargepump_set_backlight_level(bool enable);
uint32_t lge_get_bled_dim_brightness(void);
void lge_set_bled_dim_brightness(uint32_t level);
#endif
#endif
