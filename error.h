#ifndef ERROR_H
#define ERROR_H

#define ERROR_AND_CONTINUE(fmt, ...) ErrorAndContinue(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define ERROR_AND_EXIT(fmt, ...) ErrorAndExit(__FILE__, __LINE__, fmt, __VA_ARGS__)

void ErrorAndContinue(const char *src_filename, int line_number,
                      const char *fmt_string, ...);

void ErrorAndExit(const char *src_filename, int line_number,
                  const char *fmt_string, ...);

#endif
