/*
COMPUTEENGINE
*/

#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#define CE_MAKE_HANDLE(handlename) typedef struct handlename##_t *handlename;
CE_MAKE_HANDLE(CeInstance)
CE_MAKE_HANDLE(CePipeline)
CE_MAKE_HANDLE(CeCommand)

#define DEBUG

#define CE_TRUE 1
#define CE_FALSE 0

typedef enum {
    CE_SUCCESS = 0,
    CE_ERROR_GENERIC,
    CE_ERROR_NULL_PASSED,
    CE_ERROR_INVALID_ARG,
    CE_ERROR_INTERNAL,
    CE_ERROR_BINDING_NOT_MAPPED
} CeResult;

typedef enum {
    CE_VULKAN_VERSION_1_0 = 0,
    CE_VULKAN_VERSION_1_1 = 1,
    CE_VULKAN_VERSION_1_2 = 2,
    CE_VULKAN_VERSION_1_3 = 3
} CeVulkanVersion;

typedef uint32_t CeBool32;

#ifdef __cplusplus
}
#endif


