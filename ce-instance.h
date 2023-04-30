#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "ce-def.h"

typedef struct {
    const char* pApplicationName;
    uint32_t uApplicationVersion;
} CeInstanceCreationArgs;  

/**
* Create a CE instance and write its address into the supplied handle.
* \param args pointer to a CeInstanceCreationArgs structure containing parameters for instance creation
* \param instance the instance handle the function writes to
*/
CeResult 
ceCreateInstance(const CeInstanceCreationArgs* args, CeInstance* instance);

/**
* Reset every command created from an instance.
* \param instance the instance whose commands are going to be reset
*/
CeResult
ceResetInstanceCommands(CeInstance instance);

/**
* Destroy a CE instance from a CE instance handle
* \param instance the instance that is going to be destroyed
*/
void
ceDestroyInstance(CeInstance instance);

/**
* Returns a CE enum corresponding to a VK version.
*/
CeVulkanVersion
ceGetVulkanVersion(void);

#ifdef __cplusplus
}
#endif