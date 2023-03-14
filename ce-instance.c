#include "ce-instance.h"
#include "ce-def.h"
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "ce-instance-internal.h"
#include <string.h>
#include "ce-error-internal.h"

struct CeInstance_t {
    VkPhysicalDevice vulkanPhysicalDevice;
    VkInstance vulkanInstance;
    VkDevice vulkanDevice;
    VkCommandPool vulkanCommandPool;
    VkDescriptorPool vulkanDescriptorPool;
    uint32_t vulkanQueueFamily;
    uint32_t vulkanQueueCount;
    struct CeInstanceQueueList* queueListHead;
    VkDebugUtilsMessengerEXT debugMessenger;
};

struct CeInstanceQueueList {
    struct CeInstanceQueueList* next;
    CeBool32 is_free;
};

static VkResult __createVkCommandPool(CeInstance instance) {
    VkCommandPoolCreateInfo commandInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = instance->vulkanQueueFamily,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    return vkCreateCommandPool(instance->vulkanDevice, &commandInfo, NULL, &instance->vulkanCommandPool);
}

#ifdef DEBUG
CeBool32 layersAreSupported(const char** layers, uint32_t layerCount) {
    uint32_t availableLayerCount;
    vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
    VkLayerProperties *availableLayers = calloc(availableLayerCount, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers);

    for(uint32_t i = 0; i < layerCount; ++i) {
        CeBool32 layerFound = CE_FALSE;

        for(uint32_t j = 0; j < availableLayerCount; ++j) {
            if(strcmp(layers[i], availableLayers[j].layerName) == 0) {
                layerFound = CE_TRUE;
                break;
            }
        }

        if(!layerFound)
            return CE_FALSE;
    }
    return CE_TRUE;
}

#endif

static VkResult __createVkInstance(CeInstance instance, const CeInstanceCreationArgs* args) {

    VkInstanceCreateInfo instanceCreateInfo =  {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    };
    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .applicationVersion = args->uApplicationVersion,
        .pApplicationName = args->pApplicationName,
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .pEngineName = "Compute Engine (VK) 0.1.0",
    };
    #ifdef DEBUG 
    static const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    if(!layersAreSupported(validationLayers, 1))
        ceResult(CE_ERROR_INTERNAL, "validation layers were not found");
    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = validationLayers;
    #endif

    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &instance->vulkanInstance);
    
    

    return result;
}

static void __chooseVkDevice(CeInstance instance) {
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance->vulkanInstance, &physicalDeviceCount, NULL);
    VkPhysicalDevice *physicalDevices = malloc(sizeof(VkPhysicalDevice) * physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance->vulkanInstance, &physicalDeviceCount, physicalDevices);
    instance->vulkanPhysicalDevice = physicalDevices[0];
    for(uint32_t i = 0; i < physicalDeviceCount; ++i) {
        VkPhysicalDeviceProperties devProp;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &devProp);
        if(devProp.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            instance->vulkanPhysicalDevice = physicalDevices[i];
            free(physicalDevices);
            return;
        }
    }  
    free(physicalDevices);
}

static void __getOptimalVkDeviceQueueFamilyIndex(CeInstance instance) {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(instance->vulkanPhysicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(instance->vulkanPhysicalDevice, &queueFamilyCount, queueFamilies);



    for(uint32_t i = 0; i < queueFamilyCount; ++i) {
        if(queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            instance->vulkanQueueCount = queueFamilies[i].queueCount;
            instance->vulkanQueueFamily = i;
            break;
        }
    }

    free(queueFamilies);
}

static VkResult __createVkDeviceSingle(CeInstance instance) {
    __chooseVkDevice(instance);

    __getOptimalVkDeviceQueueFamilyIndex(instance);
    float* queuePriorities = calloc(instance->vulkanQueueCount, sizeof(float));

    for(int i = 0; i < instance->vulkanQueueCount; ++i) {
        queuePriorities[i] = 1.f - (i / (float)instance->vulkanQueueCount);
    }

    VkDeviceQueueCreateInfo queueInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = instance->vulkanQueueCount,
        .queueFamilyIndex = instance->vulkanQueueFamily,
        .pQueuePriorities = queuePriorities
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queueInfo,
        .queueCreateInfoCount = 1,
        
    };

    VkResult result = vkCreateDevice(instance->vulkanPhysicalDevice, &deviceCreateInfo, NULL, &instance->vulkanDevice);
    free(queuePriorities);
    return result;
}

CeResult ceCreateInstance(const CeInstanceCreationArgs * args, CeInstance *instance) {
    if(!args || !instance) 
        return ceResult(CE_ERROR_NULL_PASSED, "cannot create instance: some parameters were NULL");

    *instance = malloc(sizeof(struct CeInstance_t));
    if(__createVkInstance(*instance, args) != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL, "failed to create a Vk instance");
    if(__createVkDeviceSingle(*instance) != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL, "failed to create a Vk logical device");

    struct CeInstanceQueueList* current = (*instance)->queueListHead = malloc(sizeof(struct CeInstanceQueueList));
    for(uint32_t i = 0; i < (*instance)->vulkanQueueCount; ++i) {
        current->next = i < (*instance)->vulkanQueueCount - 1 ? malloc(sizeof(struct CeInstanceQueueList)) : NULL;
        current->is_free = 1;
        current = current->next;
    }


    if(__createVkCommandPool(*instance) != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL, "failed to create a Vk command pool");
    return CE_SUCCESS;
}

void ceDestroyInstance(CeInstance instance) {
    
    struct CeInstanceQueueList* current, *next;
    current = instance->queueListHead;
    next = current->next;
    for(; current != NULL;) {
        next = current->next;
        free(current);
        current = next;
    }


    vkDestroyCommandPool(instance->vulkanDevice, instance->vulkanCommandPool, NULL);
    vkDestroyDevice(instance->vulkanDevice, NULL);
    vkDestroyInstance(instance->vulkanInstance, NULL);
    free(instance);
} 

VkInstance
ceGetInstanceVulkanInstance(CeInstance instance) {
    return instance->vulkanInstance;
}   

VkDevice
ceGetInstanceVulkanDevice(CeInstance instance) {
    return instance->vulkanDevice;
}

uint32_t
ceGetInstanceVulkanQueueFamilyIndex(CeInstance instance) {
    return instance->vulkanQueueFamily;
}

VkCommandPool
ceGetInstanceVulkanCommandPool(CeInstance instance) {
    return instance->vulkanCommandPool;
}

VkPhysicalDevice
ceGetInstanceVulkanPhysicalDevice(CeInstance instance) {
    return instance->vulkanPhysicalDevice;
}

CeVulkanVersion
ceGetVulkanVersion() {
    uint32_t version;
    vkEnumerateInstanceVersion(&version);
    switch(VK_VERSION_MINOR(version)) {
        case 1:
            return CE_VULKAN_VERSION_1_1;
        case 2:
            return CE_VULKAN_VERSION_1_2;
        case 3:
            return CE_VULKAN_VERSION_1_3;
    }
    return CE_VULKAN_VERSION_1_0;
}

uint32_t ceGetInstanceNextFreeQueue(CeInstance instance) {
    uint32_t index = 0;
    for(struct CeInstanceQueueList* head = instance->queueListHead; head != NULL; head = head->next) {
        if(head->is_free)
            return index;
        ++index;
    }
    return 0;
}

CeResult ceSetInstanceQueueToBusy(CeInstance instance, uint32_t queueIndex) {
    struct CeInstanceQueueList* head = instance->queueListHead;
    for(uint32_t i = 0; i < queueIndex; ++i) {
        head = head->next;
    }
    head->is_free = 0;
    return CE_SUCCESS;
}

CeResult ceSetInstanceQueueToFree(CeInstance instance, uint32_t queueIndex) {
    if(!instance)
        return CE_ERROR_NULL_PASSED;
    struct CeInstanceQueueList* head = instance->queueListHead;
    for(uint32_t i = 0; i < queueIndex; ++i) {
        head = head->next;
    }
    head->is_free = 1;
    return CE_SUCCESS;
}