/** @file timer.h
 * @brief Simple Performance Measurement Utilities
 *
 * Provides macros and a simple struct for measuring execution time.
 * Uses clock() which measures CPU time (not wall-clock).
 *
 * @author Sadiq Idris (@sadichel)
 * @license MIT
 * @version 1.0 — January 2026
 *
 * @section usage Usage Examples
 *
 * **Macro style:**
 * @code
 *   TIME(expensive_function());
 *   TIME_VOID(expensive_operation());
 * @endcode
 *
 * **Timer struct style:**
 * @code
 *   Timer t = timer_start();
 *   expensive_function();
 *   double seconds = timer_elapsed(&t);
 * @endcode
 *
 * **Time multiple calls:**
 * @code
 *   double seconds = TIMEIT({
 *       for (int i = 0; i < 1000; i++) expensive_function();
 *   });
 * @endcode
 *
 * @note Uses CPU time (clock()), not wall-clock time.
 *       For wall-clock timing on POSIX, consider gettimeofday() or clock_gettime().
 */

#ifndef TIMER_PDR_H
#define TIMER_PDR_H

#include <time.h>
#include <stdio.h>

/**
 * @defgroup timer_macros Timer Macros
 * @brief Convenience macros for quick timing
 * @{
 */

/**
 * @brief Measure and print execution time of an expression that returns a value
 *
 * Executes the expression, prints the elapsed time in milliseconds,
 * and returns the expression's result.
 *
 * @param expr Expression to time (must return a value)
 * @return The result of the expression
 *
 * @par Example
 * @code
 *   int result = TIME(fibonacci(30));
 *   // Output: fibonacci(30): 12.345 ms
 * @endcode
 */
#define TIME(expr)                                                  \
    ({                                                              \
        clock_t _start = clock();                                   \
        __typeof__(expr) _result = (expr);                          \
        clock_t _end = clock();                                     \
        double _ms = 1000.0 * (_end - _start) / CLOCKS_PER_SEC;     \
        printf("%s: %.3f ms\n", #expr, _ms);                        \
        _result;                                                    \
    })

/**
 * @brief Measure and print execution time of a void expression
 *
 * Executes the expression and prints the elapsed time in milliseconds.
 *
 * @param expr Expression to time (returns void)
 *
 * @par Example
 * @code
 *   TIME_VOID(sort_large_array());
 *   // Output: sort_large_array(): 45.678 ms
 * @endcode
 */
#define TIME_VOID(expr)                                             \
    ({                                                              \
        clock_t _start = clock();                                   \
        (expr);                                                     \
        clock_t _end = clock();                                     \
        double _ms = 1000.0 * (_end - _start) / CLOCKS_PER_SEC;     \
        printf("%s: %.3f ms\n", #expr, _ms);                        \
    })

/**
 * @brief Measure execution time of a block, return seconds
 *
 * Executes a block of code and returns the elapsed time in seconds.
 * No output is printed.
 *
 * @param block Code block to time (may contain multiple statements)
 * @return Elapsed time in seconds (as double)
 *
 * @par Example
 * @code
 *   double seconds = TIMEIT({
 *       for (int i = 0; i < 1000; i++) {
 *           expensive_function();
 *       }
 *   });
 *   printf("Total time: %.3f seconds\n", seconds);
 * @endcode
 */
#define TIMEIT(block)                                               \
    ({                                                              \
        clock_t _start = clock();                                   \
        block;                                                      \
        clock_t _end = clock();                                     \
        (double)(_end - _start) / CLOCKS_PER_SEC;                   \
    })

/**
 * @brief Alias for TIMEIT
 *
 * @param block Code block to time
 * @return Elapsed time in seconds
 */
#define TIME_BLOCK(block) TIMEIT(block)

/** @} */

/**
 * @defgroup timer_struct Timer Struct API
 * @brief Struct-based timing for more control
 * @{
 */

/**
 * @brief Timer structure for manual timing control
 *
 * @var Timer::start
 * Starting clock value
 */
typedef struct {
    clock_t start;  /**< Starting clock value */
} Timer;

/**
 * @brief Create and start a new timer
 *
 * @return Initialized Timer struct with current time
 *
 * @par Example
 * @code
 *   Timer t = timer_start();
 *   // ... do work ...
 *   double elapsed = timer_elapsed(&t);
 * @endcode
 */
static inline Timer timer_start(void) {
    return (Timer){ .start = clock() };
}

/**
 * @brief Get elapsed seconds since timer was started
 *
 * @param t Pointer to Timer struct
 * @return Elapsed time in seconds (as double)
 */
static inline double timer_elapsed(Timer *t) {
    clock_t now = clock();
    return (double)(now - t->start) / CLOCKS_PER_SEC;
}

/**
 * @brief Get elapsed milliseconds since timer was started
 *
 * @param t Pointer to Timer struct
 * @return Elapsed time in milliseconds (as double)
 */
static inline double timer_elapsed_ms(Timer *t) {
    clock_t now = clock();
    return 1000.0 * (now - t->start) / CLOCKS_PER_SEC;
}

/**
 * @brief Reset timer to current time
 *
 * Sets the timer's start time to the current clock value.
 *
 * @param t Pointer to Timer struct
 *
 * @par Example
 * @code
 *   Timer t = timer_start();
 *   // ... first operation ...
 *   timer_reset(&t);
 *   // ... second operation ...
 *   double elapsed = timer_elapsed(&t);  // time of second operation only
 * @endcode
 */
static inline void timer_reset(Timer *t) {
    t->start = clock();
}

/** @} */

#endif /* TIMER_PDR_H */
