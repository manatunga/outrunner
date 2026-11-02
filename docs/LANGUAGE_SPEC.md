# Kernel Language Specification

**Status:** Exploratory proposal. No syntax, grammar, or complete semantic model has been finalized. No kernel-language compiler currently exists.

## 1. Purpose

Outrunner requires a restricted, backend-independent way to express parallel computations.

The language must eventually provide a common kernel definition that can be compiled or translated for different execution backends without requiring users to maintain separate CPU and GPU implementations.

For v0.1, the language should support enough expressive power for:

- Vector addition.
- Element-wise operations, including activation functions.
- Basic matrix multiplication.

The initial language should remain deliberately small. Every supported construct must have a defined meaning and a feasible implementation path through validation and backend lowering.

The objective is not to reproduce an existing general-purpose programming language. It is to establish a focused kernel programming model suitable for Outrunner's compiler and runtime architecture.

## 2. Candidate semantic requirements

The following requirements describe capabilities the language is expected to provide. They do not establish final syntax or implementation decisions.

### 2.1 Kernel entry points

- A kernel must have an identifiable entry point.
- A kernel executes over a set of logical work items.
- Each logical work item must be able to identify its position within the dispatched execution range.
- Kernel arguments must have defined types and usage rules.

### 2.2 Types

The initial type system should consider:

- 32-bit floating-point values.
- Suitable integer types for indexing and dimensions.
- Additional scalar types only where required by the initial workloads.

Numeric conversion, overflow, and floating-point behavior must eventually be specified consistently across supported backends.

### 2.3 Memory and buffers

The language must support:

- Buffer arguments.
- Indexed reads and writes.
- Defined memory access rules.
- Clear distinctions between supported memory regions or address spaces, if multiple address spaces are introduced.
- Defined aliasing and access restrictions.

The language must not assume that CPU and GPU memory have identical physical or ownership characteristics.

### 2.4 Computation and control flow

Candidate constructs include:

- Arithmetic operations.
- Comparisons.
- Conditional statements.
- Bounded loops where required by MVP workloads.

The supported subset should be small enough to validate and lower reliably.

### 2.5 Work-item indexing

A kernel must be able to obtain its logical execution index.

The model must eventually define:

- Global work-item indexing.
- Whether local work-item indexing is exposed.
- One-dimensional and multidimensional execution.
- Workgroup dimensions and their relationship to global dimensions.
- Behavior for work items whose indices exceed the useful input range.

The logical indexing model must be consistent across CPU and GPU backends.

## 3. Illustrative syntax

The following example illustrates one possible C-like representation:

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

**This is illustrative syntax only. It is not executable Outrunner code.**

The names `kernel`, `global`, and `global_id` are placeholders, not accepted design decisions. The example does not establish a grammar, type system, ABI, or memory model.

## 4. Current implementation boundary

The current CPU implementations are sequential reference functions:

- `outrunner::cpu::add_vector`
- `outrunner::cpu::matmul`

They operate on caller-provided host-accessible buffers and do not execute through a kernel-language frontend, logical work-item model, or general dispatch system.

These implementations establish computational correctness baselines. They do not define the final kernel language or its execution semantics.

The future language and backend model must preserve the intended computational behavior of these workloads while allowing parallel execution.

## 5. Decisions required before implementation

The following areas remain open:

1. **Kernel input format:** custom source language, constrained C-like syntax, or structured IR as the initial input.
2. **Numeric semantics:** supported numeric types, casts, integer overflow, and floating-point behavior.
3. **Indexing model:** global IDs, local IDs, dimensions, and workgroups.
4. **Memory model:** buffer/address-space representation, aliasing, and access rules.
5. **Control flow:** supported loops, conditional statements, and validation restrictions.
6. **Kernel arguments:** argument representation and ABI mapping to CPU and Vulkan backends.
7. **Bounds behavior:** how invalid or out-of-range accesses are handled.
8. **Backend equivalence:** how CPU and SPIR-V implementations are tested against the same semantic requirements.

These decisions must be documented before the corresponding features are treated as part of the supported language.

## 6. v0.1 acceptance criteria

The language/compiler path must eventually demonstrate that:

- One kernel definition can be used for both CPU and compatible Vulkan GPU execution.
- The same source does not require manually maintained device-specific kernel implementations.
- Vector addition, matrix multiplication, and the required element-wise workload execute correctly.
- Supported constructs have consistent semantics across the tested backends.
- Unsupported constructs fail with understandable diagnostics.
- Kernel arguments, indexing, and memory access follow documented rules.

## 7. Non-goals

The initial language is not intended to provide:

- A complete general-purpose programming language.
- Full C or C++ compatibility.
- CUDA source or binary compatibility.
- An unrestricted pointer and memory model.
- A complete set of advanced GPU-specific language features.

The initial objective is a small, coherent kernel language that can grow without compromising backend independence.
