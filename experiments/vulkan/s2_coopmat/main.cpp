/// \file main.cpp
/// \brief Spike S2: query cooperative-matrix support through Vulkan.
///
/// Stage 2 (current): prints the Vulkan instance version, creates an
/// instance, and lists the physical devices with their name, type, vendor ID
/// and API version, then destroys the instance. Later stages query each
/// device's cooperative-matrix extensions, features and properties.
///
/// Throwaway experiment; see docs/ROADMAP.md (S2) and D-020.
#include <vulkan/vulkan.h>
#include <iostream>
#include <cstdint>
#include <array>
#include <vector>

/// Converts a Vulkan physical device type enum into a human-readable label.
///
/// \param deviceType The device type reported in VkPhysicalDeviceProperties.
/// \return A null-terminated string literal ("discrete", "integrated",
///         "virtual", "cpu", or "other").
const char *deviceTypeToString(VkPhysicalDeviceType deviceType)
{
    switch (deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return "discrete";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return "integrated";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return "virtual";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return "cpu";
    default:
        return "other";
    }
}

/// Enumerates all physical devices visible to the instance and prints their
/// name, type, hexadecimal vendor ID, and supported Vulkan API version.
///
/// \param instance A valid VkInstance handle.
/// \return 0 if at least one device was enumerated and printed; 1 if device
///         enumeration failed or zero devices were found.
int listPhysicalDevices(VkInstance instance)
{
    std::uint32_t count{};

    VkResult countResult = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (countResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate physical device count (VkResult: " << countResult << ")" << '\n';
        return 1;
    }
    if (count == 0)
    {
        std::cerr << "No Vulkan physical devices found" << '\n';
        return 1;
    }

    std::vector<VkPhysicalDevice> devices(count);

    VkResult enumerateResult = vkEnumeratePhysicalDevices(instance, &count, devices.data());
    if (enumerateResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate physical devices (VkResult: " << enumerateResult << ")" << '\n';
        return 1;
    }

    devices.resize(count);

    for (const auto &device : devices)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(device, &props);

        std::uint32_t major = VK_API_VERSION_MAJOR(props.apiVersion);
        std::uint32_t minor = VK_API_VERSION_MINOR(props.apiVersion);
        std::uint32_t patch = VK_API_VERSION_PATCH(props.apiVersion);

        const char *type = deviceTypeToString(props.deviceType);

        std::cout << "Device Name: " << props.deviceName << '\n';
        std::cout << "Device Type: " << type << '\n';
        std::cout << "Vendor ID: 0x" << std::hex << props.vendorID << std::dec << '\n';
        std::cout << "Device Vulkan version: " << major << "." << minor << "." << patch << '\n';
    }

    return 0;
}

/// Prints the Vulkan instance version, creates an instance, lists all
/// physical devices, and destroys the instance before exiting.
///
/// \return 0 if every step succeeded; 1 if querying the version, creating
///         the instance, or enumerating physical devices failed.
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
    createInfo.enabledLayerCount = static_cast<std::uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    VkInstance instance{};
    VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &instance);
    if (instanceResult != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance (VkResult: " << instanceResult << ")" << '\n';
        return 1;
    }

    std::cout << "Instance created" << '\n';

    int status = listPhysicalDevices(instance);

    vkDestroyInstance(instance, nullptr);

    return status;
}