// VLPlayUnityBridge.mm — extern-C bridge between the Unity C# IosBridge and
// VLPlaySDK.xcframework's VLPlaySDKManager (renamed from SDKManager in v1.1.3).
//
// Dual-mode by __has_include:
//   • Pod present  → typed calls into VLPlaySDKManager + a UnityAppController
//     subclass bootstrap (handleApplication) + a VLPlaySDKManagerDelegate that
//     forwards native callbacks to C# via UnitySendMessage.
//   • Pod absent   → the C exports still link (so DllImport resolves) but no-op,
//     so the file compiles even before the iOS post-processor (U1-B-5) injects
//     the local pod.
//
// Native → C# contract (see Runtime/Internal/VLPlayCallbacks.cs):
//   UnitySendMessage("VLPlayCallbacks", "OnNativeEvent",
//                    "{\"evt\":\"signIn\",\"payload\":\"<escaped VLPlayUser json>\"}")
// Engine-specific surface is ONLY UnitySendMessage — the rest is reusable.

#import <Foundation/Foundation.h>
#import <objc/message.h>

#if __has_include(<VLPlaySDK/VLPlaySDKManager.h>)
  #import <VLPlaySDK/VLPlaySDKManager.h>
  #import <VLPlaySDK/AntiAddictionManager.h>   // debug force warn/kick (QA only)
  #import <VLPlaySDK/Global.h>                 // resolved SDK language (read-only)
  #if __has_include(<VLPlaySDK/VLPlayBridge.h>)
    // ≤ native 1.4.1: VLPlaySDKPaymentDidFinishNotification (U3 purchase observer)
    // is declared ONLY here. Native 1.4.3 dropped this header from the public set
    // and moved the extern into VLPlaySDKManager.h — so this import is conditional.
    #import <VLPlaySDK/VLPlayBridge.h>
  #endif
  #if __has_include(<VLPlaySDK/VLPlayDeepLinkRouter.h>)
    // Push deep-links. Public since the push E2E work; imported conditionally only so
    // an older vendored pin still compiles. Using the header's externs rather than the
    // literal notification/key names makes a native rename a LINK error here instead of
    // a runtime no-show.
    #import <VLPlaySDK/VLPlayDeepLinkRouter.h>
    #define VLP_HAS_DEEPLINK 1
  #endif
  #import <VLPlaySDK/VLPlayFAB.h>              // canonical FAB home (manager's FAB methods are deprecated)
  #import <VLPlaySDK/VLPlayAds.h>              // IAA facade (imports VLPlayAdsModels.h)
  #import "UnityAppController.h"   // Unity-generated Xcode project provides this
  #define VLP_HAS_SDK 1
#endif

// Mirrors the C# VLPlayOrientation enum — the ordinal is the bridge's wire value.
// Kept outside VLP_HAS_SDK so the no-op exports still compile without UIKit.
typedef NS_ENUM(NSInteger, VLPUnityOrientation) {
    VLPUnityOrientationFollowSystem = 0,
    VLPUnityOrientationLandscape    = 1,
    VLPUnityOrientationPortrait     = 2,
    // Added alongside Android's ORIENTATION_FOLLOW_APP. On iOS this is the SAME thing as
    // FollowSystem — UIInterfaceOrientationMaskAll, which the SDK header defines as
    // "FOLLOW THE APP": the SDK adopts the host's declared orientations narrowed by the
    // game's root view controller, never unlocking one the game does not support. The two
    // values exist separately only because Android distinguishes them.
    VLPUnityOrientationFollowApp    = 3,
};

#ifdef __cplusplus
extern "C" {
#endif
// Provided by the Unity player runtime.
void UnitySendMessage(const char* obj, const char* method, const char* msg);
#ifdef __cplusplus
}
#endif

#pragma mark - Shared helpers

// Copies an NSString into a malloc'd buffer for the C# string marshaller.
static char* VLPCopyCString(NSString* str) {
    if (str == nil) return NULL;
    const char* utf8 = [str UTF8String];
    if (!utf8) return NULL;
    char* buffer = (char*)malloc(strlen(utf8) + 1);
    strcpy(buffer, utf8);
    return buffer;
}

// Emits {"evt":..,"payload":..} to VLPlayCallbacks.OnNativeEvent. payload is a
// (possibly nested-JSON) string; NSJSONSerialization escapes it correctly.
// Shared, not SDK-gated: a pod-absent build must still be able to answer a request,
// otherwise the game's callback never fires.
static void VLPSend(NSString* evt, NSString* payload) {
    NSDictionary* env = @{ @"evt": evt ?: @"", @"payload": payload ?: @"" };
    NSData* data = [NSJSONSerialization dataWithJSONObject:env options:0 error:nil];
    NSString* json = data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"{}";
    UnitySendMessage("VLPlayCallbacks", "OnNativeEvent", [json UTF8String]);
}

// BE-dictionary readers. Deliberately tolerant the way org.json's optString/optInt are:
// a value of the wrong type, or NSNull, reads as the empty default rather than throwing —
// these mirror Android's parser, and it never throws on a malformed row.
static NSString* VLPDictString(NSDictionary* dict, NSString* key) {
    id v = dict[key];
    if ([v isKindOfClass:[NSString class]]) return (NSString*)v;
    // A number serialized as a bare JSON value still has a sensible string form
    // (org.json's optString does the same coercion).
    if ([v isKindOfClass:[NSNumber class]]) return [(NSNumber*)v stringValue];
    return @"";
}

// org.json's optString(key, fallback) yields the fallback ONLY when the key is absent
// (or JSON null) — a key PRESENT with "" reads as "". Android's parser leans on that
// distinction, so mirroring "empty means fallback" instead would make the two platforms
// disagree on any row the CMS shipped with an empty string.
static BOOL VLPDictHas(NSDictionary* dict, NSString* key) {
    id v = dict[key];
    return v != nil && v != (id)[NSNull null];
}

static NSInteger VLPDictInt(NSDictionary* dict, NSString* key) {
    id v = dict[key];
    return [v respondsToSelector:@selector(integerValue)] ? [v integerValue] : 0;
}

static long long VLPDictLong(NSDictionary* dict, NSString* key) {
    id v = dict[key];
    return [v respondsToSelector:@selector(longLongValue)] ? [v longLongValue] : 0LL;
}

// Account-op reply (U5) — one shape for bind/upgrade/deactivate, routed by
// requestId ("accountResult" in VLPlayCallbacks). Outside the SDK gate: a pod-absent
// build must still answer or the game's callback hangs forever.
static void VLPSendAccountReply(int requestId, BOOL ok, int errorCode,
                                NSString* errorMessage, NSString* dataJson) {
    NSDictionary* reply = @{
        @"requestId":    @(requestId),
        @"ok":           @(ok),
        @"errorCode":    @(errorCode),
        @"errorMessage": errorMessage ?: @"",
        @"dataJson":     dataJson ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:reply options:0 error:nil];
    NSString* json = data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"{}";
    VLPSend(@"accountResult", json);
}

// The SDK puts a human-readable reason in userInfo[@"message"]; fall back to
// localizedDescription so the game never gets an empty failure.
// Ads backend bridge replies (native ≥ 1.9.0) — routed by requestId ("adsRewardTicket" /
// "adsRewardStatus" in VLPlayCallbacks); the C# AdsTicketReply / AdsStatusReply mirror these
// keys exactly. Outside the SDK gate: a pod-absent build must still answer, or the game's
// callback never fires.
static void VLPSendAdsTicketReply(int requestId, BOOL ok, NSString* placementId, NSString* nonce,
                                  NSString* signature, long long expiresAtMs, NSString* error) {
    NSDictionary* dict = @{
        @"requestId":   @(requestId),
        @"ok":          @(ok),
        @"placementId": placementId ?: @"",
        @"nonce":       nonce ?: @"",
        @"signature":   signature ?: @"",
        @"expiresAtMs": @(expiresAtMs),
        @"error":       error ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:0 error:nil];
    VLPSend(@"adsRewardTicket", data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"{}");
}

static void VLPSendAdsStatusReply(int requestId, NSString* nonce, BOOL confirmed, double creditedAmount,
                                  NSString* rewardCurrency, NSString* reason) {
    NSDictionary* dict = @{
        @"requestId":      @(requestId),
        @"nonce":          nonce ?: @"",
        @"confirmed":      @(confirmed),
        @"creditedAmount": @(creditedAmount),
        @"rewardCurrency": rewardCurrency ?: @"",
        @"reason":         reason ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:0 error:nil];
    VLPSend(@"adsRewardStatus", data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"{}");
}

static NSString* VLPErrorMessage(NSError* error) {
    if (!error) return @"";
    id msg = error.userInfo[@"message"];
    if ([msg isKindOfClass:[NSString class]] && ((NSString*)msg).length > 0) return (NSString*)msg;
    return error.localizedDescription ?: @"";
}

#pragma mark - Push deep-links

// The SDK owns UNUserNotificationCenter's delegate, parses the tapped payload and
// broadcasts VLPlayDeepLinkNotification — a broadcast, so observing it takes nothing
// away from an Obj-C host that also observes. What it does NOT do is hold the link:
// an NSNotification with no observer goes nowhere, and a tap on a dead app is handled
// during launch, before Unity's managed side is up. So this buffers until C# opens the
// channel from VLPlaySDK.Init().
static NSString* sPendingDeepLinkJSON = nil;
static BOOL sDeepLinkChannelOpen = NO;

#if VLP_HAS_DEEPLINK
  #define VLP_DL_ACTION_KEY VLPlayDeepLinkActionKey
  #define VLP_DL_PATH_KEY   VLPlayDeepLinkPathKey
  #define VLP_DL_PARAMS_KEY VLPlayDeepLinkParamsKey
  #define VLP_DL_RAWURL_KEY VLPlayDeepLinkRawURLKey
#else
  // Only reached pod-absent — i.e. never on a device, and in the host harness that
  // runs the builder below. Pod-present the header's externs are used instead, which
  // is what turns a native rename into a link error rather than a silent no-show.
  #define VLP_DL_ACTION_KEY @"action"
  #define VLP_DL_PATH_KEY   @"path"
  #define VLP_DL_PARAMS_KEY @"params"
  #define VLP_DL_RAWURL_KEY @"rawURL"
#endif

// Pure: one VLPlayDeepLinkNotification userInfo → the flat payload C# parses.
// Foundation only and outside the SDK gate so it can be executed in the harness —
// compiling a mapping proves nothing about what it maps to.
// Returns nil when there is no URL, since a link without one cannot be routed.
static NSString* VLPBuildDeepLinkPayload(NSDictionary* userInfo) {
    if (![userInfo isKindOfClass:[NSDictionary class]]) return nil;

    id rawURL = userInfo[VLP_DL_RAWURL_KEY];
    if (![rawURL isKindOfClass:[NSString class]] || ((NSString*)rawURL).length == 0) return nil;

    id action = userInfo[VLP_DL_ACTION_KEY];
    id path   = userInfo[VLP_DL_PATH_KEY];
    id params = userInfo[VLP_DL_PARAMS_KEY];

    NSMutableArray<NSString*>* keys = [NSMutableArray array];
    NSMutableArray<NSString*>* values = [NSMutableArray array];
    if ([params isKindOfClass:[NSDictionary class]]) {
        // Sorted so one payload has one representation — the parallel arrays are
        // index-aligned, and a stable order is what makes them diffable in a harness.
        NSArray* sorted = [[(NSDictionary*)params allKeys]
            sortedArrayUsingSelector:@selector(compare:)];
        for (id k in sorted) {
            if (![k isKindOfClass:[NSString class]]) continue;
            id v = ((NSDictionary*)params)[k];
            NSString* sv = [v isKindOfClass:[NSString class]] ? (NSString*)v
                         : ([v isKindOfClass:[NSNumber class]] ? [(NSNumber*)v stringValue] : @"");
            [keys addObject:(NSString*)k];
            [values addObject:sv];
        }
    }

    NSDictionary* payload = @{
        @"action":      [action isKindOfClass:[NSString class]] ? action : @"",
        @"path":        [path   isKindOfClass:[NSString class]] ? path   : @"",
        @"rawUrl":      rawURL,
        @"paramKeys":   keys,
        @"paramValues": values,
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:payload options:0 error:nil];
    return data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : nil;
}

static void VLPEmitDeepLink(NSString* payload) {
    if (payload.length == 0) return;
    if (!sDeepLinkChannelOpen) {
        // Newest wins: one tap per launch is the real case, and replaying a stale
        // route after the fact is worse than dropping it.
        sPendingDeepLinkJSON = payload;
        return;
    }
    VLPSend(@"deepLink", payload);
}

#pragma mark - Environment (VLPlaySettings.useStaging)

// VLPlaySettings.useStaging reaches the build as DIRECT_VERSION in VLPlaySDK-Info.plist
// (false = staging, true = production: the post-processor writes !useStaging). The native SDK
// does NOT read that key to pick a gateway. +defaultManager starts every build on production
// (isSandbox = NO) and DIRECT_VERSION only fills a legacy "direct" request field, so the host
// must set isSandbox itself, before +handleApplication: — and a Unity game has no AppDelegate
// code to do it in. Up to 1.5.1 nothing did: every Unity iOS build talked to production
// whatever useStaging said, while the Android bridge honoured it. A game that exists only on
// staging got 403 "Permission denied" on every sign-in route (the backend finds no game for the
// clientId), and /sdk/config still answered 200 with the server defaults, so nothing looked
// misconfigured.
//
// Returns the sandbox flag DIRECT_VERSION asks for. *known is NO when the plist carries no
// usable value; the caller then leaves the SDK's own default alone. Pure and Foundation-only,
// and outside VLP_HAS_SDK so tools/csharp-harness~/ios-bridge-tests runs it without the pod.
__attribute__((unused))
static BOOL VLPSandboxFromSdkInfo(NSDictionary* info, BOOL* known) {
    id direct = [info isKindOfClass:[NSDictionary class]] ? info[@"DIRECT_VERSION"] : nil;
    // The post-processor writes a real <true/> / <false/>. A hand-edited plist may carry the
    // string; an empty one, or anything else, is not a choice the build made.
    BOOL usable = [direct isKindOfClass:[NSNumber class]]
               || ([direct isKindOfClass:[NSString class]] && [(NSString*)direct length] > 0);
    if (known) *known = usable;
    return usable ? ![direct boolValue] : NO;
}

#if VLP_HAS_SDK

// Resolve VIDUser at runtime — it is NOT a public umbrella header, so we read
// its properties through KVC / the +currentUser class method instead of import.
static id VLPCurrentUser(void) {
    Class cls = NSClassFromString(@"VIDUser");
    if (!cls) return nil;
    id (*msg)(id, SEL) = (id (*)(id, SEL))objc_msgSend;
    return msg((id)cls, sel_registerName("currentUser"));
}

static NSString* VLPUserString(id user, NSString* key) {
    if (!user) return @"";
    id v = [user valueForKey:key];
    return [v isKindOfClass:[NSString class]] ? (NSString*)v : @"";
}

// KVC scalar readers — used for AntiAddictionStatus, read dynamically so the file
// needs no import of AntiAddictionStatus.h (not in the public umbrella).
static NSInteger VLPStatusInt(id obj, NSString* key) {
    if (!obj) return 0;
    id v = [obj valueForKey:key];
    return [v respondsToSelector:@selector(integerValue)] ? [v integerValue] : 0;
}

static BOOL VLPStatusBool(id obj, NSString* key) {
    if (!obj) return NO;
    id v = [obj valueForKey:key];
    return [v respondsToSelector:@selector(boolValue)] ? [v boolValue] : NO;
}

// Serializes a VIDUser into the VLPlayUser JSON shape C# expects.
static NSString* VLPUserJSON(id user) {
    if (!user) return @"";
    NSDictionary* dict = @{
        @"accountId": VLPUserString(user, @"userId"),
        @"username":  VLPUserString(user, @"userName"),
        @"email":     VLPUserString(user, @"email"),
        @"phone":     VLPUserString(user, @"mobile"),
        @"isGuest":   @([VLPlaySDKManager isCurrentUserGuest]),
    };
    NSError* err = nil;
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:0 error:&err];
    if (!data) return @"";
    return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

// iOS has no single orientation setter: the SDK exposes two
// UIInterfaceOrientationMask properties. MaskAll is the SDK's own default and means
// "don't constrain — defer to the host Info.plist", which is our FollowSystem.
static UIInterfaceOrientationMask VLPMaskForOrientation(int orientation) {
    switch (orientation) {
        case VLPUnityOrientationLandscape: return UIInterfaceOrientationMaskLandscape;
        case VLPUnityOrientationPortrait:  return UIInterfaceOrientationMaskPortrait;
        // FollowSystem and FollowApp both land here: MaskAll is the SDK's documented
        // "follow the app" default, and iOS has no separate notion for the two.
        default:                           return UIInterfaceOrientationMaskAll;
    }
}

static int VLPOrientationForMask(UIInterfaceOrientationMask mask) {
    if (mask == UIInterfaceOrientationMaskLandscape) return VLPUnityOrientationLandscape;
    if (mask == UIInterfaceOrientationMaskPortrait)  return VLPUnityOrientationPortrait;
    // MaskAll reads back as FollowApp — the accurate name for what the SDK does with it,
    // and it keeps GetOrientation() reporting the same value as Android for the default.
    // Consequence, documented on the facade: SetOrientation(FollowSystem) then
    // GetOrientation() returns FollowApp on iOS. The getter reports real behaviour rather
    // than echoing the setter, because on this platform the two requests are one thing.
    return VLPUnityOrientationFollowApp;
}

#pragma mark - SDK delegate → C# events

@interface VLPlayUnityDelegate : NSObject <VLPlaySDKManagerDelegate>
@end

@implementation VLPlayUnityDelegate
// @required
- (void)sdkManagerDidSignInWithUser:(VIDUser *)user { VLPSend(@"signIn", VLPUserJSON(user)); }
- (void)sdkManagerDidSignOut { VLPSend(@"signOut", @""); }
// Fires for ANY successful purchase — IAP and webpay checkouts inside the SDK's own UI
// (FAB shop). No payload: the delegate method carries none.
- (void)sdkManagerDidPurchaseSuccessfully { VLPSend(@"purchaseCompleted", @""); }
// @optional
- (void)sdkManagerDidSessionExpire { VLPSend(@"sessionExpired", @""); }

// Anti-addiction (native v1.1.4+). Fired from the SDK's status chokepoint, not from
// its popup code — the game is told to stop even when no VC is available to present.
- (void)sdkManagerAntiAddictionWarn:(NSString *)warningMessage {
    VLPSend(@"aaWarn", warningMessage ?: @"");
}

- (void)sdkManagerAntiAddictionKick:(NSString *)kickReason
                      cooldownUntil:(NSString *)cooldownUntil {
    // Both values pass through verbatim — the cooldown is server-authoritative.
    NSDictionary* kick = @{
        @"reason":        kickReason ?: @"",
        @"cooldownUntil": cooldownUntil ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:kick options:0 error:nil];
    NSString* json = data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"{}";
    VLPSend(@"aaKick", json);
}

// User-facing notices (native v1.3.3+). The SDK stopped drawing its own top
// notification bar, so every validation message / BE error / operation result
// arrives here — if the game ignores VLPlayEvents.OnNotice the player sees
// nothing at all. Both fire on the main queue.
// Payload keys must stay in step with VLPlayCallbacks.Notice on the C# side.
- (void)sdkManagerDidEmitNotice:(NSString *)message sticky:(BOOL)sticky {
    if (message.length == 0) return;
    NSDictionary* notice = @{
        @"message": message,
        @"sticky":  @(sticky),
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:notice options:0 error:nil];
    NSString* json = data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"{}";
    VLPSend(@"notice", json);
}

- (void)sdkManagerDidClearNotice {
    VLPSend(@"noticeCleared", @"");
}
// sdkManagerSupportTicketCreated: / ProfileUpdated: / IdentityVerified have no
// C# event surface in v1 — intentionally not implemented.
@end

static VLPlayUnityDelegate* VLPDelegate(void) {
    static VLPlayUnityDelegate* delegate = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ delegate = [VLPlayUnityDelegate new]; });
    return delegate;
}

static void VLPInstallDelegate(void) {
    [VLPlaySDKManager defaultManager].delegate = VLPDelegate();
}

// Registered from the app controller, i.e. before the OS can deliver a notification
// response to the SDK's UNUserNotificationCenter delegate. Idempotent — the app
// controller runs once, but a second call must not double-deliver every link.
static void VLPObserveDeepLinks(void) {
#if VLP_HAS_DEEPLINK
    static BOOL observing = NO;
    if (observing) return;
    observing = YES;
    [[NSNotificationCenter defaultCenter]
        addObserverForName:VLPlayDeepLinkNotification
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification* note) {
        VLPEmitDeepLink(VLPBuildDeepLinkPayload(note.userInfo));
    }];
#endif
}

// Reads the same file the SDK's VLPlayConfig reads (main bundle, VLPlaySDK-Info.plist).
static void VLPApplyEnvironment(void) {
    NSString* path = [[NSBundle mainBundle] pathForResource:@"VLPlaySDK-Info" ofType:@"plist"];
    NSDictionary* info = path ? [NSDictionary dictionaryWithContentsOfFile:path] : nil;
    BOOL known = NO;
    BOOL sandbox = VLPSandboxFromSdkInfo(info, &known);
    if (known) [VLPlaySDKManager defaultManager].isSandbox = sandbox;
    // NSLog, not the SDK's log: Release strips that one, and which gateway a build talks to is
    // the first thing to know when sign-in answers 403.
    NSLog(@"[VLPlay] gateway: %@ (VLPlaySDK-Info.plist DIRECT_VERSION %@)",
          [VLPlaySDKManager defaultManager].isSandbox ? @"staging" : @"production",
          known ? (sandbox ? @"= false" : @"= true") : @"absent, SDK default kept");
}

#pragma mark - App-controller bootstrap

// Subclass Unity's app controller so the SDK boots at launch. The environment is
// applied first, from DIRECT_VERSION (see VLPSandboxFromSdkInfo): the SDK does not
// read that key itself, and nothing may ask for a gateway before it is set.
@interface VLPlayUnityAppController : UnityAppController
@end

@implementation VLPlayUnityAppController

- (BOOL)application:(UIApplication *)application
        didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    VLPApplyEnvironment();
    BOOL result = [super application:application didFinishLaunchingWithOptions:launchOptions];
    // Install the delegate BEFORE handleApplication so a cached auto-login
    // sign-in fired during bootstrap is captured.
    VLPInstallDelegate();
    VLPObserveDeepLinks();
    [VLPlaySDKManager handleApplication:application didFinishLaunchingWithOptions:launchOptions];
    return result;
}

- (BOOL)application:(UIApplication *)application
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
    // SDK first (Facebook / Google OAuth callbacks), then Unity's chain.
    if ([VLPlaySDKManager handleApplication:application openURL:url options:options]) {
        return YES;
    }
    return [super application:application openURL:url options:options];
}

// A OneLink tap on an installed game arrives as an NSUserActivity, not a URL — openURL above
// never sees it. Without this override AppsFlyer never learns of the tap: no route, and no click
// attribution. The native SDK holds it if AppsFlyer has not started yet (first launch: the dev key
// comes with /sdk/config). Unity's own implementation still runs after, so
// Application.deepLinkActivated keeps firing for games that listen to it.
- (BOOL)application:(UIApplication *)application
        continueUserActivity:(NSUserActivity *)userActivity
          restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> * _Nullable))restorationHandler {
    BOOL handled = [VLPlaySDKManager handleApplication:application
                                  continueUserActivity:userActivity
                                    restorationHandler:restorationHandler];
    // Every Unity from the 2021.3 floor up implements it (it feeds Application.deepLinkActivated);
    // asked anyway, since calling an unimplemented optional delegate method on super would crash.
    BOOL unity = NO;
    if ([UnityAppController instancesRespondToSelector:_cmd]) {
        unity = [super application:application continueUserActivity:userActivity restorationHandler:restorationHandler];
    }
    return handled || unity;
}

@end

IMPL_APP_CONTROLLER_SUBCLASS(VLPlayUnityAppController)

#endif // VLP_HAS_SDK

#pragma mark - C exports (both modes)

#ifdef __cplusplus
extern "C" {
#endif

void VLPlayUnity_Init(void) {
#if VLP_HAS_SDK
    VLPInstallDelegate();
    VLPObserveDeepLinks();  // idempotent; covers a host that does not use our app controller
#endif
}

// Called once C# can receive: UnitySendMessage silently drops on a target that does
// not exist yet, so anything the SDK routed before this point was parked rather than
// sent. Not gated on VLP_HAS_SDK — pod-absent it simply finds nothing parked.
void VLPlayUnity_OpenDeepLinkChannel(void) {
    sDeepLinkChannelOpen = YES;
    NSString* pending = sPendingDeepLinkJSON;
    sPendingDeepLinkJSON = nil;
    if (pending.length > 0) VLPSend(@"deepLink", pending);
}

void VLPlayUnity_SignIn(void) {
#if VLP_HAS_SDK
    [[VLPlaySDKManager defaultManager] signIn];
#else
    NSLog(@"[VLPlayUnityBridge] SignIn: VLPlaySDK not linked — wire the local pod (U1-B-5).");
#endif
}

void VLPlayUnity_SignOut(void) {
#if VLP_HAS_SDK
    [[VLPlaySDKManager defaultManager] signOut];
#endif
}

bool VLPlayUnity_IsSignedIn(void) {
#if VLP_HAS_SDK
    id user = VLPCurrentUser();
    if (!user) return false;
    BOOL signedIn = [[user valueForKey:@"signedIn"] boolValue];
    NSString* token = [user valueForKey:@"accessToken"];
    return signedIn && token.length > 0;
#else
    return false;
#endif
}

char* VLPlayUnity_GetCurrentUserJson(void) {
#if VLP_HAS_SDK
    id user = VLPCurrentUser();
    if (!user) return NULL;
    if (![[user valueForKey:@"signedIn"] boolValue]) return NULL;
    return VLPCopyCString(VLPUserJSON(user));
#else
    return NULL;
#endif
}

void VLPlayUnity_LogEvent(const char* eventName, const char* paramsJson) {
#if VLP_HAS_SDK
    if (!eventName) return;
    NSString* name = [NSString stringWithUTF8String:eventName];
    NSDictionary* values = nil;
    if (paramsJson && strlen(paramsJson) > 0) {
        NSData* data = [[NSString stringWithUTF8String:paramsJson] dataUsingEncoding:NSUTF8StringEncoding];
        id parsed = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
        if ([parsed isKindOfClass:[NSDictionary class]]) values = (NSDictionary*)parsed;
    }
    [[VLPlaySDKManager defaultManager] logEvent:name withValues:values];
#else
    (void)eventName; (void)paramsJson;
#endif
}

void VLPlayUnity_SetVerboseLogging(bool enabled) {
#if VLP_HAS_SDK
    [VLPlaySDKManager setVerboseLogging:enabled];
#else
    (void)enabled;
#endif
}

// Debug/QA only: inject a mock anti-addiction status so the warn/kick path fires without a
// minor's account and a tightened CMS cap. Goes through the SDK's own debug helpers, so the
// whole real chain runs (status → delegate → popup → host callback).
void VLPlayUnity_DebugForceAntiAddiction(bool kick) {
#if VLP_HAS_SDK
    AntiAddictionManager* aa = [AntiAddictionManager shared];
    if (kick) [aa debugForceKickPopup];
    else      [aa debugForceWarnPopup];
#else
    (void)kick;
#endif
}

// Latest server anti-addiction status, serialized to the VLPlayAntiAddictionStatus JSON
// shape C# expects. Read via KVC so no AntiAddictionStatus.h import is needed. Returns
// NULL when the SDK has not fetched a status yet (lastStatus nil). iOS status carries no
// accountId / isRestricted — sent as ""/false (Android-only fields).
char* VLPlayUnity_GetAntiAddictionStatusJson(void) {
#if VLP_HAS_SDK
    id status = [[AntiAddictionManager shared] valueForKey:@"lastStatus"];
    if (!status || status == (id)[NSNull null]) return NULL;
    NSDictionary* dict = @{
        @"accountId":               @"",
        @"ageGroup":                VLPUserString(status, @"ageGroup"),
        @"totalPlayedMinutesToday": @(VLPStatusInt(status, @"totalPlayedTodayMinutes")),
        @"currentSessionMinutes":   @(VLPStatusInt(status, @"currentSessionMinutes")),
        @"remainingMinutesToday":   @(VLPStatusInt(status, @"remainingMinutesToday")),
        @"remainingSessionMinutes": @(VLPStatusInt(status, @"remainingSessionMinutes")),
        @"isRestricted":            @NO,
        @"isCurfew":                @(VLPStatusBool(status, @"curfewActive")),
        @"nextCurfewStart":         VLPUserString(status, @"nextCurfewStart"),
        @"shouldWarn":              @(VLPStatusBool(status, @"shouldWarn")),
        @"shouldKick":              @(VLPStatusBool(status, @"shouldKick")),
        @"kickReason":              VLPUserString(status, @"kickReason"),
        @"warningMessage":          VLPUserString(status, @"warningMessage"),
        @"cooldownUntil":           VLPUserString(status, @"cooldownUntil"),
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:dict options:0 error:nil];
    if (!data) return NULL;
    return VLPCopyCString([[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]);
#else
    return NULL;
#endif
}

#pragma mark - Passthrough (language / orientation / config)

// Runtime language override (DECISION-5 layer 1). In-memory only on iOS — unlike
// Android, which persists it; the C# facade documents that drift.
void VLPlayUnity_SetLanguage(const char* code) {
#if VLP_HAS_SDK
    if (!code) return;
    [VLPlaySDKManager setLanguage:[NSString stringWithUTF8String:code]];
#else
    (void)code;
#endif
}

// Resolved 3-letter code (override → VLPlaySDK-Info.plist LANGUAGE → device locale → "vie").
char* VLPlayUnity_GetLanguage(void) {
#if VLP_HAS_SDK
    NSString* lang = [Global sharedInstance].language;
    return VLPCopyCString(lang.length > 0 ? lang : @"vie");
#else
    return NULL;
#endif
}

void VLPlayUnity_SetOrientation(int orientation) {
#if VLP_HAS_SDK
    UIInterfaceOrientationMask mask = VLPMaskForOrientation(orientation);
    VLPlaySDKManager* mgr = [VLPlaySDKManager defaultManager];
    mgr.loginViewOrientationMask = mask;
    mgr.paymentViewOrientationMask = mask;
#else
    (void)orientation;
#endif
}

int VLPlayUnity_GetOrientation(void) {
#if VLP_HAS_SDK
    // Both masks are always set together through this bridge, so the login one answers.
    return VLPOrientationForMask([VLPlaySDKManager defaultManager].loginViewOrientationMask);
#else
    return VLPUnityOrientationFollowSystem;
#endif
}

bool VLPlayUnity_IsFeatureEnabled(const char* featureName) {
#if VLP_HAS_SDK
    if (!featureName) return false;
    return [VLPlaySDKManager isFeatureEnabled:[NSString stringWithUTF8String:featureName]];
#else
    (void)featureName;
    return false;
#endif
}

bool VLPlayUnity_IsConfigReady(void) {
#if VLP_HAS_SDK
    return [VLPlaySDKManager isConfigReady];
#else
    return false;
#endif
}

// The CMS config as JSON. +currentConfig returns the SDK's re-serialized dict (the
// /sdk/config wire shape), so the key set is whatever iOS parsed — not necessarily
// byte-identical to Android's toJson(). isValidJSONObject guards against a stray
// NSNull/non-JSON value making dataWithJSONObject throw.
char* VLPlayUnity_GetConfigJson(void) {
#if VLP_HAS_SDK
    NSDictionary* cfg = [VLPlaySDKManager currentConfig];
    if (!cfg || ![NSJSONSerialization isValidJSONObject:cfg]) return NULL;
    NSData* data = [NSJSONSerialization dataWithJSONObject:cfg options:0 error:nil];
    if (!data) return NULL;
    return VLPCopyCString([[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding]);
#else
    return NULL;
#endif
}

#pragma mark - IAP: product catalog

// -getProductCatalog: hands back the BE's `data` array VERBATIM — raw dictionaries, no
// parsing. Android instead returns StorePackage POJOs whose parser already applied the
// fallbacks below. So the normalization has to be replicated HERE, or the same CMS row
// would reach the game as two different objects depending on the platform.
//
// Mirrors sdk.vlplay.vn.payment.StorePackage.from (vlgame-sdk-android @ 82cddd4).
// Keep the two in step: if that parser gains a rule, this needs it too.
static NSDictionary* VLPNormalizePackage(id rowRaw) {
    if (![rowRaw isKindOfClass:[NSDictionary class]]) return nil;
    NSDictionary* row = (NSDictionary*)rowRaw;

    // storeProductId ← storeProductId, else the legacy productId. A row with neither
    // cannot be bought (nothing to hand the store), so Android drops it — drop it too.
    NSString* storeProductId = VLPDictString(row, @"storeProductId");
    if (storeProductId.length == 0) storeProductId = VLPDictString(row, @"productId");
    if (storeProductId.length == 0) return nil;

    // name ← name, else the legacy productName; the id is the default only when
    // productName is ABSENT — a row carrying productName:"" keeps the empty name
    // (optString returns "", never the default, for a present key).
    NSString* name = VLPDictString(row, @"name");
    if (name.length == 0) {
        name = VLPDictHas(row, @"productName") ? VLPDictString(row, @"productName") : storeProductId;
    }

    // Same absent-vs-empty rule for these two defaults.
    NSString* packageId = VLPDictHas(row, @"packageId") ? VLPDictString(row, @"packageId") : storeProductId;
    NSString* currency  = VLPDictHas(row, @"currency")  ? VLPDictString(row, @"currency")  : @"VND";

    // optBoolean accepts JSON booleans and true/false strings only. Malformed
    // values (including numbers and empty strings) keep Android's default true.
    id activeRaw = row[@"isActive"];
    BOOL isActive = YES;
    if ([activeRaw isKindOfClass:[NSNumber class]]
        && CFGetTypeID((__bridge CFTypeRef)activeRaw) == CFBooleanGetTypeID()) {
        isActive = [activeRaw boolValue];
    } else if ([activeRaw isKindOfClass:[NSString class]]
               && [activeRaw caseInsensitiveCompare:@"false"] == NSOrderedSame) {
        isActive = NO;
    }

    NSMutableArray* items = [NSMutableArray array];
    id itemsRaw = row[@"items"];
    if ([itemsRaw isKindOfClass:[NSArray class]]) {
        for (id entry in (NSArray*)itemsRaw) {
            if (![entry isKindOfClass:[NSDictionary class]]) continue;
            NSDictionary* it = (NSDictionary*)entry;
            NSString* itemId = VLPDictString(it, @"itemId");
            NSString* itemName = VLPDictString(it, @"itemName");
            NSInteger quantity = VLPDictInt(it, @"quantity");
            // Android drops a wholly-empty item rather than emitting a blank row.
            if (itemId.length == 0 && itemName.length == 0 && quantity == 0) continue;
            [items addObject:@{ @"itemId": itemId, @"itemName": itemName, @"quantity": @(quantity) }];
        }
    }

    return @{
        @"packageId":      packageId,
        @"storeProductId": storeProductId,
        @"name":           name,
        @"description":    VLPDictString(row, @"description"),
        @"price":          @(VLPDictLong(row, @"price")),
        @"currency":       currency,
        @"isActive":       @(isActive),
        @"sortOrder":      @(VLPDictInt(row, @"sortOrder")),
        @"items":          items,
    };
}

static void VLPSendCatalogReply(int requestId, NSArray* packages, int errorCode, NSString* errorMessage) {
    NSDictionary* reply = @{
        @"requestId":    @(requestId),
        @"ok":           @(packages != nil),
        @"packages":     packages ?: @[],
        @"errorCode":    @(errorCode),
        @"errorMessage": errorMessage ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:reply options:0 error:nil];
    NSString* json = data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"";
    VLPSend(@"productCatalog", json);
}

#pragma mark - IAP: purchase

// Mirrors the C# VLPlayPurchaseOutcome enum — ordinals are the bridge's wire value.
enum { VLPOutcomeSuccess = 0, VLPOutcomeCancelled = 1, VLPOutcomeFailed = 2 };

// Pure decision: maps one VLPlaySDKPaymentDidFinishNotification userInfo to a reply
// fragment, or nil when the event belongs to a DIFFERENT product (a cold-launch replay
// of another queued transaction must not resolve this request). Native 1.9.4 names
// the product on every terminal event; an unattributed event belongs to no request.
// Kept outside the SDK gate (Foundation-only) so the host-side harness can execute it.
static inline NSDictionary* VLPPurchaseReplyForEvent(NSDictionary* info, NSString* pid) {
    NSString* eventPid = VLPDictString(info, @"productId");
    if (eventPid.length == 0 || ![eventPid isEqualToString:pid]) return nil;

    NSString* status = VLPDictString(info, @"status");
    if ([status isEqualToString:@"success"]) {
        return @{ @"outcome":         @(VLPOutcomeSuccess),
                  @"transactionId":   VLPDictString(info, @"transactionId"),
                  @"deliveryPending": @([info[@"deliveryPending"] boolValue]),
                  @"errorMessage":    @"" };
    }
    // The SDK posts status="cancelled" when SKErrorPaymentCancelled ends the transaction
    // (native e293e19, in every release from v1.3.0 on) — a dismissed sheet is not a failure.
    if ([status isEqualToString:@"cancelled"]) {
        return @{ @"outcome": @(VLPOutcomeCancelled), @"transactionId": @"",
                  @"deliveryPending": @NO, @"errorMessage": @"" };
    }
    NSString* msg = VLPDictString(info, @"error");
    if (msg.length == 0) msg = @"Purchase failed.";
    return @{ @"outcome": @(VLPOutcomeFailed), @"transactionId": @"",
              @"deliveryPending": @NO, @"errorMessage": msg };
}

static void VLPSendPurchaseReply(int requestId, int outcome, NSString* transactionId,
                                 NSString* productId, BOOL deliveryPending,
                                 int errorCode, NSString* errorMessage) {
    NSDictionary* reply = @{
        @"requestId":       @(requestId),
        @"outcome":         @(outcome),
        @"transactionId":   transactionId ?: @"",
        @"productId":       productId ?: @"",
        @"deliveryPending": @(deliveryPending),
        @"errorCode":       @(errorCode),
        @"errorMessage":    errorMessage ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:reply options:0 error:nil];
    NSString* json = data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"";
    VLPSend(@"purchaseResult", json);
}

#if VLP_HAS_SDK
// The SDK's session-expired relogin alert presents on this controller. Walk to the
// topmost presented VC so the alert isn't swallowed by an already-presenting root.
static UIViewController* VLPTopViewController(void) {
    UIWindow* keyWindow = nil;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if (scene.activationState != UISceneActivationStateForegroundActive) continue;
        if (![scene isKindOfClass:[UIWindowScene class]]) continue;
        for (UIWindow* w in ((UIWindowScene*)scene).windows) {
            if (w.isKeyWindow) { keyWindow = w; break; }
        }
        if (keyWindow) break;
    }
    if (!keyWindow) keyWindow = UIApplication.sharedApplication.delegate.window;
    UIViewController* top = keyWindow.rootViewController;
    while (top.presentedViewController) top = top.presentedViewController;
    return top;
}
#endif

// The per-call native completion owns the outcome. Its shared notification supplies
// transactionId and cancellation, but cannot settle a call: a refused same-SKU retry
// can observe the original sale's event. Completion-only failures (busy, session ended,
// missing delivery target) must also reply and remove the observer.
void VLPlayUnity_Purchase(int requestId, const char* productId, const char* productName,
                          long long amountVND, bool isConsumable, const char* extraData,
                          const char* roleId, const char* serverGameId) {
#if VLP_HAS_SDK
    if (!productId || strlen(productId) == 0) {
        VLPSendPurchaseReply(requestId, VLPOutcomeFailed, nil, nil, NO,
                             -3 /* VLPlaySDK.ErrInvalidArgument */, @"storeProductId is required.");
        return;
    }
    NSString* pid = [NSString stringWithUTF8String:productId];
    __block NSDictionary* eventReply = nil;
    __block BOOL finished = NO;

    __block id observer = [[NSNotificationCenter defaultCenter]
        addObserverForName:VLPlaySDKPaymentDidFinishNotification
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification* note) {
            if (finished) return;
            NSDictionary* r = VLPPurchaseReplyForEvent(note.userInfo ?: @{}, pid);
            if (!r) return;
#if !__has_feature(objc_arc)
            [eventReply release];
#endif
            eventReply = [r copy];
        }];

    [[VLPlaySDKManager defaultManager]
        purchasePackageWithProductId:pid
                         productName:(productName ? [NSString stringWithUTF8String:productName] : @"")
                              amount:(NSInteger)amountVND
                        isConsumable:(BOOL)isConsumable
                           extraData:(extraData ? [NSString stringWithUTF8String:extraData] : @"")
                              roleID:(roleId ? [NSString stringWithUTF8String:roleId] : @"")
                        serverGameID:(serverGameId ? [NSString stringWithUTF8String:serverGameId] : @"")
                    parentController:VLPTopViewController()
                  purchaseCompletion:^(BOOL status, BOOL deliveryPending, NSError* _Nullable error) {
                      // Notification observer order is unspecified. Wait one main-queue turn
                      // so both observers have seen the event before reading its metadata.
                      dispatch_async(dispatch_get_main_queue(), ^{
                          if (finished) return;
                          finished = YES;
                          [[NSNotificationCenter defaultCenter] removeObserver:observer];
                          observer = nil;
                          BOOL cancelled = !status
                              && [VLPDictString(error.userInfo, @"productId") isEqualToString:pid]
                              && [eventReply[@"outcome"] intValue] == VLPOutcomeCancelled;
                          int outcome = status ? VLPOutcomeSuccess : (cancelled ? VLPOutcomeCancelled : VLPOutcomeFailed);
                          VLPSendPurchaseReply(requestId, outcome,
                              status ? eventReply[@"transactionId"] : nil, pid, status && deliveryPending,
                              outcome == VLPOutcomeFailed ? (error ? (int)error.code : -2) : 0,
                              outcome == VLPOutcomeFailed ? VLPErrorMessage(error) : nil);
#if !__has_feature(objc_arc)
                          [eventReply release];
#endif
                          eventReply = nil;
                      });
                  }];
#else
    (void)productName; (void)amountVND; (void)isConsumable; (void)extraData; (void)productId;
    (void)roleId; (void)serverGameId;
    VLPSendPurchaseReply(requestId, VLPOutcomeFailed, nil, nil, NO,
                         -2 /* VLPlaySDK.ErrNativeCall */, @"VLPlaySDK is not linked into this build.");
#endif
}

#pragma mark - IAP: history + restore

// Mirrors sdk.vlplay.vn.payment.TransactionItem.from (vlgame-sdk-android @ 82cddd4) over
// the raw BE rows the iOS SDK hands back. Returns nil when the row has no identifier at
// all (code / transactionId / _id) — nothing to key it by, Android drops it too.
// Faithful quirks, on purpose:
//   • currency defaults to "VND" only when the key is ABSENT (a present "" stays "");
//   • the packageName → name → productId chain is on EMPTINESS (its optString default
//     is "", so absent and empty behave the same there);
//   • a legacy NUMERIC createdAt is coerced to its decimal string, because that is what
//     org.json's optString actually does — Android's numeric-fallback branch is
//     unreachable, and the wrapper mirrors real behavior, not intent. The epoch field
//     is computed C#-side from the string, one parser for both platforms.
static inline NSDictionary* VLPNormalizeTransaction(id rowRaw) {
    if (![rowRaw isKindOfClass:[NSDictionary class]]) return nil;
    NSDictionary* row = (NSDictionary*)rowRaw;

    NSString* code    = VLPDictString(row, @"code");
    NSString* txnId   = VLPDictString(row, @"transactionId");
    NSString* mongoId = VLPDictString(row, @"_id");
    NSString* identifier = code.length > 0 ? code : (txnId.length > 0 ? txnId : mongoId);
    if (identifier.length == 0) return nil;

    NSString* productId = VLPDictString(row, @"productId");

    NSString* packageName = VLPDictString(row, @"packageName");
    if (packageName.length == 0) packageName = VLPDictString(row, @"name");
    if (packageName.length == 0) packageName = productId;

    NSString* currency = VLPDictHas(row, @"currency") ? VLPDictString(row, @"currency") : @"VND";

    NSString* paymentStatus = VLPDictString(row, @"paymentStatus");
    if (paymentStatus.length == 0) paymentStatus = VLPDictString(row, @"status");

    return @{
        @"transactionId": txnId.length > 0 ? txnId : identifier,
        @"code":          code,
        @"productId":     productId,
        @"bundleId":      VLPDictString(row, @"bundleId"),
        @"packageName":   packageName,
        @"amount":        @(VLPDictLong(row, @"amount")),
        @"currency":      currency,
        @"type":          VLPDictString(row, @"type"),
        @"paymentStatus": paymentStatus,
        @"createdAt":     VLPDictString(row, @"createdAt"),
    };
}

static void VLPSendHistoryReply(int requestId, NSArray* transactions, int errorCode, NSString* errorMessage) {
    NSDictionary* reply = @{
        @"requestId":    @(requestId),
        @"ok":           @(transactions != nil),
        @"transactions": transactions ?: @[],
        @"errorCode":    @(errorCode),
        @"errorMessage": errorMessage ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:reply options:0 error:nil];
    VLPSend(@"purchaseHistory", data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"");
}

static void VLPSendRestoreReply(int requestId, NSArray* items, int errorCode, NSString* errorMessage) {
    NSDictionary* reply = @{
        @"requestId":    @(requestId),
        @"ok":           @(items != nil),
        @"items":        items ?: @[],
        @"errorCode":    @(errorCode),
        @"errorMessage": errorMessage ?: @"",
    };
    NSData* data = [NSJSONSerialization dataWithJSONObject:reply options:0 error:nil];
    VLPSend(@"restoreResult", data ? [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding] : @"");
}

// One page of IAP history. The SDK maps BE 1049 (empty history) to success + empty
// array itself, and its completion drops the response envelope — pagination metadata
// never reaches this bridge, which is why the C# side reports -1 for it on iOS.
void VLPlayUnity_GetPurchaseHistory(int requestId, int page, int size) {
#if VLP_HAS_SDK
    [[VLPlaySDKManager defaultManager]
        listSuccessfulTransactionsAtPage:(NSInteger)page
                                    size:(NSInteger)size
                              completion:^(BOOL status, NSArray* _Nullable transactions, NSError* _Nullable error) {
        if (!status) {
            VLPSendHistoryReply(requestId, nil, error ? (int)error.code : 0, VLPErrorMessage(error));
            return;
        }
        NSMutableArray* rows = [NSMutableArray array];
        for (id row in (transactions ?: @[])) {
            NSDictionary* t = VLPNormalizeTransaction(row);
            if (t) [rows addObject:t];
        }
        VLPSendHistoryReply(requestId, rows, 0, nil);
    }];
#else
    (void)page; (void)size;
    VLPSendHistoryReply(requestId, nil, -2 /* VLPlaySDK.ErrNativeCall */, @"VLPlaySDK is not linked into this build.");
#endif
}

// Restore non-consumables. Streams onRestored(productId, transactionId) per item into a
// local array, replies once at the terminal completion(restoredCount, error) — matching
// the Android proxy, which collects RestoreListener.onRestored and resolves at
// onCompleted/onFailure. All SDK callbacks arrive on the main queue.
void VLPlayUnity_RestorePurchases(int requestId) {
#if VLP_HAS_SDK
    NSMutableArray* restored = [NSMutableArray array];
    [[VLPlaySDKManager defaultManager]
        restorePurchases:^(NSString* _Nonnull productId, NSString* _Nonnull transactionId) {
            [restored addObject:@{ @"productId":     productId ?: @"",
                                   @"transactionId": transactionId ?: @"" }];
        }
              completion:^(NSInteger restoredCount, NSError* _Nullable error) {
            if (error) {
                VLPSendRestoreReply(requestId, nil, (int)error.code, VLPErrorMessage(error));
            } else {
                VLPSendRestoreReply(requestId, restored, 0, nil);
            }
        }];
#else
    VLPSendRestoreReply(requestId, nil, -2 /* VLPlaySDK.ErrNativeCall */, @"VLPlaySDK is not linked into this build.");
#endif
}

// Async: replies through the shared envelope tagged with requestId (UnitySendMessage is
// one global channel). The SDK already hops to the main queue before calling back.
void VLPlayUnity_GetProductCatalog(int requestId) {
#if VLP_HAS_SDK
    [[VLPlaySDKManager defaultManager] getProductCatalog:^(BOOL status, NSArray* _Nullable data, NSError* _Nullable error) {
        if (!status) {
            VLPSendCatalogReply(requestId, nil,
                                error ? (int)error.code : 0,
                                VLPErrorMessage(error));
            return;
        }
        NSMutableArray* packages = [NSMutableArray array];
        for (id row in (data ?: @[])) {
            NSDictionary* pkg = VLPNormalizePackage(row);
            if (pkg) [packages addObject:pkg];
        }
        VLPSendCatalogReply(requestId, packages, 0, nil);
    }];
#else
    VLPSendCatalogReply(requestId, nil, -2 /* VLPlaySDK.ErrNativeCall */, @"VLPlaySDK is not linked into this build.");
#endif
}

#pragma mark - Ads backend bridge (native ≥ 1.9.0, IAA-UNITY-FIRST §6 / B2)

// What the Unity ads core borrows from the native SDK: the server's ads block
// verbatim, the BE reward ticket, the /ads/events batch and the reconcile poll — on the
// SDK's session + refresh-token transport, usable with or without the host-owned switch,
// no vendor SDK involved. All four are typed calls on the public VLPlayAds.h (checked by
// the strict-clang run) and their names are also asserted in the binary by
// tools/verify-native-bindings.sh. Completions fire on the main thread, exactly once.

// The `ads` block of /sdk/config as the server sent it, or NULL when it sent none (= ads off).
char* VLPlayUnity_AdsGetConfigSnapshot(void) {
#if VLP_HAS_SDK
    return VLPCopyCString([VLPlayAds adsConfigSnapshotJSON]);
#else
    return NULL;
#endif
}

// provider / adsConfigRevision: "" means absent (reserved for the v2 ticket contract).
void VLPlayUnity_AdsRequestRewardTicket(int requestId, const char* placementId,
                                        const char* provider, const char* adsConfigRevision) {
    NSString* pid = placementId ? [NSString stringWithUTF8String:placementId] : @"";
#if VLP_HAS_SDK
    NSString* prov = (provider && strlen(provider) > 0) ? [NSString stringWithUTF8String:provider] : nil;
    NSString* rev  = (adsConfigRevision && strlen(adsConfigRevision) > 0) ? [NSString stringWithUTF8String:adsConfigRevision] : nil;
    [VLPlayAds requestRewardTicketForPlacement:pid
                                      provider:prov
                             adsConfigRevision:rev
                                    completion:^(NSDictionary* ticket, NSString* error) {
        if (ticket) {
            VLPSendAdsTicketReply(requestId, YES,
                                  VLPDictString(ticket, @"placementId"), VLPDictString(ticket, @"nonce"),
                                  VLPDictString(ticket, @"signature"), VLPDictLong(ticket, @"expiresAtMs"), nil);
        } else {
            VLPSendAdsTicketReply(requestId, NO, pid, nil, nil, 0, error ?: @"ticket failed");
        }
    }];
#else
    (void)provider; (void)adsConfigRevision;
    VLPSendAdsTicketReply(requestId, NO, pid, nil, nil, 0, @"VLPlaySDK is not linked into this build.");
#endif
}

// One normalized event into the SDK's /ads/events batch. false = rejected, nothing buffered.
bool VLPlayUnity_AdsReportEvent(const char* normalizedEventJson) {
#if VLP_HAS_SDK
    if (!normalizedEventJson || strlen(normalizedEventJson) == 0) return false;
    NSData* data = [[NSString stringWithUTF8String:normalizedEventJson] dataUsingEncoding:NSUTF8StringEncoding];
    id parsed = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    if (![parsed isKindOfClass:[NSDictionary class]]) return false;
    return [VLPlayAds reportAdEvent:(NSDictionary*)parsed] ? true : false;
#else
    (void)normalizedEventJson;
    return false;
#endif
}

// The SDK's own reconcile poll for a ticket nonce — one terminal answer.
void VLPlayUnity_AdsCheckRewardStatus(int requestId, const char* nonce) {
    NSString* n = nonce ? [NSString stringWithUTF8String:nonce] : @"";
#if VLP_HAS_SDK
    [VLPlayAds checkRewardStatusForNonce:n
                              completion:^(BOOL confirmed, double creditedAmount,
                                           NSString* rewardCurrency, NSString* reason) {
        VLPSendAdsStatusReply(requestId, n, confirmed, creditedAmount, rewardCurrency, reason);
    }];
#else
    VLPSendAdsStatusReply(requestId, n, NO, 0, nil, @"VLPlaySDK is not linked into this build.");
#endif
}

#pragma mark - Account ops (U5-1) + FAB (U5-2)

// Social bind — OAuth runs inside the SDK. provider: "google"/"facebook"/"apple"
// (matches VLPlayBindProvider* constant values; Apple is iOS-only — Android rejects it).
void VLPlayUnity_BindSocial(int requestId, const char* provider) {
#if VLP_HAS_SDK
    NSString* p = provider ? [NSString stringWithUTF8String:provider] : @"";
    [[VLPlaySDKManager defaultManager] bindSocialProvider:p
                                               completion:^(BOOL status, NSError* _Nullable error) {
        VLPSendAccountReply(requestId, status, error ? (int)error.code : 0, VLPErrorMessage(error), @"");
    }];
#else
    (void)provider;
    VLPSendAccountReply(requestId, NO, -2, @"VLPlaySDK is not linked into this build.", @"");
#endif
}

// Guest upgrade. A non-empty email opens the SDK's email-OTP dialog before the
// network call; "" omits the email entirely (BE Joi rejects empty strings).
void VLPlayUnity_UpgradeGuest(int requestId, const char* username, const char* password, const char* email) {
#if VLP_HAS_SDK
    [[VLPlaySDKManager defaultManager]
        upgradeGuestAccountWithUsername:(username ? [NSString stringWithUTF8String:username] : @"")
                               password:(password ? [NSString stringWithUTF8String:password] : @"")
                                  email:(email ? [NSString stringWithUTF8String:email] : @"")
                             completion:^(BOOL status, NSError* _Nullable error) {
        VLPSendAccountReply(requestId, status, error ? (int)error.code : 0, VLPErrorMessage(error), @"");
    }];
#else
    (void)username; (void)password; (void)email;
    VLPSendAccountReply(requestId, NO, -2, @"VLPlaySDK is not linked into this build.", @"");
#endif
}

void VLPlayUnity_ShowIdentityVerification(void) {
#if VLP_HAS_SDK
    [VLPlaySDKManager showIdentityVerification];
#endif
}

void VLPlayUnity_ShowGuestUpgrade(void) {
#if VLP_HAS_SDK
    [VLPlaySDKManager showGuestUpgrade];
#endif
}

// deactivateAccountWithCompletion: lives in the INTERNAL header only (demo2 reaches
// it via SDKManager+Internal), so resolve it at runtime — selector verified present
// in the shipped binary. On success the SDK signs the user out (signOut delegate
// follows the reply).
void VLPlayUnity_DeactivateAccount(int requestId) {
#if VLP_HAS_SDK
    id mgr = [VLPlaySDKManager defaultManager];
    SEL sel = sel_registerName("deactivateAccountWithCompletion:");
    if (![mgr respondsToSelector:sel]) {
        VLPSendAccountReply(requestId, NO, -2, @"deactivateAccount is unavailable in this SDK build.", @"");
        return;
    }
    void (^completion)(BOOL, NSError*) = ^(BOOL status, NSError* _Nullable error) {
        VLPSendAccountReply(requestId, status, error ? (int)error.code : 0, VLPErrorMessage(error), @"");
    };
    ((void (*)(id, SEL, void (^)(BOOL, NSError*)))objc_msgSend)(mgr, sel, completion);
#else
    VLPSendAccountReply(requestId, NO, -2, @"VLPlaySDK is not linked into this build.", @"");
#endif
}

// Hands a notice back to the SDK's own renderer. Resolved at RUNTIME on purpose: the
// wrapper must keep compiling against a vendored header older than the release that added
// +showDefaultNotice:sticky:, and against that older native the right behaviour is simply
// no fallback — which is what respondsToSelector: gives us.
void VLPlayUnity_ShowDefaultNotice(const char *message, bool sticky) {
#if VLP_HAS_SDK
    Class cls = NSClassFromString(@"VLPlaySDKManager");
    SEL sel = sel_registerName("showDefaultNotice:sticky:");
    if (!cls || ![cls respondsToSelector:sel]) return;
    NSString *text = message ? [NSString stringWithUTF8String:message] : @"";
    if (text.length == 0) return;
    void (*msg)(id, SEL, id, BOOL) = (void (*)(id, SEL, id, BOOL))objc_msgSend;
    msg((id)cls, sel, text, sticky ? YES : NO);
#endif
}

void VLPlayUnity_ClearDefaultNotice(void) {
#if VLP_HAS_SDK
    Class cls = NSClassFromString(@"VLPlaySDKManager");
    SEL sel = sel_registerName("clearDefaultNotice");
    if (!cls || ![cls respondsToSelector:sel]) return;
    void (*msg)(id, SEL) = (void (*)(id, SEL))objc_msgSend;
    msg((id)cls, sel);
#endif
}

void VLPlayUnity_ShowFloatingBall(void) {
#if VLP_HAS_SDK
    [VLPlayFAB show];
#endif
}

void VLPlayUnity_DismissFloatingBall(void) {
#if VLP_HAS_SDK
    [VLPlayFAB dismiss];
#endif
}

void VLPlayUnity_ConfigureFAB(const char* configJson) {
#if VLP_HAS_SDK
    if (!configJson || strlen(configJson) == 0) return;
    NSData* data = [[NSString stringWithUTF8String:configJson] dataUsingEncoding:NSUTF8StringEncoding];
    id parsed = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    if ([parsed isKindOfClass:[NSDictionary class]]) {
        [VLPlayFAB configure:(NSDictionary*)parsed];
    } else {
        NSLog(@"<VLPlayUnity> ConfigureFAB: not a JSON object — ignored");
    }
#else
    (void)configJson;
#endif
}

// The ONLY writer of the context the payment body and the support form read.
// -[VLPlaySDKManager updateGameInfo:roleID:roleName:] is public (VLPlaySDKManager.h)
// and writes VLPlayConfig, which NetworkModal+Payment puts on every init-payment
// body. VLPlayFAB's setContext: does NOT feed it — on the vendored 1.4.1 binary
// nothing in the SDK reads that property back (contextSnapshot: is absent there),
// so a game that only calls SetFABContext ships an empty roleId and the delivery
// callback cannot route the grant.
void VLPlayUnity_UpdateGameInfo(const char* serverGameId, const char* roleId, const char* roleName) {
#if VLP_HAS_SDK
    NSString* s = serverGameId ? [NSString stringWithUTF8String:serverGameId] : @"";
    NSString* r = roleId       ? [NSString stringWithUTF8String:roleId]       : @"";
    NSString* n = roleName     ? [NSString stringWithUTF8String:roleName]     : @"";
    [[VLPlaySDKManager defaultManager] updateGameInfo:s roleID:r roleName:n];
#else
    (void)serverGameId; (void)roleId; (void)roleName;
#endif
}

void VLPlayUnity_SetFABContext(const char* contextJson) {
#if VLP_HAS_SDK
    if (!contextJson || strlen(contextJson) == 0) return;
    NSData* data = [[NSString stringWithUTF8String:contextJson] dataUsingEncoding:NSUTF8StringEncoding];
    id parsed = data ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    if ([parsed isKindOfClass:[NSDictionary class]]) {
        [VLPlayFAB setContext:(NSDictionary*)parsed];
    } else {
        NSLog(@"<VLPlayUnity> SetFABContext: not a JSON object — ignored");
    }
#else
    (void)contextJson;
#endif
}

#ifdef __cplusplus
} // extern "C"
#endif
