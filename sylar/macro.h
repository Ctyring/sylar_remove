#ifndef __SYLAR_MACRO_H__
#define __SYLAR_Macro_H__
#include <assert.h>
#if defined __GNUC__ || defined __llvm__
#define SYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
#define SYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x) (x)
#define SYLAR_UNLIKELY(x) (x)
#endif
#define SYLAR_ASSERT(x)                                  \
    if (SYLAR_UNLIKELY(!(x))) {                          \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                \
            << "ASSERTION: " #x << "\nbacktrace:\n"      \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                       \
    }

#define SYLAR_ASSERT2(x, w)                              \
    if (SYLAR_UNLIKELY(!(x))) {                          \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                \
            << "ASSERTION: " #x << "\n"                  \
            << w << "\nbacktrace:\n"                     \
            << sylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                       \
    }
#endif