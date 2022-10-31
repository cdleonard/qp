#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include "qp.h"

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
