#include "ce-command.h"
#include "ce-def.h"
#include "ce-instance.h"
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <stdlib.h>
#include "ce-instance-internal.h"
#include "ce-pipeline-internal.h"
#include "ce-error-internal.h"

struct CeCommand_t {
    VkQueue vulkanQueue;
    VkCommandBuffer commandBuffer;
    VkFence commandFence;
    uint32_t vulkanQueueIndex;
    uint32_t workGroupCount;
};


CeResult 
ceRecordToCommand(const CeCommandRecordingArgs * args, CeCommand command) {
    if(!args || !command)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot record to command: none passed");
    if(!args->bRecordCommand && !args->pSuppliedPipeline) 
        return ceResult(CE_ERROR_INVALID_ARG, "cannot record pipeline: none passed");
    if(args->bRecordCommand && !args->pSuppliedCommand)
        return ceResult(CE_ERROR_INVALID_ARG, "cannot record secondary command: none passed");
    if(args->bRecordCommand) {
        vkCmdExecuteCommands(command->commandBuffer, 1, &args->pSuppliedCommand->commandBuffer);
    } else {
        
        vkCmdBindPipeline(command->commandBuffer, 
        VK_PIPELINE_BIND_POINT_COMPUTE, ceGetPipelineVulkanPipeline(args->pSuppliedPipeline));
        VkPipelineStageFlags waitInfo = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        VkDescriptorSet tempSet = ceGetPipelineVulkanDescriptorSet(args->pSuppliedPipeline);

        vkCmdBindDescriptorSets(command->commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE, 
            ceGetPipelineVulkanPipelineLayout(args->pSuppliedPipeline), 0, 1,
            &tempSet, 0, NULL);

        uint32_t pipelineConstantCount = ceGetPipelineConstantCount(args->pSuppliedPipeline);
        for(uint32_t i = 0; i < pipelineConstantCount; ++i) {
            uint32_t dataSize, dataOffset;
            void* pData;
            ceGetPipelineConstantData(args->pSuppliedPipeline, i, &pData, &dataSize, &dataOffset);
            vkCmdPushConstants(command->commandBuffer, ceGetPipelineVulkanPipelineLayout(args->pSuppliedPipeline),
            VK_SHADER_STAGE_COMPUTE_BIT, dataOffset, dataSize, pData);
        }

        vkCmdDispatch(command->commandBuffer, 
        ceGetPipelineDispatchWorkgroupCount(args->pSuppliedPipeline), 1, 1);
    }
    return CE_SUCCESS;
}

CeResult
ceCreateCommand(CeInstance instance, const CeCommandCreationArgs* args, CeCommand* target) {
    if(!instance || !args || !target)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot create commands: some necessary parameters were NULL");

    *target = malloc(sizeof(struct CeCommand_t));
    (*target)->vulkanQueueIndex = ceGetInstanceNextFreeQueue(instance);
    ceSetInstanceQueueToBusy(instance, (*target)->vulkanQueueIndex);


    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(ceGetInstanceVulkanPhysicalDevice(instance), &props);
        (*target)->workGroupCount = props.limits.maxComputeWorkGroupCount[0];
    }

    vkGetDeviceQueue(ceGetInstanceVulkanDevice(instance),
    ceGetInstanceVulkanQueueFamilyIndex(instance),
    (*target)->vulkanQueueIndex, &(*target)->vulkanQueue);

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };

    if(vkCreateFence(ceGetInstanceVulkanDevice(instance), &fenceInfo, NULL, &(*target)->commandFence) != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to create fence on device for command");

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ceGetInstanceVulkanCommandPool(instance),
        .level = args->bIsSecondaryCommand ? 
            VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    
    if(vkAllocateCommandBuffers(ceGetInstanceVulkanDevice(instance), &allocInfo, &(*target)->commandBuffer) != VK_SUCCESS) {
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to allocate the command buffer for CeCommand");
    }    
    return CE_SUCCESS;
}

CeResult 
ceRunCommand(CeInstance instance, CeCommand command) {
    if(!command)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot run command: none passed");
    VkSubmitInfo subInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command->commandBuffer
    };
    if(vkQueueSubmit(command->vulkanQueue, 1, &subInfo, command->commandFence) != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to run command");
    return CE_SUCCESS;
}

CeResult
ceBeginCommand(CeCommand command) {
    if(!command)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot begin command: none passed");
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    };
    if(vkBeginCommandBuffer(command->commandBuffer, &beginInfo))
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to begin command buffer recording");
    return CE_SUCCESS;
}

CeResult 
ceEndCommand(CeCommand command) {
    if(!command)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot end command: none passed");
    if(vkEndCommandBuffer(command->commandBuffer) != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to end command buffer recording");
    return CE_SUCCESS;
}

CeResult
ceWaitCommand(CeInstance instance, CeCommand command) {
    if(!instance || !command)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot wait for command: some parameters were NULL");
    
    VkResult result = vkWaitForFences(
        ceGetInstanceVulkanDevice(instance), 1,
        &command->commandFence, VK_TRUE, ~((uint64_t)0));
    //vkWaitForFences(ceGetInstanceVulkanDevice(instance), 1, &command->commandFence, VK_FALSE, ~((uint64_t)0));
    vkResetFences(ceGetInstanceVulkanDevice(instance), 1, &command->commandFence);
    if(result == VK_ERROR_DEVICE_LOST)
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to wait for a fence, device was lost");
    else if(result != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to wait for a fence");
    return CE_SUCCESS;
}

CeResult ceResetCommand(CeCommand command) {
    if(!command)
        return ceResult(CE_ERROR_NULL_PASSED, "cannot reset command: none passed");
    if(vkResetCommandBuffer(command->commandBuffer, 0)) {
        return ceResult(CE_ERROR_INTERNAL, "Vk failed to reset a command buffer");
    }
    return CE_SUCCESS;
}

void 
ceDestroyCommand(CeInstance instance, CeCommand command) {
    ceSetInstanceQueueToFree(instance, command->vulkanQueueIndex);
    vkResetCommandBuffer(command->commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    vkDestroyFence(ceGetInstanceVulkanDevice(instance), command->commandFence, NULL);
    vkFreeCommandBuffers(ceGetInstanceVulkanDevice(instance), ceGetInstanceVulkanCommandPool(instance), 1, &command->commandBuffer);
    free(command);
}
