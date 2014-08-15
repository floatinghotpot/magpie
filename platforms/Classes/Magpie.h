//
//  Magpie.h
//
//  Created by Raymond Xie on 14-7-31.
//

#ifndef __Magpie_H_
#define __Magpie_H_

#include "cocos2d.h"

USING_NS_CC;

using namespace std;

typedef void (CCObject::*SEL_MagpieEvent)(const char *event, const char *argsJson);
#define magpie_eventselector(_SELECTOR) (CCObject::SEL_MagpieEvent)(&_SELECTOR)

typedef struct {
	char*			 	event;
    CCObject*       	obj;
    SEL_MagpieEvent    func;
} MagpieEventHandler;

typedef std::vector<MagpieEventHandler*>	MagpieEventList;

typedef void (CCObject::*SEL_MagpieCallback)(bool success, int statusCode, const char *dataType, const char *data );
#define magpie_callbackselector(_SELECTOR) (CCObject::SEL_MagpieCallback)(&_SELECTOR)

typedef struct {
	char*                   callbackId;
    CCObject*               obj;
    SEL_MagpieCallback     func;
    time_t					timestamp;
} MagpieCallbackHandler;

// usually return < 1000 ms, may return longer after working in other threads
// will clean up if timeout
#define MAGPIE_TIMEOUT	30 // 30 seconds

typedef std::vector<MagpieCallbackHandler*>	MagpieCallbackList;

class Magpie {

public:
	Magpie();
	virtual ~Magpie();
    
    static Magpie* instance();

	// Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
	virtual bool init() { return true; };

public:
    // ---- for C++ code to call ---
    
	// register listener for events
    void addEventListener(const char * event_name, CCObject* obj, SEL_MagpieEvent func);
    void removeEventListener(const char * event_name, CCObject* obj, SEL_MagpieEvent func);

    // call native code
    void execute(const char *service, const char *action, const char *argsJson, CCObject* obj, SEL_MagpieCallback func);

    // --- for native code to call
    
    // for native code to callback to C++
    virtual bool event(const char *event, const char *argsJson);
    
    // callback of execute()
    virtual bool callback(const char *callbackId, bool success, int statusCode, const char *dataType, const char *data, bool keepCallback);

    // callback as js statement
    virtual bool callbackJs(const char *js);
    
protected:
    MagpieEventList		_listeners;
    MagpieCallbackList		_callbacks;
    
    unsigned long			_magic_counter;

    MagpieEventHandler* createEventHandler(const char *strId, CCObject* obj, SEL_MagpieEvent func);
    void destroyEventHandler( MagpieEventHandler* h );

    MagpieCallbackHandler* createCallbackHandler(const char *strId, CCObject* obj, SEL_MagpieCallback func);
    void destroyCallbackHandler( MagpieCallbackHandler* h );
    
protected:
    bool cordova_fireDocumentEvent(string & str);
    bool cordova_callbackSuccess(string & str);
    bool cordova_callbackError(string & str);
};

#endif /* defined(__Magpie_H_) */
