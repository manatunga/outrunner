# Learning Log

Record experiments, commands, observed output, corrections, and the direct implication for Outrunner. This log is for learning evidence, not speculative architecture.

## OR-E1 — CPU vector addition [Complete]

**Date:** October 2026

**Goal:** Compute an element-wise vector addition in standalone C++.

**Environment and commands:** GCC and Clang compiled the standalone experiment at `-O0` with standard warnings enabled. Exact commands not recorded.

**Observed results:** C++ arrays and a loop computed `[6, 8, 10, 12]` from `[1, 2, 3, 4] + [5, 6, 7, 8]`.

**Mistakes / corrections:** None recorded.

**What this teaches about CPU/GPU/runtime/compiler behavior:** Establishes the baseline computation later expressed as a reusable kernel and parallel dispatch.

**Related:** None.

**Next action:** Examine array memory layout (OR-E2).

## OR-E2 — Array layout [Complete]

**Date:** October 2026

**Goal:** Observe how a small array is laid out in memory.

**Environment and commands:** Not recorded.

**Observed results:** A four-element `int` array used 16 bytes. Consecutive elements were 4 bytes apart, with 12 bytes between `a[0]` and `a[3]`.

**Mistakes / corrections:** ASLR randomizes applicable virtual address mappings at process execution/load time, not through compiler-time random address assignment.

**What this teaches about CPU/GPU/runtime/compiler behavior:** Buffer indexing and data layout are fundamental to GPU memory access.

**Related:** None.

**Next action:** Introductory assembly orientation (OR-E3/E4).

## OR-E3/E4 — Introductory assembly [Complete as orientation]

**Date:** October 2026

**Goal:** Learn enough assembly to read the generated code for the vector-add experiment.

**Environment and commands:** Generated assembly file: `experiments/cpu/vector_add.s`. Exact commands not recorded.

**Observed results:** Investigated `cmp`, CPU flags, signed/unsigned jumps, `lea`, effective addresses, and memory operands.

**Mistakes / corrections:** None recorded.

**What this teaches about CPU/GPU/runtime/compiler behavior:** Key distinction: `lea` calculates an address expression; `mov edx, [rax]` reads the value at the address in `rax`. Inspect generated instructions when compiler lowering, memory indexing, or performance experiments require it; avoid unrelated assembly drills.

**Related:** None.

**Next action:** Write the reusable vector-addition kernel (OR-M1.1).

## OR-M1.1 — Reusable CPU vector addition [Complete]

**Date:** October 2026

**Goal:** Turn the experiment into a reusable CPU kernel with a public interface.

**Files and interfaces:**

- `include/outrunner/cpu/vector_add.hpp`
- `src/cpu/vector_add.cpp`
- `outrunner::cpu::add_vector(const float*, const float*, float*, std::size_t)`

**Environment and commands:** Not recorded.

**Observed results:** Reusable sequential element-wise addition using caller-provided arrays. Header/source separation, const inputs, writable output, and element-wise addition established.

**Mistakes / corrections:** The loop index uses `std::size_t`, avoiding narrowing the array length to `int`.

**What this teaches about CPU/GPU/runtime/compiler behavior:** Not recorded.

**Related:** D-036 (hand-written references are the second oracle).

**Next action:** Add a CMake build and tests (OR-M1.2).

## OR-M1.2 — CMake build and vector addition tests [Complete]

**Date:** October 2026

**Goal:** Establish a reproducible build and correctness tests for vector addition.

**Files and interfaces:**

- `CMakeLists.txt`
- `tests/test_vector_add.cpp`

**Environment and commands:**

- C++20 required; `outrunner_cpu` static library; independent `test_vector_add` executable; CTest registration.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

**Observed results:** Vector addition test passed. Four cases: ordinary arrays, empty input, a single element, and mixed positive/negative values.

**Mistakes / corrections:** None recorded.

**What this teaches about CPU/GPU/runtime/compiler behavior:** Not recorded.

**Related:** D-034 (CMake, C++20), D-036.

**Next action:** Matrix multiplication (OR-M1.3).

## OR-M1.3 — Sequential CPU matrix multiplication [Complete]

**Date:** November 2026

**Goal:** Add a second reference kernel: sequential row-major matrix multiplication.

**Files and interfaces:**

- `include/outrunner/cpu/matmul.hpp`
- `src/cpu/matmul.cpp`
- `tests/test_matmul.cpp`
- `outrunner::cpu::matmul(const float*, const float*, float*, std::size_t, std::size_t, std::size_t)`

**Environment and commands:** Not recorded.

**Observed results:** Sequential row-major matrix multiplication using three nested loops. Six test cases: rectangular matrices, square matrices, single-element matrices, zero shared dimension, zero output rows, and zero output columns.

**Mistakes / corrections:** None recorded.

**What this teaches about CPU/GPU/runtime/compiler behavior:**

- Compatible matrix dimensions: A is `m × k`, B is `k × n`, and output is `m × n`.
- Row-major indexing for accessing matrix elements.
- Dot products between rows of A and columns of B.
- Accumulation of products into a separate sum for each output element.
- Caller-owned input and output buffers.
- Natural zero-dimension behavior.

**Related:** D-036.

**Next action:** Integrate both tests into the build (OR-M1.4).

## OR-M1.4 — Complete test integration [Complete]

**Date:** November 2026

**Goal:** Build both CPU reference kernels and their tests together and register them independently with CTest.

**Environment and commands:** GNU C++ build through CMake in the WSL2 project environment. Both kernels are compiled into `outrunner_cpu`; their test sources are built as separate executables registered independently with CTest.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

**Observed results:**

```text
Test project /home/manatunga/projects/outrunner/build
    Start 1: vector_add
1/2 Test #1: vector_add ....................... Passed
    Start 2: matmul
2/2 Test #2: matmul ........................... Passed

100% tests passed, 0 tests failed out of 2
```

**Mistakes / corrections:** None recorded.

**What this teaches about CPU/GPU/runtime/compiler behavior:** The project now has two independently tested sequential CPU reference kernels. They provide correctness baselines for future execution backends.

**Related:** D-036.

**Next action:** Final review and milestone completion (OR-M1.5).

## OR-M1.5 — Final review and milestone completion [In progress]

**Date:** November 2026

**Goal:** Review production interfaces, implementations, tests, build configuration, and documentation before closing M1.

**Environment and commands:** Not yet run.

**Observed results:** Pending.

**Remaining work:**

- Review final source and test changes.
- Complete documentation updates.
- Perform clean build and final verification.
- Review Git diff and commit the milestone.

**Mistakes / corrections:** Pending.

**What this teaches about CPU/GPU/runtime/compiler behavior:** Pending.

**Related:** D-036.

**Next action:** Close M1, then begin spikes S4, S2, S1 and S3 (`ROADMAP.md`).

## S4 — Lavapipe and Intel Mesa feature check [Complete for lavapipe and Intel Mesa on Fedora; desktop drivers pending]

**Date:** 2026-11-24/25 (confirm)

**Goal:** Find out whether Mesa's lavapipe (software Vulkan) exposes the features Outrunner's Vulkan baseline needs, so CI can cover the Vulkan path without a GPU, and compare it with the real Intel GPU under Mesa.

**Files and interfaces:**

- `experiments/vulkan/s4_lavapipe/README.md` (full results table)
- `experiments/vulkan/s4_lavapipe/vulkaninfo_full_lvp.txt`
- `experiments/vulkan/s4_lavapipe/vulkaninfo_full_intel.txt`

**Environment and commands:** Fedora 45 on the laptop. Mesa 26.2.6, Vulkan API 1.4.354. Devices: lavapipe (llvmpipe) and Intel(R) Graphics (MTL), Intel open-source Mesa driver.

```bash
sudo dnf install mesa-vulkan-drivers vulkan-tools
vulkaninfo --summary
ls /usr/share/vulkan/icd.d/
VK_DRIVER_FILES=/usr/share/vulkan/icd.d/<icd json> vulkaninfo > vulkaninfo_full_<name>.txt
grep -n -A4 "maxComputeWorkGroupCount" vulkaninfo_full_<name>.txt
```

**Observed results:** The three gate features (buffer device address, 64-bit integers, timeline semaphores) are true on both. Key differences:

| Feature or limit                 | lavapipe                               | Intel, Mesa                |
| -------------------------------- | -------------------------------------- | -------------------------- |
| Shared-memory Int64 atomics      | true                                   | false                      |
| Shared-memory float32 atomic add | true                                   | false                      |
| Subgroup size                    | 8-8                                    | 8-32                       |
| Shared memory                    | 32 KiB                                 | 48 KiB                     |
| Work group count (x/y/z)         | 65535 / 65535 / 65535                  | 2147483647 / 65535 / 65535 |
| Cooperative matrix               | extension listed, feature not recorded | none                       |

The Windows Intel driver agreed with Intel Mesa on buffer device address, subgroup range and shared memory, and did not expose cooperative matrix. Its other values were not recorded.

**Mistakes / corrections:** The first run of the work-group rows recorded `3`, which was the array length printed by `vulkaninfo`, not the limits; the real values are on the lines below (`grep -A4`).

**What surprised me:** The asymmetry of the grid limits: the x dimension allows over two billion work groups on Intel Mesa, while y and z allow only 65535.

**What this teaches about CPU/GPU/runtime/compiler behavior:**

- Lavapipe provides the gate features, so a software Vulkan driver can exercise the baseline's pointer model and stream ordering in CI.
- Capabilities differ between drivers even for the same operations: shared-memory 64-bit and float atomic add are true on lavapipe and false on the real Intel GPU. A passing lavapipe run cannot stand in for a real-device run, which is why the capability descriptor and flagged fallbacks (D-012) are needed.
- Grid limits are per dimension and asymmetric, so grid splitting (D-027) must be per dimension, and block size is limited both per dimension and in total invocations.
- Lavapipe's fixed subgroup width of 8 and 32 KiB shared memory make it a free check that code assumes neither a particular subgroup width nor more than 32 KiB.

**Related:** D-012, D-019, D-025, D-027, D-037; `ROADMAP.md` spike S4.

**Next action:** Recheck the cooperative-matrix cells; repeat on the desktop drivers and on the laptop's Windows drivers; recheck lavapipe on the CI runner's Mesa version when CI exists; then start S2.

## S2 — Cooperative-matrix query on the RTX 5070 and AMD iGPU [Complete]

**Date:** 2026-11-26/27 (confirm)

**Goal:** Find out what the RTX 5070 driver reports for cooperative-matrix support, whether the queries behave, and compare with the AMD integrated GPU.

**Files and interfaces:**

- `experiments/vulkan/s2_coopmat/CMakeLists.txt`, `main.cpp`, `README.md`
- `experiments/vulkan/s2_coopmat/results_desktop.txt`

**Environment and commands:** Windows desktop, MSVC 19.44, CMake 4.4.4, Vulkan SDK 1.4.363, NVIDIA driver 610.88, AMD driver 25.10.30.02.

```powershell
cmake -S experiments\vulkan\s2_coopmat -B build-s2
cmake --build build-s2 --config Release
.\build-s2\Release\s2_coopmat.exe
```

**Observed results:**

- RTX 5070: `VK_KHR_cooperative_matrix` (spec 2), `VK_NV_cooperative_matrix` (1) and `VK_NV_cooperative_matrix2` (1) advertised; all feature flags true; 15 subgroup-scope shapes covering fp16, bf16, int8/uint8 and fp8 inputs. No fp32 inputs. No crash in the property query.
- AMD iGPU: KHR only; 11 shapes, all 16x16x16 (fp16 and int8 variants, some saturating).

**Mistakes / corrections:** An intermediate draft lost the assignment to `enabledLayerCount` (a bare `static_cast` statement remained), which would have left the validation layer silently off. Fixed. Review fixes also added a missing `#include <string>`, continuing past a failing device, and logging before risky calls.

**What I learned:** A very detailed hands-on dive into Vulkan instances, enumeration and queries, and the cooperative-matrix data on the GPUs in my setup.

**What surprised me:** Pretty much regarding the supported cooperative matrix sizes across the different GPUs, and how they differ from device to device.

**What this teaches about CPU/GPU/runtime/compiler behavior:**

- Matrix hardware through Vulkan on this GPU needs reduced-precision inputs, so a "matrix-assisted" float32 workload would be a different numeric mode and must be reported as its own tier (D-020).
- The AMD iGPU advertises the extension, but a 2-CU RDNA 2 part probably has no matrix hardware, so advertised does not mean useful; verify-before-trust needs a micro-benchmark (D-022).
- Property queries returned cleanly on driver 610.88; the defensive-logging pattern is still worth keeping.

**Related:** D-020, D-022; `ROADMAP.md` spike S2.

**Next action:** Spike S1, the Vulkan matmul ceiling.

## Entry template

```text
### <Checkpoint ID> — <title> [Status]
Date:
Goal:
Files and interfaces (when code changed):
Environment and commands:
Observed results:
Mistakes / corrections:
What this teaches about CPU/GPU/runtime/compiler behavior:
Related: (decision or roadmap IDs)
Next action:
```
