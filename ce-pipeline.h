#pragma once
#include "ce-def.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t uBindingElementSize;
    uint32_t uBindingElementCount;
} CePipelineBindingInfo;

typedef struct {
    void* pData;
    uint32_t uDataSize;
} CePipelineConstantInfo;

typedef struct {
    const char* pShaderFilename;
    CePipelineBindingInfo *pPipelineBindings;
    uint32_t uPipelineBindingCount;
    CePipelineConstantInfo *pPipelineConstants;
    uint32_t uPipelineConstantCount;
    uint32_t uDispatchGroupCount;
} CePipelineCreationArgs;

CeResult 
ceCreatePipeline(CeInstance, const CePipelineCreationArgs*, CePipeline*);

CeResult
ceMapPipelineBindingMemory(CeInstance, CePipeline, uint32_t bindingIndex, void**);

void
ceUnmapPipelineBindingMemory(CeInstance, CePipeline, uint32_t bindingIndex);

void
ceDestroyPipeline(CeInstance, CePipeline);
#ifdef __cplusplus
}
#endif