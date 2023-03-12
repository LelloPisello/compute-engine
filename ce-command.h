#pragma once
#include "ce-def.h"

typedef struct {
    CeBool32 bIsSecondaryCommand;
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
ceResetCommand(CeCommand);

CeResult
ceRunCommand(CeCommand);

void
ceDestroyCommand(CeInstance, CeCommand);