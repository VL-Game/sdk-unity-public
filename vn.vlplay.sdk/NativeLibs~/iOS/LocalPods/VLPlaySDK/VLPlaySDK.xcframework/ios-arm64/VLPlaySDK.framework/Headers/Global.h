//
//  Global.h
//  VLPlaySDK
//
//   7/25/16.
//  
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "VIDUser.h"
@class VLPlayShopViewController;

NS_ASSUME_NONNULL_BEGIN

// Marks which auth flow last invoked the network — read by the
// handleSignInResponse: chokepoint to fire the right AppsFlyer event
// (sdk_login_completed vs sdk_register_completed vs sdk_third_login_completed).
// Callers set this immediately before triggering the auth network call;
// handleSignInResponse: resets it to Unknown after consuming.
typedef NS_ENUM(NSInteger, VLPlayAuthFlow) {
    VLPlayAuthFlowUnknown   = 0,
    VLPlayAuthFlowLogin,
    VLPlayAuthFlowRegister,
    VLPlayAuthFlowAutoLogin,
    VLPlayAuthFlowSocial,
};

@interface Global : NSObject

+ (Global *)sharedInstance;

/*
 Device token's used to push notification
 */

@property (nonatomic) BOOL isReachable;

@property (nonatomic, copy, getter=getPushDeviceToken) NSString *pushDeviceToken;
@property (nonatomic, copy, getter=getAdvertisingIdentifier) NSString *advertisingIdentifier;

@property (nonatomic, strong) UIColor *globalMainColor;
@property (nonatomic, strong) UIColor *gButtonColor;

@property (nonatomic, copy) NSString *savedUserName;

@property (nonatomic) CGFloat gPortraitNavBarHeight;
@property (nonatomic) CGFloat gLandscapeNavBarHeight;

@property (nonatomic) BOOL flagOpenApp;
@property (nonatomic) BOOL bufferLinkChecked;
@property (nonatomic) BOOL isWaitingForShowLogin;
@property (nonatomic) BOOL isWaitingForShowRegister;

@property (nonatomic) BOOL enableFacebookLogin;
@property (nonatomic) BOOL enableGoogleLogin;
@property (nonatomic) NSString *supportURL;
@property (nonatomic, copy) NSString *hotline;
@property (nonatomic, copy) NSString *emailSupport;
@property (nonatomic, weak) VLPlayShopViewController *shopVC;

@property (nonatomic, readonly) NSString * language;
/*
 UTM distribution URL
 Ex: "democlpt://?utm_source=facebook&utm_campaign=demo&utm_medium=1706"
 
 @"clpt://?utm_source=home&utm_campaign=direct&utm_medium="
 */

@property (nonatomic) NSInteger paymentGoldNumber;
@property (nonatomic, copy) NSString *paymentMessage;
@property (nonatomic, copy) NSString *paymentCurrency;
@property (nonatomic, copy) NSString *currentOrderNo;
@property (nonatomic) BOOL showTakeScreenshot;
@property (nonatomic) BOOL loginIsUpdatePassword;
@property (nonatomic) BOOL needsIdentityVerification;
// YES once the auto-show identity popup has actually appeared for the LIVE session (in the
// VC's -viewDidAppear, auto mode only). Guards re-presenting on the same session after
// dismiss, and lets the cold-launch config re-trigger keep retrying until it truly shows.
// Kept for compatibility: it reads and writes the mark of whatever session is live — the SDK
// itself uses the session-stamped forms below (review 05/10, O10-a).
@property (nonatomic) BOOL identityAutoShownThisSession;
// YES once a guest verified identity in the LIVE session (process-local: a cold launch starts
// with NO). Kept for compatibility like the one above.
@property (nonatomic) BOOL guestVerifiedThisSession;
@property (nonatomic) BOOL antiAddictionEnabled;
@property (nonatomic) NSInteger unReadMailCount;

@property (nonatomic, assign) VLPlayAuthFlow lastAuthFlow;


- (NSString *)getUtmString;
- (void)setUtmString:(NSString *)utmString;

// Cold-launch identity re-trigger: re-evaluate the auto-show identity gate once
// the authoritative /sdk/config is applied (VLPlaySdkConfigManager.fetchConfig),
// in case the post-login gate ran before the config landed. No-op unless a
// non-guest session is active with its profile loaded and the popup hasn't shown
// yet this session.
- (void)retryIdentityGateIfReady;

// Post-identity step of the mandatory gate (IDENTITY-GATE-FLOW-2026-06-23): AA
// notice (once/day) → enter → for guests an optional dismissable upgrade nudge.
// Called by the identity popup once verification finishes (success or already
// verified). Idempotent per session.
- (void)runPostIdentitySequence;

// The identity popup is on screen (NAT-SIGNIN-BEFORE-IDENTITY-GATE): the player decides now,
// so the 10 s sign-in fallback stops counting. Called by the popup when it has appeared.
- (void)holdSignInForPlayer;

// Session-stamped forms (review 02/10): every step that decides a sign-in carries the session
// (VLPlaySessionGeneration) it was started for. A popup, a /detail answer or a fallback timer
// left over from a session that ended can neither release, hold, nor restart the sign-in of
// the session that replaced it, and a stale "Đăng xuất" never signs that one out. The
// unstamped forms above act on whatever session is live when they run.
- (void)runIdentityGateForSession:(NSUInteger)session;
- (void)runPostIdentitySequenceForSession:(NSUInteger)session;
- (void)holdSignInForPlayerSession:(NSUInteger)session;
// The once-per-session marks name the session they were set for (VLPlaySignInGate.h
// VLPlaySessionMarkIsFor): a sign-in that replaces a live session without a sign-out never
// inherits "already shown" or "gate passed" from the session before it (review 05/10, O10-a).
- (void)markIdentityAutoShownForSession:(NSUInteger)session;
- (void)markGuestVerifiedForSession:(NSUInteger)session;

// loginData may be nil: token-refresh session resume (e.g. Apple cold-launch)
// drives the post-sign-in chokepoint without a login response — -loadData:
// early-returns on nil, leaving the cached account intact.
- (void)handleSignInResponse:(nullable id)loginData authenType:(AuthenType)type containerController:(nullable UIViewController *)controller showWelcome:(BOOL)welcome;
- (void)handleSignOut;

@end

NS_ASSUME_NONNULL_END
