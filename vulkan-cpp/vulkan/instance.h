#ifndef _INSTANCE_H
#define _INSTANCE_H

#include <vulkan/vulkan.h>

namespace vk {

class Instance
{
public:
    Instance();
    ~Instance();

    VkInstance vk_instance();

    VkPhysicalDevice vk_physical_device();

private:
    VkInstance _vk_instance;
    const char* *_vk_extension_names;
    VkPhysicalDevice *_vk_physical_devices;
    VkPhysicalDevice _vk_physical_device;
};

} // namespace vk

#endif // INSTANCE_H
