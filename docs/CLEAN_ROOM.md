# Clean-Room Policy

**Status:** Draft, 2026-11-21. Subject to review by a software-IP lawyer before the public announcement or any outside contribution (see `ROADMAP.md`, v0.5).
**This is a project process, not legal advice.**
**Related:** `DESIGN_DECISIONS.md` (D-001, D-002, D-038).

## 1. Purpose

Outrunner aims to be a vendor-neutral, open-source GPU compute stack that learns from the CUDA programming model without copying it. This policy keeps the project's work independent: it says what may be used as a source, what may not, and how the project proves where its ideas came from.

## 2. Scope

The policy covers everything in the repository and everything distributed with it: source code, documentation, tests, benchmarks, examples, build scripts and contributions from anyone, including material drafted with an AI assistant. Assistant-drafted material is reviewed by the maintainer and held to exactly the same rules.

## 3. Permitted sources

- Public specifications: Vulkan, SPIR-V, the ISO C++ standard, IEEE 754.
- Public documentation of programming models, including the CUDA programming model, used for **concepts only**: ideas such as grids, blocks, streams and events are general and also appear in OpenCL, SYCL and Vulkan.
- Publicly published vendor documentation (for example, instruction-set manuals), subject to the license terms of each document.
- Academic papers and public technical talks.
- Open-source projects under licenses listed in section 6, studied for ideas and used as dependencies only as the license allows.
- The project's own original work and measurements.

## 4. Prohibited

1. NVIDIA proprietary headers, SDK files, libraries, PTX files, sample code or any other file from NVIDIA software, anywhere in the repository or its releases.
2. Copying text, code examples or diagrams from NVIDIA's documentation or samples, even small ones. Examples in this project's documentation are written originally.
3. Reverse engineering, decompiling or disassembling NVIDIA software or binaries to learn how they work.
4. Code from projects under licenses that conflict with the rules in section 6.
5. Claims or wording that imply affiliation with, endorsement by, or compatibility with NVIDIA (section 7).

Anything in doubt is treated as prohibited until the maintainer decides otherwise in writing.

## 5. Provenance

- Every entry in `DESIGN_DECISIONS.md` has a **Basis** line naming its source: original design, a public specification, or public documentation.
- Each pull request description lists the sources consulted for non-trivial work. "Original" is a valid answer.
- When an idea comes from a paper or open-source project, the source is cited in the relevant document or commit message.

## 6. Licenses and dependencies

- Core code uses permissive licenses only (Apache-2.0, MIT, BSD, Zlib) and is distributed under Apache-2.0.
- Copyleft code (for example GPL or LGPL) may be used only as a development-time tool that is never linked into, copied into or distributed with the core.
- Each vendored dependency is pinned and listed with its license and notice in `THIRD_PARTY_LICENSES`.
- A new dependency needs a license check and the maintainer's approval before merge.

## 7. Naming and wording

- Describe Outrunner as "inspired by the CUDA programming model" or "a vendor-neutral GPU compute stack".
- Do not use "CUDA-compatible", "CUDA replacement", "CUDA clone", or product names that contain "CUDA" (for example the earlier "Tiny CUDA" tagline).
- Product and technology names (CUDA, Vulkan, SPIR-V, and others) are trademarks of their owners and are used descriptively only.
- A trademark policy for the name "Outrunner" is decided with the legal review.

## 8. Benchmarks and baselines

- CUDA baseline code requires NVIDIA's toolkit, so it lives in a **separate repository** that is not part of Outrunner's releases and carries its own notice explaining why.
- The CUDA toolkit's license terms on publishing comparison results are checked before any numbers are shared.
- Inspecting NVIDIA machine code, and annotated CUDA or PTX comparisons, are **on hold** until the legal review defines what is acceptable.

## 9. Automated enforcement (CI)

A scan runs on every pull request and fails the build when it finds:

- files from NVIDIA software, or includes of NVIDIA headers (for example `cuda.h`, `cuda_runtime.h`, `nvrtc.h`, `cublas*.h`, `cudnn*.h`);
- `.cu` or `.cuh` files, or `nvcc` invocations, inside the core repository;
- a missing license header on a source file, or a vendored dependency without a notice entry;
- a dependency with a license outside section 6.

The scan's rules are versioned in the repository and reviewed when they change. The scan catches mistakes; it does not replace judgment.

## 10. Contributors

- A contributor agreement (DCO sign-off or CLA) will be chosen before the first outside contribution is accepted.
- Until then, outside contributions are not merged.
- Contributors will attest that their work is their own or under a compatible license and not derived from prohibited sources.

## 11. Pull request checklist

- [ ] Sources consulted are listed (or "original").
- [ ] No prohibited material is included.
- [ ] New dependencies have a license check and a notice entry.
- [ ] Wording follows section 7.
- [ ] CI scan passes.

## 12. If something prohibited is found

1. Stop work that builds on it.
2. Remove it from the tree, and from history where practical.
3. Record what happened and how it was found.
4. Re-implement the affected part from permitted sources by someone who has not seen the prohibited material.
5. Tell the legal reviewer.

## 13. Review

This policy is reviewed with the legal review before v0.5, and again whenever the project starts a native NVIDIA-specific route or accepts outside contributions.
