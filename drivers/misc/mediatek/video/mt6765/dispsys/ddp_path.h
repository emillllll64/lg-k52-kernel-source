/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __DDP_PATH_H__
#define __DDP_PATH_H__

#include "ddp_info.h"
#include "../../cmdq/v3/cmdq_record.h"

enum DDP_MODE {
	DDP_VIDEO_MODE = 0,
	DDP_CMD_MODE,
};

enum DDP_SCENARIO_ENUM {
	DDP_SCENARIO_PRIMARY_DISP = 0, /* main path */
	DDP_SCENARIO_PRIMARY_BYPASS_PQ_DISP,	/* bypass pq module */
	DDP_SCENARIO_PRIMARY_RDMA0_COLOR0_DISP, /* by pass ovl */
	DDP_SCENARIO_PRIMARY_RDMA0_DISP, /* debug */
	DDP_SCENARIO_PRIMARY_OVL_MEMOUT, /* dc */
	DDP_SCENARIO_PRIMARY_ALL, /* main 1to2 */
	DDP_SCENARIO_SUB_DISP,
	DDP_SCENARIO_SUB_RDMA1_DISP,
	DDP_SCENARIO_SUB_OVL_MEMOUT,
	DDP_SCENARIO_SUB_ALL,
	DDP_SCENARIO_MAX
};

enum DDP_SPM_MODE {
	DDP_CG_MODE = 0,
	DDP_PD_MODE,
};

#define DDP_ENING_NUM (24)

void ddp_connect_path(enum DDP_SCENARIO_ENUM scenario, void *handle);
void ddp_disconnect_path(enum DDP_SCENARIO_ENUM scenario, void *handle);
int ddp_get_module_num(enum DDP_SCENARIO_ENUM scenario);

void ddp_check_path(enum DDP_SCENARIO_ENUM scenario);


enum DISP_MODULE_ENUM ddp_get_dst_module(enum DDP_SCENARIO_ENUM scenario);
int ddp_set_dst_module(enum DDP_SCENARIO_ENUM scenario,
	enum DISP_MODULE_ENUM dst_module);

int *ddp_get_scenario_list(enum DDP_SCENARIO_ENUM ddp_scenario);

char *ddp_get_scenario_name(enum DDP_SCENARIO_ENUM scenario);

int ddp_path_top_clock_off(void);
int ddp_path_top_clock_on(void);

/* should remove */
int ddp_insert_config_allow_rec(void *handle);
int ddp_insert_config_dirty_rec(void *handle);

int disp_get_dst_module(enum DDP_SCENARIO_ENUM scenario);
int ddp_is_module_in_scenario(enum DDP_SCENARIO_ENUM ddp_scenario,
	enum DISP_MODULE_ENUM module);
int ddp_path_init(void);
int ddp_convert_ovl_input_to_rdma(struct RDMA_CONFIG_STRUCT *rdma_cfg,
	struct OVL_CONFIG_STRUCT *ovl_cfg,
					int dst_w, int dst_h);
int ddp_get_module_num_l(int *module_list);
char *ddp_get_mode_name(enum DDP_MODE ddp_mode);
void ddp_set_spm_mode(enum DDP_SPM_MODE mode, void *handle);

#include "ddp_mutex.h"

#endif
