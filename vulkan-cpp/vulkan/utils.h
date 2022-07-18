#ifndef _UTILS_H
#define _UTILS_H

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* vk_format_to_string(VkFormat format);

const char* vk_present_mode_to_string(VkPresentModeKHR mode);

#ifdef __cplusplus
}
#endif

#endif // _UTILS_H
