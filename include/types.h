#ifndef GFX2D_TYPES_H
#define GFX2D_TYPES_H

#include <cassert>
#include <cstdint>
#include <string>
#include <array>
#include <vector>

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

struct Color {
    u8 r = 0, g = 0, b = 0, a = 0;
};

#endif // GFX2D_TYPES_H
