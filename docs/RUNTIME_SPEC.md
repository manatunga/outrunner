# Runtime Specification

**Status:** Provisional. Defines behavioral targets for the v0.1 runtime. No stable public runtime API is implemented.

## 1. Purpose

The Outrunner runtime is responsible for executing compiled or loaded kernels on supported execution backends.

It provides a shared host-facing interface for device selection, memory management, kernel loading, dispatch, synchronization, and diagnostics.

The runtime should allow a host application to select an execution backend without requiring a separate kernel implementation for each backend.

The initial target is a CPU execution backend and one compatible Vulkan GPU backend.

## 2. Core conceptual objects

These objects describe the responsibilities of the runtime. They do not establish final C++ classes, ownership rules, or public API signatures.

### 2.1 Device

Identifies an available execution device and its capabilities.

Candidate device types:

- CPU reference/execution device.
- Compatible Vulkan compute device.

The runtime must expose enough information for callers to identify and select an available device.

### 2.2 Buffer

Represents a region of memory used by compute operations.

A buffer has:

- A defined size.
- A defined lifetime.
- An associated memory location or allocation.
- Defined rules for host and device access.

Whether buffers are owned, referenced, or device-bound remains undecided.

### 2.3 Kernel

Represents a compiled or loaded compute operation.

It must provide enough metadata for the runtime to validate its arguments and configure execution.

The runtime should not require the host application to manage backend-specific kernel representations directly.

### 2.4 Dispatch

Submits a kernel for execution over an explicit logical work range.

The runtime must define how dispatch dimensions map to logical work-item indices and ensure consistent mapping across supported backends.

### 2.5 Synchronization

Provides a way to establish when submitted work and required memory transfers have completed.

The host must have a reliable completion point before reading results that depend on dispatched work.

## 3. Required v0.1 behavior

### 3.1 Device discovery

- Enumerate the CPU execution device.
- Enumerate available compatible Vulkan compute devices.
- Report unavailable or unsupported backends clearly.
- Expose relevant device capabilities needed for selection and dispatch validation.

### 3.2 Device selection

- Allow the caller to explicitly select a device or backend.
- Avoid requiring different kernel source for CPU and GPU execution.
- Report selection failures through a defined error mechanism.

### 3.3 Memory allocation

- Allocate and release compute buffers.
- Track buffer sizes and relevant ownership/lifetime information.
- Reject invalid allocation sizes and unsupported uses with defined errors.

### 3.4 Data transfer

- Support host-to-compute and compute-to-host transfers.
- Handle GPU transfers where host and device memory are distinct.
- Allow the CPU path to use host-accessible memory.
- Ensure required transfers are complete before dependent operations access their data.

### 3.5 Kernel loading

- Load or receive a kernel representation suitable for the selected backend.
- Validate kernel compatibility with the selected device.
- Report compilation or loading failures clearly.

The exact division of responsibility between the compiler and runtime remains subject to the compiler and runtime architecture.

### 3.6 Dispatch

- Execute a kernel over an explicit logical work range.
- Validate dispatch dimensions against backend/device limits.
- Preserve the defined logical index mapping across supported backends.
- Ensure that kernel arguments and buffers are valid for the selected execution.

### 3.7 Synchronization

- Provide a reliable completion point before dependent host-side reads.
- Ensure memory transfers and dispatch operations obey the required ordering.
- Keep the initial synchronization model simple.

Advanced asynchronous execution features are deferred.

### 3.8 Validation and errors

The runtime should detect and report obvious invalid operations, including:

- Invalid or unavailable device selection.
- Invalid buffer references.
- Invalid kernel references.
- Unsupported dispatch dimensions.
- Incompatible kernel arguments.
- Allocation, transfer, loading, or execution failures.

Errors must be actionable and distinguishable enough for host applications to respond appropriately.

The final error representation remains undecided.

### 3.9 Diagnostics

The runtime should expose useful execution information, including:

- Selected device and backend.
- Kernel compilation or loading stages.
- Dispatch dimensions.
- Execution failures.
- Basic timing information.

CPU wall-clock measurements and GPU timing mechanisms must be distinguished. Timing results should not imply directly comparable measurements when their measurement methods differ.

## 4. Execution semantics requiring decisions

The following details must be resolved before the relevant runtime interfaces are finalized.

1. **Buffer ownership:** whether buffers are runtime-owned, caller-owned, or support both models.
2. **Device association:** whether buffers are tied to a device or can be transferred/shared between devices.
3. **Synchronization:** whether v0.1 dispatch is synchronous or exposes explicit completion handles.
4. **Dispatch dimensions:** workgroup dimensions, device limits, and global-index mapping.
5. **Memory safety:** bounds checking, aliasing, and buffer access guarantees.
6. **Error model:** exceptions, result values, or status codes.
7. **Kernel representation:** how compiled kernel metadata and backend-specific artifacts are provided to the runtime.
8. **Floating-point behavior:** acceptable numerical differences between CPU and GPU execution.

These are open design decisions, not existing API guarantees.

## 5. Current implementation boundary

The current `outrunner_cpu` library provides sequential reference functions for vector addition and matrix multiplication.

These functions:

- Operate directly on caller-provided host-accessible buffers.
- Execute sequentially.
- Do not discover or select devices.
- Do not allocate or manage compute buffers.
- Do not load compiled kernels.
- Do not provide logical work-item dispatch.
- Do not implement runtime synchronization.

They are computational reference implementations, not the completed CPU runtime backend described by this specification.

The runtime and execution backend milestones will introduce these capabilities incrementally.

## 6. Non-goals for v0.1

The initial runtime does not aim to provide:

- CUDA binary compatibility.
- GPU driver replacement.
- Multi-GPU scheduling.
- Unified virtual memory.
- An advanced stream/event execution model.
- A production-grade optimizing compiler.
- A complete neural-network framework.

These exclusions keep the first implementation focused on a small, testable end-to-end compute system.

## 7. v0.1 acceptance tests

The runtime is accepted for v0.1 when the following can be demonstrated:

1. The same host application can select and use the CPU and compatible Vulkan GPU backends.
2. The application can allocate buffers, transfer data, load kernels, dispatch work, and retrieve results.
3. Vector addition, matrix multiplication, and a simple element-wise operation produce correct results.
4. CPU and GPU execution use the same kernel definition and documented logical indexing semantics.
5. Invalid inputs and unsupported operations produce useful errors.
6. Device information and dispatch configuration can be inspected.
7. Basic execution timings can be collected, with CPU and GPU measurement methods clearly identified.
8. GPU numerical results are compared with the CPU reference using justified floating-point tolerances.

Acceptance must be demonstrated on actual supported hardware. A successful CPU implementation alone does not establish Vulkan backend support or cross-vendor portability.
