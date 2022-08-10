#ifndef __SYLAR_MACRO_H__
#define __SYLAR_Macro_H__
#define SYLAR_ASSERT(x)                                                                \
    if (!(x))                                                                          \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\nbacktrace:\n"                          \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }

#define SYLAR_ASSERT2(x, w)                                                            \
    if (!(x))                                                                          \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\n"                                      \
                                          << w                                         \
                                          << "\nbacktrace:\n"                          \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }
#endif