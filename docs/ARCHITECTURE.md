# Architecture

**Status:** Proposed v0.1 architecture; not an implementation description.

## Objective

Implement a small end-to-end compute stack with a shared host API, a restricted kernel language/compiler path, and interchangeable CPU and Vulkan GPU execution backends. Understand every boundary through instrumentation and experiments.

## Component boundaries

1. **Host API** — device selection, buffer creation, data transfer, kernel loading, dispatch, synchronization, and errors.
2. **Compiler frontend** — parse a deliberately small kernel language and validate its types and operations.
3. **Intermediate representation (IR)** — backend-independent representation of kernels; exact structure undecided.
4. **CPU backend** — reference execution and basic parallel dispatch, without relying on a GPU.
5. **Vulkan backend** — create Vulkan compute resources, upload buffers, dispatch SPIR-V compute shaders, synchronize, and read results.
6. **Diagnostics** — expose compilation steps, selected device, dispatch dimensions, errors, and measured timings.

```text
                      OUTRUNNER v0.1

                      Kernel Source
                            |
                            v
                    Parse / Validate
                            |
                            v
                    Intermediate Rep.
                       (Proposed)
                            |
                +-----------+-----------+
                |                       |
                v                       v
          CPU Lowering             SPIR-V Lowering
                |                       |
                v                       v
         CPU Executable          SPIR-V Module
                |                       |
                v                       v
         +-------------+         +-------------+
         | CPU Backend |         | Vulkan      |
         |             |         | GPU Backend |
         +------+------+         +------+------+
                ^                       ^
                |                       |
                +-----------+-----------+
                            |
                      Runtime Dispatch
                            ^
                            |
                       Public API
                            ^
                            |
                     Host Application

           Runtime responsibilities:
           - Device discovery and selection
           - Buffer allocation and transfers
           - Kernel loading and dispatch
           - Synchronization and error handling
```

## Execution lifecycle (target)

1. Host selects a supported device.
2. Kernel is compiled or loaded for the chosen backend.
3. Runtime allocates buffers and uploads inputs where needed.
4. Runtime configures work-item/workgroup dimensions and submits dispatch.
5. Runtime waits for completion or exposes explicit synchronization.
6. Outputs are made available to the host; diagnostics and errors are reported.

## Scope boundaries

- **Outrunner owns:** host-facing API, dispatch abstractions, runtime orchestration, CPU execution, Vulkan integration, restricted frontend/IR (once implemented).
- **External components:** OS, Vulkan loader/driver, vendor hardware compiler, GPU firmware/hardware. A SPIR-V path does not imply native GPU instruction generation by Outrunner.
- **Portability:** common source and host API are targets; hardware behavior and availability must be tested per backend.

## Open architecture questions

- Interpreter versus compiled CPU execution for kernel IR.
- Whether a tiny textual kernel language or structured IR is the first compiler input.
- Exact workgroup, bounds-checking, and address-space rules.
- Buffer ownership and synchronization semantics.
- Vulkan shader toolchain and validation strategy.

See [Design Decisions](DESIGN_DECISIONS.md) and [Technical Backlog](TECHNICAL_BACKLOG.md). Do not treat illustrative APIs as stable interfaces.
