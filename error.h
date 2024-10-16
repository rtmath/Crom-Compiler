#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h> // for variadic args, va_list et al.

#define ERROR_AND_CONTINUE(fmt, ...) ErrorAndContinue(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define ERROR_AND_EXIT(fmt, ...) ErrorAndExit(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define ERROR_AND_CONTINUE_VALIST(fmt, valist) ErrorAndContinueVAList(__FILE__, __LINE__, fmt, valist)
#define ERROR_AND_EXIT_VALIST(fmt, valist) ErrorAndExitVAList(__FILE__, __LINE__, fmt, valist)

void ErrorAndContinue(const char *src_filename, int line_number,
                      const char *fmt_string, ...);

void ErrorAndExit(const char *src_filename, int line_number,
                  const char *fmt_string, ...);

void ErrorAndContinueVAList(const char *src_filename, int line_number,
                            const char *fmt_string, va_list args);

void ErrorAndExitVAList(const char *src_filename, int line_number,
                        const char *fmt_string, va_list args);

#endif
