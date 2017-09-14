/*
 * Copyright (C) 2017 yoh2
 *
 * yes.ko is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public Licence as published by the
 * Free Software Foundation, version 2 of the License, or (at your option)
 * any later version.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/file.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");

#define YES_DEV_NAME         "yes"
#define MAX_BUF              PAGE_SIZE
#define DEFAULT_MESSAGE      "y\n"
#define DEFAULT_MESSAGE_SIZE (sizeof(DEFAULT_MESSAGE) - 1)

static DEFINE_MUTEX(yes_mutex);

struct yes_info {
	size_t msg_buf_size;
	char *msg_buf;
};

static struct yes_info sg_yes_info;

static void yes_release_msg_buf(void)
{
	kfree(sg_yes_info.msg_buf);
	sg_yes_info.msg_buf = NULL;
	sg_yes_info.msg_buf_size = 0;
}

static int yes_reset_msg_buf(const char *msg, size_t n)
{
	size_t nr_repeats = (n >= MAX_BUF) ? 1 : (MAX_BUF / n);
	size_t buf_size = nr_repeats * n;
	size_t i;

	yes_release_msg_buf();

	sg_yes_info.msg_buf = kzalloc(buf_size, GFP_KERNEL);
	if (sg_yes_info.msg_buf == NULL) {
		return -ENOMEM;
	}
	sg_yes_info.msg_buf_size = buf_size;
	for (i = 0; i < buf_size; i += n) {
		memcpy(sg_yes_info.msg_buf + i, msg, n);
	}
	return 0;
}

static ssize_t yes_copy_to_user(char __user *dst, const struct yes_info *info, size_t count, loff_t offset)
{
	off_t first_buf_offset = offset % info->msg_buf_size;
	ssize_t total_read = 0;
	if (first_buf_offset > 0) {
		size_t n = (first_buf_offset + count > info->msg_buf_size)
			? info->msg_buf_size - first_buf_offset
			: count;
		if(copy_to_user(dst, info->msg_buf + first_buf_offset, n)) {
			return -EFAULT;
		}
		total_read += n;
	}

	while (total_read + info->msg_buf_size < count) {
		if(copy_to_user(dst + total_read, info->msg_buf, info->msg_buf_size))
		{
			return -EFAULT;
		}
		total_read += info->msg_buf_size;
	}

	if (total_read < count) {
		if (copy_to_user(dst + total_read, info->msg_buf, count - total_read)) {
			return -EFAULT;
		}
	}

	return count;
}

static ssize_t yes_read(
	struct file *file, char __user *buf,
	size_t count, loff_t *ppos)
{
	ssize_t read_size;

	mutex_lock(&yes_mutex);
	read_size = yes_copy_to_user(buf, &sg_yes_info, count, *ppos);
	mutex_unlock(&yes_mutex);
	*ppos += read_size;
	return read_size;
}

static ssize_t yes_write(
	struct file *file, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int err;
	char *kbuf;

	if (count == 0) {
		return 0;
	}

	kbuf = kzalloc(count + 1, GFP_KERNEL); /* +1 for trailing '\n' */
	if (kbuf == NULL) {
		return -ENOMEM;
	}
	if (copy_from_user(kbuf, buf, count)) {
		kfree(kbuf);
		return -EFAULT;
	}
	if (kbuf[count - 1] != '\n') {
		kbuf[count++] = '\n';
	}
	mutex_lock(&yes_mutex);
	err = yes_reset_msg_buf(kbuf, count);
	mutex_unlock(&yes_mutex);
	kfree(kbuf);

	if (err) {
		return err;
	}

	return count;
}


static const struct file_operations yes_fops = {
	.owner   = THIS_MODULE,
	.llseek  = no_seek_end_llseek,
	.read    = yes_read,
	.write   = yes_write,
};

static struct miscdevice yes_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = YES_DEV_NAME,
	.fops  = &yes_fops,
};

static int __init yes_init(void)
{
	int err = yes_reset_msg_buf(DEFAULT_MESSAGE, DEFAULT_MESSAGE_SIZE);
	if (err != 0) {
		goto err_msg_buf;
	}

	err = misc_register(&yes_dev);
	if (err != 0) {
		printk(KERN_ALERT "yes: failed to register an yes device. err = %d\n", err);
		goto err_register;
	}

	return 0;

err_register:
	yes_release_msg_buf();
err_msg_buf:
	return err;
}

static void __exit yes_cleanup(void)
{
	misc_deregister(&yes_dev);
	yes_release_msg_buf();
}

module_init(yes_init);
module_exit(yes_cleanup);
