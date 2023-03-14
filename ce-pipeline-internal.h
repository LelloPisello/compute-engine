#pragma once
#include "ce-def.h"
#include "ce-pipeline.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

VkPipeline ceGetPipelineVulkanPipeline(CePipeline);

VkDescriptorSet ceGetPipelineVulkanDescriptorSet(CePipeline);

VkPipelineLayout ceGetPipelineVulkanPipelineLayout(CePipeline);

uint32_t ceGetPipelineLongestBufferSize(CePipeline);

CeResult
ceSetInstanceQueueToBusy(CeInstance, uint32_t queueIndex);

CeResult
ceSetInstanceQueueToFree(CeInstance, uint32_t queueIndex);

VkSemaphore 
ceGetPipelineBindingSemaphore(CePipeline);
