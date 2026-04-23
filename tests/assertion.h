/** @file assertion.h
 * @brief Simple Unit Testing Framework
 *
 * A lightweight unit testing framework with test counters and assertions.
 * Tests return 1 on success, 0 on failure.
 *
 * @author Sadiq Idris (@sadichel)
 * @license MIT
 * @version 1.0 — January 2026
 *
 * @section usage Usage Example
 * @code
 *   // Define test functions (return 1 on success, 0 on failure)
 *   static int test_addition(void) {
 *       int result = 2 + 2;
 *       ASSERT_EQUAL(result, 4, "addition failed");
 *       return 1;
 *   }
 *
 *   static int test_strings(void) {
 *       ASSERT_STR_EQUAL("hello", "hello", "strings not equal");
 *       return 1;
 *   }
 *
 *   // Run tests
 *   int main(void) {
 *       TEST(test_addition);
 *       TEST(test_strings);
 *       TEST_SUMMARY();
 *       return tests_passed == tests_run ? 0 : 1;
 *   }
 * @endcode
 */

#ifndef ASSERTION_PDR_H
#define ASSERTION_PDR_H

#include <stdio.h>
#include <string.h>

/**
 * @defgroup test_counters Test Counters
 * @brief Global test counters shared across test files
 * @{
 */

/** @brief Number of tests run */
static int tests_run = 0;

/** @brief Number of tests passed */
static int tests_passed = 0;

/** @} */

/**
 * @defgroup test_runner Test Runner Macros
 * @brief Macros for running and reporting tests
 * @{
 */

/**
 * @brief Run a single test function
 *
 * Executes the test function, tracks pass/fail counts, and prints result.
 *
 * @param func Test function name (returns 1 on success, 0 on failure)
 *
 * @note The test function should return 1 for PASS, 0 for FAIL.
 */
#define TEST(func)                                                          \
    do {                                                                    \
        printf("Running %s... ", #func);                                    \
        tests_run++;                                                        \
        if (func()) {                                                       \
            printf("PASS\n");                                               \
            tests_passed++;                                                 \
        } else {                                                            \
            printf("FAIL\n");                                               \
        }                                                                   \
    } while (0)

/**
 * @brief Print test summary
 *
 * Outputs total tests run, passed, and failed.
 *
 * @par Example Output
 * @code
 * ------------------------------------------------------------
 * Tests run: 10, Passed: 9, Failed: 1
 * ------------------------------------------------------------
 * @endcode
 */
#define TEST_SUMMARY()                                                      \
    do {                                                                    \
        printf("\n------------------------------------------------------------\n"); \
        printf("Tests run: %d, Passed: %d, Failed: %d\n",                   \
               tests_run, tests_passed, tests_run - tests_passed);          \
        printf("------------------------------------------------------------\n"); \
    } while (0)

/** @} */

/**
 * @defgroup assertions Assertions
 * @brief Assertion macros for test validation
 *
 * Each assertion prints a formatted error message and returns 0 from the
 * test function if the condition fails.
 * @{
 */

/**
 * @brief Assert that a condition is true
 *
 * @param condition Expression to evaluate
 * @param fmt       Printf-style format string
 * @param ...       Format arguments
 */
#define ASSERT_TRUE(condition, fmt, ...)                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            printf("FAIL: " fmt "\n", ##__VA_ARGS__);                       \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that a condition is false
 *
 * @param condition Expression to evaluate
 * @param fmt       Printf-style format string
 * @param ...       Format arguments
 */
#define ASSERT_FALSE(condition, fmt, ...)                                   \
    do {                                                                    \
        if (condition) {                                                    \
            printf("FAIL: " fmt "\n", ##__VA_ARGS__);                       \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that two integers are equal
 *
 * @param a First value
 * @param b Second value
 * @param fmt Printf-style format string
 * @param ... Format arguments
 *
 * @note Values are cast to long for printing.
 */
#define ASSERT_EQUAL(a, b, fmt, ...)                                        \
    do {                                                                    \
        if ((a) != (b)) {                                                   \
            printf("FAIL: " fmt " (%ld != %ld)\n", ##__VA_ARGS__,           \
                   (long)(a), (long)(b));                                   \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that two integers are not equal
 *
 * @param a First value
 * @param b Second value
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_NOT_EQUAL(a, b, fmt, ...)                                    \
    do {                                                                    \
        if ((a) == (b)) {                                                   \
            printf("FAIL: " fmt " (%ld == %ld)\n", ##__VA_ARGS__,           \
                   (long)(a), (long)(b));                                   \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that two pointers are equal
 *
 * @param a First pointer
 * @param b Second pointer
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_PTR_EQUAL(a, b, fmt, ...)                                    \
    do {                                                                    \
        if ((a) != (b)) {                                                   \
            printf("FAIL: " fmt " (%p != %p)\n", ##__VA_ARGS__,             \
                   (void*)(a), (void*)(b));                                 \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that two pointers are not equal
 *
 * @param a First pointer
 * @param b Second pointer
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_PTR_NOT_EQUAL(a, b, fmt, ...)                                \
    do {                                                                    \
        if ((a) == (b)) {                                                   \
            printf("FAIL: " fmt " (%p == %p)\n", ##__VA_ARGS__,             \
                   (void*)(a), (void*)(b));                                 \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that a pointer is NULL
 *
 * @param ptr Pointer to check
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_NULL(ptr, fmt, ...)                                          \
    do {                                                                    \
        if ((ptr) != NULL) {                                                \
            printf("FAIL: " fmt "\n", ##__VA_ARGS__);                       \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that a pointer is not NULL
 *
 * @param ptr Pointer to check
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_NOT_NULL(ptr, fmt, ...)                                      \
    do {                                                                    \
        if ((ptr) == NULL) {                                                \
            printf("FAIL: " fmt "\n", ##__VA_ARGS__);                       \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that two strings are equal
 *
 * Uses strcmp() for comparison.
 *
 * @param a First string
 * @param b Second string
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_STR_EQUAL(a, b, fmt, ...)                                    \
    do {                                                                    \
        if (strcmp((a), (b)) != 0) {                                        \
            printf("FAIL: " fmt " (\"%s\" != \"%s\")\n", ##__VA_ARGS__,     \
                   (a), (b));                                               \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that two strings are not equal
 *
 * Uses strcmp() for comparison.
 *
 * @param a First string
 * @param b Second string
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_STR_NOT_EQUAL(a, b, fmt, ...)                                \
    do {                                                                    \
        if (strcmp((a), (b)) == 0) {                                        \
            printf("FAIL: " fmt " (\"%s\" == \"%s\")\n", ##__VA_ARGS__,     \
                   (a), (b));                                               \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that a < b (size_t values)
 *
 * @param a First value
 * @param b Second value
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_LESS(a, b, fmt, ...)                                         \
    do {                                                                    \
        if ((a) >= (b)) {                                                   \
            printf("FAIL: " fmt " (%zu >= %zu)\n", ##__VA_ARGS__,           \
                   (size_t)(a), (size_t)(b));                               \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/**
 * @brief Assert that a > b (size_t values)
 *
 * @param a First value
 * @param b Second value
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
#define ASSERT_GREATER(a, b, fmt, ...)                                      \
    do {                                                                    \
        if ((a) <= (b)) {                                                   \
            printf("FAIL: " fmt " (%zu <= %zu)\n", ##__VA_ARGS__,           \
                   (size_t)(a), (size_t)(b));                               \
            return 0;                                                       \
        }                                                                   \
    } while (0)

/** @} */

#endif /* ASSERTION_PDR_H */