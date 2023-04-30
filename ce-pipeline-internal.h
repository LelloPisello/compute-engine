#pragma once
#include "ce-def.h"
#include "ce-pipeline.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

VkPipeline ceGetPipelineVulkanPipeline(CePipeline);

VkCommandBuffer ceGetPipelineVulkanCommand(CePipeline);

VkDescriptorSet ceGetPipelineVulkanDescriptorSet(CePipeline);

VkPipelineLayout ceGetPipelineVulkanPipelineLayout(CePipeline);

uint32_t ceGetPipelineDispatchWorkgroupCount(CePipeline);

CeResult
ceSetInstanceQueueToBusy(CeInstance, uint32_t queueIndex);

CeResult
ceSetInstanceQueueToFree(CeInstance, uint32_t queueIndex);

VkSemaphore 
ceGetPipelineBindingSemaphore(CePipeline);

uint32_t 
ceGetPipelineConstantCount(CePipeline);

void
ceGetPipelineConstantData(CePipeline, uint32_t constant_index, void** pData, uint32_t *uDataSize, uint32_t *uOffset);


uint32_t 
ceGetInstanceNextFreeQueue(CeInstance);