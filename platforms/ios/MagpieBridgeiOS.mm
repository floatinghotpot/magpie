
#include "Magpie.h"
#include "MagpieBridgeiOS.h"

// for native to call C++
void MagpieBridge_event(const char *event, const char *argsJson)
{
    NSLog(@"event: %s, %s", event, argsJson);
    
    Magpie::instance()->event(event, argsJson);
}

void MagpieBridge_callback(const char *callbackId, bool success, int statusCode, const char *dataType, const char *data, bool keepCallback)
{
    NSLog(@"callback: %s, %s, %d, %s, %s", callbackId, success?"true":"false", statusCode, dataType, data);

    Magpie::instance()->callback(callbackId, success, statusCode, dataType, data, keepCallback);
}

void MagpieBridge_callbackJs(const char * js)
{
    NSLog(@"callbackJs: %s", js);
    
    Magpie::instance()->callbackJs( js );
}

