# Outrunner

**Status:** Experimental, pre-v0.1 · **Implementation language:** C++20 · **License:** Apache-2.0

Outrunner is an experimental, vendor-independent GPU compute compiler and runtime project. Its goal is to understand and reproduce the essential mechanisms behind CUDA-like computing: kernel programming, compilation, memory management, dispatch, synchronization, and hardware execution.

Outrunner is **not** a CUDA binary-compatible implementation or a replacement GPU driver. Its first GPU backend is planned to use Vulkan compute and existing vendor drivers. CPU execution provides a reference path for correctness and development.

## v0.1 target

One restricted kernel definition can be compiled/translated and executed through Outrunner on a CPU and one compatible Vulkan GPU, using a shared host-facing API.

The release demonstrates vector addition and matrix multiplication, validates outputs, and exposes basic timing and execution diagnostics.

Multi-vendor GPU portability is an architectural goal, **not verified until tested on other vendors**.

## Project status

### Implemented

- Standalone CPU vector-addition experiment, including memory layout and introductory assembly investigations.
- Reusable sequential CPU vector addition through `outrunner::cpu::add_vector`.
- Reusable sequential CPU matrix multiplication through `outrunner::cpu::matmul`.
- Row-major matrix storage and rectangular matrix multiplication.
- CMake-based C++20 build configuration.
- Static CPU reference library: `outrunner_cpu`.
- Independent vector addition and matrix multiplication test executables.
- CTest integration with both test suites passing.

### Not yet implemented

- General CPU work-item execution and dispatch backend.
- Public runtime API.
- Device discovery and buffer management.
- Kernel language frontend and intermediate representation.
- CPU kernel compilation/lowering.
- Vulkan compute backend and SPIR-V generation.

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

## Build and test

### Requirements

- CMake 3.20 or newer
- A C++20-compatible compiler
- GNU Make or another supported CMake build tool

### Configure

From the repository root:

```bash
cmake -S . -B build
```

### Build

```bash
cmake --build build
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

The current test suite contains two independent test executables:

- `vector_add` — vector addition correctness tests.
- `matmul` — matrix multiplication correctness tests.

### Build individual targets

```bash
cmake --build build --target outrunner_cpu
cmake --build build --target test_vector_add
cmake --build build --target test_matmul
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Kernel language proposal](docs/LANGUAGE_SPEC.md)
- [Runtime contract proposal](docs/RUNTIME_SPEC.md)
- [Roadmap](docs/ROADMAP.md)
- [Design decisions](docs/DESIGN_DECISIONS.md)
- [Technical backlog](docs/TECHNICAL_BACKLOG.md)
- [Learning log](docs/LEARNING_LOG.md)
