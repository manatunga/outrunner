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

## OR-M1.1 — Reusable CPU vector addition [Complete]

**Files:**
- `include/outrunner/cpu/vector_add.hpp`
- `src/cpu/vector_add.cpp`

**Interface:** `outrunner::cpu::add_vector(const float*, const float*, float*, std::size_t)`.

**Implementation:** Reusable sequential element-wise addition using caller-provided arrays.

**Review:** Header/source separation, const inputs, writable output, and element-wise addition established. The loop index uses `std::size_t`, avoiding narrowing the array length to `int`.

## OR-M1.2 — CMake build and vector addition tests [Complete]

**Files:**
- `CMakeLists.txt`
- `tests/test_vector_add.cpp`

**Build configuration:**
- C++20 required.
- `outrunner_cpu` static library.
- Independent `test_vector_add` executable.
- CTest registration.

**Tests:** Four cases covering ordinary arrays, empty input, a single element, and mixed positive/negative values.

**Verification commands:**

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

**Observed result:** Vector addition test passed.

## OR-M1.3 — Sequential CPU matrix multiplication [Complete]

**Files:**
- `include/outrunner/cpu/matmul.hpp`
- `src/cpu/matmul.cpp`
- `tests/test_matmul.cpp`

**Interface:** `outrunner::cpu::matmul(const float*, const float*, float*, std::size_t, std::size_t, std::size_t)`.

**Implementation:** Sequential row-major matrix multiplication using three nested loops.

**Concepts established:**

- Compatible matrix dimensions: A is `m × k`, B is `k × n`, and output is `m × n`.
- Row-major indexing for accessing matrix elements.
- Dot products between rows of A and columns of B.
- Accumulation of products into a separate sum for each output element.
- Caller-owned input and output buffers.
- Natural zero-dimension behavior.

**Tests:** Six cases covering rectangular matrices, square matrices, single-element matrices, zero shared dimension, zero output rows, and zero output columns.

## OR-M1.4 — Complete test integration [Complete]

**Build configuration:** Both CPU reference kernels are compiled into `outrunner_cpu`. Their test sources are built as separate executables and registered independently with CTest.

**Verification commands:**

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

**Observed result:**

```text
Test project /home/manatunga/projects/outrunner/build
    Start 1: vector_add
1/2 Test #1: vector_add ....................... Passed
    Start 2: matmul
2/2 Test #2: matmul ........................... Passed

100% tests passed, 0 tests failed out of 2
```

**Environment:** GNU C++ build through CMake in the WSL2 project environment.

**Outrunner connection:** The project now has two independently tested sequential CPU reference kernels. They provide correctness baselines for future execution backends.

## OR-M1.5 — Final review and milestone completion [In progress]

**Goal:** Review production interfaces, implementations, tests, build configuration, and documentation before closing M1.

**Remaining work:**
- Review final source and test changes.
- Complete documentation updates.
- Perform clean build and final verification.
- Review Git diff and commit the milestone.

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
