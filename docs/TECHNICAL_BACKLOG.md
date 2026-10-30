# Technical Backlog

Ideas here are tracked rather than silently discarded. Items are **not** commitments to v0.1 unless promoted to the roadmap.

| ID | Item | Why deferred | Revisit trigger | Evidence / acceptance |
| --- | --- | --- | --- | --- |
| B-001 | Multi-vendor GPU validation | Initial hardware target is one Vulkan GPU | M7 completed / access to AMD or Intel GPU | Same kernel/tests pass on another vendor; document differences |
| B-002 | GPU timing and profiling depth | First establish correctness and basic dispatch | M4 GPU path works | Repeatable GPU timestamp-based timing, distinguish transfer/compute costs |
| B-003 | Workgroup/shared-memory optimization | Requires stable baseline semantics | M7 correctness complete | Benchmarked speedup with correctness maintained |
| B-004 | Native NVIDIA/AMD machine-code investigation | Vulkan path uses vendor drivers/compilers | Post-v0.1 compiler research | Document actual compilation layers and inspect available artifacts |
| B-005 | Advanced memory management | Basic explicit buffers sufficient for first demo | After M4 transfer implementation | Tests for lifetimes, reuse, transfer overhead, and errors |
| B-006 | Asynchronous dispatch, streams/events | Adds concurrency and lifetime complexity | After synchronous MVP | Ordering and synchronization tests; measurable overlap where supported |
| B-007 | Kernel optimization passes | Need stable IR and baseline benchmarks | After M6 compiler works | Before/after IR, correctness suite, measured effect |
| B-008 | CPU SIMD/thread scheduling | Basic CPU backend first | After M2 functional dispatch | Compare serial, SIMD, and threaded variants on representative sizes |
| B-009 | More numeric types | v0.1 focuses on simple float32 workloads | After M7 | Cross-backend conformance tests for each new type |
| B-010 | Multi-GPU execution | Beyond minimal runtime | Post-v0.1 | Two-device workload with explicit data placement and correct output |
| B-011 | CUDA/PTX comparative research | Research track, not initial runtime dependency | During M4–M6 investigations | Annotated comparisons of launch, memory, IR and generated artifacts |
| B-012 | Robust packaging/CI | Initial build and test path still forming | M1 test/CMake complete | Reproducible clean builds and automated tests |

## Promotion rule

When moving a backlog item into active work, add an owner, milestone, acceptance test, and benchmark/diagnostic plan as applicable. Record any architectural consequences in [Design Decisions](DESIGN_DECISIONS.md).
