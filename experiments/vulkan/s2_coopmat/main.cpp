/// \file main.cpp
/// \brief Spike S2: query cooperative-matrix support through Vulkan.
///
/// Stage 6 (final): prints the Vulkan instance version, creates an
/// instance, and lists the physical devices with their name, type, vendor ID
/// and API version. For each device it reports whether the cooperative-matrix
/// extensions are advertised (and their spec versions), queries
/// VK_KHR_cooperative_matrix and optional VK_NV_cooperative_matrix2 feature
/// flags via vkGetPhysicalDeviceFeatures2 when advertised, and enumerates
/// supported cooperative-matrix shapes and data types via a dynamically
/// loaded extension entry point when enabled, then destroys the instance.
///
/// Throwaway experiment; see docs/ROADMAP.md (S2) and D-020.
#include <vulkan/vulkan.h>
#include <iostream>
#include <string>
#include <cstdint>
#include <array>
#include <vector>
#include <optional>
#include <span>
#include <string_view>
#include <format>
#include <utility>

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

/// Converts a Vulkan cooperative-matrix component type enum into a readable
/// label (or a fallback string with the numeric value for unknown enums).
///
/// \param componentType The component type reported in
///        VkCooperativeMatrixPropertiesKHR.
/// \return A string describing the component data type.
std::string componentTypeToString(VkComponentTypeKHR componentType)
{
    switch (componentType)
    {
    case VK_COMPONENT_TYPE_FLOAT16_KHR:
        return "float16";
    case VK_COMPONENT_TYPE_FLOAT32_KHR:
        return "float32";
    case VK_COMPONENT_TYPE_FLOAT64_KHR:
        return "float64";
    case VK_COMPONENT_TYPE_SINT8_KHR:
        return "int8";
    case VK_COMPONENT_TYPE_SINT16_KHR:
        return "int16";
    case VK_COMPONENT_TYPE_SINT32_KHR:
        return "int32";
    case VK_COMPONENT_TYPE_SINT64_KHR:
        return "int64";
    case VK_COMPONENT_TYPE_UINT8_KHR:
        return "uint8";
    case VK_COMPONENT_TYPE_UINT16_KHR:
        return "uint16";
    case VK_COMPONENT_TYPE_UINT32_KHR:
        return "uint32";
    case VK_COMPONENT_TYPE_UINT64_KHR:
        return "uint64";
    case VK_COMPONENT_TYPE_BFLOAT16_KHR:
        return "bfloat16";
    case VK_COMPONENT_TYPE_SINT8_PACKED_NV:
        return "int8_packed";
    case VK_COMPONENT_TYPE_UINT8_PACKED_NV:
        return "uint8_packed";
    case VK_COMPONENT_TYPE_FLOAT8_E4M3_EXT:
        return "float8_e4m3";
    case VK_COMPONENT_TYPE_FLOAT8_E5M2_EXT:
        return "float8_e5m2";
#if VK_HEADER_VERSION > 350
    case VK_COMPONENT_TYPE_FLOAT6_E2M3_EXT:
        return "float6_e2m3";
    case VK_COMPONENT_TYPE_FLOAT6_E3M2_EXT:
        return "float6_e3m2";
    case VK_COMPONENT_TYPE_FLOAT4_E2M1_EXT:
        return "float4_e2m1";
    case VK_COMPONENT_TYPE_FLOAT8_UNSIGNED_E8M0_EXT:
        return "float8_e8m0";
    case VK_COMPONENT_TYPE_MXINT8_EXT:
        return "mxint8";
#endif
    default:
        return std::format("unknown({})", static_cast<int>(componentType));
    }
}

/// Converts a Vulkan execution scope enum into a human-readable label.
///
/// \param scope The execution scope reported in VkCooperativeMatrixPropertiesKHR.
/// \return A null-terminated string literal ("device", "workgroup",
///         "subgroup", "queue_family", or "unknown").
const char *scopeToString(VkScopeKHR scope)
{
    switch (scope)
    {
    case VK_SCOPE_DEVICE_KHR:
        return "device";
    case VK_SCOPE_WORKGROUP_KHR:
        return "workgroup";
    case VK_SCOPE_SUBGROUP_KHR:
        return "subgroup";
    case VK_SCOPE_QUEUE_FAMILY_KHR:
        return "queue_family";
    default:
        return "unknown";
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

/// Prints a physical device's properties, reports cooperative-matrix extension
/// advertisement and spec versions, queries KHR and optional NV2 feature flags,
/// and enumerates supported cooperative-matrix shapes and data types when
/// enabled.
///
/// \param instance A valid VkInstance handle used to resolve extension entry
///        points dynamically.
/// \param device A valid VkPhysicalDevice handle.
/// \return 0 if inspection succeeded; 1 if enumerating extensions, loading
///         the extension function, or enumerating matrix properties failed.
int inspectPhysicalDevice(VkInstance instance, VkPhysicalDevice device)
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

    constexpr std::array<const char *, 3> coopMatrixExtensions =
        {VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME,
         VK_NV_COOPERATIVE_MATRIX_EXTENSION_NAME,
         VK_NV_COOPERATIVE_MATRIX_2_EXTENSION_NAME};

    bool hasKhrCoopMatrix{};
    bool hasNvCoopMatrix2{};
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
            else if (std::string_view(macroName) == VK_NV_COOPERATIVE_MATRIX_2_EXTENSION_NAME)
            {
                hasNvCoopMatrix2 = true;
            }
        }
        else
        {
            std::cout << "  " << macroName << ": not advertised" << '\n';
        }
    }

    if (hasKhrCoopMatrix)
    {
        VkPhysicalDeviceCooperativeMatrix2FeaturesNV coop2Features{};
        coop2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_2_FEATURES_NV;
        coop2Features.pNext = nullptr;

        VkPhysicalDeviceCooperativeMatrixFeaturesKHR coopFeatures{};
        coopFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR;
        coopFeatures.pNext = hasNvCoopMatrix2 ? &coop2Features : nullptr;

        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &coopFeatures;

        std::cout << "  Querying cooperative-matrix features..." << '\n'
                  << std::flush;
        vkGetPhysicalDeviceFeatures2(device, &features2);
        std::cout << "  cooperativeMatrix: " << (coopFeatures.cooperativeMatrix == VK_TRUE ? "true" : "false") << '\n';
        std::cout << "  cooperativeMatrixRobustBufferAccess: " << (coopFeatures.cooperativeMatrixRobustBufferAccess == VK_TRUE ? "true" : "false") << '\n';

        if (hasNvCoopMatrix2)
        {
            std::array<std::pair<std::string_view, VkBool32>, 7> nv2Flags =
                {{{"cooperativeMatrixWorkgroupScope", coop2Features.cooperativeMatrixWorkgroupScope},
                  {"cooperativeMatrixFlexibleDimensions", coop2Features.cooperativeMatrixFlexibleDimensions},
                  {"cooperativeMatrixReductions", coop2Features.cooperativeMatrixReductions},
                  {"cooperativeMatrixConversions", coop2Features.cooperativeMatrixConversions},
                  {"cooperativeMatrixPerElementOperations", coop2Features.cooperativeMatrixPerElementOperations},
                  {"cooperativeMatrixTensorAddressing", coop2Features.cooperativeMatrixTensorAddressing},
                  {"cooperativeMatrixBlockLoads", coop2Features.cooperativeMatrixBlockLoads}}};

            for (const auto &[name, value] : nv2Flags)
            {
                std::cout << "  " << name << ": " << (value == VK_TRUE ? "true" : "false") << '\n';
            }
        }
        else
        {
            std::cout << "  Skipping VK_NV_cooperative_matrix2 feature query (not advertised)" << '\n';
        }

        if (coopFeatures.cooperativeMatrix == VK_TRUE)
        {
            auto getCoopMatrixProps = reinterpret_cast<PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR>(
                vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR"));

            if (getCoopMatrixProps == nullptr)
            {
                std::cerr << "Failed to load vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR for " << props.deviceName << '\n';
                return 1;
            }

            std::uint32_t propCount{};

            std::cout << "  Querying cooperative-matrix property count..." << '\n'
                      << std::flush;
            VkResult propCountResult = getCoopMatrixProps(device, &propCount, nullptr);
            if (propCountResult != VK_SUCCESS)
            {
                std::cerr << "Failed to enumerate cooperative matrix property count for " << props.deviceName << " (VkResult: " << propCountResult << ")" << '\n';
                return 1;
            }

            if (propCount == 0)
            {
                std::cout << "  No cooperative matrix properties reported" << '\n';
            }
            else
            {
                VkCooperativeMatrixPropertiesKHR defaultProp{};
                defaultProp.sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_KHR;
                defaultProp.pNext = nullptr;
                std::vector<VkCooperativeMatrixPropertiesKHR> matrixProps(propCount, defaultProp);

                std::cout << "  Querying cooperative-matrix properties..." << '\n'
                          << std::flush;
                VkResult propResult = getCoopMatrixProps(device, &propCount, matrixProps.data());
                if (propResult != VK_SUCCESS)
                {
                    std::cerr << "Failed to enumerate cooperative matrix properties for " << props.deviceName << " (VkResult: " << propResult << ")" << '\n';
                    return 1;
                }

                matrixProps.resize(propCount);

                std::cout << "  Supported cooperative matrix shapes (" << propCount << "):" << '\n';
                for (const auto &prop : matrixProps)
                {
                    std::string AType = componentTypeToString(prop.AType);
                    std::string BType = componentTypeToString(prop.BType);
                    std::string CType = componentTypeToString(prop.CType);
                    std::string DType = componentTypeToString(prop.ResultType);

                    const char *scope = scopeToString(prop.scope);

                    std::cout << "    " << prop.MSize << "x" << prop.NSize << "x" << prop.KSize << " | A=" << AType << ", B=" << BType << ", C=" << CType << ", D=" << DType << " | scope=" << scope << " | saturating=" << (prop.saturatingAccumulation == VK_TRUE ? "true" : "false") << '\n';
                }
            }
        }
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
/// \return 0 if every device was enumerated and inspected successfully; 1 if
///         enumeration failed, zero devices were found, or inspecting any
///         device failed.
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

    bool anyFailed{};
    for (const auto &device : devices)
    {
        int status = inspectPhysicalDevice(instance, device);
        if (status != 0)
        {
            anyFailed = true;
        }
    }

    return anyFailed ? 1 : 0;
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