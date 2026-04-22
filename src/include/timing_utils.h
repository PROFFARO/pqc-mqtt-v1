/* ============================================================================
 * timing_utils.h — High-resolution timing helpers for benchmarking
 * Uses CLOCK_MONOTONIC for accurate, drift-free measurements.
 * ============================================================================ */

#ifndef TIMING_UTILS_H
#define TIMING_UTILS_H

#include <time.h>
#include <stdint.h>

/* --- Timer context --- */
typedef struct {
    struct timespec start;
    struct timespec end;
} pqc_timer_t;

/**
 * Start the timer (captures current CLOCK_MONOTONIC).
 */
void timer_start(pqc_timer_t *timer);

/**
 * Stop the timer.
 */
void timer_stop(pqc_timer_t *timer);

/**
 * Get elapsed time in nanoseconds.
 */
uint64_t timer_elapsed_ns(const pqc_timer_t *timer);

/**
 * Get elapsed time in microseconds.
 */
double timer_elapsed_us(const pqc_timer_t *timer);

/**
 * Get elapsed time in milliseconds.
 */
double timer_elapsed_ms(const pqc_timer_t *timer);

#endif /* TIMING_UTILS_H */
