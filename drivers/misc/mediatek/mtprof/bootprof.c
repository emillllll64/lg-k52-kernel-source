// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/utsname.h>
#include <linux/uaccess.h>

#ifdef MODULE
#include <linux/tracepoint.h>
#include <trace/events/initcall.h>
#endif
#ifdef CONFIG_LGE_HANDLE_PANIC
#include <soc/mediatek/lge/lge_handle_panic.h>
#endif

#define BOOT_STR_SIZE 256
#define BUF_COUNT 12
#define LOGS_PER_BUF 80
#define MSG_SIZE 128

struct log_t {
	/* task cmdline for first 16 bytes
	 * and boot event for the rest
	 */
	char *comm_event;
	pid_t pid;
	u64 timestamp;
};

static struct log_t *bootprof[BUF_COUNT];
static unsigned long log_count;
static DEFINE_MUTEX(bootprof_lock);
static bool enabled;
static int bootprof_lk_t, bootprof_pl_t, bootprof_logo_t;
static u64 timestamp_on, timestamp_off;
static bool boot_finish;

#ifdef CONFIG_BOOTPROF_THRESHOLD_MS
#define BOOTPROF_THRESHOLD (CONFIG_BOOTPROF_THRESHOLD_MS*1000000)
#else
#define BOOTPROF_THRESHOLD 15000000
#endif

#ifdef MODULE
static unsigned long long start_time;

/* Data structures to store tracepoints information */
struct bf_tp {
	const char *name;
	void *func;
	struct tracepoint *tp;
	void *data;
	bool init;
};
#else /*Build-in*/
module_param_named(pl_t, bootprof_pl_t, int, 0644);
module_param_named(lk_t, bootprof_lk_t, int, 0644);
module_param_named(logo_t, bootprof_logo_t, int, 0644);
#endif

static long long msec_high(unsigned long long nsec)
{
	if ((long long)nsec < 0) {
		nsec = -nsec;
		do_div(nsec, 1000000);
		return -nsec;
	}
	do_div(nsec, 1000000);

	return nsec;
}

static unsigned long msec_low(unsigned long long nsec)
{
	if ((long long)nsec < 0)
		nsec = -nsec;

	return do_div(nsec, 1000000);
}

bool mt_boot_finish(void)
{
	return boot_finish;
}
EXPORT_SYMBOL_GPL(mt_boot_finish);

void bootprof_log_boot(char *str)
{
	unsigned long long ts;
	struct log_t *p = NULL;
	size_t n;

	if (!str) {
		pr_info("[BOOTPROF] Null buffer. Skip log.\n");
		return;
	}

	if (!enabled)
		return;
	n = strlen(str) + 1;

	ts = sched_clock();
	pr_info("BOOTPROF:%10lld.%06ld:%s\n", msec_high(ts), msec_low(ts), str);

	mutex_lock(&bootprof_lock);
	if (log_count >= (LOGS_PER_BUF * BUF_COUNT)) {
		pr_info("[BOOTPROF] not enuough bootprof buffer\n");
		goto out;
	} else if (log_count && !(log_count % LOGS_PER_BUF)) {
		bootprof[log_count / LOGS_PER_BUF] =
			kcalloc(LOGS_PER_BUF, sizeof(struct log_t),
				GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN);
	}
	if (!bootprof[log_count / LOGS_PER_BUF]) {
		pr_info("no memory for bootprof\n");
		goto out;
	}
	p = &bootprof[log_count / LOGS_PER_BUF][log_count % LOGS_PER_BUF];

	p->timestamp = ts;
	p->pid = current->pid;
	n += TASK_COMM_LEN;

	p->comm_event = kzalloc(n, GFP_ATOMIC | __GFP_NORETRY |
			  __GFP_NOWARN);
	if (!p->comm_event) {
		enabled = false;
		goto out;
	}

	memcpy(p->comm_event, current->comm, TASK_COMM_LEN);
	memcpy(p->comm_event + TASK_COMM_LEN, str, n - TASK_COMM_LEN);

#ifdef CONFIG_LGE_HANDLE_PANIC
{
	char buf[BOOT_STR_SIZE] = {0,};

#ifdef TRACK_TASK_COMM
#define FMT "%10lld.%06ld :%5d-%-16s: %s\n"
#else
#define FMT "%10lld.%06ld : %s\n"
#endif

	sprintf(buf, FMT, msec_high(p->timestamp),
				msec_low(p->timestamp),
#ifdef TRACK_TASK_COMM
				p->pid, p->comm_event, p->comm_event + TASK_COMM_LEN
#else
				p->comm_event
#endif
			);
	lge_set_bootprof(buf);
}
#endif

	log_count++;
out:
	mutex_unlock(&bootprof_lock);
}
EXPORT_SYMBOL_GPL(bootprof_log_boot);

static void bootprof_bootloader(void)
{
	struct device_node *node;
	int err = 0;

	node = of_find_node_by_name(NULL, "bootprof");
	if (node) {
		err |= of_property_read_s32(node, "pl_t", &bootprof_pl_t);
		err |= of_property_read_s32(node, "lk_t", &bootprof_lk_t);
		err |= of_property_read_s32(node, "lk_logo_t",
						&bootprof_logo_t);

		pr_info("BOOTPROF: DT(Err:0x%x) pl_t=%d, lk_t=%d, lk_logo_t=%d\n",
			err, bootprof_pl_t, bootprof_lk_t, bootprof_logo_t);
	}
}

void bootprof_initcall(initcall_t fn, unsigned long long ts)
{
	/* log more than threshold initcalls */
	unsigned long msec_rem;
	char msgbuf[MSG_SIZE];
	int len;

	if (ts > BOOTPROF_THRESHOLD) {
		msec_rem = do_div(ts, NSEC_PER_MSEC);
		len = scnprintf(msgbuf, sizeof(msgbuf),
			"initcall: %ps %5llu.%06lums",
			fn, ts, msec_rem);
		if (len < 0)
			pr_info("BOOTPROF: initcall - Invalid argument.\n");
		bootprof_log_boot(msgbuf);
	}
}

#ifndef MODULE
/*Build-in*/
void bootprof_probe(unsigned long long ts, struct device *dev,
			   struct device_driver *drv, unsigned long probe)
{
	/* log more than threshold probes*/
	unsigned long msec_rem;
	char msgbuf[MSG_SIZE];
	int pos, len;

	if (ts <= BOOTPROF_THRESHOLD)
		return;
	msec_rem = do_div(ts, NSEC_PER_MSEC);

	pos = scnprintf(msgbuf, sizeof(msgbuf), "probe: probe=%ps",
					(void *)probe);
	if (pos < 0)
		pos = 0;

	if (drv) {
		len = scnprintf(msgbuf + pos, sizeof(msgbuf) - pos,
				" drv=%s(%ps)", drv->name ? drv->name : "",
				(void *)drv);
		if (len >= 0)
			pos += len;
	}

	if (dev && dev->init_name) {
		len = scnprintf(msgbuf + pos, sizeof(msgbuf) - pos,
				" dev=%s(%ps)", dev->init_name, (void *)dev);
		if (len >= 0)
			pos += len;
	}

	scnprintf(msgbuf + pos, sizeof(msgbuf) - pos,
			" %5llu.%06lums", ts, msec_rem);
	bootprof_log_boot(msgbuf);
}

void bootprof_pdev_register(unsigned long long ts, struct platform_device *pdev)
{
	/* log more than threshold register*/
	unsigned long msec_rem;
	char msgbuf[MSG_SIZE];
	int len;

	if (ts <= BOOTPROF_THRESHOLD || !pdev)
		return;
	msec_rem = do_div(ts, NSEC_PER_MSEC);
	len = scnprintf(msgbuf, sizeof(msgbuf),
			"probe: pdev=%s(%ps) %5llu.%06lums",
			pdev->name, (void *)pdev, ts, msec_rem);
	if (len < 0)
		pr_info("BOOTPROF: pdev - Invalid argument.\n");

	bootprof_log_boot(msgbuf);
}

static void bootup_finish(void)
{
	initcall_debug = 0;
}
#endif /*MODULE END*/

static void mt_bootprof_switch(int on)
{
	mutex_lock(&bootprof_lock);
	if (enabled ^ on) {
		unsigned long long ts = sched_clock();

		pr_info("BOOTPROF:%10lld.%06ld: %s\n",
		       msec_high(ts), msec_low(ts), on ? "ON" : "OFF");

		if (on) {
			enabled = 1;
			timestamp_on = ts;
		} else {
			enabled = 0;
			timestamp_off = ts;
			if (!boot_finish)
				boot_finish = true;
#ifndef MODULE
			bootup_finish();
#endif
		}
	}
	mutex_unlock(&bootprof_lock);
}

static ssize_t
mt_bootprof_write(struct file *filp, const char *ubuf, size_t cnt, loff_t *data)
{
	char buf[BOOT_STR_SIZE];
	size_t copy_size = cnt;

	if (cnt >= sizeof(buf))
		copy_size = BOOT_STR_SIZE - 1;

	if (copy_from_user(&buf, ubuf, copy_size))
		return -EFAULT;

	if (cnt == 1 && buf[0] == '1') {
		mt_bootprof_switch(1);
		return 1;
	} else if (cnt == 1 && buf[0] == '0') {
		mt_bootprof_switch(0);
		return 1;
	}

	buf[copy_size] = 0;
	bootprof_log_boot(buf);

	return cnt;
}

static int mt_bootprof_show(struct seq_file *m, void *v)
{
	int i;
	struct log_t *p;

	if (!m) {
		pr_info("seq_file is Null.\n");
		return 0;
	}
	seq_puts(m, "----------------------------------------\n");
	seq_printf(m, "%d	    BOOT PROF (unit:msec)\n", enabled);
	seq_puts(m, "----------------------------------------\n");

	if (bootprof_pl_t > 0 && bootprof_lk_t > 0) {
		seq_printf(m, "%10d        : %s\n", bootprof_pl_t, "preloader");
		if (bootprof_logo_t > 0) {
			seq_printf(m, "%10d        : %s (%s: %d)\n",
			bootprof_lk_t, "lk", "Start->Show logo",
			bootprof_logo_t);
		} else {
			seq_printf(m, "%10d        : %s\n",
			bootprof_lk_t, "lk");
		}
		seq_puts(m, "----------------------------------------\n");
	}

	seq_printf(m, "%10lld.%06ld : ON (Threshold:%5lldms)\n",
		   msec_high(timestamp_on), msec_low(timestamp_on),
		   msec_high(BOOTPROF_THRESHOLD));

	for (i = 0; i < log_count; i++) {
		p = &bootprof[i / LOGS_PER_BUF][i % LOGS_PER_BUF];
		if (!p->comm_event)
			continue;

		seq_printf(m, "%10llu.%06lu :%5d-%-16s: %s\n",
			   msec_high(p->timestamp),
			   msec_low(p->timestamp),
			   p->pid, p->comm_event,
			   p->comm_event + TASK_COMM_LEN);
	}

	seq_printf(m, "%10lld.%06ld : OFF\n",
		   msec_high(timestamp_off), msec_low(timestamp_off));
	seq_puts(m, "----------------------------------------\n");
	return 0;
}

/*** Seq operation of mtprof ****/
static int mt_bootprof_open(struct inode *inode, struct file *file)
{
	return single_open(file, mt_bootprof_show, inode->i_private);
}

static const struct file_operations mt_bootprof_fops = {
	.open = mt_bootprof_open,
	.write = mt_bootprof_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef MODULE

/*  initcalls tracepoint cb if initcall_debug=1 */
static __init_or_module void
tp_initcall_start_cb(void *data, initcall_t fn)
{
	unsigned long long *start_ts  = (unsigned long long *)data;
	*start_ts  = sched_clock();
}

static __init_or_module void
tp_initcall_finish_cb(void *data, initcall_t fn, int ret)
{
	unsigned long long *start_ts = (unsigned long long *)data;
	unsigned long long end_ts, duration;

	/*For bootprof module without initcall_start*/
	if (*start_ts == 0) {
		bootprof_log_boot("Kernel_init_done");
		return;
	}
	end_ts = sched_clock();
	duration = end_ts - *start_ts;
	bootprof_initcall(fn, duration);
	*start_ts = 0;
}

static struct bf_tp tp_table[] = {
	{.name = "initcall_start", .func = tp_initcall_start_cb,
	.data = &start_time},
	{.name = "initcall_finish", .func = tp_initcall_finish_cb,
	.data = &start_time},
};

/* Find the struct tracepoint* associated with a given tracepoint */
/* name. */
static void tp_lookup(struct tracepoint *tp, void *ignore)
{
	int i;

	if (!tp || !tp->name)
		return;

	for (i = 0; i < sizeof(tp_table) / sizeof(struct bf_tp); i++) {
		if (strcmp(tp_table[i].name, tp->name) == 0)
			tp_table[i].tp = tp;
	}
}

/* claen up initcalls tracepoints */
static void tp_cleanup(void)
{
	int i;

	for (i = 0; i < sizeof(tp_table) / sizeof(struct bf_tp); i++) {
		if (tp_table[i].init) {
			tracepoint_probe_unregister(tp_table[i].tp,
				tp_table[i].func, tp_table[i].data);
			tp_table[i].init = false;
		}
	}
}

/* Register initcalls tracepoints */
static void tp_init(void)
{
	int i;
	/* Install the tracepoints */
	for_each_kernel_tracepoint(tp_lookup, NULL);
	for (i = 0; i < sizeof(tp_table) / sizeof(struct bf_tp); i++) {
		if (!tp_table[i].tp) {
			pr_info("[BOOTPROF]TP: %s not found\n",
					tp_table[i].name);
			/* Unload previously loaded */
			tp_cleanup();
			return;
		}
		tracepoint_probe_register(tp_table[i].tp, tp_table[i].func,
						tp_table[i].data);
		tp_table[i].init = true;
	}
}

static int __init bootprof_init(void)
{
	struct proc_dir_entry *pe;

	memset(bootprof, 0, sizeof(struct log_t *) * BUF_COUNT);
	bootprof[0] = kcalloc(LOGS_PER_BUF, sizeof(struct log_t),
			GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN);
	if (!bootprof[0])
		goto fail;

	pe = proc_create("bootprof", 0664, NULL, &mt_bootprof_fops);
	if (!pe)
		return -ENOMEM;

	bootprof_bootloader();
	tp_init();
	mt_bootprof_switch(1);
fail:
	return 0;
}

static void __exit bootprof_exit(void)
{
	struct log_t *p = NULL;
	int i;

	enabled = 0;
	tp_cleanup();

	if (log_count > 0) {
		mutex_lock(&bootprof_lock);
		for (i = 0; i < log_count; i++) {
			p = &bootprof[i / LOGS_PER_BUF][i % LOGS_PER_BUF];
			kfree(p->comm_event);
		}

		for (i = 0; i < ((log_count / LOGS_PER_BUF) + 1); i++)
			kfree(bootprof[i]);

		mutex_unlock(&bootprof_lock);
	}
	remove_proc_entry("bootprof", NULL);
	pr_info("bootprof module exit.\n");
}
module_init(bootprof_init);
module_exit(bootprof_exit);

MODULE_DESCRIPTION("MEDIATEK BOOT TIME PROFILING");
MODULE_LICENSE("GPL v2");
#else /*Build-in*/
static int __init init_boot_prof(void)
{
	struct proc_dir_entry *pe;

	pe = proc_create("bootprof", 0664, NULL, &mt_bootprof_fops);
	if (!pe)
		return -ENOMEM;
	return 0;
}

static int __init init_bootprof_buf(void)
{
	memset(bootprof, 0, sizeof(struct log_t *) * BUF_COUNT);
	bootprof[0] = kcalloc(LOGS_PER_BUF, sizeof(struct log_t),
			      GFP_ATOMIC | __GFP_NORETRY | __GFP_NOWARN);
	if (!bootprof[0])
		goto fail;

	bootprof_bootloader();
	mt_bootprof_switch(1);
fail:
	return 0;
}

early_initcall(init_bootprof_buf);
device_initcall(init_boot_prof);
#endif /*MODULE END*/
