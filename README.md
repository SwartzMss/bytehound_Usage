# Bytehound 使用示例

这个仓库用来演示如何用 Bytehound 在本地快速发现常见的内存问题，并附带可以直接运行的 C 示例程序。

## 环境要求
- Linux x86_64（Bytehound 目前仅支持 Linux）
- 已安装 Rust/Cargo（建议 1.72+）
- 能从源码编译 Bytehound（官方仓库），需要 `cmake` 等基础构建工具

## 编译 Bytehound
> 具体参数可能随版本变化，请以 `bh_agent --help` / `bh_viewer --help` 为准。

```bash
git clone https://github.com/koute/bytehound.git
cd bytehound
cargo build --release -p bh_agent -p bytehound-preload -p bh_viewer
```
编译产物：`target/release/bh_agent`、`target/release/libbytehound.so`、`target/release/bh_viewer`。

## 核心组件作用
- `bytehound-preload`（生成 `libbytehound.so`）：通过 `LD_PRELOAD` 劫持 `malloc/free` 等分配接口，把分配事件上报给 Agent。没有它就无法捕获目标进程的内存分配。
- `bh_agent`：采集和记录服务，接收来自被测进程的分配数据并写出录制文件。没有它就无法收集或保存数据。
- `bh_viewer`：本地 Web UI 分析器，读取录制文件并提供交互视图。没有它就不能可视化分析采集结果。

## 运行模式
### 一体化模式（最简单）
- 适合单进程、临时分析、新手快速定位。
- 直接运行，被测进程退出后生成一个 `.data`。
```bash
LD_PRELOAD=/path/to/libbytehound.so \
BYTEHOUND_OUT=/tmp/bh.data \
./your_program

# 查看
bh_viewer --input /tmp/bh.data --listen 127.0.0.1:1789
```

### 分离式 Agent 模式（多进程/长时场景）
- Agent 常驻，可同时采集多个进程，数据结构化保存在 `--recordings`。
- 适合长跑服务、多 worker、压测/线上复现。
```bash
# 1) 启动 Agent
bh_agent server --ipc /tmp/bytehound.sock --recordings ./recordings

# 2) 被测进程用 LD_PRELOAD 注入并连到 Agent
BYTEHOUND_SERVER=ipc:///tmp/bytehound.sock \
LD_PRELOAD=/path/to/libbytehound.so \
./your_program

# 3) 查看
bh_viewer --input recordings/<录制文件> --listen 127.0.0.1:1789
```

## Agent 关键参数（工程语义）
- `bh_agent server`：启动采集服务端，等待 `bytehound-preload.so` 连接，相当于先开“内存事件收集中心”。
- `--ipc /tmp/bytehound.sock`：使用 Unix Domain Socket 做进程间通信，preload 通过此 socket 把事件发给 Agent。本地、快速、可控。
- `--recordings ./recordings`：采集数据写入该目录。一次采集一个子目录，如：
  ```
  recordings/
   └─ recording-2026-01-16T12-00-01/
        ├─ allocations.bin
        ├─ stacks.bin
        └─ meta.json
  ```
  这些是原始采集数据，后续交给 `bh_viewer` 查看。

## 仓库结构
- `examples/alloc_spike`：周期性大额分配/释放，方便观察瞬时内存峰值（C）。
- `examples/slow_leak`：缓慢泄漏型场景，方便观察持续增长的内存占用（C）。

## 如何运行示例（配合 Bytehound，C 版本）
假设 Bytehound 编译产物在 `/path/to/bytehound/target/release`。每个示例目录自带 `Makefile`，默认 `gcc -O0 -g`。

```bash
# 终端 1：启动 Agent
/path/to/bytehound/target/release/bh_agent server --ipc /tmp/bytehound.sock --recordings ./recordings

# 终端 2：编译 + 运行示例（选择其一）
cd examples/alloc_spike
make
BYTEHOUND_SERVER=ipc:///tmp/bytehound.sock \
LD_PRELOAD=/path/to/bytehound/target/release/libbytehound.so \
./alloc_spike

cd ../slow_leak
make
BYTEHOUND_SERVER=ipc:///tmp/bytehound.sock \
LD_PRELOAD=/path/to/bytehound/target/release/libbytehound.so \
./slow_leak

# 采集完成后查看
/path/to/bytehound/target/release/bh_viewer --input recordings/<录制文件> --listen 127.0.0.1:1789
```

## 典型排查思路
- **瞬时峰值**：关注堆分配热点、调用栈，判断是否需要池化/复用。
- **慢性泄漏**：观察时间轴，确认常驻集合是否不断增长（缓存未淘汰、未关闭的资源等）。
- **线程维度**：查看线程标签/调用栈，排查具体线程的异常行为。
- **采样开销**：先在小规模/短时场景验证，再迁移到真实 workload。

后续可以在 `examples/` 中持续添加新的场景（例如碎片化、频繁 mmap/unmap 等），便于对照 Bytehound 的视图理解不同问题特征。
