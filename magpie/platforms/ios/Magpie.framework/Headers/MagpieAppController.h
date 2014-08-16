
#import <UIKit/UIKit.h>

@interface MagpieAppController : NSObject <UIApplicationDelegate> {
}

// orientation of the game
- (void) setLandscape:(BOOL) landscape;

// get window bounds for GL view
- (CGRect) getWindowBounds;

// add GL view to window
- (void) setMainView:(UIView*) theview;

@end

