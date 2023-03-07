#include "CE.h"
#include "ce-command.h"
#include "ce-def.h"
#include "CE.h"
#include "ce-instance.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>

void debugCallback(const char* errorm, int error) {
    fprintf(stderr, "CE error:\n\t(%i): \"%s\"\n", error, errorm);
    raise(SIGABRT);
}

int main() {
    printf("CE is running vulkan 1.%i\n", ceGetVulkanVersion());

    CeInstance instance;
    {
        CeInstanceCreationArgs instanceArgs = {
            .pApplicationName = "test",
        };
        printf("creating CE instance...\n");
        ceCreateInstance(&instanceArgs, &instance);
    }

    CeCommand command, a ,b ,c;
    {
        CeCommandCreationArgs commandArgs = {
        };
        printf("creating CE command...\n");
        ceCreateCommand(instance, &commandArgs, &command);
        ceCreateCommand(instance, &commandArgs, &a);
        ceCreateCommand(instance, &commandArgs, &b);
        ceCreateCommand(instance, &commandArgs, &c);
    }

    printf("starting command recording...\n");
    ceBeginCommand(command);
    printf("ending command recording...\n");
    ceEndCommand(command);

    char inputString[256] = {};
    for(; strncmp(inputString, "quit", 256) != 0; scanf("%255s",inputString)) {
        printf("type \"quit\" to exit the application\n");
    }
    printf("destroying CE command...\n");
    ceDestroyCommand(instance, command);
    ceDestroyCommand(instance, a);
    ceDestroyCommand(instance, b);
    ceDestroyCommand(instance, c);
    printf("destroying CE instance...\n");
    ceDestroyInstance(instance);
    return 0;
}