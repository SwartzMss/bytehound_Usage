# Bytehound 使用示例

这个仓库用来演示如何用 Bytehound 在本地快速发现常见的内存问题，并附带可以直接运行的 C 示例程序。

## 环境要求
- Linux x86_64（Bytehound 目前仅支持 Linux）
- 已安装 Rust/Cargo（建议 1.72+）
- 能从源码编译 Bytehound（官方仓库），需要 `cmake` 等基础构建工具

## Bytehound 快速上手（参考流程）
> 具体参数可能随版本变化，请以 `bh_agent --help` / `bh_viewer --help` 为准。

1. **编译 Bytehound**
   ```bash
   git clone https://github.com/koute/bytehound.git
   cd bytehound
   cargo build --release -p bh_agent -p bytehound-preload -p bh_viewer
   ```
   生成物：`target/release/bh_agent`、`target/release/libbytehound.so`、`target/release/bh_viewer`。

2. **启动采集 Agent**
   ```bash
   ./target/release/bh_agent server --ipc /tmp/bytehound.sock --recordings ./recordings
   ```
   - `--ipc`：Agent 监听的本地 socket。
   - `--recordings`：采集数据保存目录。

3. **用 LD_PRELOAD 注入到待测程序**
   ```bash
   BYTEHOUND_SERVER=ipc:///tmp/bytehound.sock \
   BYTEHOUND_LOG=info \
   LD_PRELOAD=/path/to/bytehound/target/release/libbytehound.so \
   ./your_program
   ```
   运行结束后，`recordings` 目录会出现本次采集文件（文件名由 Agent 输出）。

4. **查看数据**
   ```bash
   ./target/release/bh_viewer --input recordings/<录制文件> --listen 127.0.0.1:1789
   # 浏览器打开 http://127.0.0.1:1789 交互查看
   ```

## 核心组件作用
- `bytehound-preload`（生成 `libbytehound.so`）：通过 `LD_PRELOAD` 劫持 `malloc/free` 等分配接口，把分配事件上报给 Agent。没有它就无法捕获目标进程的内存分配。
- `bh_agent`：采集和记录服务，接收来自被测进程的分配数据并写出录制文件。没有它就无法收集或保存数据。
- `bh_viewer`：本地 Web UI 分析器，读取录制文件并提供交互视图。没有它就不能可视化分析采集结果。

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

BYTEHOUND_SERVER=ipc:///tmp/bytehound.sock \
LD_PRELOAD=/path/to/bytehound/target/release/libbytehound.so \
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
