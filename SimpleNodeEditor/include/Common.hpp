#ifndef COMMON_H
#define COMMON_H
#include <cassert>
#include <iostream>
#include <cstdlib>

#ifdef DEBUG
#define SNE_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                std::cerr << "[ASSERT FAILED] " << __FILE__ << ":" << __LINE__ << " " << __func__ << "\n" \
                          << "Condition: " << #condition << "\n" \
                          << "Message: " << message << "\n"; \
                std::abort(); \
            } \
        } while (0)
#else
    #define SNE_ASSERT(condition, message) ((void)0)
#endif



#endif // COMMON_H