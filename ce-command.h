#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "ce-def.h"

typedef struct {
    CeBool32 bIsSecondaryCommand;
} CeCommandCreationArgs;

typedef struct {
    CeBool32 bRecordCommand;
    union {
        CePipeline pSuppliedPipeline;
        CeCommand pSuppliedCommand;
    };
} CeCommandRecordingArgs;
/**
* Create a CE command from a CE instance using some parameters and write its address to a supplied handle.
* \param instance the instance the command is going to be created from
* \param args a pointer to a CeCommandCreationArgs structure containing parameters for command creation
* \param command a pointer to the CeCommand handle that will contain the created Command's address.
*/
CeResult 
ceCreateCommand(CeInstance instance, const CeCommandCreationArgs* args, CeCommand* command);

CeResult
ceBeginCommand(CeCommand);

CeResult
ceRecordToCommand(const CeCommandRecordingArgs* args, CeCommand);

CeResult
ceEndCommand(CeCommand);

CeResult
ceWaitCommand(CeInstance, CeCommand);

CeResult
ceResetCommand(CeCommand);

CeResult
ceRunCommand(CeInstance, CeCommand);

void
ceDestroyCommand(CeInstance, CeCommand);

#ifdef __cplusplus
}
#endif