//
// Check QP_TIME_HEADER_5_6
//
#include "test.h"

#define QP_TIME_HEADER QP_TIME_HEADER_5_6
#define QP_PRINT(str, ...) buffer_print(&pb, str, ##__VA_ARGS__)
#include <qp.h>

START_TEST(test_time_header_5_6)
{
    struct print_buffer pb;
    print_buffer_init(&pb);
    QP_PRINT_LOC("hello\n");
    //fprintf(stderr, "OUT: %s", pb.buf);
    ck_assert_int_eq(pb.buf[0], '[');
    ck_assert_int_eq(pb.buf[13], ']');
    ck_assert_int_eq(pb.buf[14], ' ');
    ck_assert_int_eq(pb.buf[QP_TIME_HEADER_LEN - 1], ' ');
    ck_assert(strstr(pb.buf, "test_time_header_5_6"));
    ck_assert(strstr(pb.buf, "hello\n"));
}
END_TEST

Suite *suite_create_time_header_5_6(void)
{
    Suite *s = suite_create("time_header_5_6");
    TCase *tc = tcase_create("time_header_5_6");
    tcase_add_test(tc, test_time_header_5_6);
    suite_add_tcase(s, tc);

    return s;
}
