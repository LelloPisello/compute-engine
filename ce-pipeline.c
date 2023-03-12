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
    pipeline->bufferCount = args->uPipelineBindingCount;
    VkBufferCreateInfo *bufferInfos = calloc(pipeline->bufferCount, sizeof(VkBufferCreateInfo));
    uint32_t familyIndex = ceGetInstanceVulkanQueueFamilyIndex(instance);
    pipeline->vulkanBuffers = calloc(pipeline->bufferCount, sizeof(VkBuffer));
    for(uint32_t i = 0; i < pipeline->bufferCount; ++i) {
        bufferInfos[i].size = args->pPipelineBindings[i].uBindingElementCount * args->pPipelineBindings[i].uBindingElementSize;
        bufferInfos[i].usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bufferInfos[i].sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferInfos[i].queueFamilyIndexCount = 1;
        bufferInfos[i].pQueueFamilyIndices = &familyIndex;
        bufferInfos[i].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        vkCreateBuffer(ceGetInstanceVulkanDevice(instance), &bufferInfos[i], NULL, &pipeline->vulkanBuffers[i]);
    }

    VkMemoryRequirements *memoryRequirements = calloc(args->uPipelineBindingCount, sizeof(VkMemoryRequirements));
    for(uint32_t i = 0; i < pipeline->bufferCount; ++i) {
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
    pipeline->vulkanBufferMemory = calloc(args->uPipelineBindingCount, sizeof(VkDeviceMemory));
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        allocInfos[i].allocationSize = memoryRequirements[i].size;
        allocInfos[i].pNext = NULL;
        allocInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfos[i].memoryTypeIndex = MemoryTypeIndex;
        vkAllocateMemory(ceGetInstanceVulkanDevice(instance), allocInfos + i, NULL, &pipeline->vulkanBufferMemory[i]);
    
        char* data;
        vkMapMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[i], 0, memoryRequirements[i].size, 0, (void**)&data);
        *data = 0;
        vkUnmapMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[i]);

        vkBindBufferMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBuffers[i], pipeline->vulkanBufferMemory[i], 0);
    }

    

    free(allocInfos);
    free(memoryRequirements);
    free(bufferInfos);
    return VK_SUCCESS;  
}

static VkResult __createVkDescriptorPool(CeInstance instance, CePipeline pipeline) {
    VkDescriptorPoolSize poolSize = {
        .descriptorCount = pipeline->bufferCount,
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .pPoolSizes = &poolSize,
        .maxSets = 1
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
    VkDescriptorSetLayoutBinding *bindings = calloc(args->uPipelineBindingCount, sizeof(VkDescriptorSetLayoutBinding));
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
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
    
    VkDescriptorBufferInfo *buffers = calloc(pipeline->bufferCount, sizeof(VkDescriptorBufferInfo));
    for(uint32_t i = 0; i < pipeline->bufferCount; ++i) {
        buffers[i].buffer = pipeline->vulkanBuffers[i];
        buffers[i].offset = 0;
        buffers[i].range = args->pPipelineBindings[i].uBindingElementCount * args->pPipelineBindings[i].uBindingElementSize;
    }
    
    VkWriteDescriptorSet *descriptorSetWrites = calloc(pipeline->bufferCount, sizeof(VkWriteDescriptorSet));
    for(uint32_t i = 0; i < args->uPipelineBindingCount; ++i) {
        //VkWriteDescriptorSet descriptorSetWrites = {};
        descriptorSetWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[i].pNext = NULL;
        descriptorSetWrites[i].dstSet = pipeline->vulkanDescriptorSet;
        descriptorSetWrites[i].dstBinding = i;
        descriptorSetWrites[i].dstArrayElement = 0;
        descriptorSetWrites[i].descriptorCount = 1;
        descriptorSetWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorSetWrites[i].pBufferInfo = &buffers[i];
        descriptorSetWrites[i].pImageInfo = NULL;
        descriptorSetWrites[i].pTexelBufferView = NULL;
        descriptorSetWrites[i].pNext = NULL;
        //vkUpdateDescriptorSets(ceGetInstanceVulkanDevice(instance), 1,
        // &descriptorSetWrites, 0, NULL);  
    }

    vkUpdateDescriptorSets(ceGetInstanceVulkanDevice(instance), pipeline->bufferCount,
     descriptorSetWrites, 0, NULL);
    free(buffers);
    free(descriptorSetWrites);
    return result;
}

static VkResult __createVkPipelineLayoutAndCache(CeInstance instance, CePipeline pipeline) {

    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pSetLayouts = &pipeline->vulkanDescriptorSetLayout,
        .setLayoutCount = 1,
    };
    VkPipelineCacheCreateInfo cacheInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
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
        
    ALIAS = calloc(1, sizeof(struct CePipeline_t));
    ALIAS->bufferCount = args->uPipelineBindingCount;

    if(
        __createVkBuffersFromBindings(instance, args, ALIAS) ||
        __createVkShaderModule(instance, ALIAS, args) ||
        __createVkDescriptorPool(instance, ALIAS) || 
        __createVkDescriptorSetLayout(instance, ALIAS, args) ||
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