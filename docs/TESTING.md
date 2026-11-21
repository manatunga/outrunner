# Testing Strategy

**Status:** Draft, 2026-11-21. Describes the planned strategy; only the CPU reference tests exist today. Updated as components land.
**Related:** `DESIGN_DECISIONS.md` (D-011, D-022, D-026, D-036, D-037), `ARCHITECTURE.md`.

## 1. Goals

1. Catch compiler and runtime bugs early and cheaply.
2. Make "correct" mean the same thing on every device.
3. Keep fast feedback on every change, and run slow, hardware-bound checks on a cadence.
4. Use the same comparison machinery at test time and at runtime (the verify-before-trust harness, D-022).

## 2. Ground truth: two independent oracles

| Oracle | What it is | Used for |
|---|---|---|
| IR interpreter | A deliberately simple, slow executor of the IR's defined semantics | Compiler and backend tests; the harness |
| Hand-written C++ references | Sequential reference implementations (the existing `outrunner_cpu` kernels and later ones) | Benchmark workloads; independent of the compiler entirely |

The CPU JIT backend is never its own oracle. Differential testing compares the interpreter, the CPU backend and every GPU backend on the same inputs.

## 3. Agreement policy

| Result kind | Agreement required |
|---|---|
| Integer and bitwise | Exact |
| Strict-mode floating point | Bit-exact for operations hardware guarantees to be correctly rounded; ULP-bounded for looser ones (division, square root, transcendentals). The bounds live in `LANGUAGE_SPEC.md` |
| Fast-mode floating point | Within the tolerance the kernel declares |

A device that cannot meet strict denormal or NaN behavior is reported by verify-before-trust, not silently accepted (D-011).

## 4. Test layers

### 4.1 Unit tests
Lexer, parser, type checker, address-space rules, the barrier-divergence check, IR verifier, cache-key logic, versioned-IR reading, error categories. The allocator gets randomized allocate-and-free sequences that check no two live blocks overlap and that alignment holds.

### 4.2 Golden-file tests
Source to AST dump, IR text, optimized IR text and SPIR-V disassembly. Expected outputs are files in the repository. A change in output fails the test until the maintainer reviews the diff and blesses the new file with the project's bless command. Blessing is always an explicit, reviewed act.

### 4.3 Execution conformance suite
A corpus of kernels, each with a source, an input generator and an expected result from an oracle. Every kernel runs on the interpreter and every available backend, compared under section 3. Required coverage per kernel:

- sizes of zero, one, and non-multiples of the block size;
- grids larger than a device's per-dimension limit (exercises grid splitting);
- unaligned and offset buffers;
- special values (zeros, negative zero, infinities, NaNs, denormals) in strict mode.

Categories: memory-bound, compute-bound, synchronization-heavy (shared memory, barriers, atomics), numerics.

### 4.4 Runtime tests
Stream and event ordering; deferred freeing (memory is not reused while work is in flight); eager configuration validation; every error category; the barrier-policy comparison (same workloads under conservative and smarter policies, results compared); persistent-cache behavior (hit, miss, invalidation on driver change). **Fault injection:** test hooks simulate a lost device so the recovery policy is exercised without a real GPU crash.

### 4.5 Sanitizers and validation
ASan and UBSan on the compiler and runtime; TSan on the threaded CPU backend. GPU runs in debug configuration enable Vulkan validation layers, including **synchronization validation**, which catches missing barriers.

### 4.6 CPU backend code tests
Disassembly-based checks of generated instruction sequences for small kernels, plus execution tests against the interpreter. AArch64 code runs under emulation in CI.

### 4.7 Performance regression
A benchmark suite with stored baselines per device. Results are noisy, so it uses warmup, repetitions and medians. It runs on owned hardware (manually or nightly), reports trends, and does not block merges by default. Methodology is in `BENCHMARKING.md`.

### 4.8 Differential fuzzing (stretch)
Random kernels are generated and compared across all backends. Generated kernels must be **race-free by construction** (for example, each thread writes only its own elements), because a legitimate data race produces legitimately different results and would make every comparison meaningless.

## 5. Cadences

| Cadence | Runs | Where |
|---|---|---|
| Every pull request | Builds (GCC, Clang, MSVC), unit tests, golden tests, interpreter and CPU conformance, lavapipe-based Vulkan tests | Hosted CI |
| Nightly | Extended conformance, sanitizer builds, fuzzing (once it exists), AArch64 under emulation | Hosted CI |
| Per release (manual) | Real-GPU conformance on every owned device, performance suite, tuning-cache tests | Owned hardware, local script |

There is no self-hosted runner on the public repository (D-037).

## 6. Real-GPU runs

A local script runs the GPU suite on one machine and writes machine-readable results. Draft shape (finalized when the script is written):

```json
{
  "schema_version": 1,
  "commit": "<git hash>",
  "date": "<ISO 8601>",
  "os": "<name and version>",
  "device": {
    "vendor": "<vendor>",
    "name": "<device name>",
    "driver": "<driver version>",
    "api_version": "<Vulkan version>",
    "capabilities": { "advertised": [], "verified": [] }
  },
  "tests": [ { "name": "<test>", "status": "pass|fail|skip", "detail": "<optional>" } ]
}
```

Results from all devices feed a tested-devices matrix in the README.

## 7. Conventions

- **Layout:** `tests/unit`, `tests/golden`, `tests/conformance`, `tests/runtime`, with shared helpers in `tests/support`.
- **Runner:** CTest. A test framework is chosen at OR-M2 as a dev-only dependency.
- **Determinism:** random inputs use a recorded seed, printed on failure so any failure can be reproduced.
- **Naming:** `<component>_<behavior>`, for example `allocator_no_overlap`.
- **Skips are visible:** a test that cannot run on a device (for example no cooperative matrix) reports "skip" with a reason and never silently passes.

## 8. Definition of done for a milestone

- New behavior has tests at the right layers.
- Golden files are reviewed and blessed.
- The relevant cadence passes: pull-request tier in CI, plus a real-GPU run for any change to a GPU path.
- Documentation and `DESIGN_DECISIONS.md` are updated for any architectural consequence.

## 9. Current status

Exists today: CPU vector-add and matrix-multiplication correctness tests under CTest (the foundation of the second oracle). Everything else in this document is planned.
