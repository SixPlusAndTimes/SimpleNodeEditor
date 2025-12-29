#ifndef COMMON_H
#define COMMON_H
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include "Log.hpp"


#define SNE_EXPAND_MACRO(x)    x
#define SNE_STRINGIFY_MACRO(x) #x

// inspired by HAZEL
#ifdef DEBUG
	#if defined(WIN32)
		#define DEBUGBREAK() __debugbreak()
	#elif defined(LINUX) ## TODO
		#include <signal.h>
		#define DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define HZ_ENABLE_ASSERTS
#else
	#define DEBUGBREAK()
#endif

#ifdef DEBUG
#define SNE_INTERNAL_ASSERT_IMPL(check, msg, ...) \
    {                                             \
        if (!(check))                             \
        {                                         \
            SNELOG_CRITICAL(msg, __VA_ARGS__);    \
            DEBUGBREAK();                         \
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

#ifndef ImGuiColor
#define ImGuiColor
#define COLOR_RED           ImVec4(1.0f, 0.f, 0.f, 1.f)
#define COLOR_RED_U32       IM_COL32(255, 0, 0, 255)
#define COLOR_GREEN         ImVec4(0.0f, 1.f, 0.f, 1.f)
#define COLOR_GREEN_U32     IM_COL32(0, 255, 0, 255)
#define COLOR_BLUE          ImVec4(0.0f, 0.f, 1.f, 1.f)
#define COLOR_BLUE_U32      IM_COL32(0, 0, 255, 255)
#endif

inline unsigned long long size_b(unsigned long long value)
{
    return value;
}

inline unsigned long long size_kb(unsigned long long value)
{
    return value * 1024;
}

inline unsigned long long size_mb(unsigned long long value)
{
    return value * 1024 * 1024;
}

inline unsigned long long size_gb(unsigned long long value)
{
    return value * 1024 * 1024 * 1024;
}

inline unsigned long long size_tb(unsigned long long value)
{
    return value * 1024 * 1024 * 1024 * 1024;
}

#endif // COMMON_H