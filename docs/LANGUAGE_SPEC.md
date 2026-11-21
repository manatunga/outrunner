# Kernel Language Specification (Skeleton)

**Status:** Skeleton, 2026-11-22. Records decided points and lists open ones. Not yet a complete specification; the compiler milestone (OR-M6) fills it in.
**Related:** `DESIGN_DECISIONS.md` (D-006 to D-013), `ARCHITECTURE.md`.

**Section status:** *Decided* (an accepted decision exists), *Proposed* (a recommendation awaiting confirmation), *Open* (not yet discussed).

## 1. Design principles — Decided

- A small, own C-like language with C++-style syntax (D-008). It is deliberately smaller than C++: no classes, exceptions, standard library, recursion, dynamic allocation or (initially) templates.
- CUDA-shaped execution model with concepts kept familiar and surface details improved where objectively better (D-004, D-006).
- Every kernel has defined semantics that the IR interpreter can execute (`ARCHITECTURE.md`, invariant 3).
- Clear errors with source locations; an unsupported C++ feature is reported as such, never silently misparsed.

## 2. Execution model — Decided

- A launch has a grid of blocks; each block has threads.
- Threads in a block share fast shared memory and can synchronize with barriers.
- Blocks have no ordering and no portable forward-progress guarantee between them (D-012).
- Subgroup (warp-like) operations and matrix operations are an **optional** layer, never required by the core (D-006).

## 3. Types — Proposed

- MVP scalar types: 32-bit signed and unsigned integers, 32-bit float, bool. More types are in the backlog (B-015).
- Pointers carry an address space (section 4).
- Fixed-size arrays and simple structs with defined layout (needed for the host/device layout guarantee, D-009).
- **Open:** integer overflow semantics. Recommendation: defined two's-complement wrapping, for determinism across devices.
- **Open:** the complete list of conversions and promotions.

## 4. Address spaces — Decided

Four spaces, part of the type system (D-013): **global**, **shared** (workgroup), **constant**, **private**. A pointer's space is checked at compile time; mismatches are errors, not silent conversions.
**Open:** syntax for declaring shared memory and constants, and the rules for converting between pointer spaces.

## 5. Functions and control flow — Proposed

- Kernel entry functions, plus device helper functions that are inlined.
- `if`/`else`, `for`, `while`, `break`, `continue`, `return`.
- No recursion, function pointers or dynamic allocation.
- **Open:** exact grammar, loop-bound restrictions (if any), and the extension for kernel files.

## 6. Built-ins — Decided (names), Open (full list)

- `global_index()` replaces hand-written block-and-thread index arithmetic (D-010).
- Block, thread and grid indices and sizes; a grid-stride loop helper; barrier; atomics (section 8); `fma()` (section 7).
- **Open:** exact names and the 2D/3D index forms.

## 7. Floating-point semantics — Decided

- Contraction is part of the language: `a*b+c` written within a single expression is fused; never across statements; identical on every device (D-011).
- `fma()` is available explicitly.
- A per-kernel opt-in **fast** mode permits reassociation, flush-to-zero and approximate math, with a declared tolerance checked by the correctness harness.
- Strict mode requires preserved denormals and NaNs; devices that cannot provide this are reported by verify-before-trust.
- **Open:** the ULP bounds for division, square root and transcendental functions in strict mode (this section is where `TESTING.md` points); the attribute syntax for selecting the mode and declaring the tolerance.

## 8. Atomics and memory model — Decided

- Scoped, ordered atomics in the style of C++ `atomic_ref` (D-012).
- Scopes: block, device. Orders: relaxed, acquire, release, acq_rel, seq_cst. Default seq_cst.
- No `volatile`-and-fence idioms.
- 64-bit and float atomic add are optional, queryable capabilities with flagged fallbacks.
- Excluded from the MVP: grid-wide synchronization and inter-block spin-waiting. The compiler marks kernels with inter-block dependence.
- **Open:** the precise memory-model rules for shared and global memory, and the syntax for atomic types or operations.

## 9. Barriers and divergence — Decided (rule), Open (analysis)

- A barrier must be reached by every thread in the block (D-013).
- The compiler flags barriers in divergent control flow, conservatively; the exact analysis is **open**.

## 10. Tunable and specialization parameters — Proposed

- Block size, shared-memory size and user tuning parameters are specialization parameters that select cached pipeline variants (D-027, D-016).
- **Open:** how a kernel declares a tunable parameter, its legal range and its default.

## 11. Kernel metadata — Decided (contents), Open (format)

The compiler emits, per kernel: name, argument types and layouts, access hints (read-only, write-only, read-write), a marker for inter-block dependence, and the tolerance for fast-mode kernels (D-009, D-026).

## 12. Host interop — Decided (mechanism), Open (syntax)

- Single-source files are split by the driver; the compiler also generates a typed host header (D-009).
- Host/device structs have defined layout.
- **Open:** how a kernel is marked in a single-source file, and how kernel parameters map to host argument types.

## 13. Out-of-bounds and undefined behavior — Open

Recommendation: bounds checking in debug builds (D-010) and undefined behavior in release builds, with the list of undefined behaviors stated explicitly.

## 14. Versioning of the language — Proposed

A language version is declared per source file; the compiler rejects versions it does not support with a clear message (consistent with D-032).

## 15. Non-normative example

Illustrative only; syntax is not final.

```text
kernel void add(const float* a, const float* b, float* c, size_t n) {
    size_t i = global_index();
    if (i < n) c[i] = a[i] + b[i];
}
```

## 16. Open items summary

Integer overflow semantics; conversions; shared and constant declaration syntax; pointer-space conversion rules; grammar and file extension; built-in names; floating-point ULP bounds and mode syntax; memory-model details
