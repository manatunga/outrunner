# S4: lavapipe feature check

**Question:** Does Mesa's lavapipe (software Vulkan) expose the features Outrunner's Vulkan baseline needs, so CI can cover the Vulkan path without a GPU?

**Run:** `./check_features.sh <path to lavapipe ICD json>`

**Outputs:** `vulkaninfo_full.txt`, `vulkaninfo_filtered.txt`

**Results:**

| Feature or limit                              | lavapipe                               | Intel, Mesa (Fedora)       | Intel, Windows driver | Needed by     |
| --------------------------------------------- | -------------------------------------- | -------------------------- | --------------------- | ------------- |
| bufferDeviceAddress                           | true                                   | true                       | true                  | D-019         |
| shaderInt64                                   | true                                   | true                       | not recorded          | D-019         |
| timelineSemaphore                             | true                                   | true                       | not recorded          | D-025         |
| vulkanMemoryModel                             | true                                   | true                       | not recorded          | D-012         |
| vulkanMemoryModelDeviceScope                  | true                                   | true                       | not recorded          | D-012         |
| vulkanMemoryModelAvailabilityVisibilityChains | true                                   | true                       | not recorded          | D-012         |
| shaderBufferInt64Atomics                      | true                                   | true                       | not recorded          | D-012         |
| shaderSharedInt64Atomics                      | true                                   | false                      | not recorded          | D-012         |
| shaderBufferFloat32AtomicAdd                  | true                                   | true                       | not recorded          | D-012         |
| shaderSharedFloat32AtomicAdd                  | true                                   | false                      | not recorded          | D-012         |
| subgroup size range                           | 8-8                                    | 8-32                       | 8-32                  | D-006         |
| maxComputeSharedMemorySize                    | 32 KiB                                 | 48 KiB                     | 48 KiB                | tile sizes    |
| maxComputeWorkGroupInvocations                | 1024                                   | 1024                       | not recorded          | launch limits |
| maxComputeWorkGroupCount                      | 65535 / 65535 / 65535                  | 2147483647 / 65535 / 65535 | not recorded          | launch limits |
| maxComputeWorkGroupSize                       | 1024 / 1024 / 1024                     | 1024 / 1024 / 1024         | not recorded          | launch limits |
| maxPushConstantsSize                          | 256                                    | 256                        | not recorded          | D-027         |
| cooperative matrix                            | extension listed, feature not recorded | none                       | not exposed           | D-020         |
| apiVersion                                    | 1.4.354                                | 1.4.354                    | 1.4.359               |               |
| driverName                                    | llvmpipe                               | Intel Mesa                 | Intel 32.0.101.9150   |               |
| MesaVersion                                   | 26.2.6                                 | 26.2.6                     | -                     |               |

**Conclusion:** CI plan stands. Lavapipe on Mesa 26.2.6 exposes buffer device address, 64-bit integers and timeline semaphores; the real devices and the CI runner's Mesa version still need rechecking.
