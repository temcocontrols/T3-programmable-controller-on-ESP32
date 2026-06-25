#ifndef MOCK_CTEST_H
#define MOCK_CTEST_H

#include <stdio.h>
#include <stdbool.h>

typedef void Test;
typedef void (*ct_test_fn)(Test*);

#define ct_test(pTest, condition) (void)(condition)
#define ct_create(name, arg) (void*)1
#define ct_addTestFunction(pTest, fn) true
#define ct_setStream(pTest, stream) (void)0
#define ct_run(pTest) (void)0
#define ct_report(pTest) 0
#define ct_destroy(pTest) (void)0

#endif // MOCK_CTEST_H
