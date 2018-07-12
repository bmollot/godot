#include "syslog.h"
#include <stdio.h>

void syslog (int __pri, const char *__fmt, ...) {
  va_list valist;

  int pri = LOG_PRI(__pri);

  FILE *out;

  if (pri <= 3) {
    out = stderr;
  } else {
    out = stdout;
  }

  vfprintf(out, __fmt, valist);
}

void syslogv (int __pri, const char *__fmt, __gnuc_va_list __ap) {
  int pri = LOG_PRI(__pri);

  FILE *out;

  if (pri <= 3) {
    out = stderr;
  } else {
    out = stdout;
  }

  vfprintf(out, __fmt, __ap);
}