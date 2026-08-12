#include "rgba_to_bgra.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#if defined(_MSC_VER) && defined(HAVE_AVX2_IMPLEMENTATION)
#include <intrin.h>
#endif

void rgba_to_bgra_scalar(const std::uint8_t* src, std::uint8_t* dst,
                         std::size_t pixel_count) {
#if defined(__clang__)
#pragma clang loop vectorize(disable) interleave(disable)
#elif defined(_MSC_VER)
#pragma loop(no_vector)
#elif defined(__GNUC__)
#pragma GCC novector
#endif
    for (std::size_t i = 0; i < pixel_count; ++i) {
        dst[i * 4 + 0] = src[i * 4 + 2];
        dst[i * 4 + 1] = src[i * 4 + 1];
        dst[i * 4 + 2] = src[i * 4 + 0];
        dst[i * 4 + 3] = src[i * 4 + 3];
    }
}

static bool cpu_has_avx2() {
#if defined(HAVE_AVX2_IMPLEMENTATION) && defined(_MSC_VER)
    int regs[4]{};
    __cpuid(regs, 1);
    const bool osxsave = (regs[2] & (1 << 27)) != 0;
    const bool avx = (regs[2] & (1 << 28)) != 0;
    if (!osxsave || !avx || (_xgetbv(0) & 0x6) != 0x6) return false;
    __cpuidex(regs, 7, 0);
    return (regs[1] & (1 << 5)) != 0;
#elif defined(HAVE_AVX2_IMPLEMENTATION) && (defined(__GNUC__) || defined(__clang__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2");
#else
    return false;
#endif
}

static bool cpu_has_ssse3() {
#if defined(HAVE_SSSE3_IMPLEMENTATION) && defined(_MSC_VER)
    int regs[4]{};
    __cpuid(regs, 1);
    return (regs[2] & (1 << 9)) != 0;
#elif defined(HAVE_SSSE3_IMPLEMENTATION) && (defined(__GNUC__) || defined(__clang__))
    __builtin_cpu_init();
    return __builtin_cpu_supports("ssse3");
#else
    return false;
#endif
}

using ConvertFn = void (*)(const std::uint8_t*, std::uint8_t*, std::size_t);

static double benchmark(ConvertFn fn, const std::vector<std::uint8_t>& src,
                        std::vector<std::uint8_t>& dst, int iterations) {
    fn(src.data(), dst.data(), src.size() / 4); // warm up code and memory
    const auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) fn(src.data(), dst.data(), src.size() / 4);
    const auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(end - begin).count() / iterations;
}

static void print_result(const std::string& name, double seconds,
                         std::size_t image_bytes) {
    // Effective bandwidth counts one full-image read plus one full-image write.
    const double gib_per_second = (2.0 * static_cast<double>(image_bytes)) /
                                  seconds / (1024.0 * 1024.0 * 1024.0);
    std::cout << std::left << std::setw(12) << name << std::right << std::fixed
              << std::setprecision(3) << seconds * 1000.0 << " ms/frame, "
              << std::setprecision(2) << gib_per_second << " GiB/s\n";
}

int main() {
    constexpr std::size_t width = 3840;
    constexpr std::size_t height = 2160;
    constexpr std::size_t pixels = width * height;
    constexpr std::size_t bytes = pixels * 4;
    constexpr int iterations = 100;

    std::vector<std::uint8_t> rgba(bytes);
    std::vector<std::uint8_t> scalar_output(bytes);
    std::vector<std::uint8_t> simd_output(bytes);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> byte_value(0, 255);
    std::generate(rgba.begin(), rgba.end(), [&] {
        return static_cast<std::uint8_t>(byte_value(rng));
    });

    std::cout << "Image: " << width << 'x' << height << " RGBA ("
              << bytes / (1024 * 1024) << " MiB), iterations: " << iterations << "\n";

    const double scalar_time =
        benchmark(rgba_to_bgra_scalar, rgba, scalar_output, iterations);
    print_result("Scalar", scalar_time, bytes);

#if defined(HAVE_SSSE3_IMPLEMENTATION)
    if (cpu_has_ssse3()) {
        const double ssse3_time =
            benchmark(rgba_to_bgra_ssse3, rgba, simd_output, iterations);
        if (scalar_output != simd_output) {
            std::cerr << "ERROR: SSSE3 output differs from scalar output\n";
            return 1;
        }
        print_result("SSSE3", ssse3_time, bytes);
        std::cout << "SSSE3 speedup: " << std::fixed << std::setprecision(2)
                  << scalar_time / ssse3_time << "x\n";
    } else {
        std::cout << "SSSE3: CPU does not support SSSE3; skipped.\n";
    }
#endif

#if defined(HAVE_AVX2_IMPLEMENTATION)
    if (cpu_has_avx2()) {
        const double avx2_time =
            benchmark(rgba_to_bgra_avx2, rgba, simd_output, iterations);
        if (scalar_output != simd_output) {
            std::cerr << "ERROR: AVX2 output differs from scalar output\n";
            return 1;
        }
        print_result("AVX2", avx2_time, bytes);
        std::cout << "AVX2 speedup: " << std::fixed << std::setprecision(2)
                  << scalar_time / avx2_time << "x\n";
    } else {
        std::cout << "AVX2: CPU or operating system does not support AVX2; skipped.\n";
    }
#else
    std::cout << "AVX2: no x86-64 AVX2 implementation in this build; skipped.\n";
#endif
    std::cout << "Correctness: PASS\n";
    return 0;
}
