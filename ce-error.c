#include "ce-error.h"
#include "ce-def.h"
#include "ce-error-internal.h"
#define NULL (void*)0

static CeErrorCallbackFunction errorCallBack = NULL;

CeResult 
ceSetErrorCallback(CeErrorCallbackFunction pFunc) {
    if(!pFunc)
        return CE_ERROR_NULL_PASSED;
    errorCallBack = pFunc;
    return CE_SUCCESS;
}

CeResult ceResult(CeResult i, const char* extra) {
    if(!errorCallBack || i == CE_SUCCESS)
        return i;
    errorCallBack(i, extra);
    return i;
}
