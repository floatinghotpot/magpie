Magpie = Cocos2d-X + Cordova Plugins
====================================

Framework for Cocos2d-X C++ based game to call hundreds of Cordova/PhoneGap plugins

Taking advantage of both projects:
* Cocos2d-X, the most popular open source game engine. Develop game with C++/javascript/lua, hardware acceleration with OpenGL, excellent performance.
* Cordova, **hundreds of existing plugins** to access native functionalities and 3rd party SDKs.

>Note: it's NOT a plugin for Cordova/PhoneGap, but a plugin framework for Cocos2d-X. It's packed like a plugin, and can be released and managed with plugman CLI.

# Project Status #

* [x] Framework works on Android and iOS.
* [ ] Utility under development.
* [ ] Documentation and example code.

# Architecture #

![Magpie Bridge Architecture](docs/architecture.jpg)

# Example Code #

This is an example to call Cordova AdMob SDK in Cocos2d-X game with C++.

```c
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#define BANNER_ADID 		"ca-app-pub-6869992474017983/9375997553"
#define INTERSTITIAL_ADID	"ca-app-pub-6869992474017983/1657046752"
#elif(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#define BANNER_ADID 		"ca-app-pub-6869992474017983/4806197152"
#define INTERSTITIAL_ADID	"ca-app-pub-6869992474017983/7563979554"
#endif

void GameScene::onReceiveAd(const char *event, const char *argsJson) {
    Magpie::instance()->execute("AdMob", "showAd", "[true]", NULL, NULL);
}

bool GameScene::init() {
    if ( ! BaseScene::init() ) {
        return false;
    }

    // register event handler
    CordovaX::instance()->addEventListener("onReceiveAd", this, (magpie_eventselector) onReceiveAd);

	string args = "";
	args = args + "[{"
		"\"bannerId\":\"" + BANNER_ADID + "\","
		"\"interstitialId\":\"" + INTERSTITIAL_ADID + "\","
		"\"isTesting\":true,"
		"\"autoShow\":true"
		"}]";
	Magpie::instance()->execute("AdMob", "setOptions", args.c_str(), NULL, NULL);

    // create banner
	args = "";
	args = args + "[{\"adId\":\"" + BANNER_ADID + "\"}]";
	Magpie::instance()->execute("AdMob", "createBanner", args.c_str(), NULL, NULL);

    return true;
}

void GameScene::onGameStart() {
	string args = "";
	args = args + "[{\"adId\":\"" + INTERSTITIAL_ADID + "\", \"autoShow\":false}]";
	Magpie::instance()->execute("AdMob", "prepareInterstitial", args.c_str(), NULL, NULL);
}

void GameScene::onGameOver() {
	Magpie::instance()->execute("AdMob", "showInterstitial", "[]", NULL, NULL);
}

```

# Why named "Magpie"? #

It comes from the Chinese legend, Magpie Bridge over the the Milky Way, which bring the longly departed lovers together.

![Magpie Bridge](docs/legend.jpg)

# Credit #

Magpie project is created by Raymond Xie. (floatinghotpot @ github)

