#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "ce-def.h"

typedef void(*CeErrorCallbackFunction)(int iErrorCode, const char* pErrorMessage);

/**
* Sets the CE global error callback to a supplied function pointer.
* \param pFunc the function pointer the error callback is going to be set to
*/
CeResult 
ceSetErrorCallback(CeErrorCallbackFunction pFunc);
#ifdef __cplusplus
}
#endif