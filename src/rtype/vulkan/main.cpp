#include <iostream>

#if defined(_WIN32)
#define RTYPE_VULKAN_API __declspec(dllexport)
#else
#define RTYPE_VULKAN_API
#endif

namespace rtype::vulkan {
extern "C" RTYPE_VULKAN_API void vulkan_hello() { std::cout << "hello world!" << std::endl; }
}  // namespace rtype::vulkan
