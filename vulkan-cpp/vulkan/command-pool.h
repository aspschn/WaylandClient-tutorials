#ifndef _COMMAND_POOL_H
#define _COMMAND_POOL_H

// C
#include <stdint.h>

// C++
#include <memory>

// Vulkan
#include <vulkan/vulkan.h>

namespace vk {

class Device;

class CommandPool
{
public:
    CommandPool(std::shared_ptr<Device> device);

    VkCommandPool vk_command_pool();

private:
    VkCommandPool _vk_command_pool;
};

} // namespace vk

#endif // _COMMAND_POOL_H
