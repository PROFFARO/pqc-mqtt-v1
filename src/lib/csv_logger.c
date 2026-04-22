/* ============================================================================
 * csv_logger.c — CSV logger implementation
 * ============================================================================ */

#include "csv_logger.h"
#include <stdarg.h>
#include <string.h>

int csv_logger_open(csv_logger_t *logger,
                    const char *filepath,
                    const char *header)
{
    if (!logger || !filepath) return -1;

    memset(logger, 0, sizeof(*logger));
    strncpy(logger->filepath, filepath, sizeof(logger->filepath) - 1);

    /* Check if file already exists and has content */
    FILE *check = fopen(filepath, "r");
    int file_exists = 0;
    if (check) {
        fseek(check, 0, SEEK_END);
        file_exists = (ftell(check) > 0);
        fclose(check);
    }

    /* Open in append mode */
    logger->file = fopen(filepath, "a");
    if (!logger->file) {
        fprintf(stderr, "[csv_logger] ERROR: Cannot open %s\n", filepath);
        return -1;
    }

    /* Write header only if file is new/empty */
    if (!file_exists && header) {
        fprintf(logger->file, "%s\n", header);
        fflush(logger->file);
        logger->header_written = 1;
    }

    return 0;
}

int csv_logger_write(csv_logger_t *logger, const char *fmt, ...)
{
    if (!logger || !logger->file) return -1;

    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(logger->file, fmt, args);
    va_end(args);

    fprintf(logger->file, "\n");
    fflush(logger->file);

    return (ret >= 0) ? 0 : -1;
}

void csv_logger_close(csv_logger_t *logger)
{
    if (logger && logger->file) {
        fflush(logger->file);
        fclose(logger->file);
        logger->file = NULL;
    }
}
