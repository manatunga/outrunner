# Design Decisions

**Status:** Draft, 2026-11-20. Replaces the earlier decision log.
**Pending:** legal review of CUDA-related wording (see D-038).

## How to read this log

- **Status** is Accepted, Provisional (revisit when the stated trigger fires) or Superseded.
- **Basis** records where the idea came from, for clean-room provenance. "Original design" means devised for this project. Public specifications and documentation are named. CUDA is referenced only as public programming-model documentation, concepts only; no NVIDIA code, headers, SDK files or binaries are used.
- Decisions are never edited away. A changed decision gets a new entry that supersedes the old one.

---

## Purpose and principles

### D-001 Identity and clean-room rule
- **Status:** Accepted.
- **Decision:** Outrunner is a clean-room, vendor-neutral GPU compute stack (language, compiler, runtime). The CUDA programming model is a source of concepts to learn from, not something to copy.
- **Rationale:** Goal is near-native speed for any parallel workload from one codebase, without legal exposure.
- **Basis:** Original design; public CUDA programming-model documentation, concepts only.

### D-002 Non-goals
- **Status:** Accepted (clean-room rule permanent; other limits Provisional).
- **Decision:** Not a driver replacement. No CUDA source or binary compatibility for now (provisional; a separate porting tool is the preferred route if ever wanted).
- **Revisit when:** there is demand and the legal review permits.
- **Basis:** Original design.

### D-003 MVP by exit criteria
- **Status:** Accepted.
- **Decision:** The MVP is reached when three workload classes (memory-bound, compute-bound, synchronization-heavy) run correctly through a single-source program on the CPU and on the RTX 5070, the Ryzen 5 7600X integrated GPU (2 RDNA 2 CUs) and the Intel Core Ultra 7 155H integrated GPU, with published three-way benchmarks meeting targets. The memory-bound target is 90% or more of measured peak bandwidth; the compute-bound target is set after spike 1.
- **Basis:** Original design.

### D-004 "Like CUDA, but better"
- **Status:** Accepted.
- **Decision:** Deviate from CUDA when an alternative (1) removes a class of silent mistakes or boilerplate, (2) costs no performance, and (3) is learnable by a CUDA developer in minutes. Concepts stay familiar; surface details may differ.
- **Basis:** Original design.

### D-005 License and ambition
- **Status:** Accepted.
- **Decision:** Apache-2.0, fully open source, no monetization features planned. Proprietary forks are accepted as the price of adoption. Core code uses permissive licenses only.
- **Basis:** Original design.

---

## Kernel model and language

### D-006 Execution model
- **Status:** Accepted.
- **Decision:** CUDA-shaped core: grid, block and thread indexing, shared (workgroup) memory, barriers, global memory. Subgroup and matrix operations are an optional layer, never required by the core.
- **Basis:** Public CUDA, Vulkan and OpenCL programming-model documentation, concepts only.

### D-007 Transition levels
- **Status:** Accepted.
- **Decision:** Target concept-level and syntax-level familiarity. Compiling existing CUDA source is a provisional non-goal.
- **Basis:** Original design.

### D-008 Kernel language
- **Status:** Accepted.
- **Decision:** A small, own C-like kernel language with C++-style syntax. No classes, exceptions, standard library or (initially) templates. A Clang-based C++ subset is the fallback.
- **Basis:** Original design.

### D-009 Single-source in the MVP
- **Status:** Accepted.
- **Decision:** Host and kernel in one file via a source-splitting driver feeding the same kernel compiler. Separate-file and runtime-string-compile paths remain supported underneath. Five design constraints from day one: the compiler is a library; it emits kernel metadata; host/device types have defined layout; the launch API is typed; the kernel language parses independently of host context. The compiler also generates a typed host header per kernel file.
- **Basis:** Original design.

### D-010 Kernel-language ergonomics
- **Status:** Accepted (planned for v0.4).
- **Decision:** `global_index()`, grid-stride loop helpers, and bounds checking in debug builds.
- **Basis:** Original design.

### D-011 Floating-point semantics
- **Status:** Accepted.
- **Decision:** The language specifies contraction: `a*b+c` within a single expression is fused, never across statements, identically on every device. Explicit `fma()` exists. A per-kernel opt-in `fast` mode permits reassociation, flush-to-zero and approximate math, with a declared tolerance checked by the correctness harness. Strict denormal and NaN behavior is a capability checked by verify-before-trust.
- **Basis:** Original design; public IEEE 754 and Vulkan precision documentation.

### D-012 Atomics and memory model
- **Status:** Accepted.
- **Decision:** Scoped, ordered atomics in the style of C++ `atomic_ref` (scope: block or device; order: relaxed, acquire, release, acq_rel, seq_cst; default seq_cst). No legacy volatile-and-fence idioms. Optional features (64-bit, float atomic add) are queryable capabilities with flagged fallbacks. Grid-wide synchronization and inter-block spin-waiting are excluded from the MVP; the compiler marks kernels with inter-block dependence.
- **Revisit when:** a verified co-resident launch capability exists.
- **Basis:** Public ISO C++ atomics documentation; public Vulkan memory model documentation.

---

## Compiler

### D-013 Frontend
- **Status:** Accepted.
- **Decision:** Lexer, parser, AST, semantic analysis, lowering to IR. Address spaces (global, shared, constant, private) are part of the type system. A frontend check flags barriers in divergent control flow.
- **Basis:** Original design.

### D-014 IR infrastructure
- **Status:** Accepted.
- **Decision:** Hand-rolled, MLIR-shaped IR (SSA, operations with attributes, nested regions, stable text format), GPU-aware and high-level.
- **Revisit when:** infrastructure burden grows beyond what is maintainable, or several existing MLIR lowerings are needed.
- **Basis:** Original design; public MLIR and Triton documentation for layering ideas.

### D-015 IR semantics
- **Status:** Accepted.
- **Decision:** Per-thread (SIMT) foundation with explicit loops and memory spaces so patterns stay recognizable. Optional tile-level constructs may layer on later.
- **Basis:** Original design.

### D-016 Autotuning
- **Status:** Accepted.
- **Decision:** Tunable parameters and per-device autotuning are core. Roofline-guided pruning of candidates is a stretch item.
- **Basis:** Original design.

### D-017 Compilation model
- **Status:** Accepted.
- **Decision:** Hybrid: ahead-of-time to serialized, versioned Outrunner IR, then runtime specialization and autotuning on the target device with a persistent cache. Cache key: kernel IR hash, device identity, driver version, tuning parameters, compiler version.
- **Basis:** Original design.

---

## Backends

### D-018 Backend strategy
- **Status:** Accepted.
- **Decision:** Portable baseline first; native vendor fast paths added later only where benchmarks show gaps. Not locked to Vulkan or SPIR-V.
- **Basis:** Original design.

### D-019 GPU baseline
- **Status:** Accepted.
- **Decision:** Vulkan compute with shader-flavor SPIR-V, pointers via `bufferDeviceAddress`, an own SPIR-V emitter and thin Vulkan loader code. OpenCL and Level Zero are candidate native paths. Mandatory runtime checks: buffer device address and the other needed features.
- **Basis:** Vulkan and SPIR-V specifications.

### D-020 Matrix hardware
- **Status:** Accepted.
- **Decision:** Cooperative matrix and tensor hardware are an optional layer. Benchmarks report two tiers: ordinary shader-core and matrix-assisted.
- **Basis:** Public Vulkan cooperative-matrix extension documentation.

### D-021 CPU backend
- **Status:** Accepted (library choice Provisional pending prototype).
- **Decision:** A fast JIT backend, not only a reference. Uses AsmJit's `x86::Assembler` and `a64::Assembler` directly (not its Compiler layer) with own instruction selection and register allocation; own encoders are a later goal. Targets x86-64 (AVX2 baseline, AVX-512 optional) and AArch64 (NEON). Blocks map to threads; barriers become phase splits; lanes map to SIMD lanes. Runtime CPU-feature dispatch.
- **Basis:** AsmJit public documentation; public literature on SPMD-on-SIMD execution.

### D-022 Devices and selection
- **Status:** Accepted.
- **Decision:** The CPU is a first-class device in the same list as GPUs. Selection order: user choice, then default heuristic, then a fallback ladder (verified native fast path, Vulkan baseline, CPU). Verify-before-trust: a capability counts only after a correctness check and micro-benchmark, cached. Multi-device scheduling is deferred.
- **Basis:** Original design.

---

## Runtime

### D-023 Memory model
- **Status:** Accepted.
- **Decision:** Layered. A low-level explicit layer first, with named memory kinds (device, host, managed); copies are skipped automatically on shared-memory devices, with a query for sharing. A simple convenience buffer layer follows in v0.4.
- **Basis:** Public CUDA and Vulkan memory-management documentation, concepts only.

### D-024 Allocation
- **Status:** Accepted.
- **Decision:** Own sub-allocator behind a small interface (block free list with coalescing first; swappable later). Staging pool for discrete GPUs. CPU memory is 64-byte aligned. Device pointers are typed handles (device, kind, size) that convert explicitly to raw addresses for kernels.
- **Basis:** Original design.

### D-025 Streams and events
- **Status:** Accepted.
- **Decision:** In-order streams and events with a simple per-device default stream without legacy implicit synchronization. Streams are deferred command lists with batched submission.
- **Basis:** Public CUDA and Vulkan synchronization documentation, concepts only.

### D-026 Barrier policy
- **Status:** Accepted.
- **Decision:** Pluggable per stream. Conservative full barriers between dependent launches are the default and the correctness baseline. A metadata-driven scheme (access metadata, analyzable-kernel soundness check, hazard tracking) is a v0.1 stretch goal, validated against the baseline by the harness, with a debug toggle forcing conservative mode.
- **Basis:** Original design.

### D-027 Launch and dispatch
- **Status:** Accepted.
- **Decision:** Plain function-call launch API; no chevron syntax. Block size, shared-memory size and tuning parameters are specialization parameters selecting cached pipeline variants, with explicit specialization handles, prewarming and shared-memory size classes. Oversized grids are split transparently, except kernels marked as inter-block dependent, which error. Configurations are validated eagerly against device limits.
- **Basis:** Vulkan specification; original design.

### D-028 Errors
- **Status:** Accepted.
- **Decision:** Code-based C core and a custom C++20 `Result` type. The C++ wrapper throws by default, with a `Result`-returning variant. Categories: invalid argument, out of memory, unsupported feature, kernel compile error (with source locations), device lost, verification failure. A lost device is marked invalid, its pending work fails, and it can be recreated. Debug builds enable the runtime's own validation and Vulkan validation layers.
- **Basis:** Original design.

### D-029 Lifetime and threading
- **Status:** Accepted.
- **Decision:** Move-only RAII handles. Freed memory is reused only after all work submitted so far on the device completes. Explicit `Runtime` and `Device` objects, no hidden global context. Devices and allocator are thread-safe; a shared stream needs external synchronization.
- **Basis:** Original design.

---

## Host API

### D-030 Layering
- **Status:** Accepted.
- **Decision:** Stable C ABI core with a header-only C++ wrapper. Python and other bindings are deferred to the sub-versions before v1.0.0; the C layer is designed so they are cheap to add.
- **Basis:** Original design.

### D-031 Object model and naming
- **Status:** Accepted.
- **Decision:** Objects: Runtime, Device, Stream, Event, Buffer<T>, Module, Kernel, plus specialization handles. The Runtime owns device enumeration, tuning-cache location, logging and the Vulkan library, with `outrunner::default_runtime()` as a convenience. C++: PascalCase types, snake_case functions, namespace `outrunner` (never `or`, a reserved word). C: prefix `outrunner_`, snake_case.
- **Basis:** Original design.

### D-032 Versioning and ABI
- **Status:** Accepted.
- **Decision:** Semantic versioning; no stability promises in 0.x; stable C ABI within a major version from v1.0.0. Opaque handles; size-prefixed configuration and info structs (fields only appended); API version negotiation at runtime creation; an experimental namespace for incubating features; a versioned serialized-IR format (newer reads older; older rejects newer with a clear error).
- **Basis:** Original design.

### D-033 Diagnostics
- **Status:** Accepted.
- **Decision:** Event timestamps and a timing helper; a callback-based logger with levels; device info separating driver-advertised from verified capabilities; an "explain" trace (why a device or path was chosen, tuning-cache hit or miss, which variant ran); Chrome trace JSON export. Off by default, near-zero cost when disabled.
- **Basis:** Original design.

---

## Build, test and process

### D-034 Build and dependencies
- **Status:** Accepted.
- **Decision:** CMake and C++20. Vendored, pinned dependencies: AsmJit source (Zlib) and the header-only Khronos Vulkan and SPIR-V headers. No LLVM. No Vulkan SDK needed to build; the runtime loads the driver's Vulkan library. Dev-only dependencies: a test framework and the SPIR-V validator.
- **Basis:** AsmJit and Khronos public documentation and licenses.

### D-035 Platforms
- **Status:** Accepted.
- **Decision:** Tier 1: Linux x86-64 (GCC, Clang) and Windows x86-64 (MSVC primary). Tier 2: Linux AArch64, correctness-only via CI or emulation. Later: macOS with a Metal backend. Code must build on MSVC from the start. GPU validation and benchmarks run natively, not under WSL2.
- **Basis:** Original design.

### D-036 Testing
- **Status:** Accepted.
- **Decision:** Two independent oracles: a deliberately simple IR interpreter, and hand-written C++ reference implementations. Differential testing across the interpreter, CPU backend and all GPU backends. Pyramid: unit tests; golden-file tests (AST, IR, SPIR-V); execution conformance suite; runtime tests including fault injection; sanitizers and Vulkan validation layers (including synchronization validation); performance regression on owned hardware; differential fuzzing of race-free kernels as a stretch goal. Agreement tiers: integers and bitwise exact; strict floating point bit-exact where hardware guarantees correct rounding, otherwise ULP-bounded; fast mode within the declared tolerance.
- **Basis:** Original design.

### D-037 CI
- **Status:** Accepted.
- **Decision:** Three cadences: per pull request (builds, unit and golden tests, CPU conformance, lavapipe-based Vulkan tests); nightly (extended conformance, sanitizers, fuzzing); per release, manual (real-GPU matrix, performance, cache tests). Software Vulkan (Mesa lavapipe) for GPU-less CI, conditional on a feature check. No self-hosted runner on the public repository; GPU tests run through a local script producing JSON results that feed a tested-devices matrix.
- **Basis:** Original design.

### D-038 Clean-room process
- **Status:** Accepted.
- **Decision:** Written clean-room policy; provenance log (this document's Basis lines); CI scan for banned NVIDIA files and includes, license headers and dependency notices; permissive-only licenses in core; careful wording that Outrunner is inspired by the CUDA programming model and implies no affiliation or compatibility. A one-time software-IP legal review precedes the public announcement and any outside contribution. The contributor agreement (DCO or CLA) is decided before the first outside contribution. Inspecting NVIDIA machine code or annotated CUDA/PTX comparisons are on hold until the review.
- **Basis:** Original design. This is a process, not legal advice.

### D-039 Benchmark methodology
- **Status:** Accepted.
- **Decision:** Three-way comparison per device: vendor-native best, hand-written Vulkan, Outrunner. Two metrics: percent of vendor-native and percent of measured peak. Rigor: warmup, repetitions with median and variance, thermal and clock awareness, cold start reported separately from steady state, transfers separate, correctness check before timing, problem-size sweep, recorded driver versions, published scripts and raw data. CUDA baseline code lives in a separate repository, outside the core and its releases.
- **Basis:** Original design.

### D-040 Sequencing
- **Status:** Accepted.
- **Decision:** Vertical slices, preceded by four spikes: (1) hand-written Vulkan matmul versus vendor-native on the RTX 5070; (2) cooperative-matrix query on the RTX 5070 driver; (3) AsmJit prototype on x86-64 and AArch64; (4) lavapipe feature check. Inside v0.1, runtime and backends come first with hand-written kernels, and the compiler after. Version ladder in `ROADMAP.md`.
- **Basis:** Original design.

### D-041 Debugging aids
- **Status:** Accepted (stretch).
- **Decision:** An interpreter-based data-race detector is a v0.3 stretch item; debugger-style kernel inspection on the CPU follows later.
- **Basis:** Original design.
