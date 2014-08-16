
#ifndef _Magpie_Bridge_iOS_h_
#define _Magpie_Bridge_iOS_h_

// for C++ to call native
// implemented in MagpieCommandQueue.mm
extern void MagpieBridge_execute(const char * service, const char * action, const char * argsJson, const char * callbackId);

// for native to call C++
// implemented in MagpieBridgeiOS.mm
extern void MagpieBridge_event(const char *event, const char *argsJson);

extern void MagpieBridge_callback(const char *callbackId, bool success, int statusCode, const char *dataType, const char *data, bool keepCallback);

extern void MagpieBridge_callbackJs(const char * js);

#endif
