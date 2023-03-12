# Compute Engine

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

