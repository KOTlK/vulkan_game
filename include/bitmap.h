#pragma once

#include "types.h"
#include "assert.h"
#include "debug.h"
#include "stdio.h"

#define BITMAP_TEMPLATE template <u32 bit_count = 256>

#define BITS_PER_SLOT (sizeof(u64) * 8)

BITMAP_TEMPLATE
struct Bitmap {
    static_assert(bit_count % BITS_PER_SLOT == 0);
    u64 bits[bit_count / BITS_PER_SLOT] = {};
};

BITMAP_TEMPLATE
static inline bool operator==(const Bitmap<bit_count> lhs, const Bitmap<bit_count> rhs);

BITMAP_TEMPLATE
static inline bool operator!=(const Bitmap<bit_count>& lhs, const Bitmap<bit_count>& rhs);

template<u32 bit_count = 256, typename... Args>
static inline Bitmap<bit_count> bitmap_make(Args... args);

BITMAP_TEMPLATE
static inline void bitmap_set_bit(Bitmap<bit_count>& bitmap, const u32 bit);

BITMAP_TEMPLATE
static inline void bitmap_clear_bit(Bitmap<bit_count>& bitmap, const u32 bit);

BITMAP_TEMPLATE
static inline void bitmap_toggle_bit(Bitmap<bit_count>& bitmap, const u32 bit);

BITMAP_TEMPLATE
static inline bool bitmap_test_bit(Bitmap<bit_count>& bitmap, const u32 bit);

BITMAP_TEMPLATE
static inline void bitmap_clear_all(Bitmap<bit_count>& bitmap);

BITMAP_TEMPLATE
static inline void bitmap_set_all(Bitmap<bit_count>& bitmap);

BITMAP_TEMPLATE
static inline void bitmap_print(const Bitmap<bit_count>& bitmap);

BITMAP_TEMPLATE
static inline Bitmap<bit_count> bitmap_and(Bitmap<bit_count>& a, Bitmap<bit_count>& b);

BITMAP_TEMPLATE
static inline u64 get_hash(Bitmap<bit_count> bitmap) {
    u32 count = bit_count / BITS_PER_SLOT;
    u64 total = 0;

    for (u32 i = 0; i < count; i++) {
        total += bitmap.bits[i];
    }

    return total;
}

#ifdef BITMAP_IMPLEMENTATION

#define BITMAP_TEMPLATE_IMPL template <u32 bit_count>

template<u32 bit_count, typename... Args>
static inline Bitmap<bit_count> bitmap_make(Args... args) {
    Bitmap<bit_count> bitmap = {};
    
    for (const auto arg : {args...}) {
        bitmap_set_bit(bitmap, arg);
    }

    return bitmap;
}

BITMAP_TEMPLATE_IMPL
static inline bool operator==(const Bitmap<bit_count> lhs, const Bitmap<bit_count> rhs) {
    u32 count = bit_count / BITS_PER_SLOT;

    for (u32 i = 0; i < count; i++) {
        if (lhs.bits[i] != rhs.bits[i]) {
            return false;
        }
    }

    return true;
}

BITMAP_TEMPLATE_IMPL
static inline bool operator!=(const Bitmap<bit_count>& lhs, const Bitmap<bit_count>& rhs) {
    return !(lhs == rhs);
}

BITMAP_TEMPLATE_IMPL
static inline void bitmap_set_bit(Bitmap<bit_count>& bitmap, const u32 bit) {
    Assertf(bit < bit_count, "Cannot set bit #%d bitmap length (%d) is less than the bit, you want to set.", bit, bit_count);
    u32 index     = bit / BITS_PER_SLOT;
    u32 local_bit = bit % BITS_PER_SLOT;

    bitmap.bits[index] |= (1ULL << local_bit);
}

BITMAP_TEMPLATE_IMPL
static inline void bitmap_clear_bit(Bitmap<bit_count>& bitmap, const u32 bit) {
    Assertf(bit < bit_count, "Cannot set bit #%d bitmap length (%d) is less than the bit, you want to set.", bit, bit_count);
    u32 index     = bit / BITS_PER_SLOT;
    u32 local_bit = bit % BITS_PER_SLOT;

    bitmap.bits[index] &= ~(1ULL << local_bit);
}

BITMAP_TEMPLATE_IMPL
static inline void bitmap_toggle_bit(Bitmap<bit_count>& bitmap, const u32 bit) {
    Assertf(bit < bit_count, "Cannot set bit #%d bitmap length (%d) is less than the bit, you want to set.", bit, bit_count);
    u32 index     = bit / BITS_PER_SLOT;
    u32 local_bit = bit % BITS_PER_SLOT;

    bitmap.bits[index] ^= (1ULL << local_bit);
}

BITMAP_TEMPLATE_IMPL
static inline bool bitmap_test_bit(Bitmap<bit_count>& bitmap, const u32 bit) {
    Assertf(bit < bit_count, "Cannot set bit #%d bitmap length (%d) is less than the bit, you want to set.", bit, bit_count);
    u32 index     = bit / BITS_PER_SLOT;
    u32 local_bit = bit % BITS_PER_SLOT;

    return (bitmap.bits[index] & (1ULL << local_bit)) != 0;
}

BITMAP_TEMPLATE_IMPL
static inline void bitmap_clear_all(Bitmap<bit_count>& bitmap) {
    u32 count = bit_count / BITS_PER_SLOT;

    for (u32 i = 0; i < count; i++) {
        bitmap.bits[i] = 0;
    }
}

BITMAP_TEMPLATE_IMPL
static inline void bitmap_set_all(Bitmap<bit_count>& bitmap) {
    u32 count = bit_count / BITS_PER_SLOT;

    for (u32 i = 0; i < count; i++) {
        bitmap.bits[i] = u64_max;
    }
}

BITMAP_TEMPLATE_IMPL
static inline Bitmap<bit_count> bitmap_and(Bitmap<bit_count>& a, Bitmap<bit_count>& b) {
    Bitmap<bit_count> res;
    u32 count = bit_count / BITS_PER_SLOT;

    for (u32 i = 0; i < count; i++) {
        res.bits[i] = a.bits[i] & b.bits[i];
    }

    return res;
}

BITMAP_TEMPLATE_IMPL
static inline void bitmap_print(const Bitmap<bit_count>& bitmap) {
    // printf("Bitmap [%u]: ", bit_count);
    
    for (u32 i = 0; i < bit_count; i++) {
        u32 index = i / BITS_PER_SLOT;
        u32 local_bit = i % BITS_PER_SLOT;
        u64 mask = 1ULL << local_bit;
        
        if (bitmap.bits[index] & mask) {
            printf("1");
        } else {
            printf("0");
        }
        
        if (i > 0 && (i + 1) < bit_count && ((i + 1) % 8) == 0) {
            printf(" ");
        }
    }
    printf("\n");
}

#endif // BITMAP_IMPLEMENTATION