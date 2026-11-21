# Architecture

**Status:** Draft, 2026-11-21. Describes the planned design; implementation has not started beyond the CPU reference kernels. Updated as components land.
**Reasoning:** every choice here traces to an entry in `DESIGN_DECISIONS.md` (cited as D-0xx).

## 1. What Outrunner is

A clean-room, vendor-neutral GPU compute stack: a small kernel language, a compiler, and a runtime that run one kernel fast on CPUs and on GPUs from different vendors (D-001). It borrows the CUDA programming model's concepts, improves its ergonomics where that is objectively better (D-004), and is fully open source under Apache-2.0 (D-005).

## 2. System overview

```text
 single-source file (host + kernels)
            |
     source-splitting driver ----> generated typed host header (D-009)
            |
       kernel text
            |
   Frontend: lex, parse, AST, semantic analysis (D-013)
            |
   High-level GPU-aware IR (hand-rolled, MLIR-shaped) (D-014, D-015)
            |   serialized, versioned artifact (D-017, D-032)
            v
 +------------------------------ Runtime ------------------------------+
 |  Runtime object: device list, tuning cache, logging (D-031)         |
 |  Device: capabilities, verification (D-022)                         |
 |  Memory: kinds, sub-allocator, staging (D-023, D-024)               |
 |  Streams and events: deferred command lists (D-025, D-026)          |
 |  Launch: specialization variants, grid splitting (D-027)            |
 |  Tuning: autotuner, persistent cache, correctness harness (D-016)   |
 +----------+------------------------------------+---------------------+
            |                                    |
      CPU backend (D-021)                 Vulkan backend (D-019)
      AsmJit assemblers, JIT              SPIR-V emitter, bufferDeviceAddress
            |                                    |
      x86-64 / AArch64 cores            GPU via vendor driver
```

Public surface: a stable C ABI with a header-only C++ wrapper on top (D-030).

## 3. Components

### 3.1 Kernel language and frontend
A small C-like language with C++-style syntax (D-008). The frontend carries address spaces (global, shared, constant, private) in the type system and checks for barriers in divergent control flow (D-013). It emits kernel metadata: name, argument types and layouts, access hints, and a marker for inter-block dependence (D-009, D-012, D-026). Floating-point behavior is defined by the language (D-011).

### 3.2 Source-splitting driver
Extracts kernel blocks from a single-source file, sends them to the compiler library, replaces them with generated launch stubs, and passes the rest to the host C++ compiler (D-009). The compiler is a library with a clean entry point, so the driver, runtime string compile and tests all call the same code.

### 3.3 IR
A hand-rolled, SSA, MLIR-shaped IR with nested regions, attributes and a stable text format (D-014). Per-thread (SIMT) semantics, with loops and memory spaces explicit so patterns stay recognizable (D-015). Includes a verifier and a versioned serialized form (D-032). The IR is the only interface between the compiler and the backends.

### 3.4 IR interpreter
A deliberately simple, slow executor of the IR's defined semantics. It is the ground truth for compiler tests (D-036). A data-race detector is a planned extension (D-041).

### 3.5 CPU backend
A fast JIT backend (D-021): AsmJit assemblers with own instruction selection and register allocation. A thread block becomes a loop over its threads, split into phases at barriers; lanes map onto SIMD lanes; blocks are distributed across a thread pool. Code paths are chosen at runtime by CPU features (AVX2 baseline, AVX-512 optional, NEON on AArch64).

### 3.6 Vulkan backend
Emits shader-flavor SPIR-V with pointers via `bufferDeviceAddress`, using an own emitter and thin loader code (D-019). Block size, shared-memory size and tuning parameters are baked into pipelines at creation (D-027). Cooperative-matrix paths are optional and only used when verified (D-020).

### 3.7 Runtime
- **Runtime object** owns device enumeration, the tuning-cache location, logging and the loaded Vulkan library (D-031).
- **Device** exposes a capability descriptor separating advertised from verified capabilities (D-022, D-033).
- **Memory:** named kinds, an own sub-allocator behind an interface, a staging pool for discrete GPUs, typed handles (D-023, D-024). Freed memory is reused only after all work submitted so far completes (D-029).
- **Streams and events:** in-order streams as deferred command lists with batched submission (D-025). A pluggable barrier policy, conservative by default (D-026).
- **Launch:** validates the configuration eagerly, resolves a specialization variant from the cache, splits oversized grids, records the command (D-027).
- **Errors:** categories, a device-lost policy, and debug validation (D-028).

### 3.8 Tuning and verification
The autotuner searches tunable parameters per device. Results go into a persistent cache keyed by kernel IR hash, device identity, driver version, parameters and compiler version (D-016, D-017). The correctness harness checks every candidate and every newly trusted capability against the oracles. It is the same machinery used by the test suite (D-022, D-036).

### 3.9 Public API
C ABI core: opaque handles, size-prefixed structs, version negotiation, an experimental namespace (D-032). C++ wrapper: typed handles, a function-call launch API, exceptions by default with a `Result` variant (D-027, D-028).

### 3.10 Diagnostics
Event timestamps, a callback logger, an "explain" trace, and Chrome trace export. Off by default (D-033).

## 4. Key flows

**Build time:** source, splitter, frontend, IR, serialized artifact (embedded or separate file).

**First launch of a kernel on a device:**
1. Runtime loads the IR and reads the device's capability descriptor.
2. Specialization: device facts and chosen parameters are applied; the backend generates SPIR-V or machine code.
3. If tuning is enabled, candidates run, are checked against the oracles, and the best is recorded in the cache.
4. The pipeline or code is cached in memory and on disk.

**Every launch:** validate, look up the variant, record on the stream, apply the barrier policy, submit in batches, signal events.

**Device selection:** user choice, then default heuristic, then the fallback ladder: verified native fast path, Vulkan baseline, CPU (D-022).

## 5. Dependency rules

- API depends on Runtime; Runtime depends on Backends and the IR; Backends depend on the IR only.
- The compiler (frontend, IR, passes) never depends on the runtime.
- The interpreter depends on the IR only.
- Tests and benchmarks may depend on everything; nothing depends on them.
- External dependencies are limited to vendored AsmJit and the Khronos headers; no LLVM, no Vulkan SDK (D-034).

## 6. Invariants

1. No backend trusts an advertised capability until it is verified (D-022).
2. All device-specific values flow through the capability descriptor.
3. Every kernel has defined semantics executable by the interpreter.
4. Strict-mode floating point is identical across devices up to the documented tolerances (D-011, D-036).
5. Freed memory is never reused while submitted work may still touch it (D-029).
6. Nothing derived from NVIDIA code, headers or binaries enters the repository (D-001, D-038).

## 7. Out of scope for the MVP

Native vendor fast paths, Metal, tile-level constructs, the standard algorithm library, multi-device scheduling, grid-wide synchronization, language bindings, AArch64 tuning (see `ROADMAP.md` and `TECHNICAL_BACKLOG.md`).

## 8. Open questions

- The IR's operation set and the exact number of IR levels (shaped by the backends).
- The kernel language's detailed grammar, types and built-ins (`LANGUAGE_SPEC.md`).
- AsmJit register-allocation approach on both architectures (spike 3).
- Whether lavapipe supports the required Vulkan features (spike 4).
- Achievable compute-bound performance through Vulkan (spike 1).

## 9. Repository layout (non-binding)

```text
include/outrunner/        public headers (C ABI, C++ wrapper)
src/lang/                 lexer, parser, semantic analysis
src/ir/                   IR, verifier, text and serialization
src/interp/               IR interpreter (oracle)
src/backend/cpu/          CPU JIT backend
src/backend/vulkan/       SPIR-V emitter, Vulkan runtime glue
src/runtime/              Runtime, devices, memory, streams, tuning
tools/                    driver, scripts, GPU test runner
tests/                    unit, golden, conformance, runtime tests
benchmarks/               benchmark harness (CUDA baselines live elsewhere)
third_party/              vendored, pinned dependencies
experiments/              isolated learning and hardware investigations
docs/                     this documentation
```
