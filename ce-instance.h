#pragma once
#include "ce-def.h"

typedef void(*CeInstanceErrorCallback)(const char* pErrorMessage, int iError);

typedef struct {
    const char* pApplicationName;
    uint32_t uApplicationVersion;
    CeInstanceErrorCallback pErrorCallback;
} CeInstanceCreationArgs;  


CeResult 
ceCreateInstance(const CeInstanceCreationArgs*, CeInstance*);

CeResult
ceResetInstanceCommands(CeInstance);

void
ceDestroyInstance(CeInstance);

void*
ceGetInstanceVulkanInstance(CeInstance);

void*
ceGetInstanceVulkanDevice(CeInstance);

uint32_t
ceGetInstanceVulkanQueueFamilyIndex(CeInstance);

void*
ceGetInstanceVulkanCommandPool(CeInstance);

CeVulkanVersion
ceGetVulkanVersion(void);

uint32_t 
ceGetInstanceNextFreeQueue(CeInstance);

CeResult
ceSetInstanceQueueToBusy(CeInstance, uint32_t queueIndex);

CeResult
ceSetInstanceQueueToFree(CeInstance, uint32_t queueIndex);