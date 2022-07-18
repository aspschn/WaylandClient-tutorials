#ifndef _DEVICE_H
#define _DEVICE_H

// C
#include <stdint.h>

// C++
#include <memory>
#include <vector>
#include <optional>

// Vulkan
#include <vulkan/vulkan.h>

namespace vk {

class Instance;
class Surface;

class Device
{
public:
    class QueueFamilies
    {
    public:
        QueueFamilies();

        uint32_t graphics_family_index() const;
        uint32_t present_family_index() const;

        void set_graphics_family_index(uint32_t index);
        void set_present_family_index(uint32_t index);

        std::vector<uint32_t> indices() const;

    private:
        std::optional<uint32_t> _graphics_family_index;
        std::optional<uint32_t> _present_family_index;
    };

public:
    Device(std::shared_ptr<Instance> instance,
            std::shared_ptr<Surface> surface);

private:
    VkDevice _vk_device;
    QueueFamilies _queue_families;
    float _queue_priority;
    VkPhysicalDeviceFeatures _enabled_features;
    VkQueue _graphics_queue;
    VkQueue _present_queue;
};

} // namespace vk

#endif // _DEVICE_H
