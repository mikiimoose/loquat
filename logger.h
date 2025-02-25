#ifndef LOGGER_H
#define LOGGER_H

#include <syslog.h>

// Log output type
typedef enum {
    LOG_OUTPUT_SYSLOG,
    LOG_OUTPUT_CONSOLE
} log_output_t;

// Initialize logging system
void log_init(const char *program_name, log_output_t output_type);

// Log message
void log_message(int priority, const char *format, ...);

// Cleanup logging system
void log_cleanup(void);

#endif // LOGGER_H 