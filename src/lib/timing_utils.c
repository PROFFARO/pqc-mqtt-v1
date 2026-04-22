/* ============================================================================
 * timing_utils.c — High-resolution timer implementation
 * ============================================================================ */

#include "timing_utils.h"

void timer_start(pqc_timer_t *timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->start);
}

void timer_stop(pqc_timer_t *timer)
{
    clock_gettime(CLOCK_MONOTONIC, &timer->end);
}

uint64_t timer_elapsed_ns(const pqc_timer_t *timer)
{
    uint64_t start_ns = (uint64_t)timer->start.tv_sec * 1000000000ULL
                      + (uint64_t)timer->start.tv_nsec;
    uint64_t end_ns   = (uint64_t)timer->end.tv_sec * 1000000000ULL
                      + (uint64_t)timer->end.tv_nsec;
    return end_ns - start_ns;
}

double timer_elapsed_us(const pqc_timer_t *timer)
{
    return (double)timer_elapsed_ns(timer) / 1000.0;
}

double timer_elapsed_ms(const pqc_timer_t *timer)
{
    return (double)timer_elapsed_ns(timer) / 1000000.0;
}
