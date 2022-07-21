#include "command-pool.h"

// C
#include <stdio.h>

#include "device.h"

namespace vk {

CommandPool::CommandPool(std::shared_ptr<Device> device)
{
    // Init.
    this->_vk_command_pool = nullptr;

    // Create.
    VkResult result;

    VkCommandPoolCreateInfo create_info;
    create_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex =
        device->graphics_queue_family_index();

    // Zero or null.
    create_info.pNext = NULL;

    result = vkCreateCommandPool(device->vk_device(),
        &create_info, NULL, &this->_vk_command_pool);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool!\n");
        return;
    }
    fprintf(stderr, "Command pool created. - command pool: %p\n",
        this->_vk_command_pool);
}

VkCommandPool CommandPool::vk_command_pool()
{
    return this->_vk_command_pool;
}

} // namespace vk
