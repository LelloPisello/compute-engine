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

CeResult ceResult(CeResult i) {
    if(!errorCallBack)
        return i;
    const char* message;
    switch(i) {
        case CE_SUCCESS:
            return CE_SUCCESS;
        case CE_ERROR_INTERNAL:
            message = "there was an internal failure within CE";
            break;
        case CE_ERROR_GENERIC:
            message = "generic CE failure";
            break;
        case CE_ERROR_INVALID_ARG:
            message = "an invalid argument has been passed to a CE function";
            break;
        case CE_ERROR_NULL_PASSED:
            message = "a nullptr was passed to a CE function which requires a valid pointer";
            break;
    }
    errorCallBack(i, message);
    return i;
}
