#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef int8_t   s8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;
typedef uint64_t u64;
typedef int64_t  s64;

// #define s8  int8_t
// #define u8  uint8_t
// #define s16 int16_t
// #define u16 uint16_t
// #define s32 int32_t
// #define u32 uint32_t
// #define s64 int64_t
// #define u64 uint64_t

#define u8_max  UINT8_MAX
#define u16_max UINT16_MAX
#define u32_max UINT32_MAX
#define u64_max UINT64_MAX
#define s8_max  INT8_MAX
#define s16_max INT16_MAX
#define s32_max INT32_MAX
#define s64_max INT64_MAX
#define s8_min  INT8_MIN
#define s16_min INT16_MIN
#define s32_min INT32_MIN
#define s64_min INT64_MIN