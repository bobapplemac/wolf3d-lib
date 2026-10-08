#ifndef DBOPL_COMPAT_H
#define DBOPL_COMPAT_H

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef signed __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
#if defined(_WIN64)
typedef signed __int64 intptr_t;
typedef unsigned __int64 uintptr_t;
#else
typedef signed int intptr_t;
typedef unsigned int uintptr_t;
#endif
#else
#include <stdint.h>
#endif

#endif
