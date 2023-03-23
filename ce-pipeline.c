#include "ce-pipeline.h"
#include "ce-def.h"
#include "ce-instance.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <stdlib.h>
#include "ce-instance-internal.h"
#include "ce-pipeline-internal.h"
#include "ce-error-internal.h"
#include <string.h>

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
    VkDeviceSize *vulkanBufferMemorySize;
    void** buffersMappedData;
    uint32_t bufferCount;
    //uint32_t longestBufferSize;
    uint32_t dispatchGroupCount;
    CePipelineConstantInfo* constantsData;
    uint32_t* constantOffsets;
    uint32_t constantCount;
};

VkPipeline ceGetPipelineVulkanPipeline(CePipeline pipeline) {
    return pipeline->vulkanFinishedPipeline;
}

VkDescriptorSet ceGetPipelineVulkanDescriptorSet(CePipeline pipeline) {
    return pipeline->vulkanDescriptorSet;
}

VkPipelineLayout ceGetPipelineVulkanPipelineLayout(CePipeline pipeline) {
    return pipeline->vulkanPipelineLayout;
}

uint32_t ceGetPipelineDispatchWorkgroupCount(CePipeline pipeline) {
    return pipeline->dispatchGroupCount;
}

#include <stdio.h>

static VkResult __createVkBuffersFromBindings(CeInstance instance, const CePipelineCreationArgs* args, CePipeline pipeline) {
    pipeline->bufferCount = args->uBindingCount;
    pipeline->buffersMappedData = calloc(pipeline->bufferCount, sizeof(void*));
    //VkBufferCreateInfo *bufferInfos = calloc(pipeline->bufferCount, sizeof(VkBufferCreateInfo));
    uint32_t familyIndex = ceGetInstanceVulkanQueueFamilyIndex(instance);
    pipeline->vulkanBuffers = calloc(pipeline->bufferCount, sizeof(VkBuffer));
    pipeline->vulkanBufferMemory = calloc(pipeline->bufferCount, sizeof(VkDeviceMemory));
    pipeline->vulkanBufferMemorySize = calloc(pipeline->bufferCount, sizeof(VkDeviceSize));

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

    uint32_t longestBufferSize = 0;
    for(uint32_t i = 0; i < pipeline->bufferCount; ++i) {
        pipeline->vulkanBufferMemorySize[i] = args->pBindings[i].uElementCount * args->pBindings[i].uElementSize;
        VkBufferCreateInfo bufferInfo = {
            .size = pipeline->vulkanBufferMemorySize[i],
            .pQueueFamilyIndices = &familyIndex,
            .queueFamilyIndexCount = 1,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .usage = args->pBindings[i].bIsUniform ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        };
        
        longestBufferSize = 
            longestBufferSize < args->pBindings[i].uElementCount ? 
            args->pBindings[i].uElementCount :
            longestBufferSize;
        /*bufferInfos[i].pNext = NULL;
        bufferInfos[i].size = args->pPipelineBindings[i].uBindingElementCount * args->pPipelineBindings[i].uBindingElementSize;
        bufferInfos[i].usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        bufferInfos[i].sharingMode = VK_SHARING_MODE_CONCURRENT;
        bufferInfos[i].queueFamilyIndexCount = 1;
        bufferInfos[i].pQueueFamilyIndices = &familyIndex;
        bufferInfos[i].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;*/
        vkCreateBuffer(ceGetInstanceVulkanDevice(instance), &bufferInfo, NULL, &pipeline->vulkanBuffers[i]);
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBuffers[i], &memoryRequirements);
        VkMemoryAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memoryRequirements.size,
            .memoryTypeIndex = MemoryTypeIndex
        };
        vkAllocateMemory(ceGetInstanceVulkanDevice(instance), &allocInfo, NULL, &pipeline->vulkanBufferMemory[i]);
        if(args->pBindings[i].bKeepMapped || args->pBindings[i].pInitialData) {
            vkMapMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[i], 0, memoryRequirements.size, 0, &pipeline->buffersMappedData[i]);
            if(args->pBindings[i].pInitialData)
                memcpy(pipeline->buffersMappedData[i], args->pBindings[i].pInitialData, memoryRequirements.size);
            if(!args->pBindings[i].bKeepMapped) {
                vkUnmapMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[i]);
                pipeline->buffersMappedData[i] = NULL;
            }
        }
        
        vkBindBufferMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBuffers[i], pipeline->vulkanBufferMemory[i], 0);
    }
    if(!pipeline->dispatchGroupCount)
        pipeline->dispatchGroupCount = longestBufferSize;
    //free(bufferInfos);
    return VK_SUCCESS;  
}

CeResult
ceMapPipelineBindingMemory(CeInstance instance, CePipeline pipeline, uint32_t bindingIndex, void** target) {
    if(vkMapMemory(ceGetInstanceVulkanDevice(instance),
    pipeline->vulkanBufferMemory[bindingIndex], 0, pipeline->vulkanBufferMemorySize[bindingIndex], 
    0, target) != CE_SUCCESS)
        return CE_ERROR_INTERNAL;
    return CE_SUCCESS;
}

void
ceUnmapPipelineBindingMemory(CeInstance instance, CePipeline pipeline, uint32_t bindingIndex) {
    vkUnmapMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[bindingIndex]);
}

static VkResult __createVkDescriptorPool(CeInstance instance, CePipeline pipeline) {
    VkDescriptorPoolSize poolSizes[] = {
        {
            .descriptorCount = pipeline->bufferCount,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        }, {
            .descriptorCount = pipeline->bufferCount,
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        }
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 2,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .pPoolSizes = poolSizes,
        .maxSets = 1
    };
    return vkCreateDescriptorPool(ceGetInstanceVulkanDevice(instance), &poolInfo, NULL, &pipeline->vulkanDescriptorPool);
}

static VkResult __createVkShaderModule(CeInstance instance, CePipeline pipeline, const CePipelineCreationArgs* args) {
    FILE *file;
	char *buffer;
	unsigned long fileLen;

	//Open file
	file = fopen(args->pShaderFilename, "rb");
	if (!file)
	{
		//fprintf(stderr, "Unable to open file %s", name);
		return (VkResult)ceResult(CE_ERROR_INTERNAL, "stdlib failed to open a file");
	}
	
	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

	//Allocate memory
	buffer=(char *)malloc(fileLen+1);
	if (!buffer)
	{
		//fprintf(stderr, "Memory error!");
        //                        fclose(file);
		return (VkResult)ceResult(CE_ERROR_INTERNAL, "stdlib failed to allocate a file buffer");
	}

	//Read file contents into buffer
	fread(buffer, fileLen, 1, file);
	fclose(file);

	//Do what ever with buffer

	
    
    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fileLen,
        .pCode = (uint32_t*)buffer,
    };

    VkResult result = vkCreateShaderModule(ceGetInstanceVulkanDevice(instance), &shaderInfo, NULL, &pipeline->vulkanShader);
    
    free(buffer);
    return result;
}

static VkResult __createVkDescriptorSetLayout(CeInstance instance, CePipeline pipeline, const CePipelineCreationArgs* args) {
    VkDescriptorSetLayoutBinding *bindings = calloc(pipeline->bufferCount, sizeof(VkDescriptorSetLayoutBinding));
    for(uint32_t i = 0; i < args->uBindingCount; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = args->pBindings[i].bIsUniform ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_ALL;
    }
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = pipeline->bufferCount,
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
    VkWriteDescriptorSet *descriptorSetWrites = calloc(pipeline->bufferCount, sizeof(VkWriteDescriptorSet));
    
    for(uint32_t i = 0; i < pipeline->bufferCount; ++i) {
        buffers[i].buffer = pipeline->vulkanBuffers[i];
        buffers[i].offset = 0;
        buffers[i].range = args->pBindings[i].uElementCount * args->pBindings[i].uElementSize;
        //VkWriteDescriptorSet descriptorSetWrites = {};
        descriptorSetWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[i].pNext = NULL;
        descriptorSetWrites[i].dstSet = pipeline->vulkanDescriptorSet;
        descriptorSetWrites[i].dstBinding = i;
        descriptorSetWrites[i].dstArrayElement = 0;
        descriptorSetWrites[i].descriptorCount = 1;
        descriptorSetWrites[i].descriptorType = args->pBindings[i].bIsUniform ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorSetWrites[i].pBufferInfo = &buffers[i];
        //vkUpdateDescriptorSets(ceGetInstanceVulkanDevice(instance), 1,
        // &descriptorSetWrites, 0, NULL);  
    }

    vkUpdateDescriptorSets(ceGetInstanceVulkanDevice(instance), pipeline->bufferCount,
     descriptorSetWrites, 0, NULL);
    free(buffers);
    free(descriptorSetWrites);
    return result;
}

static VkResult __createVkPipelineLayoutAndCache(CeInstance instance, const CePipelineCreationArgs* args, CePipeline pipeline) {
    VkPushConstantRange *constants = calloc(args->uConstantCount, sizeof(VkPushConstantRange));
    pipeline->constantsData = calloc(args->uConstantCount, sizeof(void*));
    pipeline->constantCount = args->uConstantCount;
    pipeline->constantOffsets = calloc(args->uConstantCount, sizeof(uint32_t));
    uint32_t accumulatedOffset = 0;
    for(uint32_t i = 0; i < pipeline->constantCount; ++i) {
        constants[i].offset = accumulatedOffset;
        constants[i].size = args->pConstants[i].uDataSize;
        constants[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        if(!pipeline->constantsData[i].bIsLiveConstant) {
            pipeline->constantsData[i].pData = calloc(args->pConstants[i].uDataSize, 1);
            memcpy(pipeline->constantsData[i].pData, args->pConstants[i].pData, args->pConstants[i].uDataSize);
        } else {
            pipeline->constantsData[i].pData = args->pConstants[i].pData;
        }
        pipeline->constantsData[i].uDataSize = args->pConstants[i].uDataSize;
        pipeline->constantOffsets[i] = accumulatedOffset;
        accumulatedOffset += constants[i].size;
    }
    VkPipelineLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pSetLayouts = &pipeline->vulkanDescriptorSetLayout,
        .pushConstantRangeCount = pipeline->constantCount,
        .pPushConstantRanges = constants,
        .setLayoutCount = 1,
    };
    VkPipelineCacheCreateInfo cacheInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        
    };
    VkResult result = vkCreatePipelineLayout(ceGetInstanceVulkanDevice(instance), &layoutInfo, NULL, &pipeline->vulkanPipelineLayout);
    vkCreatePipelineCache(ceGetInstanceVulkanDevice(instance), &cacheInfo, NULL, &pipeline->vulkanPipelineCache);
    free(constants);
    return result;
}


uint32_t 
ceGetPipelineConstantCount(CePipeline pipeline) {
    return pipeline->constantCount;
}

void
ceGetPipelineConstantData(CePipeline pipeline, uint32_t constant_index, void** pData, uint32_t *uDataSize, uint32_t *uOffset) {
    (*pData) = pipeline->constantsData[constant_index].pData;
    (*uDataSize) = pipeline->constantsData[constant_index].uDataSize;
    (*uOffset) = pipeline->constantOffsets[constant_index];
}

CeResult 
ceGetPipelineBindingMemory(CePipeline pipeline, uint32_t bindingIndex, void** pData) {
    if(!pipeline->buffersMappedData[bindingIndex])
        return ceResult(CE_ERROR_BINDING_NOT_MAPPED, "requested access to binding memory but it was not mapped");
    *pData = pipeline->buffersMappedData[bindingIndex];
    return CE_SUCCESS;
}

static VkResult __createVkPipeline(CeInstance instance, CePipeline pipeline) {
    VkPipelineShaderStageCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = pipeline->vulkanShader,
        .pName = "main",
    };
    VkComputePipelineCreateInfo pipeInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .layout = pipeline->vulkanPipelineLayout,
        .stage = shaderInfo,
    };
    return vkCreateComputePipelines(ceGetInstanceVulkanDevice(instance),
    pipeline->vulkanPipelineCache, 1, &pipeInfo, 
    NULL, &pipeline->vulkanFinishedPipeline);
}

CeResult ceCreatePipeline(CeInstance instance, const CePipelineCreationArgs * args, CePipeline * pipeline) {
#define ALIAS (*pipeline)
    if(!instance || !args || !pipeline)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot create pipeline: some parameters were NULL");
        
    ALIAS = calloc(1, sizeof(struct CePipeline_t));
    ALIAS->bufferCount = args->uBindingCount;
    ALIAS->dispatchGroupCount = args->uDispatchGroupCount;

    VkSemaphoreCreateInfo semInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    if(__createVkBuffersFromBindings(instance, args, ALIAS))
        return ceResult(CE_ERROR_INTERNAL, "failed to create Vk buffers");
    if(__createVkShaderModule(instance, ALIAS, args))
        return ceResult(CE_ERROR_INTERNAL, "failed to create Vk shader module");
    if(__createVkDescriptorPool(instance, ALIAS))
        return ceResult(CE_ERROR_INTERNAL, "failed to create Vk descriptor pool");
    if(__createVkDescriptorSetLayout(instance, ALIAS, args))
        return ceResult(CE_ERROR_INTERNAL, "failed to create Vk descriptor set layout");
    if(__createVkDescriptorSet(instance, args, ALIAS))
        return ceResult(CE_ERROR_INTERNAL, "failed to create Vk descriptor set");
    if(__createVkPipelineLayoutAndCache(instance, args, ALIAS))
        return ceResult(CE_ERROR_INTERNAL, "failed to create Vk pipeline layout and cache");
    if(__createVkPipeline(instance, ALIAS))
        return ceResult(CE_ERROR_INTERNAL, "failed to create Vk pipeline");
    return CE_SUCCESS;
#undef ALIAS
}

void ceDestroyPipeline(CeInstance instance, CePipeline pipeline) {
    for(uint32_t i = 0; i < pipeline->bufferCount; ++i) {
        if(pipeline->buffersMappedData[i])
            vkUnmapMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[i]);
        vkFreeMemory(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBufferMemory[i], NULL);
        vkDestroyBuffer(ceGetInstanceVulkanDevice(instance), pipeline->vulkanBuffers[i], NULL);
    }
    for(uint32_t i = 0; i < pipeline->constantCount; ++i) {
        if(!pipeline->constantsData[i].bIsLiveConstant)
            free(pipeline->constantsData[i].pData);
    }
    free(pipeline->buffersMappedData);
    free(pipeline->constantsData);
    free(pipeline->constantOffsets);
    free(pipeline->vulkanBuffers);
    free(pipeline->vulkanBufferMemory);
    free(pipeline->vulkanBufferMemorySize);
    vkFreeDescriptorSets(ceGetInstanceVulkanDevice(instance), pipeline->vulkanDescriptorPool, 1, &pipeline->vulkanDescriptorSet);
    vkDestroyDescriptorSetLayout(ceGetInstanceVulkanDevice(instance), pipeline->vulkanDescriptorSetLayout, NULL);
    vkDestroyDescriptorPool(ceGetInstanceVulkanDevice(instance), pipeline->vulkanDescriptorPool, NULL);
    vkDestroyPipelineLayout(ceGetInstanceVulkanDevice(instance), pipeline->vulkanPipelineLayout, NULL);
    vkDestroyPipelineCache(ceGetInstanceVulkanDevice(instance), pipeline->vulkanPipelineCache, NULL);
    vkDestroyShaderModule(ceGetInstanceVulkanDevice(instance), pipeline->vulkanShader, NULL);
    vkDestroyPipeline(ceGetInstanceVulkanDevice(instance), pipeline->vulkanFinishedPipeline, NULL);
    free(pipeline);
}