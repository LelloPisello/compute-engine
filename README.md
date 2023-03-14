
### What is Compute Engine?
**CE is a C library that aims to bring parallel buffer work closer to beginners.**
Working on big buffers is always time consuming, both for the CPU and the programmer.

Imagine you're a newcomer to the coding world, and you decide to write some very CPU intensive application:
You start typing away code, but it runs really slow, so you start looking at how you can optimize it and you read 
About compute shaders. 
All you can see is pages of tutorials on OpenGL, OpenCL and Vulkan, so you turn away.
CE can be the solution.

## Simple and clean code
CE users only have to worry about very few new types:
```C
    CeInstance
    CeCommand
    CePipeline
```
All of these are opaque handle types, and are allocated on the heap via their respective creator functions:
```C
    ceCreateInstance
    ceCreateCommand
    ceCreatePipeline
```
 By CE's chosen convention we chose to pass all the parameters to function via structure pointers,
 and all the function except destroying function return a CeResult value:
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
### CeResult

CeResult is an enumeration type which represents a function's success/failure,
and how that failure was generated.
**All** functions that do not destroy CE objects or return global variables defined in the library return a CeResult.
the enumerations are self explanatory.

### CeInstance

the CeInstance is CE's basic building block: it is required to run most commands in CE,
and internally it represents the library's VK state, plus a few other things.

#### Creation

a CeInstance is created using the ceCreateInstance function, which takes two parameters:
. a CeInstanceCreationArgs pointer
. a CeInstance pointer
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

#### Destruction

CeInstances are destroyed using the function ceDestroyInstance,
the function returns void and takes a single parameter of type CeInstance.

```C
CeInstance instance;
//... code ...
ceDestroyInstance(instance);
```

Instances **must not** be destroyed before objects created from them have already been destroyed.

#### Usage

CeInstances are used in the creation of most other CE objects, and do not serve a lot of purpose otherwise.

### CeCommand

CeCommands are objects that represents a command buffer: a list of commands which can be run from the GPU
in an unordered manner.

Commands can be created, recorded, submitted and waited upon.

A command can be either a primary or secondary command.

#### Creation

CeCommands are created in a similar way to CeInstances: using the ceCreateCommand function.
The function returns a CeResult and take three parameters:
. a CeInstance
. a CeCommandCreationArgs pointer
. a CeCommand pointer
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

#### Recording

Recording to a command means adding instructions to it.
There are two types of instructions you can record: 
. secondary commands
. compute pipelines
Users **must** record at most one pipeline to a command, but can record as many buffers as they want.

To begin recording, use the function ceBeginCommand.
the function returns a CeResult and takes one parameter: a CeCommand.
```C
CeCommand command;
//command creation...
ceBeginCommand(command);
```
To record to a command, use the function ceRecordToCommand.
The function returns a CeResult and 