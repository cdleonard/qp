#include <check.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define QP_PRINT(str, ...) buffer_print(&pb, str, ##__VA_ARGS__)
#include <qp.h>

#define PRINT_BUFFER_SIZE 1234
struct print_buffer
{
    char buf[PRINT_BUFFER_SIZE];
    char *curptr;
};

void print_buffer_init(struct print_buffer *pb)
{
    pb->buf[0] = 0;
    pb->curptr = pb->buf;
}

int buffer_print(struct print_buffer *pb, const char *fmt, ...)
{
    int ret;
    va_list args;

    va_start(args, fmt);
    ret = vsnprintf(pb->curptr, PRINT_BUFFER_SIZE - (pb->curptr - pb->buf), fmt, args);
    va_end(args);

    pb->curptr += ret;

    return ret;
}

START_TEST(test_dump_mac)
{
    uint8_t mac[6] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    struct print_buffer pb;

    print_buffer_init(&pb);
    QP_DUMP_MAC(mac);
    ck_assert(strstr(pb.buf, "02:03:04:05:06:07"));
}
END_TEST

START_TEST(test_run_system)
{
    struct print_buffer pb;

    print_buffer_init(&pb);
    QP_RUN_SYSTEM("echo $(( 123 + 456 ))");
    fprintf(stderr, "output:\n%s", pb.buf);
    ck_assert(strstr(pb.buf, "RUN: echo $(( 123 + 456 ))\n"));
    ck_assert(strstr(pb.buf, "579\n"));
}
END_TEST

START_TEST(test_run_system_print_exit_status)
{
    struct print_buffer pb;

    print_buffer_init(&pb);
    QP_RUN_SYSTEM("exit 3");
    ck_assert(strstr(pb.buf, "exit status 3\n"));
}
END_TEST

START_TEST(test_run_system_print_exit_signal)
{
    struct print_buffer pb;

    print_buffer_init(&pb);
    QP_RUN_SYSTEM("kill -6 $$");
    ck_assert(strstr(pb.buf, "exit signal 6\n"));
}
END_TEST

Suite *main_suite(void)
{
    Suite *s = suite_create("main");
    TCase *tc = tcase_create("main");

    tcase_add_test(tc, test_dump_mac);
    tcase_add_test(tc, test_run_system);
    tcase_add_test(tc, test_run_system_print_exit_status);
    tcase_add_test(tc, test_run_system_print_exit_signal);
    suite_add_tcase(s, tc);

    return s;
}

int main(void)
{
    int ntests_failed;
    Suite *s;
    SRunner *sr;

    s = main_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    ntests_failed = srunner_ntests_failed(sr);

    return !!ntests_failed;
}
