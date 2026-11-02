# Roadmap

**Target:** Outrunner v0.1 — a small working CPU/GPU compiler and runtime.

**Status key:** Complete / In progress / Planned / Deferred.

| Milestone | Status | Deliverable | Exit criterion |
|---|---|---|---|
| M1 — CPU reference kernel foundation | In progress | Reusable vector addition and matrix multiplication, tests, CMake integration | Both kernels compile and pass correctness tests; documented interfaces and reproducible build |
| M2 — CPU execution backend | Planned | Buffers, logical work-item indexing, basic CPU dispatch | Launch an N-item kernel through a backend abstraction |
| M3 — Runtime API | Planned | Device, buffer, kernel, dispatch, sync interfaces | Host program runs a CPU kernel via public API |
| M4 — Vulkan GPU backend | Planned | Vulkan device, buffer, shader dispatch | Real GPU computes and returns correct vector addition |
| M5 — Unified execution | Planned | Shared host API across CPU/GPU | Select backend without rewriting host application |
| M6 — Tiny kernel compiler | Planned | Restricted frontend/IR and CPU/SPIR-V paths | One kernel definition runs on both backends |
| M7 — MVP demonstration | Planned | End-to-end demos, diagnostics, benchmarks | Vector add and matrix multiplication, CPU/GPU verification, inspectable execution |

## M1 — CPU reference kernel foundation

### OR-E1 — Standalone CPU vector addition [Complete]

- [x] Implement standalone CPU vector addition.
- [x] Investigate memory layout and element spacing.
- [x] Perform introductory assembly and addressing investigations.

### OR-M1.1 — Reusable vector addition [Complete]

- [x] Create public vector addition interface.
- [x] Implement reusable sequential CPU vector addition.
- [x] Use `std::size_t` for element indexing.

### OR-M1.2 — Build and vector addition tests [Complete]

- [x] Establish CMake project configuration.
- [x] Build the `outrunner_cpu` static library.
- [x] Create vector addition correctness tests.
- [x] Integrate the vector addition test executable with CTest.
- [x] Verify the test suite.

### OR-M1.3 — Matrix multiplication [Complete]

- [x] Create public matrix multiplication interface.
- [x] Implement sequential row-major matrix multiplication.
- [x] Support compatible rectangular matrix dimensions.
- [x] Create matrix multiplication correctness tests.
- [x] Cover zero-dimension behavior.
- [x] Integrate the matrix multiplication test executable with CTest.
- [x] Verify the complete test suite.

### OR-M1.4 — Final review and documentation [In progress]

- [ ] Review production source and public interfaces.
- [ ] Review correctness tests and test integration.
- [ ] Update project documentation.
- [ ] Perform clean build and final verification.
- [ ] Commit the completed milestone.

## Parallel research track

Start Vulkan device discovery and a minimal compute experiment early, while CPU runtime work continues. Do not defer all GPU learning until the CPU backend is feature-complete.

## v0.1 release gate

- [ ] One restricted kernel definition usable on CPU and compatible Vulkan GPU.
- [ ] Device discovery, buffers, transfers, dispatch, synchronization.
- [ ] Vector addition and matrix multiplication demonstrated.
- [ ] Correctness checks and basic timing/trace output.
- [ ] Clear build/run instructions and known limitations.

**Portability note:** NVIDIA GPU + CPU is the initial hardware acceptance target; AMD/Intel support remains unverified until tested.
