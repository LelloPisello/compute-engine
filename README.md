# Compute Engine

## What is Compute Engine?
**CE is a C library that aims to bring parallel buffer work closer to beginners.**
Working on big buffers is always time consuming, both for the CPU and the programmer.

Imagine you're a newcomer to the coding world, and you decide to write some very CPU intensive application:
You start typing away code, but it runs really slow, so you start looking at how you can optimize it and you read 
About compute shaders. 
All you can see is pages of tutorials on OpenGL, OpenCL and Vulkan, so you turn away.
CE can be the solution.

# Simple and clean code
The CE workflow is summarized like this:
- Create an instance of the library
- Create command buffers to record compute operations to
- Create pipelines containing compute operations and record them to a command buffer
- Submit the command buffer to your GPU operation queues
- Destroy everything

CE users only have to worry about very few new types, including:
```C
    CeInstance
    CeCommand
    CePipeline
```

 By CE's chosen convention we chose to pass most of the parameters to functions via structure pointers,
 and all the functions except those which destroy or free objects return a CeResult value:
 Every function in CE that requires a large amount of data takes a pointer to a user-filled structure
 That contains all the info the function needs.
 For example:
 
```C
    //The function ceCreateInstance takes three parameters:
    //a pointer to a CeInstanceCreationArgs structure and a CeInstance*
    // to write the finished structure's address to
    ...
    //we declare a handle to a CE instance
    CeInstance instance;
    //we define a structure containing the info CE needs to create an instance
    CeInstanceCreationArgs args = {
	    .pApplicationName = "test",
	    .uApplicationVersion = 0,
	};
	//we create a CeInstance that we store in "instance" and check whether creation succeeded
	if(ceCreateInstance(&args, &instance) != CE_SUCCESS) {
		//handle error
	}
```
# Installation

Requirements:
- Clang
- Vulkan SDK
- Git
- GNU make

### Linux:
```bash
#download the repo on your machine
git clone https://github.com/LelloPisello/compute-engine.git
cd compute-engine
mkdir build
make
sudo make install
```
To link against the library on GCC / Clang:
```bash
gcc <source_files> -lCE
#or
clang <source_files> -lCE
```
The library's header files are contained within /usr/include/CE, with the main one being /usr/include/CE/CE.h.
You should be able to include them like this:
```C
#include <CE/CE.h>
```

# Usage

## CeResult

CeResult is an enumeration type which represents a function's success/failure,
and how that failure was generated.
**All** functions that do not destroy CE objects or return global variables defined in the library return a CeResult.
the enumerations are self explanatory.

## CeInstance

the CeInstance is CE's basic building block: it is required to run most commands in CE,
and internally it represents the library's VK state, plus a few other things.
It creates a logical device on one of your physical GPUs, preferrably a discrete one if it is available.

### Creation

a CeInstance is created using the ceCreateInstance function, which takes two parameters:
- a CeInstanceCreationArgs pointer
- a CeInstance pointer
and returns a parameter of type CeResult.

the CeInstanceCreationArgs pointer **must** point to a structure of that type, which
is to be filled with some information, although most of it is optional.

the CeInstance pointer **must** point to a CeInstance variable, which is going to be filled 
if the function returns successfully.

the function returns CE_SUCCESS if it succeeds (as any other function).

example:
```C
CeInstance instance;
CeInstanceCreationArgs args;
args.pApplicationName = "test";
args.uApplicationVersion = 0; //not yet used
ceCreateInstance(&args, &instance); //creates a CeInstance inside "instance" using CeInstanceCreationArgs "args"
```

### Destruction

CeInstances are destroyed using the function ceDestroyInstance,
the function returns void and takes a single parameter of type CeInstance.

```C
CeInstance instance;
//... code ...
ceDestroyInstance(instance);
```

Instances **must not** be destroyed before objects created from them have already been destroyed.

### Usage

CeInstances are used in the creation of most other CE objects, and do not serve a lot of purpose otherwise.

## CeCommand

CeCommands are objects that represents a command buffer: a list of commands which can be run from the GPU
in an unordered manner.

Commands can be created, recorded, submitted and waited upon.

A command can be either a primary or secondary command.

### Creation

CeCommands are created in a similar way to CeInstances: using the ceCreateCommand function.
The function returns a CeResult and take three parameters:
- a CeInstance
- a CeCommandCreationArgs pointer
- a CeCommand pointer
All of the parameters **must** be valid (non NULL) pointers / handles.
The CeInstance is used during creation.
The CeCommandCreationArgs pointer is a pointer to a structure of the same type, which contains the creation parameters.
The CeCommand pointer is a pointer to a CeCommand handle, which is going to be filled if the function succeeds.

example:
```C
CeCommand command;
CeCommandCreationArgs args;
args.bIsSecondaryCommand = CE_FALSE; //sets the command type to primary
ceCreateCommand(instance, &args, &command);
```
the function return CE_SUCCESS if it succeeds.

### Recording

Recording to a command means adding instructions to it.
There are two types of instructions you can record: 
- secondary commands
- compute pipelines
Users **must** record at most one pipeline to a command, but can record as many buffers as they want.

To begin recording, use the function ceBeginCommand.
the function returns a CeResult and takes one parameter: a CeCommand.
```C
CeCommand command;
//command creation...
ceBeginCommand(command);
```

To record to a command, use the function ceRecordToCommand.
The function returns a CeResult and takes two parameters:
- a CeCommandRecordingArgs pointer
- a CeCommand 

Both parameters **must** be valid pointers / handles.

The first parameter points to a CeCommandRecordingArgs structure,
which contains information about the recording.

The second parameter is the Command to record to.

To end a command recording, use the function ceEndCommand.
it takes one parameter of type CeCommand, the command whose recording is being stopped.

example:
```C
CeCommand command;
//creation
ceBeginCommand(command); //begin command recording
CeCommandRecordingArgs args;
args.bRecordCommand = CE_FALSE; //this says we are recording a pipeline
args.pSuppliedPipeline = pipeline; //more about pipelines later
ceRecordToCommand(&args, command); //record our pipeline to the command
ceEndCommand(command); //end command recording.
```

### Submission

CeCommands can be submitted to a GPU queue using the function ceRunCommand.
It returns a CeResult and takes two parameters:
- a CeInstance
- a CeCommand
The CeCommand submitted uses the first available Queue inside the CeInstance.

```C
CeInstance instance;
//instance creation
CeCommand command;
//command creation and recording...
ceRunCommand(instance, command);//submits the command
```

If the command is submitted correctly the function returns CE_SUCCESS.

### Waiting

You can wait for command completion with the function ceWaitCommand.
it takes one parameter, a CeCommand, which is the command whose completion is going to be waited for.
```C
CeInstance instance;
//instance creation
CeCommand command;
//command creation, recording and submission
ceWaitCommand(instance, command); //wait for command completion
```

By default the function is going to wait the maximum time allowed by VK.

### Destruction and Resetting

Commands can be destructed and reset as well.

Resetting a command simply empties its buffer, so that it no longer contains any instructions.
Command buffers can be reset using the function ceResetCommand.
The function returns a CeResult and takes one parameter, the CeCommand to reset.
```C
CeCommand command;
//creation and recording...
ceResetCommand(command);//the command is reset and is now empty.
```

Commands can be destroyed with the function ceDestroyCommand.
It returns void and takes two parameters:
- a CeInstance
- a CeCommand
The CeInstance is used to free the command buffer from its logical device, and the CeCommand is the command to be destroyed.

```C
CeInstance instance;
//creation
CeCommand command;
//creation + other operations...
ceDestroyCommand(instance, command);
```

Note: the command **must** be destroyed before destroying the instance it was created upon.

## Pipelines

Pipelines are an almost direct wrapping to compute pipelines in vulkan.
A pipeline is kind of like a structure which defines the environment a/some shader/s is run in.
A CePipeline contains one compute shader and information about the data it works on.

CePipelines can be created, recorded to commands, destroyed,
and their data can be read and written to.

### Creation

Pipelines are created using the ceCreatePipeline function.
It returns a CeResult and takes three parameters:
- a CeInstance
- a pointer to a CePipelineCreationArgs structure
- a pointer to a CePipeline handle
The function creates a pipeline from the data inside the CeInstance, using the arguments
inside the CePipelineCreationArgs, and writes the finished pipeline's address into the CePipeline handle.
All of the parameters **must** be valid pointers/handles.

#### CePipelineCreationArgs and CePipelineBindingInfo

This structure contains all the info necessary to build a pipeline,
and is defined like so:
```C
typedef struct {
    const char* pShaderFilename;
    CePipelineBindingInfo *pPipelineBindings;
    uint32_t uPipelineBindingCount;
    CePipelineConstantInfo *pPipelineConstants;
    uint32_t uPipelineConstantCount;
    uint32_t uDispatchGroupCount;
} CePipelineCreationArgs;
```
The pShaderFilename is a string containing the filename of the compiled shader
the pipeline uses (SPIR-V). It **must** not be NULL.
Note: the shader's entry point should be "main" 

The pPipelineBindings is a pointer to a CePipelineBindingInfo structure array 
of uPipelineBindingCount elements. It **must** be a valid pointer if uPipelineBindingCount is not 0.

The pPipelineConstants is a pointer to a CePipelineConstantInfo structure array
of uPipelineConstantCount elements. It **must** be a valid pointer if uPipelineConstantCount is not 0.

The uDispatchGroupCount member is the number of work groups which are going to be dispatched in your shader.
If set to 0 the shader will run on N workgroups, where N is equal to the pipeline's longest binding's length.

the CePipelineBindingInfo structure is defined like so:
```C
typedef struct {
    uint32_t uBindingElementSize;
    uint32_t uBindingElementCount;
} CePipelineBindingInfo;
```

The two members of the structure are respectively the size of an element of the buffer and its length.
The binding number of a buffer are going to be the same as their index as the array passed to the
CePipelineCreationArgs structure.

the CePipelineConstantInfo structure is defined like so:
```C
typedef struct {
    void* pData;
    uint32_t uDataSize;
    CeBool32 bIsLiveConstant;
} CePipelineConstantInfo;
```
The pData member is a pointer to the constant data of size uDataSize you want to pass to your pipeline.

The bIsLiveConstant member is a boolean representing whether the constant should be updated live or should be copied
to CE's memory and saved. 
If bIsLiveConstant is set to either CE_TRUE or a non-zero value the data pData points to is used by CE, and should
not be deallocated before destroying the pipeline/s which uses it. 
If it is set to either CE_FALSE or 0, CE will create a copy of the constant data and store it.

A sample program might look like this:
```C
CeInstance instance;
//instance creation
CePipeline pipeline;
CePipelineCreationArgs args;
CePipelineBindingInfo bindings[2];
bindings[0] = {sizeof(int), 4};
bindings[1] = {sizeof(int), 10};
//two bindings, two arrays with element size sizeof(int) and 4 and 10 elements each
//note: the number of groups working on the array is going to be equal to the size of
//the largest binding, or the GPU's limit.
args.pShaderFilename = "shader.spv";
args.pPipelineBindings = bindings;
args.uPipelineBindingCount = 2;
ceCreatePipeline(instance, &args, &pipeline);
```

### Recording

Recording a pipeline to a command is explained in the CeCommand section above.

### Data Reading/Writing

Pipeline buffer data can be read/written with the function ceMapPipelineBindingMemory and ceUnmapPipelineBindingMemory.

To start reading/writing data use the function ceMapPipelineBindingMemory.
it returns a CeResult and takes four parameters: 
- a CeInstance
- a CePipeline
- a uint32_t "bindingIndex"
- a pointer to a (void*) variable
The function maps the data of the CePipeline's "bindingIndex"th binding to an area in memory,
and writes its address inside the (void*) variable supplied.
The instance is necessary because the mapped binding's actual memory is used by
the instance's logical device.
The binding index **must** not be higher than the pipeline's binding count.
The other parameters **must** be valid pointers/handles.

To stop reading/writing data from/to a binding, use the function ceUnmapPipelineBindingMemory.
it returns void and takes three parameters:
- a CeInstance
- a CePipeline
- a uint32_t "bindingIndex"
The function unmaps the pipeline's "bindingIndex"th buffer's previously mapped data by ceMapPipelineBindingMemory.
The binding index **must** not be higher than the pipeline's binding count.
The other parameters **must** be valid pointers/handles.

code sample:
```C
CeInstance instance;
CePipeline pipeline;
//pipeline and instance creation
//suppose pipeline's 0th binding is an integer array of 1000 elements.
int* data;
//converting an int** to a void** is technically unsafe, but whatever
ceMapPipelineBindingMemory(instance, pipeline, 0, (void**)&data);
for(int i = 0; i < 1000; ++i) {
    printf("%i\n", data[i]);
}
ceUnmapPipelineBindingMemory(instance, pipeline, 0);
//this prints every element of the 0th binding to screen.
```

### Destruction

CePipelines can be destroyed with the function ceDestroyPipeline.
it returns void and takes two parameters:
- a CeInstance
- a CePipeline
the function destroys the CePipeline created from the CeInstance.
Note: the pipeline **must** be destroyed before the instance it was created from is.

Code sample:
```C
CeInstance instance;
//creation
CePipeline pipeline;
//creation etc. etc.
ceDestroyPipeline(instance, pipeline);
```
Note: pipeline **should** be destroyed before commands.

## Error Callbacks

If the user so pleases, error callbacks can be setup with the function ceSetErrorCallback.
it returns a CeResult and takes one parameter: a function pointer to a callback.
The function you're passing should have the signature void(*)(int, const char*);
When your callback is called, the error code is passed as the first parameter,
and the error message as the second.

Code sample:
```C
#include <stdio.h>
#include <signal.h>
void callback(int errorCode, const char* errorMessage) {
    printf("error %i: %s\n", errorCode, errorMessage);
    raise(SIGABRT);
}
int main() {
    ceSetErrorCallback(callback);
    //CE code ...
    return 0;
}
```
In this code whichever error CE raises is printed to screen and the program is aborted.
Error severity might be implemented later in the library's development.

