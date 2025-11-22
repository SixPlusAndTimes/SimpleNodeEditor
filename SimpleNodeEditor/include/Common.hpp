#ifndef COMMON_H
#define COMMON_H
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include "Log.hpp"

// inspired by HAZEL
#define SNE_EXPAND_MACRO(x)    x
#define SNE_STRINGIFY_MACRO(x) #x

#ifdef DEBUG
#define SNE_INTERNAL_ASSERT_IMPL(check, msg, ...) \
    {                                             \
        if (!(check))                             \
        {                                         \
            SNELOG_CRITICAL(msg, __VA_ARGS__);    \
            std::abort();                         \
        }                                         \
    }
#else
#define SNE_INTERNAL_ASSERT_IMPL(check, msg, ...) \
    {                                             \
        if (!(check))                             \
        {                                         \
            SNELOG_CRITICAL(msg, __VA_ARGS__);    \
        }                                         \
    }
#endif
#define SNE_INTERNAL_ASSERT_WITH_MSG(check, ...) \
    SNE_INTERNAL_ASSERT_IMPL(check, "Assertion failed: {0}", __VA_ARGS__)
#define SNE_INTERNAL_ASSERT_NO_MSG(check)                                \
    SNE_INTERNAL_ASSERT_IMPL(check, "Assertion '{0}' failed at {1}:{2}", \
                             SNE_STRINGIFY_MACRO(check),                 \
                             std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define SNE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define SNE_INTERNAL_ASSERT_GET_MACRO(...)                                                         \
    SNE_EXPAND_MACRO(SNE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, SNE_INTERNAL_ASSERT_WITH_MSG, \
                                                        SNE_INTERNAL_ASSERT_NO_MSG))

#define SNE_ASSERT(...) SNE_EXPAND_MACRO(SNE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))

#endif // COMMON_H