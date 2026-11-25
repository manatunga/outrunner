#!/usr/bin/env bash
# S4: dump lavapipe's Vulkan features and limits relevant to Outrunner.
# Usage: ./check_features.sh /usr/share/vulkan/icd.d/lvp_icd.x86_64.json"
set -euo pipefail

ICD="${1:?usage: $0 /usr/share/vulkan/icd.d/lvp_icd.x86_64.json}"
if [ ! -f "$ICD" ]; then
  echo "ICD file not found: $ICD" >&2
  exit 1
fi

OUT_DIR="$(dirname "$0")"
FULL="$OUT_DIR/vulkaninfo_full.txt"
FILTERED="$OUT_DIR/vulkaninfo_filtered.txt"

# Both variables, because different loader versions read different ones.
export VK_DRIVER_FILES="$ICD"
export VK_ICD_FILENAMES="$ICD"

vulkaninfo --summary
vulkaninfo > "$FULL"

grep -E "deviceName|driverName|driverInfo|apiVersion|driverVersion|bufferDeviceAddress|shaderInt64|timelineSemaphore|synchronization2|vulkanMemoryModel|shaderBufferInt64Atomics|shaderSharedInt64Atomics|shaderBufferFloat32Atomic|cooperativeMatrix|subgroupSize|minSubgroupSize|maxSubgroupSize|maxComputeSharedMemorySize|maxComputeWorkGroupInvocations|maxComputeWorkGroupSize|maxComputeWorkGroupCount|maxPushConstantsSize" "$FULL" > "$FILTERED" || true

echo "Full report:     $FULL"
echo "Filtered report: $FILTERED"