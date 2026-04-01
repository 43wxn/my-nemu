# C 测试文件一键测试说明

本文档说明如何对仓库根目录下 `test_*.c`（例如 `test_add.c`, `test_fact.c`, `test_fib.c`, `test_if_else.c`）进行自动化测试，并把结果保存到 `result/` 目录。

## 1. 本项目的 C 测试方式结论

本仓库是 NEMU/AM/Nanos-lite 工程。标准 PA 流程中，C 测试通常会通过 AbstractMachine 工具链编译为目标 ISA（二进制镜像），再由 NEMU 执行。

但当前仓库中并未包含完整的 `am-kernels/tests/cpu-tests` 目录与 `trap.h` 测试头文件实现，因此根目录新增的 4 个 `test_*.c` 不能直接使用原 PA 流程开箱运行。

因此这里采用“本地 C 程序测试脚本”方式：

- 用 `gcc` 编译每个 `test_*.c`；
- 使用 `test_support/trap.h` 提供 `check(...)` 宏；
- 逐个运行可执行文件并记录耗时、退出码、PASS/FAIL；
- 保存单测详情文件与一份总表（CSV）。

## 2. 一键运行

在仓库根目录执行：

```bash
bash scripts/run_c_tests.sh
```

## 3. 输出结果

每次运行使用 UTC 时间戳作为 `run_id`（例如 `20260401T120000Z`），因此多次运行可区分。

输出在 `result/`：

- `summary_<run_id>.csv`：总表（每个测试一行）。
- `<test_name>_<run_id>.txt`：单个测试详情。

每个详情文件包含：

- 测试文件名与构建命令；
- 构建是否成功；
- 执行时间（毫秒）；
- 程序退出码；
- PASS/FAIL 结果；
- 程序输出（stdout/stderr）。

## 4. 字段说明

- `pass_fail`：若输出包含 `CHECK_FAIL:` 或程序被信号终止（退出码 >= 128）记为 `FAIL`，否则记为 `PASS`。
- `computed_result`：当前记录程序退出码（与 `exit_code` 一致），作为程序计算结果的统一可比输出。
- `elapsed_ms`：单个测试进程运行耗时（毫秒）。

## 5. 可选：如果未来补齐 AM/PA 测试框架

若你后续把 `am-kernels/tests/cpu-tests` 及原始 `trap.h` 引入仓库，可以进一步改造脚本为“交叉编译到 RISC-V 并通过 NEMU 执行”的流程。
