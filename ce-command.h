#pragma once
#include "ce-def.h"

typedef struct {
    CeBool32 bIsSecondaryCommand;
    uint32_t uMaxCommands;
} CeCommandCreationArgs;

typedef struct {
    CeBool32 bRecordCommand;
    union {
        CeCommand pSuppliedCommand;
        CePipeline pSuppliedPipeline;
    };
} CeCommandRecordingArgs;

CeResult 
ceCreateCommand(CeInstance, const CeCommandCreationArgs*, CeCommand*);

CeResult
ceBeginCommand(CeCommand);

CeResult
ceRecordToCommand(const CeCommandRecordingArgs*, CeCommand);

CeResult
ceEndCommand(CeCommand);

CeResult
ceWaitCommand(CeInstance, CeCommand);

CeResult
ceResetCommand(CeInstance, CeCommand);

CeResult
ceRunCommand(CeInstance, CeCommand);

void
ceDestroyCommand(CeInstance, CeCommand);