#include "ce-instance.h"
#include "ce-def.h"
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include "ce-instance-internal.h"
#include <string.h>
//#define DEBUG
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

#include <signal.h>
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
    static const char* required_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties *layerProperties = calloc(layerCount, sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);

    for(uint32_t i = 0; i < 1; ++i) {
        VkBool32 layerFound = VK_FALSE;
        for(uint32_t j = 0; j < layerCount; ++j) {
            if(strcmp(layerProperties[j].layerName, required_layers[i]) == 0) {
                layerFound = VK_TRUE;
                break;
            }
        }

        if(!layerFound) {
            raise(SIGABRT);
        }
    }
    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = required_layers;
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    //createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = NULL;

    #endif


    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    VkResult result = vkCreateInstance(&instanceCreateInfo, NULL, &instance->vulkanInstance);
    #ifdef DEBUG
    {
        PFN_vkCreateDebugUtilsMessengerEXT createFunc = 
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
        if(!createFunc)
            raise(SIGABRT);
            
        createFunc(instance->vulkanInstance, &createInfo, NULL, &instance->debugMessenger);
    }
    #endif

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
        queuePriorities[i] = 1.f - ((float)i) / instance->vulkanQueueCount;
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
        return CE_ERROR_NULL_PASSED;

    *instance = malloc(sizeof(struct CeInstance_t));
    if(__createVkInstance(*instance, args) != VK_SUCCESS)
        return CE_ERROR_INTERNAL;
    if(__createVkDeviceSingle(*instance) != VK_SUCCESS)
        return CE_ERROR_INTERNAL;

    struct CeInstanceQueueList* current = (*instance)->queueListHead = malloc(sizeof(struct CeInstanceQueueList));
    for(uint32_t i = 0; i < (*instance)->vulkanQueueCount; ++i) {
        current->next = i < (*instance)->vulkanQueueCount - 1 ? malloc(sizeof(struct CeInstanceQueueList)) : NULL;
        current->is_free = 1;
        current = current->next;
    }


    if(__createVkCommandPool(*instance) != VK_SUCCESS)
        return ceResult(CE_ERROR_INTERNAL);
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

    #ifdef DEBUG 
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyFunc = 
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance->vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
        if(!destroyFunc)
            raise(SIGABRT);
            
        destroyFunc(instance->vulkanInstance, instance->debugMessenger, NULL);
    }
    #endif

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
    if(!instance)
        return CE_ERROR_NULL_PASSED;
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