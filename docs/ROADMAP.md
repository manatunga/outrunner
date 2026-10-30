# Roadmap

**Target:** Outrunner v0.1 — a small working CPU/GPU compiler and runtime.

**Status key:** Complete / In progress / Planned / Deferred.

| Milestone | Status | Deliverable | Exit criterion |
| --- | --- | --- | --- |
| M1 — CPU kernel foundation | In progress | Reusable vector-add kernel; basic tests and build | Correct outputs for multiple lengths, including zero; no narrowing of size indices |
| M2 — CPU execution backend | Planned | Buffers, logical work-item indexing, basic CPU dispatch | Launch an N-item kernel through a backend abstraction |
| M3 — Runtime API | Planned | Device, buffer, kernel, dispatch, sync interfaces | Host program runs a CPU kernel via public API |
| M4 — Vulkan GPU backend | Planned | Vulkan device, buffer, shader dispatch | Real GPU computes and returns correct vector addition |
| M5 — Unified execution | Planned | Shared host API across CPU/GPU | Select backend without rewriting host application |
| M6 — Tiny kernel compiler | Planned | Restricted frontend/IR and CPU/SPIR-V paths | One kernel definition runs on both backends |
| M7 — MVP demonstration | Planned | End-to-end demos, diagnostics, benchmarks | Vector add and matrix multiplication, CPU/GPU verification, inspectable execution |

## Current checkpoint — OR-M1.2

- [x] OR-E1: standalone CPU vector addition.
- [x] OR-E2: investigate array addresses and element spacing.
- [x] OR-E3/E4: introductory comparison/flags and addressing investigations.
- [x] OR-M1.1: reusable `add_vector` header/source drafted and reviewed.
- [ ] Replace narrowed `int` loop index with `std::size_t`.
- [ ] Compile kernel into object file and resolve editor/build issues.
- [ ] Write and run `tests/test_vector_add.cpp`.
- [ ] Test empty, small, and larger input arrays.
- [ ] Introduce minimal CMake build after manual compile/link verification.

## Parallel research track

Start Vulkan device discovery and a minimal compute experiment early, while CPU runtime work continues. Do not defer all GPU learning until the CPU backend is feature-complete.

## v0.1 release gate

- [ ] One restricted kernel definition usable on CPU and compatible Vulkan GPU.
- [ ] Device discovery, buffers, transfers, dispatch, synchronization.
- [ ] Vector addition and matrix multiplication demonstrated.
- [ ] Correctness checks and basic timing/trace output.
- [ ] Clear build/run instructions and known limitations.

**Portability note:** NVIDIA GPU + CPU is the initial hardware acceptance target; AMD/Intel support remains unverified until tested.
