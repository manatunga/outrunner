/// \file main.cpp
/// \brief Spike S2: query cooperative-matrix support through Vulkan.
///
/// Stage 4 (current): prints the Vulkan instance version, creates an
/// instance, and lists the physical devices with their name, type, vendor ID
/// and API version. For each device it reports whether the cooperative-matrix
/// extensions are advertised (and their spec versions) and, when
/// VK_KHR_cooperative_matrix is advertised, queries its feature flags via
/// vkGetPhysicalDeviceFeatures2, then destroys the instance. Later stages
/// query each device's supported cooperative-matrix shapes and data types.
///
/// Throwaway experiment; see docs/ROADMAP.md (S2) and D-020.
#include <vulkan/vulkan.h>
#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <optional>
#include <span>
#include <string_view>

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

/// Searches a list of Vulkan device extensions for a matching extension name.
///
/// \param extensions The span of extension properties enumerated from a device.
/// \param targetName The exact extension name to search for.
/// \return The advertised specVersion if the extension is present; std::nullopt
///         otherwise.
std::optional<std::uint32_t> findExtensionSpecVersion(
    std::span<const VkExtensionProperties> extensions,
    std::string_view targetName) noexcept
{
    for (const auto &ext : extensions)
    {
        if (std::string_view(ext.extensionName) == targetName)
        {
            return ext.specVersion;
        }
    }

    return std::nullopt;
}

/// Prints a physical device's properties, reports whether the target
/// cooperative-matrix extensions are advertised along with their spec versions,
/// and queries VK_KHR_cooperative_matrix feature flags when advertised.
///
/// \param device A valid VkPhysicalDevice handle.
/// \return 0 if the device's properties, extensions, and features were queried
///         and printed; 1 if enumerating device extensions failed.
int inspectPhysicalDevice(VkPhysicalDevice device)
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

    std::uint32_t extCount{};

    VkResult countResult = vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
    if (countResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate device extension count for " << props.deviceName << " (VkResult: " << countResult << ")" << '\n';
        return 1;
    }

    std::vector<VkExtensionProperties> extensions(extCount);

    VkResult extResult = vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, extensions.data());
    if (extResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate device extensions for " << props.deviceName << " (VkResult: " << extResult << ")" << '\n';
        return 1;
    }

    extensions.resize(extCount);

    constexpr std::array<const char *, 3> coopMatrixExtensions = {
        VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME,
        VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME,
        VK_NV_COOPERATIVE_MATRIX_2_EXTENSION_NAME};

    bool hasKhrCoopMatrix{};
    for (const auto &macroName : coopMatrixExtensions)
    {
        std::optional<std::uint32_t> specVersion = findExtensionSpecVersion(extensions, macroName);
        if (specVersion)
        {
            std::cout << "  " << macroName << ": advertised (spec version " << *specVersion << ")" << '\n';
            if (std::string_view(macroName) == VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME)
            {
                hasKhrCoopMatrix = true;
            }
        }
        else
        {
            std::cout << "  " << macroName << ": not advertised" << '\n';
        }
    }

    if (hasKhrCoopMatrix)
    {
        VkPhysicalDeviceCooperativeMatrixFeaturesKHR coopFeatures{};
        coopFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR;
        coopFeatures.pNext = nullptr;

        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &coopFeatures;

        vkGetPhysicalDeviceFeatures2(device, &features2);
        std::cout << "  cooperativeMatrix: " << (coopFeatures.cooperativeMatrix == VK_TRUE ? "true" : "false") << '\n';
        std::cout << "  cooperativeMatrixRobustBufferAccess: " << (coopFeatures.cooperativeMatrixRobustBufferAccess == VK_TRUE ? "true" : "false") << '\n';
    }
    else
    {
        std::cout << "  Skipping cooperative-matrix feature query (VK_KHR_cooperative_matrix not advertised)" << '\n';
    }

    return 0;
}

/// Enumerates all physical devices visible to the instance and inspects each
/// device's properties and cooperative-matrix extension support.
///
/// \param instance A valid VkInstance handle.
/// \return 0 if at least one device was enumerated and inspected; 1 if device
///         enumeration failed, zero devices were found, or inspecting a device
///         failed.
int listPhysicalDevices(VkInstance instance)
{
    std::uint32_t deviceCount{};

    VkResult countResult = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (countResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate physical device count (VkResult: " << countResult << ")" << '\n';
        return 1;
    }
    if (deviceCount == 0)
    {
        std::cerr << "No Vulkan physical devices found" << '\n';
        return 1;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);

    VkResult enumerateResult = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    if (enumerateResult != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate physical devices (VkResult: " << enumerateResult << ")" << '\n';
        return 1;
    }

    devices.resize(deviceCount);

    for (const auto &device : devices)
    {
        int status = inspectPhysicalDevice(device);
        if (status != 0)
        {
            return 1;
        }
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