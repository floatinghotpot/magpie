//
//  Magpie.cpp
//
//  Created by Raymond Xie on 14-7-31.
//

#include <ostream>
#include <stdexcept>
#include <string>

#include "cocos2d.h"
#include "Magpie.h"

#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#pragma message("building android, supported")
#include "../android/jni/MagpieBridgeJni.h"

#elif(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#pragma message("building ios, supported")
#include "MagpieBridgeiOS.h"

#else
#error platform not supported yet
#endif

static Magpie* _magpie_singleton = NULL;

Magpie::Magpie(): _magic_counter(0){

    assert( _magpie_singleton == NULL );
    
    _magpie_singleton = this;
}

Magpie* Magpie::instance()
{
    // a instance of Magpie or it's derived class must be initialized, for example:
    // static Magpie cordovax;
    // or, new ChildMagpie();
    assert( _magpie_singleton != NULL );
    
    return _magpie_singleton;
}

Magpie::~Magpie() {

    while(_listeners.size() > 0) {
        MagpieEventHandler* h = _listeners.back();
        _listeners.pop_back();
        
        destroyEventHandler(h);
    }
    
    while (_callbacks.size() > 0) {
        MagpieCallbackHandler* h = _callbacks.back();
        _callbacks.pop_back();
        
        destroyCallbackHandler(h);
    }
}

void Magpie::addEventListener(const char * event_name, CCObject* obj, SEL_MagpieEvent func)
{
	_listeners.push_back( createEventHandler(event_name, obj, func) );
}

void Magpie::removeEventListener(const char * event_name, CCObject* obj, SEL_MagpieEvent func)
{
	for(MagpieEventList::iterator i = _listeners.begin(), e = _listeners.end(); i != e; /**/ ) {
		MagpieEventHandler* h = * i;
		if(h->obj == obj && h->func == func && (strcmp(h->event, event_name) == 0)) {
			i = _listeners.erase(i);
			destroyEventHandler( h );
		} else {
			i ++;
		}
	}
}

MagpieEventHandler* Magpie::createEventHandler(const char *strId, CCObject* obj, SEL_MagpieEvent func)
{
	MagpieEventHandler* h = new MagpieEventHandler;
	h->event = strdup( strId );
	h->obj = obj;
	h->func = func;

	return h;
}

void Magpie::destroyEventHandler( MagpieEventHandler* h )
{
	if(h) {
		if(h->event) free( h->event );
		delete h;
	}
}

void Magpie::execute(const char *service, const char *action, const char *args, CCObject* obj, SEL_MagpieCallback func)
{
	string callbackId = "";

	if(obj && func) {
		ostringstream oss;
		oss << (void*)(++ this->_magic_counter) << (void*)obj << (void*) &func;
		callbackId = oss.str();
		_callbacks.push_back( createCallbackHandler(callbackId.c_str(), obj, func) );

	} else {
		callbackId = "NULL";
	}

#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	MagpieBridge_execute(service, action, args, callbackId.c_str());

#elif(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	MagpieBridge_execute(service, action, args, callbackId.c_str());

#else
#error not implemented yet

#endif
}

MagpieCallbackHandler* Magpie::createCallbackHandler(const char *strId, CCObject* obj, SEL_MagpieCallback func)
{
	MagpieCallbackHandler* h = new MagpieCallbackHandler;
	h->callbackId = strdup( strId );
	h->obj = obj;
	h->func = func;
	h->timestamp = time(NULL);
    
	return h;
}

void Magpie::destroyCallbackHandler( MagpieCallbackHandler* h )
{
	if(h) {
		if(h->callbackId) free( h->callbackId );
		delete h;
	}
}

bool Magpie::event(const char *event_name, const char * args)
{
    bool ok = false;
    
	for(MagpieEventList::iterator i = _listeners.begin(), e = _listeners.end(); i != e; i++ ) {
		MagpieEventHandler* h = * i;
		if(strcmp(h->event, event_name) == 0) {
	        ((h->obj) ->* (h->func))( event_name, args );
            ok = true;

	        // don't break, we will continue to check multiple listeners on same event
		}
	}
    
    return ok;
}

bool Magpie::callback(const char *callbackId, bool success, int statusCode, const char* dataType, const char* data, bool keepCallback)
{
	if(strcmp(callbackId, "NULL") == 0) return true;
    
    bool ok = true;

	time_t oldTime = time(NULL) - MAGPIE_TIMEOUT;

	for(MagpieCallbackList::iterator i = _callbacks.begin(), e = _callbacks.end(); i != e; /**/ ) {
		MagpieCallbackHandler* h = * i;
		if(strcmp(h->callbackId, callbackId) == 0) {
	        ((h->obj) ->* (h->func))( success, statusCode, dataType, data );
            ok = true;

	        if(keepCallback) {
	        	i ++;
	        } else {
                i = _callbacks.erase(i);
                destroyCallbackHandler( h );
            
	        }
			break;

		} else if(h->timestamp < oldTime) {
			// the callback is too old, clean up.
            i = _callbacks.erase(i);
            destroyCallbackHandler( h );

		} else {
			i ++;
		}
	}
    
    return ok;
}

// common
const char * str_try = "try{";
const char * str_catch = "}catch";
const char * str_finally = "}finally";
const char * str_cordova_fireDocumentEvent = "cordova.fireDocumentEvent";

// legacy style, marked @Deprecated
const char * str_cordova_success = "cordova.callbackSuccess";
const char * str_cordova_error = "cordova.callbackError";

bool Magpie::callbackJs(const char *js)
{
	string str = js;
    
    bool ok = false;
    
    if(str.find( str_cordova_fireDocumentEvent ) != string::npos) {
    	ok = cordova_fireDocumentEvent( str );
        
    } else if(str.find( str_cordova_success ) != string::npos) {
    	ok = cordova_callbackSuccess( str );
        
    } else if(str.find( str_cordova_error ) != string::npos) {
    	ok = cordova_callbackError( str );
        
    }
    
    return ok;
    
}

/* Events
 *
 * javascript:cordova.fireDocumentEvent('backbutton');
 * javascript:cordova.fireDocumentEvent('onFailedToReceiveAd', { 'error': %d, 'reason':'%s' });
 * javascript:try{cordova.fireDocumentEvent('resume');}catch(e){console.log('exception firing resume event from native');};
 */
bool Magpie::cordova_fireDocumentEvent(string & str)
{
    string event_name = "";
    
	string::size_type ntry, nfire, n;
    
	nfire = str.find(str_cordova_fireDocumentEvent);
    
	ntry = str.find( str_try );
    if( ntry != string::npos && ntry < nfire ) {
        string::size_type ncatch, nfinally, ntryend = 0;
        
        nfinally = str.rfind( str_finally );
        if(nfinally != string::npos) ntryend = nfinally;
        
        ncatch = str.rfind( str_catch );
        if(ncatch != string::npos) ntryend = ncatch;
        
        if(nfire < ntryend) {
            ntry += strlen( str_try );
            nfire -= ntry;
            ntryend -= ntry;
            str.erase(0, ntry).erase(ntryend);
        }
	}
    
    do {
        str.erase(0, nfire + strlen(str_cordova_fireDocumentEvent));
        
        if((n = str.find("(")) == string::npos) break;
        str.erase(0, n+1);
        
        if((n = str.rfind(")")) == string::npos) break;
        str.erase(n);
        
        if((n = str.find("'")) == string::npos) break;
        str.erase(0, n+1);
        
        if((n = str.find("'")) == string::npos) break;
        event_name = str.substr(0, n);
        str.erase(0, n+1);
        
        if((n = str.find(",")) != string::npos) {
            str.erase(0, n+1);
        } else {
            str = "";
        }
        
        event( event_name.c_str(), str.c_str() );
        return true;
        
    } while(0);
    
    return false;
}

/*
 * cordova.callbackSuccess('%@',%@);
 */

bool Magpie::cordova_callbackSuccess(string & str)
{
	do {
		string::size_type n, n2;
		if((n = str.find(str_cordova_success)) == string::npos) break;
		n += strlen(str_cordova_success);
        
        n = str.find_first_not_of("(' \t", n);
        if((n2 = str.find("'", n)) == string::npos) break;
        string callbackId = str.substr(n, n2-n);
        
        if((n = str.find(",", n2)) == string::npos) break;
        n += 1;
        if((n2 = str.rfind(")")) == string::npos) break;
        str = str.substr(n, n2-n);
        
        callback(callbackId.c_str(), true, 1, "json", str.c_str(), false);
        
		return true;
	} while(0);
    
	return false;
}

/*
 * cordova.callbackError('%@',%@);
 */

bool Magpie::cordova_callbackError(string & str)
{
	do {
		string::size_type n, n2;
		if((n = str.find(str_cordova_error)) == string::npos) break;
		n += strlen(str_cordova_error);
        
        n = str.find_first_not_of("(' \t", n);
        if((n2 = str.find("'", n)) == string::npos) break;
        string callbackId = str.substr(n, n2-n);
        
        if((n = str.find(",", n2)) == string::npos) break;
        n += 1;
        if((n2 = str.rfind(")")) == string::npos) break;
        str = str.substr(n, n2-n);
        
        callback(callbackId.c_str(), false, 1, "json", str.c_str(), false);
        
		return true;
	} while(0);
    
	return false;
}

