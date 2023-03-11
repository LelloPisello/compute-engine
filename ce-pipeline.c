#include "ce-pipeline.h"
#include "ce-def.h"
#include "ce-instance.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <stdlib.h>

struct CePipeline_t { 
    VkPipeline vulkanFinishedPipeline;
    VkPipelineLayout vulkanPipelineLayout;
    VkPipelineCache vulkanPipelineCache;
    VkDescriptorSetLayout vulkanDescriptorSetLayout;
    VkDescriptorSet vulkanDescriptorSet;
    VkShaderModule vulkanShader;
};

#include <stdio.h>

static void __createVkShaderModule(CeInstance instance, CePipeline pipeline, const CePipelineCreationArgs* args) {
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

    vkCreateShaderModule(ceGetInstanceVulkanDevice(instance), &shaderInfo, NULL, &pipeline->vulkanShader);

    free(finalBuffer);
}

static void __createVkDescriptorSetLayout(CeInstance instance, CePipeline pipeline, const CePipelineCreationArgs* args) {
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
    vkCreateDescriptorSetLayout(ceGetInstanceVulkanDevice(instance), &layoutInfo, NULL, &pipeline->vulkanDescriptorSetLayout);
    free(bindings);
}

static void __createVkDescriptorSet(CePipeline pipeline) {

}

static void __createVkPipelineLayoutAndCache(CePipeline pipeline) {

}

static void __createVkPipeline(CePipeline pipeline) {

}

CeResult ceCreatePipeline(CeInstance instance, const CePipelineCreationArgs * args, CePipeline * pipeline) {
#define ALIAS (*pipeline)
    if(!instance || !args || !pipeline)
        return CE_ERROR_NULL_PASSED;
        
    ALIAS = malloc(sizeof(struct CePipeline_t));
    
    __createVkShaderModule(instance, ALIAS, args);
    __createVkDescriptorSetLayout(instance, ALIAS, args);
    __createVkDescriptorSet(ALIAS);
    __createVkPipelineLayoutAndCache(ALIAS);
    __createVkPipeline(ALIAS);
    
    return CE_SUCCESS;
#undef ALIAS
}

void ceDestroyPipeline(CeInstance instance, CePipeline pipeline) {
    vkDestroyDescriptorSetLayout(ceGetInstanceVulkanDevice(instance), pipeline->vulkanDescriptorSetLayout, NULL);
    vkDestroyShaderModule(ceGetInstanceVulkanDevice(instance), pipeline->vulkanShader, NULL);
}