# Bytehound 使用示例

这个仓库用来演示如何用 Bytehound 在本地快速发现常见的内存问题，并附带可以直接运行的 C 示例程序。

## 环境要求
- Linux x86_64（Bytehound 目前仅支持 Linux）
- 已安装 Rust/Cargo（建议 1.72+）
- 能从源码编译 Bytehound（官方仓库），需要 `cmake` 等基础构建工具
- Node.js + Yarn（构建 WebUI 时需要 Yarn，否则编译会失败）

## 编译 Bytehound
> 具体参数可能随版本变化，请以官方 README 为准。

```bash
git clone https://github.com/koute/bytehound.git
cd bytehound
cargo install -f wasm-pack # 如尚未安装且提示缺失时（可选）
# 确保已装 Yarn（推荐 corepack）：corepack enable && corepack prepare yarn@1.22.22 --activate && hash -r && yarn --version
cargo build --release
```

## 核心组件作用
- `bytehound-preload`（生成 `libbytehound.so`）：通过 `LD_PRELOAD` 劫持 `malloc/free` 等分配接口，把分配事件写到采集输出。
- `bytehound`：查看/分析工具，读取录制文件并提供 Web UI。

## 基本用法
- 适合单进程、临时分析、快速定位。
- 被测进程退出后生成一个 `.data`。
```bash
LD_PRELOAD=/path/to/libbytehound.so \
MEMORY_PROFILER_OUTPUT=/tmp/bh.data \
./your_program

# 查看
/path/to/bytehound server /tmp/bh.data --port 1789 -d /path/to/debug-symbols
# 如端口冲突，调整为其他端口（例如 8087）：/path/to/bytehound server /tmp/bh.data --port 8087 -d /path/to/debug-symbols
```
- 如果不指定 `MEMORY_PROFILER_OUTPUT`，默认会生成类似 `memory-profiling_%e_%t_%p.dat` 的文件名。

## 仓库结构
- `examples/alloc_spike`：周期性大额分配/释放，方便观察瞬时内存峰值（C）。
- `examples/slow_leak`：缓慢泄漏型场景，方便观察持续增长的内存占用（C）。

## 调试符号（强烈建议保留）
- Bytehound 不依赖符号也能采集，但没有符号 UI 里只会显示地址（分析效率极低）。
- 最简单：直接编译带 `-g`，保留符号即可：
  ```bash
  gcc -g -O2 main.c -o app
  ```
- 如果想把符号单独存放，可以额外生成一个调试符号文件（可选）：
  ```bash
  objcopy --only-keep-debug app app.debug
  # 需要瘦身再 strip 主程序时，再配合 --add-gnu-debuglink 标记符号文件：
  # strip --strip-debug app
  # objcopy --add-gnu-debuglink=app.debug app
  ```
  - `app.debug`：符号/行号文件，UI 可直接解析出栈帧信息
- 查看时用 `-d` 指向符号目录或符号文件所在位置，例如 `/path/to/bytehound server data.dat --port 1789 -d /path/to/debug-symbols`。

## 如何运行示例（配合 Bytehound，C 版本）
假设 Bytehound 编译产物在 `/path/to/bytehound/target/release`。示例目录自带 `Makefile`，默认 `gcc -O0 -g`。示例运行后直接生成 `.data`。

```bash
cd examples/alloc_spike
make
mkdir -p ../../recordings
LD_PRELOAD=/path/to/bytehound/target/release/libbytehound.so \
MEMORY_PROFILER_OUTPUT=../../recordings/alloc_spike.data \
./alloc_spike

# 采集完成后查看
/path/to/bytehound/target/release/bytehound server ../../recordings/alloc_spike.data --port 1789 -d /path/to/debug-symbols
```

## 典型排查思路
- **瞬时峰值**：关注堆分配热点、调用栈，判断是否需要池化/复用。
- **慢性泄漏**：观察时间轴，确认常驻集合是否不断增长（缓存未淘汰、未关闭的资源等）。
- **线程维度**：查看线程标签/调用栈，排查具体线程的异常行为。
- **采样开销**：先在小规模/短时场景验证，再迁移到真实 workload。

后续可以在 `examples/` 中持续添加新的场景（例如碎片化、频繁 mmap/unmap 等），便于对照 Bytehound 的视图理解不同问题特征。
