# 4K RGBA -> BGRA CPU benchmark

这个 demo 比较三种实现：禁用编译器自动向量化、逐像素交换 R/B 的基线版本；
使用 SSSE3 `PSHUFB`、每次处理 4 个像素的版本；以及使用 AVX2 `VPSHUFB`、
每次处理 8 个像素的版本。程序会在运行时检测对应指令集，比较输出以验证正确性，
并打印平均帧耗时、有效内存带宽和加速比。

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/Release/rgba_to_bgra.exe
```

对于 Ninja 等单配置生成器，可执行文件通常位于 `build/rgba_to_bgra.exe`。
务必使用 Release 构建；Debug 下的性能数据没有参考价值。
