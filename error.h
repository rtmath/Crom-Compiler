#ifndef ERROR_H
#define ERROR_H

#define ERROR_EXIT(fmt, ...) ErrorExit(__FILE__, __LINE__, fmt, __VA_ARGS__)

void ErrorExit(const char* src_filename, int line_number,
               const char *fmt_string, ...);

#endif
