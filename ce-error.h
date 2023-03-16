#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "ce-def.h"

typedef void(*CeErrorCallbackFunction)(int iErrorCode, const char* pErrorMessage);

CeResult 
ceSetErrorCallback(CeErrorCallbackFunction pFunc);
#ifdef __cplusplus
}
#endif