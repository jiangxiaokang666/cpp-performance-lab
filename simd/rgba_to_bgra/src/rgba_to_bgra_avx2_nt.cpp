#include "rgba_to_bgra.h"

#include <immintrin.h>

// AVX2 variant that writes dst with non-temporal (streaming) stores.
// A normal store on a write miss triggers a read-for-ownership: the CPU first
// pulls the whole cache line from DRAM, then modifies it. For a streaming
// kernel that never reads dst back, that extra read is pure waste. Streaming
// stores bypass the cache and skip the RFO, cutting DRAM traffic from three
// image-sized transfers (read src + RFO dst + write-back dst) down to two.
//
// _mm256_stream_si256 requires a 32-byte aligned destination, so we first run a
// scalar prefix until dst is aligned, then stream the aligned middle, then a
// scalar tail. A single _mm_sfence at the end makes the weakly-ordered stores
// visible before the function returns.
void rgba_to_bgra_avx2_nt(const std::uint8_t* src, std::uint8_t* dst,
                          std::size_t pixel_count) {
    const __m256i shuffle = _mm256_setr_epi8(
         2,  1,  0,  3,  6,  5,  4,  7,
        10,  9,  8, 11, 14, 13, 12, 15,
         2,  1,  0,  3,  6,  5,  4,  7,
        10,  9,  8, 11, 14, 13, 12, 15);

    std::size_t i = 0;

    // Scalar prefix: advance until dst is 32-byte aligned. Each pixel is four
    // bytes, so alignment is reached on a whole-pixel boundary.
    while (i < pixel_count &&
           (reinterpret_cast<std::uintptr_t>(dst + i * 4) & 31) != 0) {
        dst[i * 4 + 0] = src[i * 4 + 2];
        dst[i * 4 + 1] = src[i * 4 + 1];
        dst[i * 4 + 2] = src[i * 4 + 0];
        dst[i * 4 + 3] = src[i * 4 + 3];
        ++i;
    }

    // Aligned middle: eight pixels per iteration, streamed straight to DRAM.
    for (; i + 8 <= pixel_count; i += 8) {
        const __m256i rgba =
            _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i * 4));
        const __m256i bgra = _mm256_shuffle_epi8(rgba, shuffle);
        _mm256_stream_si256(reinterpret_cast<__m256i*>(dst + i * 4), bgra);
    }

    // Scalar tail for the remaining pixels.
    for (; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 4 + 2];
        dst[i * 4 + 1] = src[i * 4 + 1];
        dst[i * 4 + 2] = src[i * 4 + 0];
        dst[i * 4 + 3] = src[i * 4 + 3];
    }

    // NT stores are weakly ordered; ensure they are globally visible.
    _mm_sfence();
}
