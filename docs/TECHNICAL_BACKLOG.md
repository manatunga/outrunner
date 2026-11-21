# Technical Backlog

**Status:** Draft, 2026-11-21. Replaces the earlier backlog.

Ideas here are tracked rather than silently discarded. Items are **not** commitments to any version unless promoted to `ROADMAP.md`.

## Promotion rule

When moving an item into active work, add an owner, a milestone, an acceptance test, and a benchmark or diagnostic plan as applicable. Record any architectural consequence in `DESIGN_DECISIONS.md` as a new entry.

## Backlog

| ID | Item | Why deferred | Revisit trigger | Evidence / acceptance |
|---|---|---|---|---|
| B-001 | Native vendor fast paths (OpenCL or Level Zero, AMD and NVIDIA native routes) | The portable Vulkan baseline comes first (D-018) | Benchmarks show a gap on a named workload, or spike S1 shows a low Vulkan ceiling | Native path beats the baseline on that workload with correctness maintained; any NVIDIA-specific route cleared by the legal review |
| B-002 | Metal backend for Apple GPUs and macOS | No native Vulkan on macOS; beyond MVP platforms (D-035) | After the MVP | The Tier 1 workloads pass on an Apple GPU |
| B-003 | Tile-level constructs as an optional IR and language layer | Per-thread foundation first (D-015) | v0.2 tuning shows hand-written tiling is the main burden | A tile operation lowers to per-thread IR and matches hand-tuned results within a documented margin |
| B-004 | Standard algorithm library (scan, reduce, histogram) choosing the best technique per device | Needs the synchronization workloads and atomics first | After v0.3 | Each algorithm verified on every device, with a deterministic option |
| B-005 | Multi-device scheduling | Beyond the minimal runtime (D-022) | After the MVP | A two-device workload with explicit data placement and correct output |
| B-006 | Grid-wide synchronization and co-resident launch capability | No portable forward-progress guarantee (D-012) | A verified mechanism plus demand | Passes watchdog, verify-before-trust and device-lost recovery tests on every supported device |
| B-007 | Language bindings, Python first | C ABI must stabilize first (D-030) | v0.6 to v0.9, before v1.0.0 | Bindings pass a conformance subset against the stable C ABI |
| B-008 | AArch64 performance tuning and real-hardware testing | MVP is correctness-only on AArch64 (D-035) | After the MVP, or when an ARM machine is available | Tuned NEON paths benchmarked on real hardware; note that AsmJit does not support SVE or SME |
| B-009 | Matrix-assisted tier (cooperative matrix) | Optional layer (D-020); depends on spike S2 | S2 succeeds; v0.3 stretch | Verified speedup on the RTX 5070 with correctness maintained, reported as its own benchmark tier |
| B-010 | Own x86-64 and AArch64 encoders replacing AsmJit | AsmJit first (D-021) | CPU backend design stable | Equivalent generated code and performance, dependency removed |
| B-011 | Metadata-driven barrier scheme | Stretch; conservative baseline first (D-026) | v0.3 | Workloads agree under both policies in the harness; measurable benefit; validated on three vendors |
| B-012 | Differential fuzzing of race-free kernels | Needs a solid conformance suite (D-036) | After the conformance suite is stable | Fuzzer runs nightly and has found or ruled out backend disagreements |
| B-013 | Interpreter-based data-race detector, then debugger-style inspection of kernels on the CPU | Needs the interpreter (D-041) | v0.3 | Detector catches seeded races in a test corpus with no false positives on the conformance suite |
| B-014 | Roofline-guided pruning of tuning candidates | Needs measured-peak machinery (D-016) | v0.2 | Same best variant found with fewer candidates tried |
| B-015 | More numeric types (fp16, bf16, int8, 64-bit) | MVP focuses on float32 and int32 | After the MVP | Cross-backend conformance tests for each type |
| B-016 | Command-graph capture and launch batching beyond deferred lists | Batching first (D-025) | Small-kernel launch overhead measured as a bottleneck | Measurable overhead reduction on a small-kernel benchmark |
| B-017 | CUDA source porting tool | Provisional non-goal (D-002) | Legal review plus demand | Translates a defined CUDA subset to Outrunner source and passes its tests |
| B-018 | Discrete AMD and Intel GPU validation (for example rented cloud GPUs) | Integrated GPUs only on owned hardware | After the MVP, or when credibility needs it | Same suite passes on discrete AMD and Intel cards; differences documented |
| B-019 | Subgroup operations layer with per-vendor width handling | Optional layer (D-006) | After v0.3 | Portable subgroup API verified on three vendors |
| B-020 | Packaging and distribution beyond CI | CI first (D-037) | Before v1.0.0 | Reproducible release builds, documented install paths |
| B-021 | Compare Vulkan's reported shared-memory limits with what CUDA allows on the same GPU | Not yet measured | Spike S1 | Documented effect on achievable tile sizes |

## Open decisions and research (not engineering items)

| Item | Due |
|---|---|
| Legal review of the clean-room policy and CUDA-related wording | v0.5, before announcement or outside contributions |
| Contributor agreement: DCO or CLA | Before the first outside contribution |
| Trademark policy for the name | With the legal review |
| CUDA toolkit license terms on publishing benchmark comparisons | v0.2, before sharing numbers |
| NVIDIA machine-code inspection and annotated CUDA/PTX comparisons (earlier B-004 and B-011) | On hold until the legal review defines the boundary (D-038) |
| Research into PTX emission and driver terms | Before any NVIDIA-specific native path |

## What happened to the earlier backlog

| Earlier item | Now |
|---|---|
| B-001 multi-vendor validation | In `ROADMAP.md` v0.3; discrete cards in B-018 |
| B-002 GPU timing depth | `ROADMAP.md` v0.2 and D-033 |
| B-003 shared-memory optimization | `ROADMAP.md` v0.2 and v0.3 |
| B-004 native machine-code investigation | Open decisions (on hold); native paths in B-001 |
| B-005 advanced memory management | D-023 and D-024 |
| B-006 streams and events | D-025 |
| B-007 optimization passes | `ROADMAP.md` v0.2 |
| B-008 CPU SIMD and thread scheduling | D-021 and OR-M2 |
| B-009 more numeric types | B-015 |
| B-010 multi-GPU | B-005 |
| B-011 CUDA/PTX comparative research | Open decisions (on hold) |
| B-012 CI and packaging | D-037 and B-020 |
