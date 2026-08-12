#include "rgba_to_bgra.h"

#include <tmmintrin.h>

void rgba_to_bgra_ssse3(const std::uint8_t* src, std::uint8_t* dst,
                        std::size_t pixel_count) {
    const __m128i shuffle = _mm_setr_epi8(
         2,  1,  0,  3,  6,  5,  4,  7,
        10,  9,  8, 11, 14, 13, 12, 15);

    std::size_t i = 0;
    for (; i + 4 <= pixel_count; i += 4) {
        const __m128i rgba =
            _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i * 4));
        const __m128i bgra = _mm_shuffle_epi8(rgba, shuffle);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i * 4), bgra);
    }

    for (; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 4 + 2];
        dst[i * 4 + 1] = src[i * 4 + 1];
        dst[i * 4 + 2] = src[i * 4 + 0];
        dst[i * 4 + 3] = src[i * 4 + 3];
    }
}

