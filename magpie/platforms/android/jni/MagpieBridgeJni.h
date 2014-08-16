
#ifndef __MagpieBridgeJni_H__
#define __MagpieBridgeJni_H__

extern "C"
{

void MagpieBridge_execute(const char * service, const char * action, const char * argsJson, const char * callbackId);

}

#endif
