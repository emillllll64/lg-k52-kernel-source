/**
 * Copyright (c) 2018 WinTech Technologies Co., Ltd. 2018-2019. All rights reserved.
 *Description: Core Defination For Foursemi Device .
 *Author: Fourier Semiconductor Inc.
 * Create: 2019-03-17 File created.
 */

#ifndef __FSM_DEV_H__
#define __FSM_DEV_H__

#undef CONFIG_REGULATOR
//#define FSM_DEBUG

#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "%s:%d: " fmt, __func__, __LINE__
#else
#define pr_fmt(fmt) "%s:%d: " fmt, __func__, __LINE__
#endif
#if defined(__KERNEL__)
#include <linux/module.h>
#include <linux/regmap.h>
//#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#if defined(CONFIG_REGULATOR)
#include <linux/regulator/consumer.h>
#endif
#elif defined(FSM_HAL_V2_SUPPORT)
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

/* debug info */
#ifdef FSM_DEBUG
#ifndef LOG_NDEBUG
#define LOG_NDEBUG 1
#endif
#endif

#if defined(LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "fsm_hal"

#if defined(FSM_APP)
#define pr_debug(fmt, args...) printf("%s: " fmt, __func__, ##args)
#define pr_info(fmt, args...) printf("%s: " fmt, __func__, ##args)
#define pr_err(fmt, args...) printf("%s: " fmt, __func__, ##args)
#define pr_warning(fmt, args...) printf("%s: " fmt, __func__, ##args)
#elif defined(__NDK_BUILD__)
#include<android/log.h>
#define pr_debug(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s: " fmt, __func__, ##args)
#define pr_info(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s: " fmt, __func__, ##args)
#define pr_err(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args)
#define pr_warning(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args)
#elif defined(__ANDROID__)
#include <utils/Log.h>
#define pr_debug(fmt, args...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s: " fmt, __func__, ##args)
#define pr_info(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s: " fmt, __func__, ##args)
#define pr_err(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args)
#define pr_warning(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args)
#else
#define pr_debug(fmt, args...)
#define pr_info(fmt, args...)
#define pr_err(fmt, args...)
#define pr_warning(fmt, args...)
#endif
#endif

/* device id */
#define FS1601S_DEV_ID       0x03
#define FS1603_DEV_ID        0x05
#define FS1818_DEV_ID        0x06
#define FS1860_DEV_ID        0x07
#define MAX_DEV_COUNT        2

#define CUST_NAME_SIZE        32
#define FSM_OTP_ACC_KEY2      0xCA91
#define FSM_SPKR_ALLOWANCE    (30)
#define FSM_WAIT_STABLE_RETRY (200)
#define FSM_ZMDELTA_MAX       0x150
#define FS160X_RS2RL_RATIO    (2700)
#define FSM_RS_TRIM_DEFAULT   0x8f
#define FSM_MAGNIF_TEMPR_COEF 0xFFFF

#define PRESET_MUSIC          0
#define PRESET_VOICE          1
#define PRESET_VOICE_EARPIECE 2
#define PRESET_BYPASS         3
#define PRESET_MMI_LEFT       4
#define PRESET_MMI_RIGHT      5
#define PRESET_RINGTONE       6


#define FSM_DEV_LEFT          0x34
#define FSM_DEV_RIGHT         0x36

#define STEREO_COEF_LEN       (10)

#define FSM_DATA_TYPE_RE25    0
#define FSM_DATA_TYPE_ZMDATA  1

/* device mask */
#define BIT(nr)            (1UL << (nr))
#define FSM_DEV_MAX        (4)
#define FSM_ADDR_BASE      (0x34)

/*
#define FSM_SCENE_MUSIC            BIT(0)
#define FSM_SCENE_VOICE            BIT(1)
#define FSM_SCENE_VOIP             BIT(2)
#define FSM_SCENE_RING             BIT(3)
#define FSM_SCENE_LOW_PWR          BIT(4)
#define FSM_SCENE_MMI_ALL          BIT(5)
#define FSM_SCENE_MMI_ALL_BYPASS   BIT(6)
#define FSM_SCENE_TOP_LEFT         BIT(7)
#define FSM_SCENE_TOP_LEFT_BYPASS  BIT(8)
#define FSM_SCENE_TOP_RIGHT        BIT(9)
#define FSM_SCENE_TOP_RIGHT_BYPASS BIT(10)
#define FSM_SCENE_BOT_LEFT         BIT(11)
#define FSM_SCENE_BOT_LEFT_BYPASS  BIT(12)
#define FSM_SCENE_BOT_RIGHT        BIT(13)
#define FSM_SCENE_BOT_RIGHT_BYPASS BIT(14)
#define FSM_SCENE_MAX              (15)*/

#define FSM_SCENE_MUSIC            BIT(0)
#define FSM_SCENE_VOICE            BIT(1)
#define FSM_SCENE_EARPIECE         BIT(2)
#define FSM_SCENE_BYPASS           BIT(3)
#define FSM_SCENE_MMI_LEFT         BIT(4)
#define FSM_SCENE_MMI_RIGHT        BIT(5)
#define FSM_SCENE_RINGTONE         BIT(6)

#define FSM_SCENE_COMMON           (0xffff)
#define FSM_SCENE_UNKNOW           (0)

#define FSM_FLAGS_BYPASS BIT(0)
#define FSM_DISABLE      0
#define FSM_ENABLE       1

#define STATE(tag)           FSM_STATE_##tag
#define FSM_STATE_UNKNOWN    0
#define FSM_STATE_FW_INITED  BIT(0)
#define FSM_STATE_DEV_INITED BIT(1)
#define FSM_STATE_OTP_STORED BIT(2)
#define FSM_STATE_CALIB_DONE BIT(3)
#define FSM_STATE_AMP_ON     BIT(4)
#define FSM_STATE_MAX        0xffffffff// 32bit

#define ACS_COEF_COUNT     12
#define COEF_LEN           5
#define FREQ_START         200
#define F0_COUNT_MAX       20
#define FREQ_EXT_COUNT     300

#define F0_MMI_COUNT       12
#define F0_MMI_FREQ_STEP   150
#define F0_MMI_DELAY_MS    305
#define F0_MMI_FIRST_DELAY 600

#define F0_RT_COUNT        18
#define F0_RT_FREQ_STEP    100
#define F0_RT_DELAY_MS     430
#define F0_RT_FIRST_DELAY  850

#define ZMDATA_NOISE_LEVEL 0x0800
#define ZMDATA_VALID_MIN   10000
#define ZMDATA_VALID_MAX   60000
#define ZM_COMPENS_COEF    ((2 * 3.14 * 40) / (1000000 * 8))

enum fsm_cmd {
    FSM_CMD_UNKNOWN = -1,
    FSM_CMD_LOAD_FW,
    FSM_CMD_INIT,
    FSM_CMD_SWH_PRESET,
    FSM_CMD_START_UP,
    FSM_CMD_SET_MUTE,
    FSM_CMD_WAIT_STABLE,
    FSM_CMD_SHUT_DOWN,
    FSM_CMD_CALIB_STEP1,
    FSM_CMD_CALIB_STEP2,
    FSM_CMD_CALIB_STEP3,
    FSM_CMD_GET_RE25,
    FSM_CMD_F0_STEP1,
    FSM_CMD_F0_STEP2,
    FSM_CMD_GET_ZMDATA,
    FSM_CMD_DUMP_REG,
    FSM_CMD_SELECT_DEV,
    FSM_CMD_SET_BYPASS,
    FSM_CMD_PARSE_FIRMW,
    FSM_CMD_MAX,
};

enum fsm_event {
    FSM_EVENT_UNKNOWN = -1,
    FSM_EVENT_LOAD_FW,
    FSM_EVENT_SET_SRATE,
    FSM_EVENT_SET_BCLK,
    FSM_EVENT_SET_SCENE,
    FSM_EVENT_SET_VOLUME,
    FSM_EVENT_SPEAKER_ON,
    FSM_EVENT_SPEAKER_OFF,
    FSM_EVENT_INIT,
    FSM_EVENT_CALIBRATE,
    FSM_EVENT_GET_RE25,
    FSM_EVENT_GET_DEVICE,
    FSM_EVENT_GET_SCENE,
    FSM_EVENT_MAX,
};

enum test_type {
    FSM_UNKNOW_TEST = -1,
    FSM_MMI_TEST = 0,
    FSM_RT_TEST = 1,
};

enum fsm_error {
    FSM_ERROR_OK = 0,
    FSM_ERROR_BAD_PARMS = -101,
    FSM_ERROR_I2C_FATAL,
    FSM_ERROR_NO_SCENE_MATCH,
    FSM_ERROR_RE25_INVAILD,
    FSM_ERROR_READ_FILE_FAILED,
    FSM_ERROR_INIT_FAILED,
    FSM_ERROR_OPEN_DEV_FAILED,
    FSM_ERROR_SPEAKER_ON_FAILED,
    FSM_ERROR_CALIB_FAILED,
    FSM_ERROR_OC_DETECTED,
    FSM_ERROR_OV_DETECTED,
    FSM_ERROR_UV_DETECTED,
    FSM_ERROR_OT_DETECTED,
    FSM_ERROR_NO_CLK,
    FSM_ERROR_MAX,
};

enum fsm_srate {
    FSM_RATE_08000 = 0,
    FSM_RATE_16000 = 3,
    FSM_RATE_32000 = 6,
    FSM_RATE_44100 = 7,
    FSM_RATE_48000 = 8,
    FSM_RATE_88200 = 9,
    FSM_RATE_96000 = 10,
    FSM_RATE_MAX,
};

enum fsm_channel {
    FSM_CHN_LEFT = 1,
    FSM_CHN_RIGHT = 2,
    FSM_CHN_MONO = 3,
};

enum fsm_format {
    FSM_FMT_DSP = 1,
    FSM_FMT_MSB = 2,
    FSM_FMT_I2S = 3,
    FSM_FMT_LSB_16 = 4,
    FSM_FMT_LSB_20 = 6,
    FSM_FMT_LSB_24 = 7,
};

enum fsm_bstmode {
    FSM_BSTMODE_FLW = 0,
    FSM_BSTMODE_BST = 1,
    FSM_BSTMODE_ADP = 2,
};

enum fsm_dsp_state {
    FSM_DSP_OFF = 0,
    FSM_DSP_ON = 1,
};

enum fsm_pll_state {
    FSM_PLL_OFF = 0,
    FSM_PLL_ON = 1,
};

enum fsm_mute_type {
    FSM_MUTE_UNKNOW = -1,
    FSM_UNMUTE = 0,
    FSM_MUTE = 1,
};

enum fsm_wait_type {
    FSM_WAIT_AMP_ON,
    FSM_WAIT_AMP_OFF,
    FSM_WAIT_AMP_ADC_OFF,
    FSM_WAIT_AMP_ADC_PLL_OFF,
    FSM_WAIT_TSIGNAL_ON,
    FSM_WAIT_TSIGNAL_OFF,
    FSM_WAIT_OTP_READY,
};

enum fsm_eq_ram_type {
    FSM_EQ_RAM0 = 0,
    FSM_EQ_RAM1,
    FSM_EQ_RAM_MAX,
};

struct fsm_pll_config {
    unsigned int bclk;
    uint16_t c1;
    uint16_t c2;
    uint16_t c3;
};
typedef struct fsm_pll_config fsm_pll_config_t;

enum fsm_dsc_type {
    FSM_DSC_DEV_INFO = 0,
    FSM_DSC_SPK_INFO,
    FSM_DSC_REG_COMMON,
    FSM_DSC_REG_SCENES,
    FSM_DSC_PRESET_EQ,
    FSM_DSC_STEREO_COEF,
    FSM_DSC_EXCER_RAM,
    FSM_DSC_MAX,
};

#pragma pack(push, 1)

struct fsm_date {
    uint32_t min : 6;
    uint32_t hour : 5;
    uint32_t day : 5;
    uint32_t month : 4;
    uint32_t year : 12;
};
typedef struct fsm_date fsm_date_t;

struct data32_list {
    uint16_t len;
    uint32_t data[];
};
typedef struct data32_list preset_data_t;
typedef struct data32_list ram_data_t;

struct uint24 {
    uint8_t DL;
    uint8_t DM;
    uint8_t DH;
};
typedef struct uint24 uint24_t;

struct data24_list {
    uint16_t len;
    uint24_t data[];
};

struct data16_list {
    uint16_t len;
    uint16_t data[];
};
typedef struct data16_list info_list_t;
typedef struct data16_list stereo_coef_t;

struct fsm_index {
    uint16_t offset;
    uint16_t type;
};
typedef struct fsm_index fsm_index_t;

struct dev_list {
    uint16_t preset_ver;
    char project[8];
    char customer[8];
    fsm_date_t date;
    uint16_t data_len;
    uint16_t crc16;
    uint16_t len;
    uint16_t bus;
    uint16_t addr;
    uint16_t dev_type;
    uint16_t npreset;
    uint16_t reg_scenes;
    uint16_t eq_scenes;
    fsm_index_t index[];
};
typedef struct dev_list dev_list_t;

struct preset_list {
    uint16_t len;
    uint16_t scene;
    uint32_t data[];
};
typedef struct preset_list preset_list_t;

struct reg_unit {
    uint16_t addr : 8;
    uint16_t pos : 4;
    uint16_t len : 4;
    uint16_t value;
};
typedef struct reg_unit reg_unit_t;

struct reg_comm {
    uint16_t len;
    reg_unit_t reg[];
};
typedef struct reg_comm reg_comm_t;

struct regs_unit {
    uint16_t scene;
    reg_unit_t reg;
};
typedef struct regs_unit regs_unit_t;

struct reg_scene {
    uint16_t len;
    regs_unit_t regs[];
};
typedef struct reg_scene reg_scene_t;

struct preset_header {
    uint16_t version;
    char customer[8];
    char project[8];
    fsm_date_t date;
    uint16_t size;
    uint16_t crc16;
    uint16_t ndev;
};

struct preset_file {
    struct preset_header hdr;
    fsm_index_t index[];
};

#pragma pack(pop)

struct fsm_dev;

struct fsm_ops {
    int (*init_register)(struct fsm_dev *fsm);
    int (*switch_preset)(struct fsm_dev *fsm, int force);
    int (*check_otp)(struct fsm_dev *fsm);
    int (*start_up)(struct fsm_dev *fsm);
    int (*shut_down)(struct fsm_dev *fsm);
    int (*set_mute)(struct fsm_dev *fsm, int mute, uint8_t volume);
    int (*wait_stable)(struct fsm_dev *fsm, int type);
    int (*calib_step1)(struct fsm_dev *fsm, int force);
    int (*calib_step2)(struct fsm_dev *fsm);
    int (*calib_step3)(struct fsm_dev *fsm, int store);
    int (*f0_step1)(struct fsm_dev *dev);
    int (*f0_step2)(struct fsm_dev *dev);
    int (*dsp_bypass)(struct fsm_dev *fsm_dev, int bypass);
    int (*write_default_r25)(struct fsm_dev *fsm, int store);
};
typedef struct fsm_ops fsm_ops_t;

struct fsm_config {
    uint16_t event;
    uint16_t next_scene;
    uint16_t test_freq;
    uint8_t volume;
    uint8_t channel;
    uint8_t state;
    uint8_t dev_sel;
    uint8_t flags;
    uint8_t f0cal_mode;
    unsigned int bclk;
    unsigned int srate;
};
typedef struct fsm_config fsm_config_t;

struct fsm_calib_data {
    uint16_t preval;
    uint16_t minval;
    uint16_t zmdata;
    uint16_t count;
    int calib_count;
    unsigned int calib_re25;
    int store_otp;
};
typedef struct fsm_calib_data fsm_calib_data_t;

struct fsm_f0 {
    uint16_t freq;
    int gain;
    unsigned int f0;
};

struct fsm_bpcoef {
    int freq;
    uint32_t coef[COEF_LEN];
};

typedef struct fsm_preset_info Fsm_Preset_Info;

struct fsm_dev {
#if defined(CONFIG_I2C)
    struct i2c_client *i2c;
#endif
    struct fsm_ops dev_ops;
    struct fsm_config *config;
    struct preset_file *preset;
    struct dev_list *dev_list;
    struct fsm_calib_data calib_data;
    struct fsm_f0 f0_data;
    uint8_t i2c_addr;
    int spkr;
    uint16_t version;
    uint16_t flag;
    uint16_t ram_scene[FSM_EQ_RAM_MAX];
    uint16_t own_scene;
    uint16_t cur_scene;
    int zmdata_done;
    uint8_t dev_mask;
    uint16_t last_zmdata;
    int dev_state;
    int errcode;
    const char *preset_name;
    char const *dev_name;
};
typedef struct fsm_dev fsm_dev_t;

#include <linux/version.h>

#define FIRMWARE_CHECKER

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 1)
#define KERNEL_VER_OVER_4_19_1
#endif

#ifdef FIRMWARE_CHECKER
#include <linux/timer.h>
#endif
struct fsm_pdata {
#ifdef __KERNEL__
    //struct miscdevice misc_dev;
    struct regulator *vdd;
#ifdef KERNEL_VER_OVER_4_19_1
    struct snd_soc_component *component;
#else
    struct snd_soc_codec *codec;
#endif
    struct workqueue_struct *fsm_wq;
    struct delayed_work init_work;
    struct delayed_work monitor_work;
#ifdef FIRMWARE_CHECKER
    struct timer_list firmware_checker_timer;
    struct work_struct firmware_checker;
    struct workqueue_struct *firmware_checker_wq;
#endif

    struct mutex i2c_lock;
    wait_queue_head_t wq;
    struct device *dev;
    struct list_head list;
#endif
    struct fsm_dev fsm_dev;
};
typedef struct fsm_pdata fsm_pdata_t;

#endif
