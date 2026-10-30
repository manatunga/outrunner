# Outrunner

**Status:** Experimental, pre-v0.1 · **Implementation language:** C++20 · **License:** Apache 2.0

Outrunner is an experimental, vendor-independent GPU compute compiler and runtime project. Its goal is to understand and reproduce the essential mechanisms behind CUDA-like computing: kernel programming, compilation, memory management, dispatch, synchronization, and hardware execution.

Outrunner is **not** a CUDA binary-compatible implementation or a replacement GPU driver. Its first GPU backend is planned to use Vulkan compute and existing vendor drivers. CPU execution provides a reference path for correctness and development.

## v0.1 target

One restricted kernel definition can be compiled/translated and executed through Outrunner on a CPU and one compatible Vulkan GPU, using a shared host-facing API. The release demonstrates vector addition and matrix multiplication, validates outputs, and exposes basic timing and execution diagnostics. Multi-vendor GPU portability is an architectural goal, **not verified until tested on other vendors**.

## Project status

- Initial CPU vector-addition experiment completed, including memory layout and introductory assembly investigations.
- Reusable `outrunner::cpu::add_vector` kernel written; review identified an index-type correction (`std::size_t` instead of narrowing to `int`).
- **Current checkpoint:** OR-M1.2 — correct the loop index, compile, and test the reusable kernel.
- Compiler, runtime API, CPU dispatch backend, Vulkan backend, and kernel language are not yet implemented.

## Planned system

```text
Kernel source -> Frontend / IR -> Backend compilation
                                      |           |
                                   CPU path   SPIR-V path
                                      |           |
Host application -> Outrunner API -> Runtime -> CPU / Vulkan GPU
```

## Repository

- `include/` — public interfaces
- `src/` — production implementation
- `experiments/` — isolated learning and hardware investigations
- `tests/` — correctness and regression tests
- `benchmarks/` — performance experiments
- `docs/` — project specifications, decisions, progress, and learning

## Build status

No supported top-level build procedure exists yet. CMake integration is planned. Current standalone CPU kernel compilation, once the source is ready:

```bash
mkdir -p build
g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude -c src/cpu/vector_add.cpp -o build/vector_add.o
```

This compiles an object file; it does not run a test or produce a standalone executable.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Kernel language proposal](docs/LANGUAGE_SPEC.md)
- [Runtime contract proposal](docs/RUNTIME_SPEC.md)
- [Roadmap](docs/ROADMAP.md)
- [Design decisions](docs/DESIGN_DECISIONS.md)
- [Technical backlog](docs/TECHNICAL_BACKLOG.md)
- [Learning log](docs/LEARNING_LOG.md)
