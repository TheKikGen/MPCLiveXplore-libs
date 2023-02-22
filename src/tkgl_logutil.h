enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define tklog_trace(...) tklog(LOG_TRACE,  __VA_ARGS__)
#define tklog_debug(...) tklog(LOG_DEBUG,  __VA_ARGS__)
#define tklog_info(...)  tklog(LOG_INFO,   __VA_ARGS__)
#define tklog_warn(...)  tklog(LOG_WARN,   __VA_ARGS__)
#define tklog_error(...) tklog(LOG_ERROR,  __VA_ARGS__)
#define tklog_fatal(...) tklog(LOG_FATAL,  __VA_ARGS__)

static const char *tklog_level_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "***ERROR", "***FATAL"
};

static void tklog(int level, const char *fmt, ...) {

  va_list ap;

  //time_t timestamp = time( NULL );
  //struct tm * now = localtime( & timestamp );

  //char buftime[16];
  //buftime[strftime(buftime, sizeof(buftime), "%H:%M:%S", now)] = '\0';

  fprintf(stdout, "[tkgl %-8s]  ",tklog_level_strings[level]);

  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  va_end(ap);

  fflush(stdout);

}
