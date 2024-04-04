#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include "qp.h"

__maybe_unused static void qp_dump_skb_compile_test(void)
{
    struct sk_buff *skb = NULL;
    QP_DUMP_SKB(skb, true);
}

static int qp_kmod_test_init(void)
{
    QP_PRINT_LOC("hello\n");
    return 0;
}

static void qp_kmod_test_exit(void)
{
}

module_init(qp_kmod_test_init)
module_exit(qp_kmod_test_exit)
MODULE_LICENSE("MIT");
