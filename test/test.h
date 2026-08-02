/*
 * CBoot 测试框架
 * 提供断言宏、测试用例注册和统计
 */
#ifndef CBOOT_TEST_H
#define CBOOT_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 测试统计 */
static int g_test_total = 0;
static int g_test_pass = 0;
static int g_test_fail = 0;
static int g_test_assert_total = 0;
static int g_test_assert_pass = 0;

/* 当前测试用例名 */
static const char *g_current_test = "";

/* 测试用例开始 */
#define TEST_BEGIN(name) \
    do { \
        g_current_test = name; \
        g_test_total++; \
        printf("  [RUN] %s\n", name); \
    } while (0)

/* 测试用例结束 */
#define TEST_END() \
    do { \
        g_test_pass++; \
        printf("  [ OK] %s\n", g_current_test); \
    } while (0)

/* 断言宏 */
#define ASSERT_TRUE(cond, msg) \
    do { \
        g_test_assert_total++; \
        if (!(cond)) { \
            printf("    [FAIL] %s:%d: %s (条件: %s)\n", \
                   __FILE__, __LINE__, msg, #cond); \
            g_test_assert_pass++; /* 计入已检查 */ \
            return; \
        } \
        g_test_assert_pass++; \
    } while (0)

#define ASSERT_FALSE(cond, msg) ASSERT_TRUE(!(cond), msg)

#define ASSERT_EQ_INT(actual, expected, msg) \
    do { \
        g_test_assert_total++; \
        int _a = (int)(actual); \
        int _e = (int)(expected); \
        if (_a != _e) { \
            printf("    [FAIL] %s:%d: %s (期望 %d, 实际 %d)\n", \
                   __FILE__, __LINE__, msg, _e, _a); \
            g_test_assert_pass++; \
            return; \
        } \
        g_test_assert_pass++; \
    } while (0)

#define ASSERT_EQ_STR(actual, expected, msg) \
    do { \
        g_test_assert_total++; \
        const char *_a = (actual); \
        const char *_e = (expected); \
        if (_a == NULL || _e == NULL || strcmp(_a, _e) != 0) { \
            printf("    [FAIL] %s:%d: %s (期望 '%s', 实际 '%s')\n", \
                   __FILE__, __LINE__, msg, \
                   _e ? _e : "(null)", _a ? _a : "(null)"); \
            g_test_assert_pass++; \
            return; \
        } \
        g_test_assert_pass++; \
    } while (0)

#define ASSERT_NOT_NULL(ptr, msg) \
    do { \
        g_test_assert_total++; \
        if ((ptr) == NULL) { \
            printf("    [FAIL] %s:%d: %s (指针为 NULL)\n", \
                   __FILE__, __LINE__, msg); \
            g_test_assert_pass++; \
            return; \
        } \
        g_test_assert_pass++; \
    } while (0)

#define ASSERT_NULL(ptr, msg) \
    do { \
        g_test_assert_total++; \
        if ((ptr) != NULL) { \
            printf("    [FAIL] %s:%d: %s (指针非 NULL: %p)\n", \
                   __FILE__, __LINE__, msg, (void*)(ptr)); \
            g_test_assert_pass++; \
            return; \
        } \
        g_test_assert_pass++; \
    } while (0)

/* 测试套件声明 */
#define TEST_SUITE(name) \
    void name(void); \
    void name(void)

/* 运行测试套件 */
#define RUN_SUITE(suite) \
    do { \
        printf("\n=== 运行 %s ===\n", #suite); \
        suite(); \
    } while (0)

/* 打印总结 */
static inline void test_summary(void) {
    printf("\n");
    printf("========================================\n");
    printf("  测试总结\n");
    printf("========================================\n");
    printf("  测试用例: %d (通过 %d, 失败 %d)\n",
           g_test_total, g_test_pass, g_test_fail);
    printf("  断言数:   %d (通过 %d)\n",
           g_test_assert_total, g_test_assert_pass);
    if (g_test_assert_total > 0) {
        printf("  断言通过率: %.1f%%\n",
               (double)g_test_assert_pass / g_test_assert_total * 100.0);
    }
    printf("========================================\n");
}

/* 标记失败 */
#define TEST_FAIL(msg) \
    do { \
        g_test_fail++; \
        printf("  [FAIL] %s: %s\n", g_current_test, msg); \
    } while (0)

#endif /* CBOOT_TEST_H */
