#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static log_output_t current_output;

// Convert syslog priority to text for console output
static const char* priority_to_string(int priority) {
    switch (priority) {
        case LOG_EMERG:   return "EMERGENCY";
        case LOG_ALERT:   return "ALERT";
        case LOG_CRIT:    return "CRITICAL";
        case LOG_ERR:     return "ERROR";
        case LOG_WARNING: return "WARNING";
        case LOG_NOTICE:  return "NOTICE";
        case LOG_INFO:    return "INFO";
        case LOG_DEBUG:   return "DEBUG";
        default:          return "UNKNOWN";
    }
}

void log_init(const char *program_name, log_output_t output_type) {
    current_output = output_type;
    
    if (current_output == LOG_OUTPUT_SYSLOG) {
        openlog(program_name, LOG_PID | LOG_CONS, LOG_USER);
        log_message(LOG_INFO, "Logging initialized (syslog)");
    } else {
        log_message(LOG_INFO, "Logging initialized (console)");
    }
}

void log_message(int priority, const char *format, ...) {
    va_list args;
    
    if (current_output == LOG_OUTPUT_SYSLOG) {
        va_start(args, format);
        vsyslog(priority, format, args);
        va_end(args);
    } else {
        // For console output, add timestamp and priority level
        time_t now;
        struct tm *tm_info;
        char timestamp[26];
        
        time(&now);
        tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(stderr, "[%s] [%s] ", timestamp, priority_to_string(priority));
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

void log_cleanup(void) {
    log_message(LOG_INFO, "Logging terminated");
    if (current_output == LOG_OUTPUT_SYSLOG) {
        closelog();
    }
} 