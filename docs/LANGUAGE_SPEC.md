# Kernel Language Specification (Proposal)

**Status:** Exploratory; no syntax or grammar is finalized. No kernel-language compiler currently exists.

## Purpose

A restricted, backend-independent way to express parallel computations. v0.1 needs enough expressive power for vector addition, element-wise activation, and basic matrix multiplication, while remaining feasible to parse, validate, and lower to CPU execution and SPIR-V.

## Candidate semantic requirements

- A kernel entry point executed for a set of logical work items.
- Scalar types including 32-bit floating point and suitable integer index types.
- Buffer arguments and indexed reads/writes.
- Arithmetic, comparisons, conditional statements, and bounded loops as needed for MVP workloads.
- A built-in way to obtain a logical global work-item index; multidimensional indexing for matrix multiplication.
- Clear out-of-range behavior, memory access rules, and restrictions on unsupported features.

## Illustrative syntax ONLY

```text
kernel void vector_add(
    global float* a,
    global float* b,
    global float* out, 
    int n
) {
    int i = global_id(0);
    if (i < n) out[i] = a[i] + b[i];
}
```

This is **not executable Outrunner code**, and keywords (`kernel`, `global`, `global_id`) are placeholders, not accepted design decisions.

## Decisions required before implementation

1. Kernel input format: custom source, constrained C-like syntax, or structured IR.
2. Numeric types, casts, overflow, and floating-point behavior.
3. Indexing model: global IDs, local IDs, dimensions, and workgroups.
4. Buffer/address-space model and aliasing rules.
5. Loop and control-flow subset; validation failures.
6. Kernel arguments and ABI mapping to CPU/Vulkan backends.
7. CPU execution and SPIR-V lowering equivalence tests.

## v0.1 acceptance

One kernel definition (not two manually maintained device-specific kernels) executes correctly on CPU and one compatible Vulkan GPU for the required workloads. Unsupported language constructs must fail with understandable diagnostics.
