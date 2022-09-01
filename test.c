#include <check.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

START_TEST(test_dump_ipv4)
{
    struct print_buffer pb;
    uint8_t ipv4[4] = {0x02, 0x03, 0x04, 0x05};

    print_buffer_init(&pb);
    QP_DUMP_IPV4_ADDR(ipv4);
    ck_assert(strstr(pb.buf, "2.3.4.5\n"));
}
END_TEST

START_TEST(test_dump_ipv6)
{
    struct print_buffer pb;
    uint8_t ipv6[16];

    inet_pton(AF_INET6, "2000::1234", ipv6);
    print_buffer_init(&pb);
    QP_DUMP_IPV6_ADDR(ipv6);
    ck_assert(strstr(pb.buf, "2000:0000:0000:0000:0000:0000:0000:1234\n"));
}
END_TEST

START_TEST(test_dump_var)
{
    struct print_buffer pb;
    int val_int = 213451234;
    uint32_t val_u32 = 4275878552;
    unsigned char val_uchar = 254;
    signed char val_schar = -126;
    char val_char = 126;
    bool val_bool_true = true;
    bool val_bool_false = false;

    print_buffer_init(&pb);
    QP_DUMP_VAR(val_int);
    ck_assert(strstr(pb.buf, "val_int=213451234\n"));
    QP_DUMP_VAR(val_u32);
    ck_assert(strstr(pb.buf, "val_u32=4275878552\n"));
    QP_DUMP_VAR(val_bool_true);
    QP_DUMP_VAR(val_bool_false);
    ck_assert(strstr(pb.buf, "val_bool_true=true\n"));
    ck_assert(strstr(pb.buf, "val_bool_false=false\n"));
    QP_DUMP_VAR(val_char);
    QP_DUMP_VAR(val_schar);
    QP_DUMP_VAR(val_uchar);
    ck_assert(strstr(pb.buf, "val_char=126\n"));
    ck_assert(strstr(pb.buf, "val_schar=-126\n"));
    ck_assert(strstr(pb.buf, "val_uchar=254\n"));
}
END_TEST

START_TEST(test_dump_var_ptr)
{
    struct print_buffer pb;
    char *x = (char*)0x12345678;

    print_buffer_init(&pb);
    ck_assert(QP_ARG_IS_POINTER((int*)0));
    ck_assert(QP_ARG_IS_POINTER((void*)0));
    ck_assert(QP_ARG_IS_POINTER((char*)0));
    ck_assert(!QP_ARG_IS_POINTER(0));
    ck_assert(!QP_ARG_IS_POINTER(0.0f));
    QP_DUMP_VAR(x);
    ck_assert(strstr(pb.buf, "x=0x12345678"));
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
    tcase_add_test(tc, test_dump_ipv4);
    tcase_add_test(tc, test_dump_ipv6);
    tcase_add_test(tc, test_dump_var);
    tcase_add_test(tc, test_dump_var_ptr);
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
