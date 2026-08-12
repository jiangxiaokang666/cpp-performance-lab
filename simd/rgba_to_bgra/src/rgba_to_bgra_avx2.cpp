#include "rgba_to_bgra.h"

#include <immintrin.h>

void rgba_to_bgra_avx2(const std::uint8_t* src, std::uint8_t* dst,
                       std::size_t pixel_count) {
    // _mm256_shuffle_epi8 works independently on the two 128-bit lanes.
    // Each lane contains four pixels, so the same BGRA permutation repeats.
    const __m256i shuffle = _mm256_setr_epi8(
         2,  1,  0,  3,  6,  5,  4,  7,
        10,  9,  8, 11, 14, 13, 12, 15,
         2,  1,  0,  3,  6,  5,  4,  7,
        10,  9,  8, 11, 14, 13, 12, 15);

    std::size_t i = 0;
    for (; i + 8 <= pixel_count; i += 8) {
        const __m256i rgba =
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i * 4));
        const __m256i bgra = _mm256_shuffle_epi8(rgba, shuffle);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i * 4), bgra);
    }

    // Supports arbitrary image sizes, not only widths divisible by eight.
    for (; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 4 + 2];
        dst[i * 4 + 1] = src[i * 4 + 1];
        dst[i * 4 + 2] = src[i * 4 + 0];
        dst[i * 4 + 3] = src[i * 4 + 3];
    }
}

