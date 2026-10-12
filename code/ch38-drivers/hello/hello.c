// SPDX-License-Identifier: GPL-2.0
/*
 * 38장: 가장 작은 커널 모듈
 *   make && sudo insmod hello.ko && dmesg | tail -2 && sudo rmmod hello
 */
#include <linux/init.h>
#include <linux/module.h>

static int __init hello_init(void)
{
	pr_info("ch38 hello: loaded into the kernel\n");   /* printk: dmesg로 본다 */
	return 0;                                           /* 0이 아니면 모듈이 올라가지 않는다 */
}

static void __exit hello_exit(void)
{
	pr_info("ch38 hello: unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Embedded systems book, chapter 38: hello module");
