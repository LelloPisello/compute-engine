#pragma once

#include <vulkan/vulkan_core.h>
#ifdef __cplusplus 
extern "C" {
#endif

#include <vulkan/vulkan.h>
#include "ce-def.h"

VkInstance
ceGetVulkanInstance(CeInstance);

VkDevice 
ceGetVulkanDevice(CeInstance);

#ifdef __cplusplus
}
#endif
