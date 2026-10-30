# Design Decisions

This log records **decisions**, **proposals**, and **open questions** separately. A proposal is not a binding architectural choice.

## D-001 — Build-first mentorship [Agreed]

**Decision:** The project owner implements production code. The assistant drafts/maintains documentation and guides, reviews, and explains experiments and implementation. Teach C++, assembly, GPU concepts, and compiler/runtime topics when needed by an active engineering task.

**Rationale:** Avoid indefinite prerequisite study and preserve hands-on understanding.

**Revisit if:** The owner explicitly changes collaboration preferences.

## D-002 — C++20 for host/runtime [Agreed direction]

**Rationale:** Direct access to memory, threading, low-level interfaces, and established Vulkan tooling.

**Consequence:** C++ language features are introduced incrementally while building.

**Revisit if:** Build portability or API constraints materially favor another choice.

## D-003 — CPU reference backend [Agreed direction]

**Rationale:** Enables correctness comparisons, isolated runtime tests, and experiments without a GPU.

**Consequence:** CPU and GPU must share defined kernel semantics.

## D-004 — Vulkan compute as first GPU path [Proposed; preferred]

**Rationale:** Cross-vendor GPU interface using existing drivers; avoids making CUDA a runtime dependency.

**Alternatives:** CUDA-specific backend, HIP, OpenCL, SYCL, native driver interfaces.

**Revisit at:** M4, after checking platform/toolchain and device capabilities.

## D-005 — Restricted kernel frontend and IR [Proposed]

**Rationale:** Makes compiler engineering manageable while preserving a path to multiple backends.

**Undecided:** Textual syntax, IR format, CPU lowering method, SPIR-V generation tooling.

**Revisit at:** M2/M3 interface design, finalize before M6 implementation.

## D-006 — No CUDA binary compatibility in v0.1 [Agreed scope]

**Rationale:** Focus on a working end-to-end compute stack rather than compatibility with NVIDIA-specific binary formats or proprietary drivers.

**Revisit at:** Post-v0.1 research planning.

## D-007 — Cross-vendor claims require evidence [Agreed]

**Decision:** Design for portability; verify on specific hardware before claiming vendor support. First release target is CPU plus one compatible Vulkan GPU.

## Open decisions

- Buffer ownership and error model.
- Dispatch/workgroup semantics and synchronization model.
- Kernel-language syntax and IR architecture.
- Compiler dependencies and generated-code inspection.
- CPU interpreter versus compiled kernel execution.

Each new decision should record its date, status, rationale, alternatives, consequences, and revisit trigger.
