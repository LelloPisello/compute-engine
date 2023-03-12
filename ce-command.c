#include "ce-command.h"
#include "ce-def.h"
#include "ce-instance.h"
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <stdlib.h>
#include "ce-instance-internal.h"
#include "ce-pipeline-internal.h"

struct CeCommand_t {
    VkQueue vulkanQueue;
    VkCommandBuffer commandBuffer;
    VkFence commandFence;
    uint32_t vulkanQueueIndex;
};


CeResult 
ceRecordToCommand(const CeCommandRecordingArgs * args, CeCommand command) {
    if(args->bRecordCommand) {
        vkCmdExecuteCommands(command->commandBuffer, 1, &args->pSuppliedCommand->commandBuffer);
    } else {
        VkDescriptorSet target = ceGetPipelineVulkanDescriptorSet(args->pSuppliedPipeline);
        vkCmdBindPipeline(command->commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE, ceGetPipelineVulkanPipeline(args->pSuppliedPipeline));

        vkCmdBindDescriptorSets(command->commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        ceGetPipelineVulkanPipelineLayout(args->pSuppliedPipeline), 0, 1, 
        &target, 0, NULL);

        vkCmdDispatch(command->commandBuffer, 
        ceGetPipelineLongestBufferSize(args->pSuppliedPipeline), 1, 1);
    }
    return CE_SUCCESS;
}

CeResult
ceCreateCommand(CeInstance instance, const CeCommandCreationArgs* args, CeCommand* target) {
    *target = malloc(sizeof(struct CeCommand_t));
    (*target)->vulkanQueueIndex = ceGetInstanceNextFreeQueue(instance);
    ceSetInstanceQueueToBusy(instance, (*target)->vulkanQueueIndex);

    vkGetDeviceQueue(ceGetInstanceVulkanDevice(instance),
    ceGetInstanceVulkanQueueFamilyIndex(instance),
    (*target)->vulkanQueueIndex, &(*target)->vulkanQueue);

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };

    if(vkCreateFence(ceGetInstanceVulkanDevice(instance), &fenceInfo, NULL, &(*target)->commandFence) != VK_SUCCESS)
        return CE_ERROR_INTERNAL;

    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ceGetInstanceVulkanCommandPool(instance),
        .level = args->bIsSecondaryCommand ? 
            VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    
    if(vkAllocateCommandBuffers(ceGetInstanceVulkanDevice(instance), &allocInfo, &(*target)->commandBuffer) != VK_SUCCESS) {
        return CE_ERROR_INTERNAL;
    }    
    return CE_SUCCESS;
}

CeResult 
ceRunCommand(CeCommand command) {
    if(!command)
        return CE_ERROR_NULL_PASSED;
    VkSubmitInfo subInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command->commandBuffer
    };
    if(vkQueueSubmit(command->vulkanQueue, 1, &subInfo, command->commandFence) != VK_SUCCESS)
        return CE_ERROR_INTERNAL;
    return CE_SUCCESS;
}

CeResult
ceBeginCommand(CeCommand command) {
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
    };
    if(vkBeginCommandBuffer(command->commandBuffer, &beginInfo))
        return CE_ERROR_INTERNAL;
    return CE_SUCCESS;
}

CeResult 
ceEndCommand(CeCommand command) {
    if(!command)
        return CE_ERROR_NULL_PASSED;
    if(vkEndCommandBuffer(command->commandBuffer) != VK_SUCCESS)
        return CE_ERROR_INTERNAL;
    return CE_SUCCESS;
}

CeResult
ceWaitCommand(CeInstance instance, CeCommand command) {
    if(!instance || !command)
        return CE_ERROR_NULL_PASSED;
    
    if(vkWaitForFences(ceGetInstanceVulkanDevice(instance), 1, &command->commandFence, VK_TRUE, (uint64_t)-1))
        return CE_ERROR_INTERNAL;
    return CE_SUCCESS;
}

CeResult ceResetCommand(CeCommand command) {
    if(!command)
        return CE_ERROR_NULL_PASSED;
    if(vkResetCommandBuffer(command->commandBuffer, 0))
        return CE_ERROR_INTERNAL;
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
