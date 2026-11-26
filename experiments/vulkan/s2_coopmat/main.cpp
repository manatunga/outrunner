/// \file main.cpp
/// \brief Spike S2: query cooperative-matrix support through Vulkan.
///
/// Stage 0 (current): prints the Vulkan instance version reported by the
/// loader. Later stages create an instance, list physical devices, and query
/// their cooperative-matrix extensions, features and properties.
///
/// Throwaway experiment; see docs/ROADMAP.md (S2) and D-020.
#include <vulkan/vulkan.h>
#include <iostream>
#include <cstdint>

/// Prints the Vulkan instance version reported by the loader.
///
/// \return 0 if the version was queried and printed; 1 if the query failed
///         (the Vulkan result code is printed to explain why).
int main()
{
    std::uint32_t version{};

    VkResult result = vkEnumerateInstanceVersion(&version);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to enumerate Vulkan instance version (VkResult: " << result << ")" << '\n';
        return 1;
    }

    std::uint32_t major = VK_API_VERSION_MAJOR(version);
    std::uint32_t minor = VK_API_VERSION_MINOR(version);
    std::uint32_t patch = VK_API_VERSION_PATCH(version);

    std::cout << "Vulkan instance version: " << major << "." << minor << "." << patch << '\n';
    return 0;
}