# Learning Log

Record experiments, commands, observed output, corrections, and the direct implication for Outrunner. This log is for learning evidence, not speculative architecture.

## OR-E1 — CPU vector addition [Complete]

**Experiment:** C++ arrays and a loop computed `[6, 8, 10, 12]` from `[1, 2, 3, 4] + [5, 6, 7, 8]`.

**Observation:** GCC and Clang compiled the standalone experiment at `-O0` with standard warnings enabled.

**Outrunner connection:** Establishes baseline computation later expressed as a reusable kernel and parallel dispatch.

## OR-E2 — Array layout [Complete]

**Observed:** Four-element `int` array used 16 bytes; consecutive elements were 4 bytes apart, with 12 bytes between `a[0]` and `a[3]`.

**Correction:** ASLR randomizes applicable virtual address mappings at process execution/load time, not through compiler-time random address assignment.

**Outrunner connection:** Buffer indexing and data layout are fundamental to GPU memory access.

## OR-E3/E4 — Introductory assembly [Complete as orientation]

**Investigated:** `cmp`, CPU flags, signed/unsigned jumps, `lea`, effective addresses, and memory operands. Generated assembly file: `experiments/cpu/vector_add.s`.

**Key distinction:** `lea` calculates an address expression; `mov edx, [rax]` reads the value at the address in `rax`.

**Outrunner connection:** Inspect generated instructions when compiler lowering, memory indexing, or performance experiments require it; avoid unrelated assembly drills.

## OR-M1.1 — Reusable CPU kernel [Implementation drafted; correction pending]

**Files:** `include/outrunner/cpu/vector_add.hpp`, `src/cpu/vector_add.cpp`.

**Interface:** `outrunner::cpu::add_vector(const float*, const float*, float*, std::size_t)`.

**Review:** Header/source separation, const inputs, writable output, and element-wise addition correct. Replace conversion from `std::size_t` to `int` with a `std::size_t` loop index to avoid narrowing.

## Next: OR-M1.2

Compile and test the reusable kernel with multiple lengths, then add minimal CMake integration. Record compiler commands, test outputs, and failures here when observed.

## Entry template

```text
### Checkpoint / date
Goal:
Environment and commands:
Observed results:
Mistakes / corrections:
What this teaches about CPU/GPU/runtime/compiler behavior:
Next action:
```
