# Runtime Specification (Skeleton)

**Status:** Skeleton, 2026-11-22. Records decided points and lists open ones. The runtime milestones (OR-M2 to OR-M5) fill it in.
**Related:** `DESIGN_DECISIONS.md` (D-022 to D-033), `ARCHITECTURE.md`, `LANGUAGE_SPEC.md`.

**Section status:** *Decided*, *Proposed* (recommendation awaiting confirmation), *Open*.

## 1. Objects and ownership — Decided

Objects: Runtime, Device, Stream, Event, Buffer<T>, Module, Kernel, specialization handles (D-031).
- Handles are move-only and release their resources when destroyed (D-029).
- A Runtime owns device enumeration, the tuning-cache location, logging configuration and the loaded Vulkan library. There is no hidden global context; `default_runtime()` is a convenience over an explicit object.
- **Open:** exact ownership relations between Module, Kernel and specialization handles.

## 2. Devices and capabilities — Decided (model), Open (fields)

- The CPU is a device in the same list as GPUs (D-022).
- Each device has a **capability descriptor** separating what the driver *advertises* from what Outrunner has *verified* (D-033). A capability counts only after a correctness check and micro-benchmark, cached (D-022).
- Candidate fields (**Open**): vendor, name, driver and API versions, shared-memory size, subgroup size range, per-dimension block and grid limits, push-constant size, `bufferDeviceAddress`, atomic capabilities (64-bit, float), memory-model support, cooperative-matrix support, whether memory is shared with the host.
- Selection order: user choice, then default heuristic, then a fallback ladder (verified native fast path, Vulkan baseline, CPU).
- **Baseline requirement — Proposed:** a GPU qualifies for the Vulkan baseline only if it provides `bufferDeviceAddress`. The remaining minimum feature set is **Open** and depends on the pending `vulkaninfo` check (`ROADMAP.md`, v0.3).

## 3. Memory — Decided

- Memory kinds: device, host, managed; CPU memory is 64-byte aligned (D-023, D-024).
- Allocation goes through Outrunner's own sub-allocator behind an interface; users never see raw driver allocations.
- Device pointers are typed handles carrying device, kind and size, converting explicitly to raw addresses when passed to kernels.
- Copies are skipped automatically on devices that share memory with the host; a query reports sharing.
- Staging for discrete GPUs is internal.
- Freed memory is reused only after all work submitted so far on that device completes (D-029).
- **Open:** allocation alignment options, zero-size allocations, behavior when memory is exhausted beyond the error category, managed-memory migration policy.

## 4. Streams and events — Decided

- A stream is an in-order queue; work in one stream runs in order; different streams may overlap (D-025).
- Each device has a simple default stream with no implicit synchronization with other streams.
- Streams are deferred command lists: work is recorded and submitted in batches, at synchronization points, on size thresholds or on explicit flush.
- Dependent launches in one stream are separated by conservative full barriers; the barrier policy is pluggable (D-026).
- Events provide cross-stream ordering and timestamps.
- **Open:** the precise batching thresholds, flush rules and event-wait semantics across devices.

## 5. Launch and dispatch — Decided

- A launch is a function call taking a kernel, grid, block, stream and arguments (D-027).
- Configurations are validated eagerly against device limits; errors return immediately.
- Block size, shared-memory size and tuning parameters are specialization parameters selecting cached pipeline variants, with explicit specialization handles, prewarming and shared-memory size classes.
- Oversized grids are split transparently; kernels marked as inter-block dependent return an error instead.
- Arguments are passed through small constants where they fit and through a runtime-packed buffer otherwise.
- **Open:** argument-packing layout, limits on argument size, behavior when specialization creation fails mid-stream.

## 6. Errors — Decided (model), Open (codes)

- Code-based C core and a `Result` type; the C++ wrapper throws by default with a `Result` variant (D-028).
- Categories: invalid argument, out of memory, unsupported feature, kernel compile error (with source locations), device lost, verification failure.
- Device-side errors surface at synchronization points.
- A lost device is marked invalid, its pending work fails, and it can be recreated.
- **Open:** the numeric error codes and the structure carrying details and locations.

## 7. Threading — Decided

Devices and the allocator are thread-safe. A stream shared between threads needs external synchronization (D-029).
**Open:** thread-safety of Module and Kernel creation, and of the tuning cache.

## 8. Tuning, caching and verification — Decided (design), Open (details)

- Autotuning searches tunable parameters per device; results go to a persistent cache keyed by kernel IR hash, device identity, driver version, parameters and compiler version (D-016, D-017).
- Every candidate and every newly trusted capability is checked against the oracles by the correctness harness (D-022, D-036).
- **Open:** cache file format, concurrency between processes, invalidation and eviction, and user-controlled tuning budgets.

## 9. Diagnostics — Decided

Event timestamps and a timing helper; a callback-based logger with levels; an "explain" trace; Chrome trace export; off by default with near-zero cost when disabled (D-033).
**Open:** the logger callback signature and the explain-trace record format.

## 10. C ABI conventions — Proposed

- Prefix `outrunner_`, snake_case (D-031); opaque handles; every function returns an error code and writes outputs through pointers.
- Size-prefixed structs: new fields are only appended, and the library reads the size to know the caller's version (D-032).
- API version negotiation at runtime creation; an experimental namespace for incubating features.
- No function in the stable surface throws or unwinds across the ABI boundary.
- Illustrative, not final:

```text
outrunner_result outrunner_runtime_create(const outrunner_runtime_desc* desc,
                                          outrunner_runtime* out_runtime);
```

## 11. C++ wrapper conventions — Proposed

Header-only; PascalCase types, snake_case functions, namespace `outrunner` (never `or`); typed `Buffer<T>`; exceptions by default, with a `Result`-returning variant (D-028, D-031). Illustrative, not final:

```text
auto dev = outrunner::Device::best();
auto stream = dev.default_stream();
auto a = dev.alloc<float>(n);
a.upload(host_a, stream);
stream.launch(add, grid_for(n, 256), 256, a, b, c, n);
stream.synchronize();
```

## 12. Backend interface (internal) — Proposed

The runtime depends on backends through one internal interface, which a backend implements. It must cover: device and capability discovery; memory allocation, free and copy; creating an executable pipeline from IR plus specialization parameters; recording a launch and a barrier; submitting and waiting; timestamps; and fault reporting. The CPU and Vulkan backends are the first two implementations. **Open:** the exact interface, settled at OR-M2 and OR-M4.

## 13. Versioning — Decided

Semantic versioning; no stability promises in 0.x; the C ABI is stable within a major version from v1.0.0 (D-032). The serialized IR is versioned: newer readers accept older IR; older readers reject newer IR with a clear error.

## 14. Open items summary

Object ownership details; capability descriptor fields and the baseline feature set; allocation options and exhaustion behavior; batching and flush rules; argument packing; error codes and detail structure; thread-safety of module creation and the cache; cache format and invalidation; logger and trace formats; the exact backend interface.
