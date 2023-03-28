/*
 * Test Helpers and forward declarations
 */
#ifndef QP_TEST_H_INCLUDE
#define QP_TEST_H_INCLUDE

#include <check.h>

#define PRINT_BUFFER_SIZE 1234
struct print_buffer
{
    char buf[PRINT_BUFFER_SIZE];
    char *curptr;
};

void print_buffer_init(struct print_buffer *pb);
int buffer_print(struct print_buffer *pb, const char *fmt, ...);

#endif // QP_TEST_H_INCLUDE
