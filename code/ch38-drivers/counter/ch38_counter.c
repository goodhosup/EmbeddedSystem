// SPDX-License-Identifier: GPL-2.0
/*
 * 38장: 문자 장치 — /dev/ch38counter
 *   cat /dev/ch38counter       → "count=N" (읽을 때마다 1 증가)
 *   echo 100 > /dev/ch38counter → 값을 100으로
 * miscdevice로 등록하면 주 번호 할당, 장치 파일 생성(udev)을 커널이 대신 해 준다.
 */
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

static DEFINE_MUTEX(lock);          /* 여러 프로세스가 동시에 열 수 있다 (27장) */
static unsigned long count;

static ssize_t counter_read(struct file *f, char __user *buf, size_t len, loff_t *pos)
{
	char tmp[32];
	int n;

	if (*pos > 0)
		return 0;                    /* 한 번 읽으면 파일 끝 (cat이 멈추게) */
	mutex_lock(&lock);
	n = scnprintf(tmp, sizeof(tmp), "count=%lu\n", count++);
	mutex_unlock(&lock);
	if (len < n)
		return -EINVAL;
	if (copy_to_user(buf, tmp, n))   /* 사용자 메모리는 직접 쓰지 않는다 */
		return -EFAULT;
	*pos += n;
	return n;
}

static ssize_t counter_write(struct file *f, const char __user *buf, size_t len, loff_t *pos)
{
	unsigned long v;
	int ret = kstrtoul_from_user(buf, len, 10, &v);   /* 바깥에서 온 데이터: 검사하며 변환 */

	if (ret)
		return ret;
	mutex_lock(&lock);
	count = v;
	mutex_unlock(&lock);
	return len;
}

static const struct file_operations counter_fops = {
	.owner = THIS_MODULE,
	.read  = counter_read,
	.write = counter_write,
};

static struct miscdevice counter_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "ch38counter",
	.fops  = &counter_fops,
	.mode  = 0666,
};

module_misc_device(counter_dev);     /* init/exit에서 등록·해제를 해 주는 매크로 */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Embedded systems book, chapter 38: character device");
