
/* LGE_CHANGE_S
 *
 * do read/mmap profiling during booting
 * in order to use the data as readahead args
 *
 * matia.kim@lge.com 20130403
 */
#include "mount.h"
#include "ext4/ext4.h"
#include "sreadahead_prof.h"

static struct sreadahead_prof prof_buf;

#ifdef CONFIG_VM_EVENT_COUNTERS
static unsigned long vm_chk_jiffies;
#endif

static DECLARE_WAIT_QUEUE_HEAD(prof_state_wait);

#ifdef CONFIG_READAHEAD_MMAP_SIZE_ENABLE
unsigned long sreadahead_ra_pages = CONFIG_READAHEAD_MMAP_PAGE_CNT;
#else
unsigned long sreadahead_ra_pages = VM_MIN_READAHEAD * 2;
#endif

static void prof_buf_free_work(struct work_struct *data)
{
	mutex_lock(&prof_buf.ulock);
	if (prof_buf.state == PROF_DONE) {
		mutex_unlock(&prof_buf.ulock);
		return;
	}

	prof_buf.state = PROF_NOT;
	wake_up_interruptible(&prof_state_wait);
	vfree(prof_buf.data);
	prof_buf.data = NULL;
	_DBG("mem of prof_buf is freed by vfree()");
	mutex_unlock(&prof_buf.ulock);
}

static void prof_timer_handler(struct timer_list *unused)
{
	_DBG("profiling state - %d\n", prof_buf.state);
	schedule_work(&prof_buf.free_work);
}

#ifdef CONFIG_DEBUG_FS
static ssize_t sreadahead_dbgfs_read(
		struct file *file,
		char __user *buff,
		size_t buff_count,
		loff_t *ppos)
#else
static ssize_t sreadahead_dbgfs_read(
		struct file *filp,
		struct kobject *kobj,
		struct bin_attribute *attr,
		char *buff,
		loff_t off,
		size_t count)
#endif
{
	struct sreadahead_profdata data;

	mutex_lock(&prof_buf.ulock);
	if (prof_buf.data == NULL) {
		mutex_unlock(&prof_buf.ulock);
		return 0;
	}

	if (prof_buf.read_cnt >= prof_buf.file_cnt) {
		vfree(prof_buf.data);
		prof_buf.data = NULL;
		mutex_unlock(&prof_buf.ulock);
		return 0;
	}

	data = prof_buf.data[prof_buf.read_cnt++];
	mutex_unlock(&prof_buf.ulock);

	_DBG("%s:%lld:%lld#%s -- read_cnt:%d",
		data.name, data.pos[0],
		data.len, data.procname, prof_buf.read_cnt);

#ifdef CONFIG_DEBUG_FS
	if (copy_to_user(buff, &data, sizeof(struct sreadahead_profdata)))
		return 0;

	(*ppos) = 0;
	return 1;
#else
	memcpy(buff, &data, sizeof(struct sreadahead_profdata));
	return sizeof(struct sreadahead_profdata);
#endif
}

#ifdef CONFIG_DEBUG_FS
static ssize_t sreadaheadflag_dbgfs_read(
		struct file *file,
		char __user *buff,
		size_t buff_count,
		loff_t *ppos)
#else
static ssize_t sreadaheadflag_dbgfs_read(
		struct file *filp,
		struct kobject *kobj,
		struct bin_attribute *attr,
		char *buff,
		loff_t off,
		size_t count)
#endif
{
	int ret = 0;

	DECLARE_WAITQUEUE(wait, current);
	add_wait_queue(&prof_state_wait, &wait);
	__set_current_state(TASK_INTERRUPTIBLE);

	do {
		if (prof_buf.state == PROF_DONE || prof_buf.state == PROF_NOT) {
			break;
		}
		schedule();
	} while(1);

#ifdef CONFIG_DEBUG_FS
	if (copy_to_user(buff, &prof_buf.state, sizeof(int))) {
		ret = 0;
	} else {
		ret = sizeof(int);
	}

	(*ppos) = 0;
#else
	memcpy(buff, &prof_buf.state, sizeof(int));
	ret = sizeof(int);
#endif
	__set_current_state(TASK_RUNNING);
	remove_wait_queue(&prof_state_wait, &wait);

	return ret;
}

#ifdef CONFIG_DEBUG_FS
static ssize_t sreadaheadflag_dbgfs_write(
		struct file *file,
		const char __user *buff,
		size_t count,
		loff_t *ppos)
#else
static ssize_t sreadaheadflag_dbgfs_write(
		struct file *filp,
		struct kobject *kobj,
		struct bin_attribute *attr,
		char *buff,
		loff_t off,
		size_t count)
#endif
{
	int state;

#ifdef CONFIG_DEBUG_FS
	if (copy_from_user(&state, buff, sizeof(int)))
		return 0;
#else
	memcpy(&state, buff, count);
#endif
	if (state == PROF_INIT) {
		mutex_lock(&prof_buf.ulock);
		if (prof_buf.state != PROF_NOT) {
			mutex_unlock(&prof_buf.ulock);
			return 0;
		}
		_DBG("PROF_INT");
		prof_buf.state = state;

		_DBG("add timer");
		prof_buf.timer.expires = get_jiffies_64() + (PROF_TIMEOUT * HZ);
		add_timer(&prof_buf.timer);
		mutex_unlock(&prof_buf.ulock);
	} else if (state == PROF_DONE) {
		mutex_lock(&prof_buf.ulock);
		if (prof_buf.state != PROF_RUN) {
			mutex_unlock(&prof_buf.ulock);
			return 0;
		}
		_DBG("PROF_DONE by user daemon(boot_completed)");
		prof_buf.state = state;
		wake_up_interruptible(&prof_state_wait);
		_DBG("del timer");
		del_timer(&prof_buf.timer);
		mutex_unlock(&prof_buf.ulock);
	}

#ifdef CONFIG_DEBUG_FS
	(*ppos) = 0;
#endif
	return sizeof(int);
}

#ifdef CONFIG_DEBUG_FS
static const struct file_operations sreadaheadflag_dbgfs_fops = {
	.read = sreadaheadflag_dbgfs_read,
	.write = sreadaheadflag_dbgfs_write,
};

static const struct file_operations sreadahead_dbgfs_fops = {
	.read = sreadahead_dbgfs_read,
};
#else
static struct kobject *sreadahead_kobj;

static struct bin_attribute sra_data_attr_data =
	__BIN_ATTR(profilingdata, S_IRUSR, sreadahead_dbgfs_read, NULL, 0);

static struct bin_attribute sra_flag_attr_data =
	__BIN_ATTR(profilingflag, (S_IRUSR | S_IWUSR), sreadaheadflag_dbgfs_read, sreadaheadflag_dbgfs_write, 0);

static struct bin_attribute *sra_bin_attrs[] = {
	&sra_data_attr_data,
	&sra_flag_attr_data,
	NULL,
};

static struct attribute_group sreadahead_group = {
	.bin_attrs = sra_bin_attrs,
};
#endif


static int __init sreadahead_init(void)
{
#ifdef CONFIG_DEBUG_FS
	struct dentry *dbgfs_dir;
#endif
	int ret = 0;

	/* state init */
	prof_buf.state = PROF_NOT;

	/* lock init */
	mutex_init(&prof_buf.ulock);

	/* timer init */
	timer_setup(&prof_buf.timer, prof_timer_handler, 0);

	/* work struct init */
	INIT_WORK(&prof_buf.free_work, prof_buf_free_work);

#ifdef CONFIG_DEBUG_FS
	/* debugfs init for sreadahead */
	dbgfs_dir = debugfs_create_dir("sreadahead", NULL);
	if (!dbgfs_dir)
		return -ENOENT;
	debugfs_create_file("profilingdata",
			0444, dbgfs_dir, NULL,
			&sreadahead_dbgfs_fops);
	debugfs_create_file("profilingflag",
			0644, dbgfs_dir, NULL,
			&sreadaheadflag_dbgfs_fops);
#else
	sreadahead_kobj = kobject_create_and_add("sreadahead", NULL);

	if(!sreadahead_kobj) {
		pr_err("%s: kobject_create_and_add() failed\n", __func__);
		return -1;
	}

	ret = sysfs_create_group(sreadahead_kobj, &sreadahead_group);
	if (ret)
		pr_err("%s: sysfs_create_group() failed. ret=%d\n", __func__, ret);
#endif
	return ret;
}

device_initcall(sreadahead_init);

static int get_absolute_path(unsigned char *buf, int buflen, struct file *filp)
{
	unsigned char tmpstr[buflen];
	struct dentry *tmpdentry = 0;
	struct mount *tmpmnt;
	struct mount *tmpoldmnt;
	tmpmnt = real_mount(filp->f_path.mnt);

	tmpdentry = filp->f_path.dentry->d_parent;
	do {
		tmpoldmnt = tmpmnt;
		while (!IS_ROOT(tmpdentry)) {
			strlcpy(tmpstr, buf, buflen);
			/* byungchul.park@lge.com */
			/* make codes robust */
			strlcpy(buf, tmpdentry->d_name.name, buflen);
			buf[buflen - 1] = '\0';
			if (strlen(buf) + strlen("/") > buflen - 1)
				return -ENOMEM;
			else
				strlcat(buf, "/", buflen);

			if (strlen(buf) + strlen(tmpstr) > (buflen - 1))
				return -ENOMEM;
			else
				strlcat(buf, tmpstr, buflen);

			tmpdentry = tmpdentry->d_parent;
		}
		tmpdentry = tmpmnt->mnt_mountpoint;
		tmpmnt = tmpmnt->mnt_parent;
	} while (tmpmnt != tmpoldmnt);

	/* if A/B update, should remove the mount point of first_stage_ramdisk */ 
	if (strncmp(buf, "first_stage_ramdisk/", 20) == 0) {
		strlcpy(tmpstr, (const char *)&buf[20], buflen);
	}
	else
		strlcpy(tmpstr, buf, buflen);

	strlcpy(buf, "/", 2);
	/* byungchul.park@lge.com */
	/* make codes robust */
	if (strlen(buf) + strlen(tmpstr) > (buflen - 1))
		return -ENOMEM;
	strlcat(buf, tmpstr, buflen);

	return 0;
}

static int is_prof_partition(unsigned char *fn) {
	if (strncmp((const char*)fn, "/system/", 8) == 0 ||
        strncmp((const char*)fn, "/product/", 9) == 0 ||
        strncmp((const char*)fn, "/system_ext/", 12) == 0 ||
        strncmp((const char*)fn, "/apex/", 6) == 0 ||
        strncmp((const char*)fn, "/vendor/", 8) == 0)
		return 1;
	else
		return 0;
}

#ifdef CONFIG_VM_EVENT_COUNTERS
static int check_vm_pgsteal_events(void)
{
	int cpu;
	int i;

	for_each_online_cpu(cpu) {
		struct vm_event_state *this = &per_cpu(vm_event_states, cpu);

		for (i = PGSTEAL_DIRECT; i > PGREFILL; i--) {
			if (this->event[i] > 0) {
				return 1;
			}
		}
	}

	return 0;
}

static int check_vm_events(void)
{
	int ret;

	get_online_cpus();
	ret = check_vm_pgsteal_events();
	put_online_cpus();

	return ret;
}
#endif

static int sreadahead_prof_RUN(struct file *filp, size_t len, loff_t pos)
{
	int i;
	int buflen;
	struct sreadahead_profdata data;

	memset(&data, 0x00, sizeof(struct sreadahead_profdata));
	data.len = (long long)len;
	data.pos[0] = pos;
	data.pos[1] = 0;
	data.procname[0] = '\0';
	get_task_comm(data.procname, current);

	buflen = FILE_PATH_LEN;
	if (get_absolute_path(data.name, buflen, filp) < 0)	{
		_DBG("no entry");
		return -ENOENT;
	}
	strlcat(data.name, filp->f_path.dentry->d_name.name, buflen);

	if (is_prof_partition(data.name) == 0)
	{
		_DBG("no system or product - %s", data.name);
		return 0;
	}

	mutex_lock(&prof_buf.ulock);

	/* vfree called or profiling is already done */
	if (prof_buf.data == NULL || prof_buf.state != PROF_RUN) {
		mutex_unlock(&prof_buf.ulock);

		return -EADDRNOTAVAIL;
	}

	for (i = 0; i < prof_buf.file_cnt; ++i) {
		if (strncmp(prof_buf.data[i].name, data.name, FILE_PATH_LEN) == 0)
			break;
	}
	/* add a new entry */
	if (i == prof_buf.file_cnt && i < PROF_BUF_SIZE) {
		strlcpy(prof_buf.data[i].procname, data.procname, PROC_NAME_LEN);
		prof_buf.data[i].procname[PROC_NAME_LEN - 1] = '\0';
		strlcpy(prof_buf.data[i].name, data.name, FILE_PATH_LEN);
		prof_buf.data[i].name[FILE_PATH_LEN - 1] = '\0';
		prof_buf.data[i].pos[0] = prof_buf.data[i].pos[1]
			= ALIGNPAGECACHE(data.pos[0]);
		prof_buf.data[i].pos[1] +=
			E_ALIGNPAGECACHE((long long)data.len);
		prof_buf.data[i].len = prof_buf.data[i].pos[1]
			- prof_buf.data[i].pos[0];
		prof_buf.file_cnt++;

		_DBG("New Entry - %s:%lld:%lld#%s -- cnt:%d",
				prof_buf.data[i].name,
				prof_buf.data[i].pos[0],
				prof_buf.data[i].len,
				prof_buf.data[i].procname,
				prof_buf.file_cnt);
	}

	if (prof_buf.file_cnt >= PROF_BUF_SIZE) {
		_DBG("PROF_DONE by kernel(file_cnt) & del timer");
		prof_buf.state = PROF_DONE;
		wake_up_interruptible(&prof_state_wait);
		del_timer(&prof_buf.timer);
	}

#ifdef CONFIG_VM_EVENT_COUNTERS
	if (time_after(jiffies, vm_chk_jiffies + VM_CHK_INTERVAL)) {
		vm_chk_jiffies = jiffies;
		if (check_vm_events()) {
			_DBG("PROF_DONE by pgsteal");
			prof_buf.state = PROF_DONE;
			wake_up_interruptible(&prof_state_wait);
			del_timer(&prof_buf.timer);
		}
	}
#endif

	mutex_unlock(&prof_buf.ulock);

	return 0;
}

static int sreadahead_profdata_init(void)
{
	mutex_lock(&prof_buf.ulock);
	if (prof_buf.state != PROF_INIT) {
		mutex_unlock(&prof_buf.ulock);
		return 0;
	}

	prof_buf.data = (struct sreadahead_profdata *)
		vmalloc(sizeof(struct sreadahead_profdata) * PROF_BUF_SIZE);

	if (prof_buf.data == NULL)
		return -EADDRNOTAVAIL;

	memset(prof_buf.data, 0x00,
		sizeof(struct sreadahead_profdata) * PROF_BUF_SIZE);
	prof_buf.state = PROF_RUN;

#ifdef CONFIG_VM_EVENT_COUNTERS
	vm_chk_jiffies = jiffies;
#endif
	mutex_unlock(&prof_buf.ulock);

	return 0;
}

int sreadahead_prof(struct file *filp, size_t len, loff_t pos)
{
	if (prof_buf.state == PROF_NOT || prof_buf.state == PROF_DONE) {
#ifdef CONFIG_READAHEAD_MMAP_SIZE_ENABLE
		sreadahead_ra_pages = CONFIG_READAHEAD_MMAP_PAGE_CNT;
#else
		sreadahead_ra_pages = VM_MIN_READAHEAD * 2;
#endif
		return 0;
	} else
		sreadahead_ra_pages = 4;

	if (prof_buf.state == PROF_INIT)
		sreadahead_profdata_init();
	if (prof_buf.state == PROF_RUN) {
		if (filp->f_op == &ext4_file_operations)
			sreadahead_prof_RUN(filp, len, pos);
	}
	return 0;
}
/* LGE_CHANGE_E */
