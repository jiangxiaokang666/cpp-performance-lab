#pragma once

#include <cstddef>
#include <cstdint>

// src and dst each contain pixel_count tightly packed four-byte pixels.
// RGBA byte order: R,G,B,A. BGRA byte order: B,G,R,A.
void rgba_to_bgra_scalar(const std::uint8_t* src, std::uint8_t* dst,
                         std::size_t pixel_count);

#if defined(HAVE_SSSE3_IMPLEMENTATION)
void rgba_to_bgra_ssse3(const std::uint8_t* src, std::uint8_t* dst,
                        std::size_t pixel_count);
#endif

#if defined(HAVE_AVX2_IMPLEMENTATION)
void rgba_to_bgra_avx2(const std::uint8_t* src, std::uint8_t* dst,
                       std::size_t pixel_count);
#endif

