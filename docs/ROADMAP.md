# Roadmap

**Status:** Draft, 2026-11-21. Replaces the earlier roadmap. This is an order of work, not a schedule; contents shift as spikes report.
**Related:** `DESIGN_DECISIONS.md` (D-003, D-040), `ARCHITECTURE.md`, `TECHNICAL_BACKLOG.md`.

**Status key:** Complete / In progress / Planned / Stretch / Deferred.

## Principles

- **Vertical slices.** Every milestone runs a real kernel end to end on a real device, and each slice widens the system.
- **Risk first.** Four small spikes test the assumptions the strategy rests on before the main build.
- **Runtime first inside v0.1.** Runtime and backends are built with hand-written kernels, and the compiler follows, so two large risks are never debugged at once.
- **The MVP is defined by exit criteria** (D-003), not by a feature list.

## Foundation (CPU reference kernels)

| ID | Item | Status |
|---|---|---|
| OR-E1 | Standalone CPU vector addition, memory-layout and introductory assembly investigations | Complete |
| OR-M1.1 | Reusable vector addition, `std::size_t` indexing | Complete |
| OR-M1.2 | CMake project, `outrunner_cpu` library, vector-add tests, CTest | Complete |
| OR-M1.3 | Sequential row-major matrix multiplication and tests | Complete |
| OR-M1.4 | Final review and documentation of the foundation | In progress |

These hand-written kernels become the independent reference implementations (second oracle, D-036).

## Spikes (before the main build)

Throwaway experiments, kept under `experiments/`. Each ends with a learning-log entry recording numbers, driver versions and the conclusion.

| ID | Question | Output | Decision it informs |
|---|---|---|---|
| S1 | How close can a hand-written Vulkan compute matmul get to vendor-native on the RTX 5070? | Measured percent of native and of measured peak across tile sizes | The compute-bound MVP target (D-003); whether native fast-path research moves earlier |
| S2 | How does the RTX 5070 driver behave for cooperative-matrix queries? | Supported types and sizes; any crash or failure, with driver version | Whether the matrix-assisted tier is feasible (D-020) |
| S3 | Can AsmJit's assemblers carry the CPU backend on x86-64 and AArch64 (QEMU)? | A hand-assembled vector-add loop running on both; a note on the register-allocation approach | Confirms D-021 |
| S4 | Does lavapipe support `bufferDeviceAddress` and the other required features? | A feature report | Whether GPU-less CI covers the Vulkan path (D-037) |

Rule: if S1 shows the Vulkan ceiling is far below native, native fast-path research is scheduled before v0.2 and D-018 is revisited.

## Version ladder

### v0.1 Walking skeleton
Target: vector add and naive matmul run correctly on the x86-64 CPU backend and on one Vulkan GPU, through the public API, validated against the references, with basic timing.

| ID | Milestone | Exit criterion |
|---|---|---|
| OR-M1 | Reference kernel foundation (above) | Kernels and tests pass; clean build documented |
| OR-M2 | Runtime skeleton and CPU device | Runtime, Device, Buffer and Stream exist; a hand-written kernel runs over a grid via a thread pool; basic allocator |
| OR-M3 | C ABI and C++ wrapper, first cut | A host program drives the CPU device through the public API |
| OR-M4 | Vulkan device, buffers and dispatch | Real GPU computes correct vector add from a pre-built shader; capability descriptor reports `bufferDeviceAddress`; lavapipe tests in CI |
| OR-M5 | Unified execution | The same host program selects CPU or GPU; copies skipped on shared-memory devices; basic event timing |
| OR-M6 | Kernel language, frontend, IR, interpreter | Vector add and matmul parse, verify, and run in the interpreter, matching the references |
| OR-M7 | Backends from IR | One kernel definition runs on both backends via a SPIR-V emitter and a first CPU code generator |
| OR-M8 | v0.1 demonstration | Both workloads validated on CPU and GPU, timing and diagnostics, clear build and run instructions, known limitations listed |

CI starts at OR-M2 with builds and unit tests, and grows as components arrive (D-037). The local GPU test script producing JSON results arrives by OR-M8.

### v0.2 Performance machinery
Specialization variants, autotuning, the persistent cache, the verify-before-trust harness, fast-math opt-in, a tuned matmul and a memory-bound kernel, first three-way benchmarks on the RTX 5070 (baseline repository created; CUDA toolkit terms on publishing comparisons checked first; Windows GPU watchdog respected when sizing kernels). Stretch: roofline-guided pruning.
**Exit:** the memory-bound kernel reaches the D-003 target on the RTX 5070; tuning results persist and are re-verified after a driver change.

### v0.3 Synchronization and portability
Third workload (reduction or stencil) with shared memory, barriers and atomics; pluggable barrier policy; all three GPUs plus CPU; AArch64 correctness through CI; lavapipe in CI. Before the atomic lowering: `vulkaninfo` check of the memory model, 64-bit atomics and float atomics on all devices. Stretch: smarter barrier scheme, differential fuzzing, interpreter-based race detector.
**Exit:** all three workloads correct on the CPU and all three GPUs; agreement tiers (D-036) met.

### v0.4 Developer experience
Single-source driver, polished diagnostics including the explain trace, error-message quality, `global_index()` and other ergonomics, the convenience buffer layer, hardened C ABI and C++ wrapper.
**Exit:** a CUDA developer can write the vector-add example from the quickstart without help.

### v0.5 MVP gate
All MVP exit criteria met (D-003); benchmarks and raw data published; clean-room documentation complete; legal review done; contributor agreement decided; trademark policy for the name; tutorials and a CUDA-to-Outrunner migration guide; public announcement.

### v0.6 to v0.9 Beyond the MVP
Driven by benchmark evidence: native fast paths where the portable route falls short, the matrix-assisted tier, tile-level constructs, the standard algorithm library, AArch64 tuning, a Metal backend, and language bindings in the last of these sub-versions.

### v1.0.0 Stable release
Stable C ABI, complete specifications, a published conformance suite, multi-vendor results.

## Gates and due points

| Item | Due |
|---|---|
| Reword the repository description away from "Tiny CUDA" | Now |
| Spikes S1 to S4 | Before OR-M2 starts in earnest (S4 before OR-M4) |
| Separate baseline repository; CUDA toolkit license terms on publishing comparisons | v0.2, before sharing numbers |
| `vulkaninfo` memory-model, 64-bit atomics, float atomics | v0.3, before the Vulkan atomic lowering |
| Legal review; contributor agreement; trademark policy | v0.5, before announcement or outside contributions |
| NVIDIA machine-code inspection and annotated CUDA/PTX comparisons | On hold until the legal review (D-038) |

## Changing this roadmap

Backlog items are promoted into a version by adding an owner, a milestone, an acceptance test and a benchmark or diagnostic plan, and recording any architectural consequence in `DESIGN_DECISIONS.md`.
