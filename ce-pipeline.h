#pragma once
#include "ce-def.h"

typedef struct {
    uint32_t uBindingElementSize;
    uint32_t uBindingElementCount;
} CePipelineBindingInfo;

typedef struct {

} CePipelineUniformInfo;

typedef struct {
    const char* pShaderFilename;
    CePipelineBindingInfo *pPipelineBindings;
    uint32_t uPipelineBindingCount;
} CePipelineCreationArgs;

CeResult 
ceCreatePipeline(CeInstance, const CePipelineCreationArgs*, CePipeline*);

CeResult
ceMapPipelineBindingMemory(CeInstance, CePipeline, uint32_t bindingIndex, void**);

void
ceUnmapPipelineBindingMemory(CeInstance, CePipeline, uint32_t bindingIndex);

void
ceDestroyPipeline(CeInstance, CePipeline);