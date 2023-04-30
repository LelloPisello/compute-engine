#pragma once
#include "ce-def.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t uElementSize;
    uint32_t uElementCount;
    CeBool32 bIsUniform;
    void* pInitialData;
    CeBool32 bKeepMapped;
} CePipelineBindingInfo;

typedef struct {
    void* pData;
    uint32_t uDataSize;
    CeBool32 bIsLiveConstant;
} CePipelineConstantInfo;

typedef struct {
    const char* pShaderFilename;
    CePipelineBindingInfo *pBindings;
    uint32_t uBindingCount;
    CePipelineConstantInfo *pConstants;
    uint32_t uConstantCount;
    uint32_t uDispatchGroupCount;
    CeBool32 bIsPriorityPipeline;
} CePipelineCreationArgs;

CeResult 
ceCreatePipeline(CeInstance, const CePipelineCreationArgs*, CePipeline*);

CeResult
ceMapPipelineBindingMemory(CeInstance, CePipeline, uint32_t bindingIndex, void**);

void
ceUnmapPipelineBindingMemory(CeInstance, CePipeline, uint32_t bindingIndex);

CeResult 
ceGetPipelineBindingMemory(CePipeline, uint32_t bindingIndex, void**);

void
ceDestroyPipeline(CeInstance, CePipeline);
#ifdef __cplusplus
}
#endif