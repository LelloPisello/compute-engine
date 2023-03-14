#pragma once
#include "ce-def.h"

typedef void(*CeErrorCallbackFunction)(int iErrorCode, const char* pErrorMessage);

CeResult 
ceSetErrorCallback(CeErrorCallbackFunction pFunc);