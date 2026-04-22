/* ============================================================================
 * csv_logger.h — Structured CSV logging for benchmark results
 * Each benchmark run appends rows to a CSV file for later analysis in Python.
 * ============================================================================ */

#ifndef CSV_LOGGER_H
#define CSV_LOGGER_H

#include <stdio.h>

/* --- Logger handle --- */
typedef struct {
    FILE   *file;
    char    filepath[512];
    int     header_written;
} csv_logger_t;

/**
 * Open (or create) a CSV file for logging.
 *
 * @param logger    Logger handle to initialize.
 * @param filepath  Path to the CSV file.
 * @param header    Comma-separated header line (e.g., "iteration,mode,latency_ns").
 *                  Written only if the file is new/empty.
 * @return          0 on success, -1 on error.
 */
int csv_logger_open(csv_logger_t *logger,
                    const char *filepath,
                    const char *header);

/**
 * Write a single row to the CSV.
 * The format string and args work like printf.
 */
int csv_logger_write(csv_logger_t *logger, const char *fmt, ...);

/**
 * Flush and close the CSV file.
 */
void csv_logger_close(csv_logger_t *logger);

#endif /* CSV_LOGGER_H */
