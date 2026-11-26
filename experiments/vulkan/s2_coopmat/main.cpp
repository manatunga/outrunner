/// \file main.cpp
/// \brief Spike S2: query cooperative-matrix support through Vulkan.
///
/// Stage 1 (current): prints the Vulkan instance version reported by the
/// loader, then creates and destroys a Vulkan instance. Later stages list
/// physical devices and query their cooperative-matrix extensions, features
/// and properties.
///
/// Throwaway experiment; see docs/ROADMAP.md (S2) and D-020.
#include <vulkan/vulkan.h>
#include <iostream>
#include <cstdint>
#include <array>

/// Prints the Vulkan instance version, then creates and destroys an instance.
///
/// \return 0 if every step succeeded; 1 if querying the version or creating
///         the instance failed (the Vulkan result code is printed).
int main()
{
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

    std::cout << "Vulkan instance version: " << major << "." << minor << "." << patch << '\n';

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::array<const char *, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkInstance instance{};
    VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &instance);
    if (instanceResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance (VkResult: " << instanceResult << ")" << '\n';
        return 1;
    }

    std::cout << "Instance created" << '\n';

    vkDestroyInstance(instance, nullptr);
    return 0;
}