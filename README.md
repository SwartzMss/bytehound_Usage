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
常用产物：`target/release/libbytehound.so`（注入库）、`target/release/bytehound`（查看工具，取代旧版 `bh_viewer`）。新版已整合，无需指定单独 package。

## 核心组件作用
- `bytehound-preload`（生成 `libbytehound.so`）：通过 `LD_PRELOAD` 劫持 `malloc/free` 等分配接口，把分配事件写到采集输出（本指南用一体化模式写入 `.data`）。
- `bytehound`：查看/分析工具，读取录制文件并提供 Web UI。

## 最简单的一体化用法（推荐起步）
- 适合单进程、临时分析、快速定位。
- 被测进程退出后生成一个 `.data`，无需单独启动 Agent。
```bash
LD_PRELOAD=/path/to/libbytehound.so \
BYTEHOUND_OUT=/tmp/bh.data \
./your_program

# 查看
/path/to/bytehound --input /tmp/bh.data --listen 127.0.0.1:1789
```

## 仓库结构
- `examples/alloc_spike`：周期性大额分配/释放，方便观察瞬时内存峰值（C）。
- `examples/slow_leak`：缓慢泄漏型场景，方便观察持续增长的内存占用（C）。

## 如何运行示例（配合 Bytehound，C 版本）
假设 Bytehound 编译产物在 `/path/to/bytehound/target/release`。每个示例目录自带 `Makefile`，默认 `gcc -O0 -g`。示例使用一体化模式，直接生成 `.data`。

```bash
cd examples/alloc_spike
make
mkdir -p ../../recordings
LD_PRELOAD=/path/to/bytehound/target/release/libbytehound.so \
BYTEHOUND_OUT=../../recordings/alloc_spike.data \
./alloc_spike

cd ../slow_leak
make
LD_PRELOAD=/path/to/bytehound/target/release/libbytehound.so \
BYTEHOUND_OUT=../../recordings/slow_leak.data \
./slow_leak

# 采集完成后查看
/path/to/bytehound/target/release/bytehound --input ../../recordings/alloc_spike.data --listen 127.0.0.1:1789
/path/to/bytehound/target/release/bytehound --input ../../recordings/slow_leak.data --listen 127.0.0.1:1789
```

## 典型排查思路
- **瞬时峰值**：关注堆分配热点、调用栈，判断是否需要池化/复用。
- **慢性泄漏**：观察时间轴，确认常驻集合是否不断增长（缓存未淘汰、未关闭的资源等）。
- **线程维度**：查看线程标签/调用栈，排查具体线程的异常行为。
- **采样开销**：先在小规模/短时场景验证，再迁移到真实 workload。

后续可以在 `examples/` 中持续添加新的场景（例如碎片化、频繁 mmap/unmap 等），便于对照 Bytehound 的视图理解不同问题特征。
