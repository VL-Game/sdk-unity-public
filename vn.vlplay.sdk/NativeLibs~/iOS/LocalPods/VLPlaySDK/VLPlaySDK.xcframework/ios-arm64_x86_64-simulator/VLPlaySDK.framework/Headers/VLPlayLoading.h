//
//  VLPlayLoading.h
//  VLPlaySDK
//
//  SDK-owned loading overlay — replaces SVProgressHUD.
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, VLPlayLoadingStyle) {
    VLPlayLoadingStyleLight = 0,  // white card, dark spinner (default)
    VLPlayLoadingStyleDark  = 1,  // dark card, white spinner
};

/// Deterministic, scene-safe loading overlay owned by the SDK.
///
/// Lives on its own retained `UIWindow` bound to the foreground-active
/// `UIWindowScene` (`windowLevel = UIWindowLevelAlert + 1`), so it never has to
/// guess which window to attach to and always sits above every modal — this is
/// what kills the SVProgressHUD corner-square + mask-leak class of bugs.
///
/// All methods are safe to call from any thread (they hop to main) and safe to
/// call before any window exists.
@interface VLPlayLoading : NSObject

/// Ref-counted spinner. The overlay is visible while the show count > 0, so a
/// background call nested inside a foreground flow no longer tears the other's
/// loading down. An unbalanced extra `dismiss` floors at 0 (harmless).
+ (void)show;
+ (void)dismiss;

/// Force the show count to 0 and hide the spinner immediately. Used as a hard
/// boundary (e.g. before presenting the native auth popup) so a new screen
/// never inherits stale loading.
+ (void)dismissAll;

/// YES while the spinner show count > 0.
+ (BOOL)isVisible;

/// Light (default) or dark card. Backs the public +[VLPlaySDKManager setHUDStyle:].
+ (void)setStyle:(VLPlayLoadingStyle)style;

/// Full-screen 0.5-black input blocker — independent of the spinner ref-count.
/// Used by the IAP flow to block input during App Store / receipt round-trips.
+ (void)showFullScreenBlocker;
+ (void)dismissFullScreenBlocker;

@end

NS_ASSUME_NONNULL_END
