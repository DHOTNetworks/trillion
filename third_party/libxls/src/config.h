#ifndef LIBXLS_CONFIG_H
#define LIBXLS_CONFIG_H

#define PACKAGE_VERSION "1.6.3"

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup
#define HAVE_MEMMOVE 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define HAVE_LOCALE_H 1
#else
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_MEMMOVE 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define HAVE_LOCALE_H 1
#define HAVE_ICONV 1
#define ICONV_CONST
#endif

#if defined(__APPLE__)
#include <xlocale.h>
#define HAVE_WCSTOMBS_L 1
#elif defined(__linux__)
#include <locale.h>
#define HAVE_WCSTOMBS_L 1
#endif

#endif
