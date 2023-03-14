#pragma once
#include "ce-instance.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

VkInstance
ceGetInstanceVulkanInstance(CeInstance);

VkDevice
ceGetInstanceVulkanDevice(CeInstance);

uint32_t
ceGetInstanceVulkanQueueFamilyIndex(CeInstance);

VkCommandPool
ceGetInstanceVulkanCommandPool(CeInstance);

VkPhysicalDevice
ceGetInstanceVulkanPhysicalDevice(CeInstance);