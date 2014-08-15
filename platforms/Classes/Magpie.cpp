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
#include "../proj.android/jni/Magpie/MagpieBridgeJni.h"

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

	        //if(keepCallback) {
	        //	i ++;
	        //} else {
                i = _callbacks.erase(i);
                destroyCallbackHandler( h );
            
	        //}
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

// @Deprecated
const char * str_cordova_success = "cordova.callbackSuccess";
const char * str_cordova_error = "cordova.callbackError";

bool Magpie::callbackJs(const char *js)
{
	string str = js;
    
    bool ok = false;
    
    /*
     if(str.find( str_cordova_nativeEvalAndFetch ) != string::npos) {
     ok = JsParser_nativeEvalAndFetch( str );
     
     } else if(str.find( str_cordova_nativeCallback ) != string::npos) {
     ok = JsParser_nativeCallback( str );
     
     } else if(str.find( str_cordova_callbackFromNative ) != string::npos) {
     ok = JsParser_callbackFromNative( str );
     
     } else */
    
    if(str.find( str_cordova_fireDocumentEvent ) != string::npos) {
    	ok = cordova_fireDocumentEvent( str );
        
    } else if(str.find( str_cordova_success ) != string::npos) {
    	ok = cordova_callbackSuccess( str );
        
    } else if(str.find( str_cordova_error ) != string::npos) {
    	ok = cordova_callbackError( str );
        
    }
    
    /*else {
     ok = JsParser_callbackWithEncodedMsg( str );
     }*/
    
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

// ------- following js parser is not needed, as native code directly call event or callback -----

bool JsParser_callbackJs( const char * str );


// callback from ios
const char * str_cordova_nativeCallback = "cordova.require('cordova/exec').nativeCallback";
const char * str_cordova_nativeEvalAndFetch = "cordova.require('cordova/exec').nativeEvalAndFetch(function(){";

// callback from android
const char * str_cordova_callbackFromNative = "cordova.callbackFromNative";

/*
 * "cordova.require('cordova/exec').nativeEvalAndFetch(function(){%@})", js
 *
 */
bool JsParser_nativeEvalAndFetch(string & str)
{
    string::size_type n;
    
    do {
        if((n = str.find(str_cordova_nativeEvalAndFetch)) == string::npos) break;
        n += strlen(str_cordova_nativeEvalAndFetch);
        str.erase(0, n);
        
        if((n = str.rfind("}")) == string::npos) break;
        str.erase( n );
        
        return JsParser_callbackJs( str.c_str() );
    } while (0);
    
    return false;
}

/*
 * cordova.require('cordova/exec').nativeCallback('NULL',1, "OK" ,0)
 * "('%@',%d,%@,%d)", callbackId, status, argumentsAsJSON, keepCallback;
 */
bool JsParser_nativeCallback(string & str)
{
    string callbackId, data;
    int statusCode, keepCallback;
    
    string::size_type n, n2;
    do {
        if((n = str.find(str_cordova_nativeCallback)) == string::npos) break;
        n += strlen(str_cordova_nativeCallback);
        str.erase(0, n);
        
        if((n = str.find("(")) == string::npos) break;
        str.erase(0, n+1);
        
        if((n = str.rfind(")")) == string::npos) break;
        str.erase(n);
        
        if((n = str.find("'")) == string::npos) break;
        n += 1;
        
        if((n2 = str.find("'", n)) == string::npos) break;
        callbackId = str.substr(n, (n2-n));
        
        if((n = str.find(",", n2+1)) == string::npos) break;
        n += 1;
        
        if((n2 = str.find(",", n)) == string::npos) break;
        data = str.substr(n, (n2-n));
        statusCode = atoi( data.c_str() );
        str.erase(0, n2+1);
        
        if((n = str.rfind(",")) == string::npos) break;
        data = str.substr(n+1);
        keepCallback = atoi(data.c_str());
        str.erase(n);
        
        n = str.find_first_not_of(" \t\n\r");
        n2 = str.find_last_not_of(" \t\n\r");
        data = str.substr(n, (n2-n+1));
        
        string dataType = "";
        char c = data.at(0);
        if( c == 't' ||  c == 'f' ) {
            dataType = "bool";
        } else if( isdigit(c) ) {
            dataType = "number";
        } else if(c == '"') {
            dataType = "string";
        } else if( c == '[' || c == '{' ) {
            dataType = "json";
        }
        
        bool success = (statusCode == 0) || (statusCode == 1);
        
        Magpie::instance()->callback(callbackId.c_str(), success, statusCode, dataType.c_str(), data.c_str(), keepCallback);
        return true;
        
    } while(0);
    
    return false;
}

/*
 * Message encoding rule:
 * J<jscode>
 * <S/F><keepCallback><statusCode>< ><callbackId><typeValue>
 */
bool JsParser_callbackWithEncodedMsg(string & msg)
{
    CCLOG("callbackWithEncodedMsg: %s", msg.c_str());
    
	string::size_type n, n2;
    
	try {
		do {
			char c = msg.at(0);
			if(c == 'J') {
				msg.erase(0, 1);
				// TODO: run javascript if we can, for example, in js binding engine
				return JsParser_callbackJs( msg.c_str() );
                
			} else {
				c = msg.at(0);
				if(c != 'S' && c != 'F') break;
				bool success = (c == 'S');
                
				c = msg.at(1);
				if(c != '1' && c != '0') break;
				bool keepCallback = (c == '1');
                
				n = msg.find(' ');
				if(n == string::npos) break;
				int statusCode = atoi( msg.substr(2, n-2).c_str() );
                
				n2 = msg.find(' ', n+1);
				if(n2 == string::npos) break;
				string callbackId = msg.substr(n, n2-(n+1));
                
				msg.erase(0, n2+1);
                
			    c = msg.at(0);
			    msg.erase(0, 1);
                
                string dataType = "";
                
			    switch(c) {
                    case 't':
                        dataType = "bool";
                        msg = (c == 't') ? "true" : "false";
                        break;
                    case 'N':
                        dataType = "NULL";
                        msg = "";
                        break;
                    case 'n':
                        dataType = "number";
                        break;
                    case 's':
                        dataType = "string";
                        break;
                    case 'S': // binary string
                        dataType = "binarystring";
                        break;
                    case 'A':
                        dataType = "arraybuffer";
                        break;
                    case '[':
                    case '{':
                        dataType = "json";
                        break;
			    }
                
			    Magpie::instance()->callback(callbackId.c_str(), success, statusCode, dataType.c_str(), msg.c_str(), keepCallback);
			    return true;
			}
		} while(0);
        
	} catch (std::out_of_range const& e) {
	    CCLOG("Magpie received encoded message in wrong format: %s", e.what() );
    }
    
    return false;
}

/*
 * Callback
 *
 * javascript:cordova.callbackFromNative('xxId',isSuccss,statusCode,[xxx],isKeepCallback);
 * javascript:cordova.callbackFromNative('xxId',isSuccss,statusCode,[atob('xxx')],isKeepCallback);
 * javascript:cordova.callbackFromNative('xxId',isSuccss,statusCode,[cordova.require('cordova/base64').toArrayBuffer('xxx')],isKeepCallback);
 */

#define str_atob                    "atob('"
#define str_baset64_toArrayBuffer     "cordova.require('cordova/base64').toArrayBuffer('"

bool JsParser_callbackFromNative(string & str)
{
    CCLOG("callbackFromNative: %s", str.c_str());
    
	string::size_type nfire, n, n2;
	nfire = str.find(str_cordova_callbackFromNative);
    
    do {
        str.erase(0, nfire + strlen(str_cordova_callbackFromNative));
        
        if((n = str.find("(")) == string::npos) break;
        str.erase(0, n+1);
        
        if((n = str.rfind(")")) == string::npos) break;
        str.erase(n);
        
        string callbackId = "";
        
        if((n = str.find("'")) == string::npos) break;
        n += 1;
        
        if((n2 = str.find("'",n)) == string::npos) break;
        callbackId = str.substr(n, n2-n);
        
        if((n = str.find_first_not_of(", \t",n2+1)) == string::npos) break;
        if((n2 = str.find(",",n)) == string::npos) break;
        bool success = (str.substr(n, n2-n) == "true");
        
        if((n = str.find_first_not_of(", \t",n2+1)) == string::npos) break;
        if((n2 = str.find(",",n)) == string::npos) break;
        int statusCode = atoi(str.substr(n, n2-n).c_str());
        
        if((n = str.find_first_not_of(", \t",n2+1)) == string::npos) break;
        if((n2 = str.rfind(",")) == string::npos) break;
        
        bool keepCallback = (str.substr(n2+1) == "true");
        
        if((n = str.find("[", n)) == string::npos) break;
        str.erase(0, n+1);
        if((n2 = str.rfind("]", n2)) == string::npos) break;
        str.erase(n2);
        
        string dataType = "";
        if((n = str.find( str_atob )) != string::npos) { // binary string
            n += strlen( str_atob );
            if( (n2 = str.rfind("')")) == string::npos ) break;
            str = str.substr(n, n2-n);
            dataType = "binarystring";
            
        } else if((n = str.find( str_baset64_toArrayBuffer )) != string::npos) { // base64 array buffer
            n += strlen( str_baset64_toArrayBuffer );
            if( (n2 = str.rfind("')")) == string::npos ) break;
            str = str.substr(n, n2-n);
            dataType = "arraybuffer";
            
        } else { // other data
            char c = str.at(0);
            if( c == 't' ||  c == 'f' ) {
                dataType = "bool";
            } else if( isdigit(c) ) {
                dataType = "number";
            } else if(c == '"') {
                dataType = "string";
            } else if( c == '[' || c == '{' ) {
                dataType = "json";
            }
        }
        
        Magpie::instance()->callback(callbackId.c_str(), success, statusCode, dataType.c_str(), str.c_str(), keepCallback);
        return true;
        
    } while(0);
    
    return false;
}


bool JsParser_callbackJs(const char * js)
{
	string str = js;
    
    bool ok = false;
    
    if(str.find( str_cordova_nativeEvalAndFetch ) != string::npos) {
        ok = JsParser_nativeEvalAndFetch( str );
        
    } else if(str.find( str_cordova_nativeCallback ) != string::npos) {
        ok = JsParser_nativeCallback( str );
        
    } else if(str.find( str_cordova_callbackFromNative ) != string::npos) {
        ok = JsParser_callbackFromNative( str );
        
    } else {
        ok = JsParser_callbackWithEncodedMsg( str );
    }
    
    return ok;
}



