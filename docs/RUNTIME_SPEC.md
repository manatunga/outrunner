# Runtime Specification (Provisional)

**Status:** Behavioral targets; no stable API is implemented.

## Core objects (conceptual)

- **Device:** identifies a CPU or compatible Vulkan compute device and capabilities.
- **Buffer:** owns or references a region of compute memory with a defined size and lifetime.
- **Kernel:** compiled or loaded compute operation plus metadata about its arguments.
- **Dispatch:** launches logical work items using specified dimensions.
- **Synchronization:** establishes when dispatched work and memory transfers have completed.

## Required v0.1 behavior

1. **Discovery:** enumerate CPU reference device and available compatible Vulkan GPU devices; report unavailable backends clearly.
2. **Selection:** let caller explicitly select a device/backend without rewriting kernel source.
3. **Allocation:** allocate and release buffers; reject invalid sizes/uses with defined errors.
4. **Transfer:** copy input/output between host and GPU memory where necessary; CPU path may use host-accessible memory.
5. **Dispatch:** execute a kernel over an explicit logical work range; ensure the same logical index mapping across supported backends.
6. **Synchronization:** provide a reliable completion point before reading results; asynchronous features can follow later.
7. **Validation:** check obvious mismatches (invalid device, buffer, kernel, dimensions) and report actionable errors.
8. **Diagnostics:** report device/backend, kernel compilation/dispatch stages, and basic timings; distinguish CPU wall-clock from GPU timing mechanisms.

## Execution semantics to decide

- Buffer lifetime and ownership; whether buffers are device-bound.
- Whether dispatch is synchronous in v0.1 or has explicit completion handles.
- Workgroup dimensions, limits, and mapping of global IDs.
- Bounds checking, aliasing, and buffer access guarantees.
- Numeric tolerance for cross-backend floating-point validation.
- Error model (exceptions, result values, or status codes).

## Non-goals for v0.1

CUDA binary compatibility; vendor-driver replacement; multi-GPU scheduling; unified virtual memory; advanced stream/event model; production-grade optimizing compiler; complete neural-network framework.

## Acceptance tests

- Same host application selects CPU and GPU.
- Correct vector addition, matrix multiplication, and a simple element-wise operation.
- Invalid inputs produce useful errors.
- Device and dispatch information can be inspected.
- GPU results are compared with CPU reference using justified numerical tolerances.
