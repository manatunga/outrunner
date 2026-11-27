# S2: cooperative-matrix query

**Question:** What does each GPU's driver report for cooperative-matrix support, and do the queries behave?

**Run:**

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\s2_coopmat.exe
```

**Output:** `results_desktop.txt` (RTX 5070 driver 610.88, AMD driver 25.10.30.02, Vulkan SDK 1.4.363).

## Results

|                                        | RTX 5070                                 | AMD Radeon iGPU  |
| -------------------------------------- | ---------------------------------------- | ---------------- |
| `VK_KHR_cooperative_matrix`            | spec 2                                   | spec 2           |
| `VK_NV_cooperative_matrix` / `matrix2` | spec 1 / spec 1                          | not advertised   |
| Feature `cooperativeMatrix`            | true                                     | true             |
| Robust buffer access                   | false                                    | true             |
| Reported shapes                        | 15                                       | 11               |
| Scope                                  | subgroup                                 | subgroup         |
| Input types                            | fp16, bf16, int8/uint8, fp8 (e4m3, e5m2) | fp16, int8/uint8 |
| fp32 inputs                            | none                                     | none             |

RTX 5070 shapes: fp16 16x16x16, 16x8x16 and 16x8x8 (fp16 or fp32 accumulate); int8/uint8 16x16x32 and 16x8x32 (32-bit accumulate); bf16 16x16x16 (fp32 accumulate); fp8 16x16x32 (fp16 or fp32 accumulate).
AMD shapes: 16x16x16 only (fp16 with fp32 or fp16 accumulate; int8/uint8 combinations; some saturating).

## Conclusion

The matrix-assisted tier is feasible on the RTX 5070, with reduced-precision inputs. The AMD result is advertised but unverified for speed. No driver crash was observed on these queries.
