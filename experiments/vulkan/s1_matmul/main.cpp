/// \file main.cpp
/// \brief Spike S1: measure how close a hand-written Vulkan compute matmul
///        gets to vendor-native performance on the RTX 5070.
///
/// Stage B2 (current): parses an optional matrix dimension N from the command
/// line, allocates storage buffers A, B, and C in DEVICE_LOCAL VRAM, uploads
/// and downloads host data via temporary host-visible staging buffers,
/// verifies each shader variant against a double-precision CPU reference with
/// N-scaled tolerance, and prints CSV throughput rows across naive,
/// shared-memory tiled, and register-blocked shaders.
///
/// Throwaway experiment; see docs/ROADMAP.md (S1) and D-039.
#include <vulkan/vulkan.h>
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef SHADER_DIR
#define SHADER_DIR "."
#endif

/// Selects the first discrete GPU exposed by the instance, falling back to
/// the first available physical device.
///
/// \param instance Valid Vulkan instance handle.
/// \return Chosen physical device handle, or VK_NULL_HANDLE on failure.
VkPhysicalDevice pickPhysicalDevice(VkInstance instance)
{
    std::uint32_t deviceCount{};

    VkResult countResult = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (countResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate physical device count (VkResult: " << countResult << ")" << '\n';
        return VK_NULL_HANDLE;
    }
    if (deviceCount == 0)
    {
        std::cerr << "No Vulkan physical devices found" << '\n';
        return VK_NULL_HANDLE;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);

    VkResult enumerateResult = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    if (enumerateResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate physical devices (VkResult: " << enumerateResult << ")" << '\n';
        return VK_NULL_HANDLE;
    }

    devices.resize(deviceCount);

    VkPhysicalDevice chosen = devices[0];
    VkPhysicalDeviceProperties chosenProps{};

    vkGetPhysicalDeviceProperties(chosen, &chosenProps);
    for (const auto device : devices)
    {
        VkPhysicalDeviceProperties props{};

        vkGetPhysicalDeviceProperties(device, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            chosen = device;
            chosenProps = props;
            break;
        }
    }

    std::cerr << "Chosen GPU: " << chosenProps.deviceName << '\n';

    return chosen;
}

/// Queries and prints the queue families exposed by a physical device.
///
/// \param device Physical device handle to inspect.
/// \return List of queue family properties for the device.
std::vector<VkQueueFamilyProperties> getQueueFamilies(VkPhysicalDevice device)
{
    std::uint32_t familyCount{};

    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

    for (std::uint32_t i = 0; i < familyCount; ++i)
    {
        VkQueueFlags flags = families[i].queueFlags;
        bool hasCompute = (flags & VK_QUEUE_COMPUTE_BIT) != 0;
        bool hasGraphics = (flags & VK_QUEUE_GRAPHICS_BIT) != 0;
        bool hasTransfer = (flags & VK_QUEUE_TRANSFER_BIT) != 0;

        std::cerr << std::boolalpha
                  << "  Queue Family " << i
                  << ": compute=" << hasCompute
                  << ", graphics=" << hasGraphics
                  << ", transfer=" << hasTransfer
                  << ", count=" << families[i].queueCount
                  << ", timestampValidBits=" << families[i].timestampValidBits
                  << '\n';
    }

    return families;
}

/// Selects a compute-capable queue family, preferring one without graphics
/// support if available.
///
/// \param families Queue family properties of the target physical device.
/// \return Index of the selected queue family, or std::nullopt if none
///         supports compute.
std::optional<std::uint32_t> selectComputeQueueFamily(std::span<const VkQueueFamilyProperties> families)
{
    std::optional<std::uint32_t> fallback = std::nullopt;

    for (std::uint32_t i = 0; i < families.size(); ++i)
    {
        VkQueueFlags flags = families[i].queueFlags;
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            if (!(flags & VK_QUEUE_GRAPHICS_BIT))
            {
                return i;
            }
            if (!fallback)
            {
                fallback = i;
            }
        }
    }

    return fallback;
}

/// Holds a Vulkan storage buffer and its bound backing device memory.
struct BufferResource
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size{};
};

/// Finds the first memory type index matching both the buffer's type mask
/// and the caller's required memory property flags.
///
/// \param memProps Memory properties of the physical device.
/// \param typeFilter Bitmask of allowed memory types from
///        vkGetBufferMemoryRequirements.
/// \param requiredFlags Required VkMemoryPropertyFlagBits flags.
/// \return Matching memory type index, or std::nullopt if none fits.
std::optional<uint32_t> findMemoryType(
    const VkPhysicalDeviceMemoryProperties memProps,
    std::uint32_t typeFilter,
    VkMemoryPropertyFlags requiredFlags)
{
    for (std::uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
    {
        bool typeSupported = (typeFilter & (1u << i)) != 0;
        bool flagsMatch = (memProps.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags;
        if (typeSupported && flagsMatch)
        {
            return i;
        }
    }

    return std::nullopt;
}

/// Creates a Vulkan buffer with the requested usage and memory property flags,
/// allocates compatible device memory, and binds them together.
///
/// \param device Logical device that will own the buffer and memory.
/// \param memProps Memory properties of the physical device.
/// \param byteSize Requested buffer size in bytes.
/// \param usageFlags Required VkBufferUsageFlagBits mask.
/// \param requiredFlags Required VkMemoryPropertyFlagBits mask.
/// \return Populated BufferResource on success, or an empty BufferResource
///         with VK_NULL_HANDLE handles on failure.
BufferResource createBuffer(
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memProps,
    VkDeviceSize byteSize,
    VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags requiredFlags)
{
    BufferResource res{};

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = byteSize;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult bufferResult = vkCreateBuffer(device, &bufferInfo, nullptr, &res.buffer);
    if (bufferResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create storage buffer (VkResult: " << bufferResult << ")" << '\n';
        return BufferResource{};
    }

    VkMemoryRequirements memReqs{};

    vkGetBufferMemoryRequirements(device, res.buffer, &memReqs);

    std::optional<std::uint32_t> memTypeIndex = findMemoryType(memProps, memReqs.memoryTypeBits, requiredFlags);
    if (!memTypeIndex)
    {
        std::cerr << "No compatible memory type found for storage buffer" << '\n';
        vkDestroyBuffer(device, res.buffer, nullptr);
        return BufferResource{};
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = *memTypeIndex;

    VkResult allocResult = vkAllocateMemory(device, &allocInfo, nullptr, &res.memory);
    if (allocResult != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate buffer memory (VkResult: " << allocResult << ")" << '\n';
        vkDestroyBuffer(device, res.buffer, nullptr);
        return BufferResource{};
    }

    VkResult bindResult = vkBindBufferMemory(device, res.buffer, res.memory, 0);
    if (bindResult != VK_SUCCESS)
    {
        std::cerr << "Failed to bind buffer memory (VkResult: " << bindResult << ")" << '\n';
        vkFreeMemory(device, res.memory, nullptr);
        vkDestroyBuffer(device, res.buffer, nullptr);
        return BufferResource{};
    }

    res.size = byteSize;
    std::cerr << "Allocated storage buffer: " << byteSize << " bytes (memory type " << *memTypeIndex << ")" << '\n';

    return res;
}

/// Destroys a buffer and frees its backing memory if either handle is
/// non-null.
///
/// \param device Logical device that owns the buffer and memory.
/// \param res Buffer resource to tear down.
void destroyBuffer(VkDevice device, BufferResource &res)
{
    if (res.buffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device, res.buffer, nullptr);
    }
    if (res.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, res.memory, nullptr);
    }
}

/// Copies host bytes into a mapped buffer and unmaps the memory.
///
/// \param device Logical device that owns the buffer memory.
/// \param res Target buffer with host-visible memory.
/// \param sourceData Pointer to host source data.
/// \param byteSize Number of bytes to copy.
/// \return true on success; false if mapping the memory failed.
bool uploadFloats(VkDevice device, const BufferResource &res, const void *sourceData, VkDeviceSize byteSize)
{
    void *mapped = nullptr;

    VkResult mapResult = vkMapMemory(device, res.memory, 0, byteSize, 0, &mapped);
    if (mapResult != VK_SUCCESS)
    {
        std::cerr << "Failed to map buffer memory (VkResult: " << mapResult << ")" << '\n';
        return false;
    }

    std::memcpy(mapped, sourceData, static_cast<std::size_t>(byteSize));

    vkUnmapMemory(device, res.memory);

    return true;
}

/// Matrix dimensions (M x K times K x N -> M x N) passed to the compute shader
/// via push constants.
struct PushConstants
{
    std::uint32_t M{};
    std::uint32_t N{};
    std::uint32_t K{};
};

/// Owns the descriptor objects, pipeline layout, and compute pipeline handle
/// for a matrix-multiply shader.
struct ComputePipelineResources
{
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

/// Reads a compiled SPIR-V binary from disk into 32-bit words.
///
/// \param path Path to the .spv file on disk.
/// \return Vector of 32-bit SPIR-V words, or an empty vector if opening,
///         size validation, or reading failed.
std::vector<std::uint32_t> readSpirvFile(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        std::cerr << "Failed to open SPIR-V file: " << path << '\n';
        return {};
    }

    std::streampos byteSize = file.tellg();
    if (byteSize <= 0 || (byteSize % sizeof(std::uint32_t)) != 0)
    {
        std::cerr << "Invalid SPIR-V file size (" << byteSize << " bytes): " << path << '\n';
        return {};
    }

    std::vector<std::uint32_t> words(static_cast<std::size_t>(byteSize) / sizeof(std::uint32_t));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char *>(words.data()), byteSize))
    {
        std::cerr << "Failed to read SPIR-V file: " << path << '\n';
        return {};
    }

    return words;
}

/// Destroys the compute pipeline, pipeline layout, descriptor pool, and
/// descriptor set layout if their handles are non-null.
///
/// \param device Logical device that owns the pipeline resources.
/// \param res Pipeline resource bundle to tear down.
void destroyComputePipeline(VkDevice device, ComputePipelineResources &res)
{
    if (res.pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, res.pipeline, nullptr);
    }
    if (res.pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, res.pipelineLayout, nullptr);
    }
    if (res.descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, res.descriptorPool, nullptr);
    }
    if (res.descriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, res.descriptorSetLayout, nullptr);
    }
}

/// Loads a SPIR-V compute shader, creates descriptor bindings for buffers A,
/// B, and C, and compiles the compute pipeline.
///
/// \param device Logical device that will own the pipeline resources.
/// \param spvPath Path to the compiled SPIR-V compute shader.
/// \param bufA Storage buffer bound at set 0, binding 0 (input matrix A).
/// \param bufB Storage buffer bound at set 0, binding 1 (input matrix B).
/// \param bufC Storage buffer bound at set 0, binding 2 (output matrix C).
/// \return Populated ComputePipelineResources on success, or an empty
///         struct with VK_NULL_HANDLE handles on failure.
ComputePipelineResources createMatmulPipeline(
    VkDevice device,
    const std::filesystem::path &spvPath,
    const BufferResource &bufA,
    const BufferResource &bufB,
    const BufferResource &bufC)
{
    ComputePipelineResources res{};

    std::vector<uint32_t> spirvWords = readSpirvFile(spvPath);
    if (spirvWords.empty())
    {
        return res;
    }

    VkShaderModuleCreateInfo shaderModuleInfo{};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = spirvWords.size() * sizeof(std::uint32_t);
    shaderModuleInfo.pCode = spirvWords.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;

    VkResult smResult = vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule);
    if (smResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create shader module (VkResult: " << smResult << ")" << '\n';
        return res;
    }

    std::array<VkDescriptorSetLayoutBinding, 3> bindings{{{.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
                                                          {.binding = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
                                                          {.binding = 2, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT}}};

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.bindingCount = static_cast<std::uint32_t>(bindings.size());
    setLayoutInfo.pBindings = bindings.data();

    VkResult dslResult = vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &res.descriptorSetLayout);
    if (dslResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create descriptor set layout (VkResult: " << dslResult << ")" << '\n';
        vkDestroyShaderModule(device, shaderModule, nullptr);
        return res;
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 3;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    VkResult poolResult = vkCreateDescriptorPool(device, &poolInfo, nullptr, &res.descriptorPool);
    if (poolResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create descriptor pool (VkResult: " << poolResult << ")" << '\n';
        vkDestroyShaderModule(device, shaderModule, nullptr);
        destroyComputePipeline(device, res);
        return ComputePipelineResources{};
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = res.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &res.descriptorSetLayout;

    VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, &res.descriptorSet);
    if (allocResult != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate descriptor set (VkResult: " << allocResult << ")" << '\n';
        vkDestroyShaderModule(device, shaderModule, nullptr);
        destroyComputePipeline(device, res);
        return ComputePipelineResources{};
    }

    VkDescriptorBufferInfo bufInfoA{};
    bufInfoA.buffer = bufA.buffer;
    bufInfoA.offset = 0;
    bufInfoA.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo bufInfoB{};
    bufInfoB.buffer = bufB.buffer;
    bufInfoB.offset = 0;
    bufInfoB.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo bufInfoC{};
    bufInfoC.buffer = bufC.buffer;
    bufInfoC.offset = 0;
    bufInfoC.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 3> writes{{{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = res.descriptorSet, .dstBinding = 0, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &bufInfoA},
                                                {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = res.descriptorSet, .dstBinding = 1, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &bufInfoB},
                                                {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = res.descriptorSet, .dstBinding = 2, .descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &bufInfoC}}};

    vkUpdateDescriptorSets(device, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &res.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushRange;

    VkResult plResult = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &res.pipelineLayout);
    if (plResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create pipeline layout (VkResult: " << plResult << ")" << '\n';
        vkDestroyShaderModule(device, shaderModule, nullptr);
        destroyComputePipeline(device, res);
        return ComputePipelineResources{};
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = res.pipelineLayout;

    VkResult cpResult = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &res.pipeline);
    vkDestroyShaderModule(device, shaderModule, nullptr);
    if (cpResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create compute pipeline (VkResult: " << cpResult << ")" << '\n';
        destroyComputePipeline(device, res);
        return ComputePipelineResources{};
    }

    std::cerr << "Compute pipeline created from " << spvPath.filename().string() << '\n';

    return res;
}

/// Owns the command pool, primary command buffer, and submission fence used
/// to dispatch compute work.
struct CommandResources
{
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
};

/// Maps a host-visible buffer and copies its contents into host memory.
///
/// \param device Logical device that owns the buffer memory.
/// \param res Source buffer with host-visible memory.
/// \param destData Pointer to host destination buffer.
/// \param byteSize Number of bytes to read back.
/// \return true on success; false if mapping the memory failed.
bool downloadFloats(VkDevice device, const BufferResource &res, void *destData, VkDeviceSize byteSize)
{
    void *mapped = nullptr;

    VkResult mapResult = vkMapMemory(device, res.memory, 0, byteSize, 0, &mapped);
    if (mapResult != VK_SUCCESS)
    {
        std::cerr << "Failed to map buffer memory for readback (VkResult: " << mapResult << ")" << '\n';
        return false;
    }

    std::memcpy(destData, mapped, static_cast<std::size_t>(byteSize));

    vkUnmapMemory(device, res.memory);
    return true;
}

/// Destroys the fence and command pool (implicitly freeing its command
/// buffers) if their handles are non-null.
///
/// \param device Logical device that owns the command resources.
/// \param res Command resource bundle to tear down.
void destroyCommandResources(VkDevice device, CommandResources &res)
{
    if (res.fence != VK_NULL_HANDLE)
    {
        vkDestroyFence(device, res.fence, nullptr);
    }
    if (res.commandPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, res.commandPool, nullptr);
    }
}

/// Creates a command pool for the target queue family, allocates a primary
/// command buffer, and creates an unsignaled fence.
///
/// \param device Logical device that will own the command resources.
/// \param queueFamilyIndex Queue family index matching the compute queue.
/// \return Populated CommandResources on success, or an empty struct with
///         VK_NULL_HANDLE handles on failure.
CommandResources createCommandResources(VkDevice device, std::uint32_t queueFamilyIndex)
{
    CommandResources res{};

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;

    VkResult poolResult = vkCreateCommandPool(device, &poolInfo, nullptr, &res.commandPool);
    if (poolResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create command pool (VkResult: " << poolResult << ")" << '\n';
        return res;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = res.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkResult allocResult = vkAllocateCommandBuffers(device, &allocInfo, &res.commandBuffer);
    if (allocResult != VK_SUCCESS)
    {
        std::cerr << "Failed to allocate command buffer (VkResult: " << allocResult << ")" << '\n';
        destroyCommandResources(device, res);
        return CommandResources{};
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkResult fenceResult = vkCreateFence(device, &fenceInfo, nullptr, &res.fence);
    if (fenceResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create fence (VkResult: " << fenceResult << ")" << '\n';
        destroyCommandResources(device, res);
        return CommandResources{};
    }

    return res;
}

/// Records and submits a synchronous vkCmdCopyBuffer transfer between two
/// buffers and waits on the command fence until completion.
///
/// \param device Logical device that owns the fence.
/// \param transferQueue Transfer-capable queue handle for submission.
/// \param cmdRes Command pool, command buffer, and fence bundle.
/// \param srcBuffer Source buffer handle.
/// \param dstBuffer Destination buffer handle.
/// \param byteSize Number of bytes to copy.
/// \return true if recording, submission, and fence wait all succeeded;
///         false otherwise.
bool copyBuffer(
    VkDevice device,
    VkQueue transferQueue,
    const CommandResources &cmdRes,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize byteSize)
{
    VkResult resetResult = vkResetFences(device, 1, &cmdRes.fence);
    if (resetResult != VK_SUCCESS)
    {
        std::cerr << "Failed to reset fence before buffer copy (VkResult: " << resetResult << ")" << '\n';
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult beginResult = vkBeginCommandBuffer(cmdRes.commandBuffer, &beginInfo);
    if (beginResult != VK_SUCCESS)
    {
        std::cerr << "Failed to begin command buffer for copy (VkResult: " << beginResult << ")" << '\n';
        return false;
    }

    VkBufferCopy copyRegion{.srcOffset = 0, .dstOffset = 0, .size = byteSize};

    vkCmdCopyBuffer(cmdRes.commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    VkResult endResult = vkEndCommandBuffer(cmdRes.commandBuffer);
    if (endResult != VK_SUCCESS)
    {
        std::cerr << "Failed to end command buffer for copy (VkResult: " << endResult << ")" << '\n';
        return false;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdRes.commandBuffer;

    VkResult submitResult = vkQueueSubmit(transferQueue, 1, &submitInfo, cmdRes.fence);
    if (submitResult != VK_SUCCESS)
    {
        std::cerr << "Failed to submit buffer copy (VkResult: " << submitResult << ")" << '\n';
        return false;
    }

    VkResult waitResult = vkWaitForFences(device, 1, &cmdRes.fence, VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS)
    {
        std::cerr << "Failed waiting for buffer copy fence (VkResult: " << waitResult << ")" << '\n';
        return false;
    }

    return true;
}

/// Uploads host memory into a DEVICE_LOCAL buffer through a temporary
/// host-visible staging buffer.
///
/// \param device Logical device that owns the buffers.
/// \param memProps Memory properties of the physical device.
/// \param transferQueue Transfer-capable queue handle for the copy.
/// \param cmdRes Command pool, command buffer, and fence bundle.
/// \param dstDeviceBuf Destination DEVICE_LOCAL buffer.
/// \param sourceData Pointer to host source bytes.
/// \param byteSize Number of bytes to upload.
/// \return true on success; false if staging allocation, mapping, or copy
///         failed.
bool uploadToDeviceBuffer(
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memProps,
    VkQueue transferQueue,
    const CommandResources &cmdRes,
    const BufferResource &dstDeviceBuf,
    const void *sourceData,
    VkDeviceSize byteSize)
{
    constexpr VkMemoryPropertyFlags stagingFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    BufferResource staging = createBuffer(device, memProps, byteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingFlags);
    if (staging.buffer == VK_NULL_HANDLE)
    {
        return false;
    }

    bool ok = uploadFloats(device, staging, sourceData, byteSize) && copyBuffer(device, transferQueue, cmdRes, staging.buffer, dstDeviceBuf.buffer, byteSize);

    destroyBuffer(device, staging);

    return ok;
}

/// Downloads data from a DEVICE_LOCAL buffer into host memory through a
/// temporary host-visible staging buffer.
///
/// \param device Logical device that owns the buffers.
/// \param memProps Memory properties of the physical device.
/// \param transferQueue Transfer-capable queue handle for the copy.
/// \param cmdRes Command pool, command buffer, and fence bundle.
/// \param srcDeviceBuf Source DEVICE_LOCAL buffer.
/// \param destData Pointer to host destination buffer.
/// \param byteSize Number of bytes to download.
/// \return true on success; false if staging allocation, copy, or mapping
///         failed.
bool downloadFromDeviceBuffer(
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memProps,
    VkQueue transferQueue,
    const CommandResources &cmdRes,
    const BufferResource &srcDeviceBuf,
    void *destData,
    VkDeviceSize byteSize)
{
    constexpr VkMemoryPropertyFlags stagingFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    BufferResource staging = createBuffer(device, memProps, byteSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, stagingFlags);
    if (staging.buffer == VK_NULL_HANDLE)
    {
        return false;
    }

    bool ok = copyBuffer(device, transferQueue, cmdRes, srcDeviceBuf.buffer, staging.buffer, byteSize) && downloadFloats(device, staging, destData, byteSize);

    destroyBuffer(device, staging);

    return ok;
}

/// Records a compute matmul dispatch into the command buffer, submits it to
/// the compute queue, and blocks on the fence until execution finishes.
///
/// \param device Logical device that owns the fence.
/// \param computeQueue Queue handle on which to submit the command buffer.
/// \param cmdRes Command pool, command buffer, and fence bundle.
/// \param pipelineRes Compute pipeline, pipeline layout, and descriptor set.
/// \param dims Matrix dimensions (M, N, K) pushed to the shader.
/// \return true if recording, submission, and fence wait all succeeded;
///         false otherwise.
bool dispatchMatmul(
    VkDevice device, VkQueue computeQueue,
    const CommandResources &cmdRes,
    const ComputePipelineResources &pipelineRes,
    PushConstants dims)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult beginResult = vkBeginCommandBuffer(cmdRes.commandBuffer, &beginInfo);
    if (beginResult != VK_SUCCESS)
    {
        std::cerr << "Failed to begin command buffer (VkResult: " << beginResult << ")" << '\n';
        return false;
    }

    vkCmdBindPipeline(cmdRes.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineRes.pipeline);
    vkCmdBindDescriptorSets(
        cmdRes.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineRes.pipelineLayout,
        0, 1, &pipelineRes.descriptorSet,
        0, nullptr);
    vkCmdPushConstants(
        cmdRes.commandBuffer,
        pipelineRes.pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(PushConstants), &dims);

    constexpr std::uint32_t tileSize = 16u;
    std::uint32_t groupCountX = (dims.N + tileSize - 1u) / tileSize;
    std::uint32_t groupCountY = (dims.M + tileSize - 1u) / tileSize;

    vkCmdDispatch(cmdRes.commandBuffer, groupCountX, groupCountY, 1u);

    VkResult endResult = vkEndCommandBuffer(cmdRes.commandBuffer);
    if (endResult != VK_SUCCESS)
    {
        std::cerr << "Failed to end command buffer (VkResult: " << endResult << ")" << '\n';
        return false;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdRes.commandBuffer;

    VkResult submitResult = vkQueueSubmit(computeQueue, 1, &submitInfo, cmdRes.fence);
    if (submitResult != VK_SUCCESS)
    {
        std::cerr << "Failed to submit compute queue (VkResult: " << submitResult << ")" << '\n';
        return false;
    }

    VkResult waitResult = vkWaitForFences(device, 1, &cmdRes.fence, VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS)
    {
        std::cerr << "Failed waiting for compute fence (VkResult: " << waitResult << ")" << '\n';
        return false;
    }

    return true;
}

/// Computes a reference matrix multiplication C = A * B on the CPU using
/// double-precision accumulation.
///
/// \param A Input matrix A of dimensions M x K in row-major order.
/// \param B Input matrix B of dimensions K x N in row-major order.
/// \param dims Matrix dimensions (M, N, K).
/// \return Output matrix C of dimensions M x N rounded to single precision.
std::vector<float> cpuMatmulReference(std::span<const float> A, std::span<const float> B, PushConstants dims)
{
    std::vector<float> refC(static_cast<std::size_t>(dims.M) * dims.N, 0.0f);

    for (std::size_t row = 0; row < dims.M; ++row)
    {
        for (std::size_t col = 0; col < dims.N; ++col)
        {
            double sum = 0.0;

            for (std::size_t k = 0; k < dims.K; ++k)
            {
                double aVal = static_cast<double>(A[static_cast<std::size_t>(row) * dims.K + k]);
                double bVal = static_cast<double>(B[static_cast<std::size_t>(k) * dims.N + col]);
                sum += aVal * bVal;
            }

            refC[static_cast<std::size_t>(row) * dims.N + col] = static_cast<float>(sum);
        }
    }

    return refC;
}

/// Parses an optional positive matrix dimension N from command-line arguments,
/// defaulting to 1024 when no positional argument is provided.
///
/// \param argc Argument count passed to main.
/// \param argv Argument vector passed to main.
/// \return Parsed matrix dimension N on success, or std::nullopt if invalid.
std::optional<std::uint32_t> parseMatrixSize(int argc, char **argv)
{
    if (argc <= 1)
    {
        return 1024u;
    }
    if (argc != 2)
    {
        std::cerr << "Usage: s1_matmul [N]" << '\n';
        return std::nullopt;
    }

    std::string_view arg(argv[1]);
    if (arg.empty())
    {
        std::cerr << "Invalid matrix size N: empty argument" << '\n';
        return std::nullopt;
    }

    std::uint32_t value{};

    const auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value, 10);

    if (ec == std::errc::result_out_of_range)
    {
        std::cerr << "Invalid matrix size N (out of range for uint32_t): " << arg << '\n';
        return std::nullopt;
    }

    if (ec != std::errc{} || ptr != arg.data() + arg.size())
    {
        std::cerr << "Invalid matrix size N (expected positive integer): " << arg << '\n';
        return std::nullopt;
    }

    if (value == 0u)
    {
        std::cerr << "Invalid matrix size N (must be greater than 0)" << '\n';
        return std::nullopt;
    }

    return value;
}

/// Verifies that every element of the GPU output matrix is finite and within
/// an absolute error tolerance of the CPU reference result.
///
/// \param gpuC Matrix output read back from the GPU.
/// \param refC CPU reference output matrix.
/// \param tolerance Maximum allowed element-wise absolute difference.
/// \param outMaxAbsError Receives the largest element-wise absolute error.
/// \return true if sizes match, all GPU outputs are finite, and
///         maxAbsError <= tolerance; false otherwise.
bool verifyMatmulResult(std::span<const float> gpuC, std::span<const float> refC, float tolerance, float &outMaxAbsError)
{
    if (gpuC.size() != refC.size())
    {
        std::cerr << "Verification size mismatch: gpuC=" << gpuC.size() << " vs refC=" << refC.size() << '\n';
        return false;
    }

    float maxAbsError = 0.0f;
    std::size_t worstIndex{0};

    for (std::size_t i = 0; i < gpuC.size(); ++i)
    {
        if (!std::isfinite(gpuC[i]))
        {
            std::cerr << "Verification FAILED: non-finite value at index " << i << " (" << gpuC[i] << ")" << '\n';
            return false;
        }

        float err = std::abs(gpuC[i] - refC[i]);
        if (err > maxAbsError)
        {
            maxAbsError = err;
            worstIndex = i;
        }
    }

    outMaxAbsError = maxAbsError;
    if (maxAbsError > tolerance)
    {
        std::cerr << "Verification FAILED: maxAbsError=" << maxAbsError
                  << " > tolerance=" << tolerance
                  << " at index " << worstIndex
                  << " (gpu=" << gpuC[worstIndex]
                  << ", ref=" << refC[worstIndex]
                  << ")" << '\n';
        return false;
    }

    std::cerr << "Verification PASSED: maxAbsError=" << maxAbsError
              << " (tolerance=" << tolerance
              << ", elements=" << gpuC.size()
              << ")" << '\n';
    return true;
}

/// Owns a two-entry GPU timestamp query pool and its hardware tick period in
/// nanoseconds.
struct TimestampQueryResources
{
    VkQueryPool queryPool = VK_NULL_HANDLE;
    float timestampPeriodNs = 0.0f;
};

/// Destroys the timestamp query pool if its handle is non-null.
///
/// \param device Logical device that owns the query pool.
/// \param res Timestamp query resource bundle to tear down.
void destroyTimestampQueryPool(VkDevice device, TimestampQueryResources &res)
{
    if (res.queryPool != VK_NULL_HANDLE)
    {
        vkDestroyQueryPool(device, res.queryPool, nullptr);
    }
}

/// Creates a timestamp query pool with two slots (start and end) for GPU-side
/// kernel timing.
///
/// \param device Logical device that will own the query pool.
/// \param timestampPeriodNs Nanoseconds per GPU timestamp tick from
///        VkPhysicalDeviceLimits::timestampPeriod.
/// \return Populated TimestampQueryResources on success, or an empty struct
///         with VK_NULL_HANDLE on failure.
TimestampQueryResources createTimestampQueryPool(VkDevice device, float timestampPeriodNs)
{
    TimestampQueryResources res{};
    res.timestampPeriodNs = timestampPeriodNs;

    VkQueryPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    poolInfo.queryCount = 2;

    VkResult poolResult = vkCreateQueryPool(device, &poolInfo, nullptr, &res.queryPool);
    if (poolResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create timestamp query pool (VkResult: " << poolResult << ")" << '\n';
        return TimestampQueryResources{};
    }

    return res;
}

/// Resets the fence and query pool, brackets a compute matmul dispatch with
/// top-of-pipe and bottom-of-pipe GPU timestamps, waits for completion, and
/// returns the elapsed GPU time in nanoseconds.
///
/// \param device Logical device that owns the fence and query pool.
/// \param computeQueue Queue handle on which to submit the command buffer.
/// \param cmdRes Command pool, command buffer, and fence bundle.
/// \param pipelineRes Compute pipeline, pipeline layout, and descriptor set.
/// \param queryRes Timestamp query pool and tick period.
/// \param dims Matrix dimensions (M, N, K) pushed to the shader.
/// \param tileWidth Output columns covered per workgroup along X.
/// \param tileHeight Output rows covered per workgroup along Y.
/// \return Elapsed GPU execution time in nanoseconds, or std::nullopt on
///         failure.
std::optional<double> dispatchMatmulTimed(
    VkDevice device,
    VkQueue computeQueue,
    const CommandResources &cmdRes,
    const ComputePipelineResources &pipelineRes,
    const TimestampQueryResources &queryRes,
    PushConstants dims,
    std::uint32_t tileWidth,
    std::uint32_t tileHeight)
{
    VkResult resetResult = vkResetFences(device, 1, &cmdRes.fence);
    if (resetResult != VK_SUCCESS)
    {
        std::cerr << "Failed to reset compute fence (VkResult: " << resetResult << ")" << '\n';
        return std::nullopt;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult beginResult = vkBeginCommandBuffer(cmdRes.commandBuffer, &beginInfo);
    if (beginResult != VK_SUCCESS)
    {
        std::cerr << "Failed to begin command buffer (VkResult: " << beginResult << ")" << '\n';
        return std::nullopt;
    }

    vkCmdResetQueryPool(cmdRes.commandBuffer, queryRes.queryPool, 0, 2);
    vkCmdWriteTimestamp(cmdRes.commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryRes.queryPool, 0);

    vkCmdBindPipeline(cmdRes.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineRes.pipeline);
    vkCmdBindDescriptorSets(
        cmdRes.commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineRes.pipelineLayout,
        0, 1, &pipelineRes.descriptorSet,
        0, nullptr);
    vkCmdPushConstants(
        cmdRes.commandBuffer,
        pipelineRes.pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(PushConstants), &dims);

    std::uint32_t groupCountX = (dims.N + tileWidth - 1u) / tileWidth;
    std::uint32_t groupCountY = (dims.M + tileHeight - 1u) / tileHeight;

    vkCmdDispatch(cmdRes.commandBuffer, groupCountX, groupCountY, 1u);
    vkCmdWriteTimestamp(cmdRes.commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryRes.queryPool, 1);

    VkResult endResult = vkEndCommandBuffer(cmdRes.commandBuffer);
    if (endResult != VK_SUCCESS)
    {
        std::cerr << "Failed to end command buffer (VkResult: " << endResult << ")" << '\n';
        return std::nullopt;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdRes.commandBuffer;

    VkResult submitResult = vkQueueSubmit(computeQueue, 1, &submitInfo, cmdRes.fence);
    if (submitResult != VK_SUCCESS)
    {
        std::cerr << "Failed to submit compute queue (VkResult: " << submitResult << ")" << '\n';
        return std::nullopt;
    }

    VkResult waitResult = vkWaitForFences(device, 1, &cmdRes.fence, VK_TRUE, UINT64_MAX);
    if (waitResult != VK_SUCCESS)
    {
        std::cerr << "Failed waiting for compute fence (VkResult: " << waitResult << ")" << '\n';
        return std::nullopt;
    }

    std::array<std::uint64_t, 2> ticks{};

    VkResult queryResult = vkGetQueryPoolResults(
        device, queryRes.queryPool, 0, 2,
        sizeof(ticks), ticks.data(),
        sizeof(std::uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    if (queryResult != VK_SUCCESS)
    {
        std::cerr << "Failed to read timestamp query results (VkResult: " << queryResult << ")" << '\n';
        return std::nullopt;
    }

    std::uint64_t elapsedTicks = ticks[1] - ticks[0];
    double elapsedNs = static_cast<double>(elapsedTicks) * static_cast<double>(queryRes.timestampPeriodNs);

    return elapsedNs;
}

/// Describes a compiled compute shader variant and the output tile dimensions
/// covered by a single workgroup dispatch.
struct VariantSpec
{
    const char *label;
    const char *spvFilename;
    std::uint32_t outputTileWidth;
    std::uint32_t outputTileHeight;
};

/// Resets output buffer C, builds the pipeline for a shader variant, runs
/// warmup dispatches, verifies the output against the CPU reference, measures
/// median GPU throughput over timed dispatches, and cleans up the pipeline.
///
/// \param device Logical device that owns the pipeline and buffers.
/// \param memProps Memory properties of the physical device.
/// \param computeQueue Queue handle on which to submit compute work.
/// \param cmdRes Command pool, command buffer, and fence bundle.
/// \param queryRes Timestamp query pool and tick period.
/// \param bufA Storage buffer for input matrix A.
/// \param bufB Storage buffer for input matrix B.
/// \param bufC Storage buffer for output matrix C.
/// \param zeroC Zero-filled host buffer used to clear bufC before each run.
/// \param refC Precomputed double-accumulated CPU reference matrix C.
/// \param dims Matrix dimensions (M, N, K) pushed to the shader.
/// \param spec Shader variant metadata and workgroup output tile size.
/// \return true if pipeline creation, warmup, verification, and timed runs all
///         succeeded; false otherwise.
bool runShaderVariant(
    VkDevice device,
    const VkPhysicalDeviceMemoryProperties &memProps,
    VkQueue computeQueue,
    const CommandResources &cmdRes,
    const TimestampQueryResources &queryRes,
    const BufferResource &bufA,
    const BufferResource &bufB,
    const BufferResource &bufC,
    std::span<const float> zeroC,
    std::span<const float> refC,
    PushConstants dims,
    VariantSpec spec)
{
    std::cerr << "\n--- Variant: " << spec.label << " (" << spec.spvFilename << ") ---" << '\n';

    bool hasUploaded = uploadToDeviceBuffer(device, memProps, computeQueue, cmdRes, bufC, zeroC.data(), bufC.size);
    if (!hasUploaded)
    {
        return false;
    }

    std::filesystem::path spvPath = std::filesystem::path(SHADER_DIR) / spec.spvFilename;

    ComputePipelineResources pipelineRes = createMatmulPipeline(device, spvPath, bufA, bufB, bufC);
    if (pipelineRes.pipeline == VK_NULL_HANDLE)
    {
        return false;
    }

    bool ok = true;

    constexpr int warmupRuns = 3;

    for (int i = 0; i < warmupRuns; ++i)
    {
        std::optional<double> warmupElapsedNs = dispatchMatmulTimed(
            device, computeQueue, cmdRes,
            pipelineRes, queryRes, dims,
            spec.outputTileWidth, spec.outputTileHeight);
        if (!warmupElapsedNs)
        {
            ok = false;
            break;
        }
    }

    float maxAbsError = 0.0f;

    if (ok)
    {
        std::vector<float> gpuC = std::vector<float>(refC.size(), 0.0f);

        bool hasDownloaded = downloadFromDeviceBuffer(device, memProps, computeQueue, cmdRes, bufC, gpuC.data(), bufC.size);
        if (!hasDownloaded)
        {
            ok = false;
        }
        else
        {
            float tolerance = 1e-4f * (static_cast<float>(dims.K) / 1024.0f);

            bool passedVerification = verifyMatmulResult(gpuC, refC, tolerance, maxAbsError);
            if (!passedVerification)
            {
                ok = false;
            }
        }
    }

    if (ok)
    {
        constexpr int timedRuns = 10;

        std::vector<double> samplesNs{};
        samplesNs.reserve(timedRuns);

        for (int i = 0; i < timedRuns; ++i)
        {
            std::optional<double> elapsedNs = dispatchMatmulTimed(
                device, computeQueue, cmdRes,
                pipelineRes, queryRes, dims,
                spec.outputTileWidth, spec.outputTileHeight);
            if (!elapsedNs)
            {
                ok = false;
                break;
            }

            samplesNs.push_back(*elapsedNs);
        }

        if (ok)
        {
            std::sort(samplesNs.begin(), samplesNs.end());

            double medianNs = samplesNs[samplesNs.size() / 2];
            double minNs = samplesNs.front();

            double flops = 2.0 * static_cast<double>(dims.M) * dims.N * dims.K;
            double gflops = flops / medianNs;

            double medianMs = medianNs / 1e6;
            double minMs = minNs / 1e6;

            std::cout << spec.label << ","
                      << dims.N << ","
                      << medianMs << ","
                      << minMs << ","
                      << gflops << ","
                      << maxAbsError << '\n';
        }
    }

    destroyComputePipeline(device, pipelineRes);

    return ok;
}

/// Picks a physical device, selects a compute queue family, creates a
/// logical device and compute queue, allocates and populates DEVICE_LOCAL
/// storage buffers A, B, and C for an N x N x N matmul, runs verification
/// and timestamp-timed benchmarks across shader variants, prints CSV rows,
/// and cleans up all device-owned resources before returning.
///
/// \param instance Valid Vulkan instance handle.
/// \param matrixSize Square matrix dimension N (M = N = K = matrixSize).
/// \return 0 if every step succeeded; 1 if any device, queue, buffer,
///         upload, verification, or timing step failed.
int setupComputeDevice(VkInstance instance, std::uint32_t matrixSize)
{
    VkPhysicalDevice physicalDevice = pickPhysicalDevice(instance);
    if (physicalDevice == VK_NULL_HANDLE)
    {
        return 1;
    }

    VkPhysicalDeviceProperties deviceProps{};
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

    std::vector<VkQueueFamilyProperties> families = getQueueFamilies(physicalDevice);

    std::optional<std::uint32_t> familyIndex = selectComputeQueueFamily(families);
    if (!familyIndex)
    {
        std::cerr << "No compute-capable queue family found" << '\n';
        return 1;
    }

    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = *familyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

    VkDevice device{};

    VkResult deviceResult = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    if (deviceResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create logical device (VkResult: " << deviceResult << ")" << '\n';
        return 1;
    }

    VkQueue computeQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, *familyIndex, 0, &computeQueue);

    std::cerr << "Logical device and compute queue ready (family " << *familyIndex << ")" << '\n';

    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

    PushConstants dims{.M = matrixSize, .N = matrixSize, .K = matrixSize};

    std::size_t countA = static_cast<std::size_t>(dims.M) * dims.K;
    std::size_t countB = static_cast<std::size_t>(dims.K) * dims.N;
    std::size_t countC = static_cast<std::size_t>(dims.M) * dims.N;

    VkDeviceSize bytesA = static_cast<VkDeviceSize>(countA) * sizeof(float);
    VkDeviceSize bytesB = static_cast<VkDeviceSize>(countB) * sizeof(float);
    VkDeviceSize bytesC = static_cast<VkDeviceSize>(countC) * sizeof(float);

    CommandResources cmdRes = createCommandResources(device, *familyIndex);
    if (cmdRes.fence == VK_NULL_HANDLE)
    {
        vkDestroyDevice(device, nullptr);
        return 1;
    }

    constexpr VkBufferUsageFlags deviceUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    BufferResource bufA = createBuffer(device, memProps, bytesA, deviceUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    BufferResource bufB = createBuffer(device, memProps, bytesB, deviceUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    BufferResource bufC = createBuffer(device, memProps, bytesC, deviceUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (bufA.buffer == VK_NULL_HANDLE || bufB.buffer == VK_NULL_HANDLE || bufC.buffer == VK_NULL_HANDLE)
    {
        destroyBuffer(device, bufC);
        destroyBuffer(device, bufB);
        destroyBuffer(device, bufA);
        destroyCommandResources(device, cmdRes);
        vkDestroyDevice(device, nullptr);
        return 1;
    }

    std::vector<float> hostA(countA);
    std::vector<float> hostB(countB);
    std::vector<float> hostC(countC, 0.0f);

    for (std::size_t i = 0; i < countA; ++i)
    {
        hostA[i] = static_cast<float>(i % 7) * 0.25f;
    }
    for (std::size_t i = 0; i < countB; ++i)
    {
        hostB[i] = static_cast<float>((i + 3) % 5) * 0.5f;
    }

    bool ok = uploadToDeviceBuffer(device, memProps, computeQueue, cmdRes, bufA, hostA.data(), bytesA) &&
              uploadToDeviceBuffer(device, memProps, computeQueue, cmdRes, bufB, hostB.data(), bytesB) &&
              uploadToDeviceBuffer(device, memProps, computeQueue, cmdRes, bufC, hostC.data(), bytesC);

    if (ok)
    {
        std::cerr << "Uploaded host data to A(" << dims.M << "x" << dims.K
                  << "), B(" << dims.K << "x" << dims.N
                  << "), zeroed C(" << dims.M << "x" << dims.N
                  << ")" << '\n';
    }

    int status = ok ? 0 : 1;
    if (status == 0 && families[*familyIndex].timestampValidBits == 0)
    {
        std::cerr << "Selected queue family does not support timestamps" << '\n';
        status = 1;
    }

    if (status == 0)
    {
        std::vector<float> refC = cpuMatmulReference(hostA, hostB, dims);

        TimestampQueryResources queryRes = createTimestampQueryPool(device, deviceProps.limits.timestampPeriod);

        if (cmdRes.fence == VK_NULL_HANDLE || queryRes.queryPool == VK_NULL_HANDLE)
        {
            status = 1;
        }
        else
        {
            constexpr std::array<VariantSpec, 3> variants{{{.label = "Naive 16x16", .spvFilename = "matmul_naive.spv", .outputTileWidth = 16u, .outputTileHeight = 16u},
                                                           {.label = "Shared-Memory Tiled 16x16", .spvFilename = "matmul_tiled.spv", .outputTileWidth = 16u, .outputTileHeight = 16u},
                                                           {.label = "Register-Blocked 64x64 (4x4/thread)", .spvFilename = "matmul_regblocked.spv", .outputTileWidth = 64u, .outputTileHeight = 64u}}};

            std::cout << "variant,N,median_ms,min_ms,GFLOPs,maxAbsError\n";
            for (const auto &spec : variants)
            {
                bool variantOk = runShaderVariant(
                    device, memProps, computeQueue,
                    cmdRes, queryRes,
                    bufA, bufB, bufC,
                    hostC, refC, dims, spec);
                if (!variantOk)
                {
                    status = 1;
                    break;
                }
            }
        }

        destroyTimestampQueryPool(device, queryRes);
    }

    destroyBuffer(device, bufC);
    destroyBuffer(device, bufB);
    destroyBuffer(device, bufA);
    destroyCommandResources(device, cmdRes);
    vkDestroyDevice(device, nullptr);

    return status;
}

/// Parses the optional matrix dimension N from the command line, prints the
/// Vulkan instance version, creates an instance, sets up a logical device and
/// compute queue on the discrete GPU via setupComputeDevice, and destroys the
/// instance before exiting.
///
/// \param argc Command-line argument count.
/// \param argv Command-line argument vector.
/// \return 0 if every step succeeded; 1 if argument parsing, querying the
///         version, creating the instance, or setting up the compute device
///         failed.
int main(int argc, char **argv)
{

    std::optional<std::uint32_t> matrixSize = parseMatrixSize(argc, argv);
    if (!matrixSize)
    {
        return 1;
    }

    std::uint32_t version{};

    VkResult enumerateResult = vkEnumerateInstanceVersion(&version);
    if (enumerateResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate Vulkan instance version (VkResult: " << enumerateResult << ")" << '\n';
        return 1;
    }

    std::uint32_t major = VK_API_VERSION_MAJOR(version);
    std::uint32_t minor = VK_API_VERSION_MINOR(version);
    std::uint32_t patch = VK_API_VERSION_PATCH(version);

    std::cerr << "Vulkan instance version: " << major << "." << minor << "." << patch << '\n';

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::array<const char *, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<std::uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkInstance instance{};
    VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &instance);
    if (instanceResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance (VkResult: " << instanceResult << ")" << '\n';
        return 1;
    }

    std::cerr << "Instance created" << '\n';

    int status = setupComputeDevice(instance, *matrixSize);

    vkDestroyInstance(instance, nullptr);

    return status;
}