#include "ce-pipeline.h"
#include "ce-def.h"
#include "ce-instance.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <stdlib.h>
#include "ce-instance-internal.h"

struct CePipeline_t { 
    VkPipeline vulkanFinishedPipeline;
    VkPipelineLayout vulkanPipelineLayout;
    VkPipelineCache vulkanPipelineCache;
    VkDescriptorSetLayout vulkanDescriptorSetLayout;
    VkShaderModule vulkanShader;
    VkDescriptorPool vulkanDescriptorPool;
    VkDescriptorSet vulkanDescriptorSet;
    VkBuffer *vulkanBuffers;
    VkDeviceMemory *vulkanBufferMemory;
    uint32_t bufferCount;
};

#include <stdio.h>

static VkResult __createVkBuffersFromBindings(CeInstance instance, const CePipelineCreationArgs* args, CePipeline pipeline) {
    VkBufferCreateInfo *bufferInfos = calloc(args->uPipelineBindingCount, sizeof(VkBufferCreateInfo));
    uint32_t familyIndex = ceGetInstanceVulkanQueueFamilyIndex(instance);
    pipeline->vulkanBuffers = calloc(args->uPipelineBindingCount, sizeof(VkBuffer));
    pipeline->bufferCount =  args->uPipelineBindingCount;
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        bufferInfos[i].size = args->pPipelineBindings[i].uBindingElementCount * args->pPipelineBindings[i].uBindingElementSize;
        bufferInfos[i].usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bufferInfos[i].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfos[i].queueFamilyIndexCount = 1;
        bufferInfos[i].pQueueFamilyIndices = &familyIndex;
        bufferInfos[i].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkCreateBuffer(ceGetInstanceVulkanDevice(instance), bufferInfos + i, NULL, &pipeline->vulkanBuffers[i]);
    }

    VkMemoryRequirements *memoryRequirements = malloc(sizeof(VkMemoryRequirements) * args->uPipelineBindingCount);
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        vkGetBufferMemoryRequirements(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBuffers[i], memoryRequirements + i);
    }

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(ceGetInstanceVulkanPhysicalDevice(instance), &memoryProperties);
    uint32_t MemoryTypeIndex = ~((uint32_t)0);
    VkDeviceSize MemoryHeapSize = ~((uint32_t)0);
    for (uint32_t CurrentMemoryTypeIndex = 0; CurrentMemoryTypeIndex < memoryProperties.memoryTypeCount; ++CurrentMemoryTypeIndex)
    {
        VkMemoryType MemoryType = memoryProperties.memoryTypes[CurrentMemoryTypeIndex];
        if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & MemoryType.propertyFlags) &&
            (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & MemoryType.propertyFlags))
        {
            MemoryHeapSize = memoryProperties.memoryHeaps[MemoryType.heapIndex].size;
            MemoryTypeIndex = CurrentMemoryTypeIndex;
            break;
        }
    }

    VkMemoryAllocateInfo *allocInfos = calloc( args->uPipelineBindingCount, sizeof(VkMemoryAllocateInfo));
    pipeline->vulkanBufferMemory = malloc(sizeof(VkDeviceMemory) * args->uPipelineBindingCount);
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        allocInfos[i].allocationSize = memoryRequirements[i].size;
        allocInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfos[i].memoryTypeIndex = MemoryTypeIndex;
        vkAllocateMemory(ceGetInstanceVulkanDevice(instance), allocInfos + i, NULL, &pipeline->vulkanBufferMemory[i]);
        vkBindBufferMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBuffers[i], pipeline->vulkanBufferMemory[i], 0);
    }

    

    free(allocInfos);
    free(memoryRequirements);
    free(bufferInfos);
    return VK_SUCCESS;  
}

static VkResult __createVkDescriptorPool(CeInstance instance, CePipeline pipeline, const CePipelineCreationArgs* args) {
    VkDescriptorPoolSize poolSize = {
        .descriptorCount = args->uPipelineBindingCount,
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };
    return vkCreateDescriptorPool(ceGetInstanceVulkanDevice(instance), &poolInfo, NULL, &pipeline->vulkanDescriptorPool);
}

static VkResult __createVkShaderModule(CeInstance instance, CePipeline pipeline, const CePipelineCreationArgs* args) {
    FILE* file = fopen(args->pShaderFilename, "rb");
    
    uint32_t* finalBuffer;
    uint32_t fileSize;

    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    rewind(file);

    finalBuffer = malloc(fileSize * sizeof(char));
    fread(finalBuffer, sizeof(uint32_t), fileSize / sizeof(uint32_t), file);
    
    fclose(file);
    
    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fileSize,
        .pCode = finalBuffer
    };

    VkResult result = vkCreateShaderModule(ceGetInstanceVulkanDevice(instance), &shaderInfo, NULL, &pipeline->vulkanShader);
    
    free(finalBuffer);
    return result;
}

static VkResult __createVkDescriptorSetLayout(CeInstance instance, CePipeline pipeline, const CePipelineCreationArgs* args) {
    VkDescriptorSetLayoutBinding *bindings = malloc(sizeof(VkDescriptorSetLayoutBinding) * args->uPipelineBindingCount);
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = args->uPipelineBindingCount,
        .pBindings = bindings,
    };
    VkResult result = vkCreateDescriptorSetLayout(ceGetInstanceVulkanDevice(instance), &layoutInfo, NULL, &pipeline->vulkanDescriptorSetLayout);
    free(bindings);
    return result;
}

static VkResult __createVkDescriptorSet(CeInstance instance, const CePipelineCreationArgs* args, CePipeline pipeline) {
    VkDescriptorSetAllocateInfo setInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pipeline->vulkanDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &pipeline->vulkanDescriptorSetLayout,
    };
    VkResult result = vkAllocateDescriptorSets(ceGetInstanceVulkanDevice(instance), &setInfo, &pipeline->vulkanDescriptorSet);
    
    VkDescriptorBufferInfo *buffers = calloc(args->uPipelineBindingCount, sizeof(VkDescriptorBufferInfo));
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        buffers[i].buffer = pipeline->vulkanBuffers[i];
        buffers[i].offset = 0;
        buffers[i].range = args->pPipelineBindings[i].uBindingElementCount * args->pPipelineBindings[i].uBindingElementSize;
    }

    VkWriteDescriptorSet *descriptorSets = calloc(args->uPipelineBindingCount, sizeof(VkWriteDescriptorSet));
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        descriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSets[i].dstSet = pipeline->vulkanDescriptorSet;
        descriptorSets[i].dstBinding = i;
        descriptorSets[i].dstArrayElement = 0;
        descriptorSets[i].descriptorCount = 1;
        descriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorSets[i].pBufferInfo = &buffers[i];
        descriptorSets[i].pNext = NULL;
    }

    vkUpdateDescriptorSets(ceGetInstanceVulkanDevice(instance), args->uPipelineBindingCount,
     descriptorSets, 0, NULL);
    free(descriptorSets);
    free(buffers);
    return result;
}

static VkResult __createVkPipelineLayoutAndCache(CeInstance instance, CePipeline pipeline) {

    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pSetLayouts = &pipeline->vulkanDescriptorSetLayout,
        .setLayoutCount = 1,
    };
    VkPipelineCacheCreateInfo cacheInfo = {
        
    };
    VkResult result = vkCreatePipelineLayout(ceGetInstanceVulkanDevice(instance), &layoutInfo, NULL, &pipeline->vulkanPipelineLayout);
    vkCreatePipelineCache(ceGetInstanceVulkanDevice(instance), &cacheInfo, NULL, &pipeline->vulkanPipelineCache);
    return result;
}

static VkResult __createVkPipeline(CePipeline pipeline) {
    return VK_SUCCESS;
}

CeResult ceCreatePipeline(CeInstance instance, const CePipelineCreationArgs * args, CePipeline * pipeline) {
#define ALIAS (*pipeline)
    if(!instance || !args || !pipeline)
        return CE_ERROR_NULL_PASSED;
        
    ALIAS = malloc(sizeof(struct CePipeline_t));
    
    if(
        __createVkBuffersFromBindings(instance, args, ALIAS) ||
        __createVkShaderModule(instance, ALIAS, args) ||
        __createVkDescriptorSetLayout(instance, ALIAS, args) ||
        __createVkDescriptorPool(instance, ALIAS, args) || 
        __createVkDescriptorSet(instance, args, ALIAS) ||
        __createVkPipelineLayoutAndCache(instance, ALIAS) ||
        __createVkPipeline(ALIAS))
        return CE_ERROR_INTERNAL;
    return CE_SUCCESS;
#undef ALIAS
}

void ceDestroyPipeline(CeInstance instance, CePipeline pipeline) {
    for(uint32_t i = 0; i < pipeline->bufferCount; ++i) {
        vkFreeMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[i], NULL);
        vkDestroyBuffer(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBuffers[i], NULL);
    }
    free(pipeline->vulkanBuffers);
    free(pipeline->vulkanBufferMemory);
    vkFreeDescriptorSets(ceGetInstanceVulkanDevice(instance), pipeline->vulkanDescriptorPool, 1, &pipeline->vulkanDescriptorSet);
    vkDestroyDescriptorSetLayout(ceGetInstanceVulkanDevice(instance), pipeline->vulkanDescriptorSetLayout, NULL);
    vkDestroyDescriptorPool(ceGetInstanceVulkanDevice(instance), pipeline->vulkanDescriptorPool, NULL);
    vkDestroyPipelineLayout(ceGetInstanceVulkanDevice(instance), pipeline->vulkanPipelineLayout, NULL);
    vkDestroyPipelineCache(ceGetInstanceVulkanDevice(instance), pipeline->vulkanPipelineCache, NULL);
    vkDestroyShaderModule(ceGetInstanceVulkanDevice(instance), pipeline->vulkanShader, NULL);
    free(pipeline);
}