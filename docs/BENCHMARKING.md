# Benchmarking Methodology

**Status:** Draft, 2026-11-21. No benchmarks have been run yet. Updated as spikes and milestones report.
**Related:** `DESIGN_DECISIONS.md` (D-003, D-020, D-039), `ROADMAP.md`, `CLEAN_ROOM.md` (section 8), `TESTING.md`.

## 1. Purpose

Show, with evidence others can reproduce, how close Outrunner gets to vendor-native performance across devices, and where it does not. Benchmarks are a credibility asset only if they are fair, complete and repeatable.

## 2. Workloads

Three classes, each chosen to stress a different bottleneck (D-003):

| Class | What it tests | Candidate workloads |
|---|---|---|
| Memory-bound | Saturating memory bandwidth | Vector add, stream-style copy and triad, reduction |
| Compute-bound | Arithmetic throughput and tiling | Dense float32 matrix multiplication |
| Synchronization-heavy | Shared memory, barriers, atomics | Stencil, histogram, scan |

Final picks are fixed at the staging step of each version. Every workload has a hand-written C++ reference for correctness (`TESTING.md`).

## 3. Three-way comparison

On every device, each workload is measured as three rows:

1. **Vendor-native best:** the best available native implementation for that device (for NVIDIA: hand-written CUDA, and for matmul the vendor's tuned library as the ceiling).
2. **Hand-written Vulkan:** a kernel written directly as a Vulkan compute shader, tuned by hand.
3. **Outrunner:** the same workload through the language, compiler and runtime.

The middle row separates two questions: is Outrunner's compiler good (row 3 versus row 2), and is the Vulkan route good enough (row 2 versus row 1)? A gap is attributed to the right layer.

Where no vendor-native baseline exists (for example the Intel and AMD integrated GPUs on owned hardware), rows 2 and 3 plus the measured-peak metric still apply, and the missing row is stated as such.

CPU results use a hand-written reference and an established optimized library as baselines.

## 4. Metrics

| Metric | Definition |
|---|---|
| Time | Median of repeated runs after warmup, with spread |
| Percent of vendor-native | Outrunner throughput divided by the vendor-native row |
| Percent of measured peak | Throughput divided by the device's **measured** peak |
| Cold start | First launch including specialization and compile, reported separately |

**Measured peak** comes from microbenchmarks run on that same device in the same session: a memory-bandwidth test (copy and triad) and an arithmetic-throughput test (a long chain of fused multiply-adds). Spec-sheet numbers are not used. On integrated GPUs, memory bandwidth is shared with the CPU, and the measurement says so.

## 5. Targets

- Memory-bound: 90% or more of measured peak bandwidth (D-003).
- Compute-bound: set after spike S1 shows what the Vulkan route can deliver on the RTX 5070.
- Synchronization-heavy: set after the v0.3 baseline exists.

Targets are recorded here when set, never adjusted after seeing a result without a dated note explaining why.

## 6. Run protocol

1. Record the environment: commit, OS, device, driver version, Vulkan version, compiler versions, power mode.
2. Run the correctness check first; a benchmark with a wrong result is discarded and reported.
3. Warm up, then run at least 30 repetitions per size (more if variance is high); report median and spread.
4. Sweep problem sizes from small (launch-overhead dominated) to large (bandwidth or compute dominated).
5. Time kernels with device timestamps where available, and report end-to-end wall-clock separately.
6. Report **compute-only** and **with transfers** separately.
7. Report Outrunner with a **cold** and a **warm** tuning cache.
8. Run the matrix-assisted tier separately from the shader-core tier (D-020).

## 7. Fairness rules

- Same algorithmic work, data type, problem size and input data on every row. Inputs are random with a recorded seed.
- Same numeric mode on every row: strict against strict; fast against fast with a declared tolerance (D-011). A row using fused or approximate math the others do not is flagged.
- Vendor-native rows use the best-known configuration for that device, not a deliberately weak one.
- Prevent the compiler from removing work (results are consumed and checked).
- Every comparison states the Outrunner version and the baseline versions.

## 8. Environment control

- Native Windows or native Linux only. **No WSL2** for GPU numbers, because Vulkan there may go through a translation layer.
- Laptop devices plugged in, in a fixed power mode, with clocks and temperature logged where tools allow; discard runs showing thermal throttling or note it.
- No other heavy processes during runs.
- On Windows, kernels are sized to stay well under the GPU watchdog limit, or split.
- Driver versions are pinned per report; a driver update starts a new report.

## 9. Reporting and publication

- Publish tables and plots with the **raw data** (JSON), the scripts and the exact commands.
- Publish **all** runs, including losses and failures. Nothing is selected after the fact.
- Every report ends with a limitations section (what was not measured, which devices were absent).
- The CUDA toolkit's license terms on publishing comparisons are checked before any numbers are shared (`ROADMAP.md`).

Draft raw-result record (finalized with the harness):

```json
{
  "schema_version": 1,
  "workload": "<name>",
  "tier": "shader-core|matrix-assisted",
  "row": "native|hand-vulkan|outrunner",
  "device": "<name>",
  "driver": "<version>",
  "size": "<problem size>",
  "cache": "cold|warm",
  "numeric_mode": "strict|fast",
  "runs_ms": [],
  "median_ms": 0,
  "throughput": { "value": 0, "unit": "GB/s|GFLOP/s" },
  "percent_of_measured_peak": 0,
  "percent_of_native": 0
}
```

## 10. Where things live

- `benchmarks/` in the core repository: the Outrunner-side harness, hand-written Vulkan baselines and measured-peak microbenchmarks.
- A **separate baselines repository** (name to be chosen): CUDA and other vendor-native baseline code, kept outside the core and its releases (`CLEAN_ROOM.md`, section 8).
- Published reports and raw data: versioned in the repository's release assets or a results directory.

## 11. Current status

No results exist. The first measurements are spike S1 (hand-written Vulkan matmul against vendor-native on the RTX 5070), recorded in the learning log.
