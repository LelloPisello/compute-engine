#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "ce-def.h"

typedef struct {
    const char* pApplicationName;
    uint32_t uApplicationVersion;
} CeInstanceCreationArgs;  


CeResult 
ceCreateInstance(const CeInstanceCreationArgs*, CeInstance*);

CeResult
ceResetInstanceCommands(CeInstance);

void
ceDestroyInstance(CeInstance);


CeVulkanVersion
ceGetVulkanVersion(void);

uint32_t 
ceGetInstanceNextFreeQueue(CeInstance);
#ifdef __cplusplus
}
#endif