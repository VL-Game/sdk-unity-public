//
//  AppsFlyerHelper.h
//  VLPlaySDK
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

#pragma mark - The AppsFlyer master switch, as three states and four rules

/**
 The AppsFlyer master switch mirrors CMS `features.appsFlyerTracking` — but it
 has three states, not two.

 A boolean cannot say "we have not been told yet", and that is the state a first
 cold install is actually in while its config fetch is in flight or has failed.
 Both previous attempts at this switch only got to pick the boolean's default,
 so each could only choose a direction to be wrong in: defaulting to on sent
 VLPlay telemetry from devices never told to send (which is what iOS did until
 this change — see `-[VLPlaySDKManager handleApplication:...]`), and defaulting
 to off stopped AppsFlyer from starting at all, losing the install postback —
 the loss behind the LDPlayer incident, 9,831 of 9,847 accounts with no
 AppsFlyer ID.

 Chen's hybrid rule, locked 2026-09-02, needs the third state:

     UNKNOWN   AppsFlyer bootstrap ALLOWED   VLPlay custom events HELD (queued)
     ON        bootstrap allowed             custom events sent
     OFF       bootstrap BLOCKED             custom events blocked

 The install postback is the one thing an install only gets one chance at, so
 UNKNOWN keeps it; every VLPlay-authored event can wait for a verdict, so
 UNKNOWN holds those. Same three states and same four rules on Android
 (`AppsFlyerHelper.TrackingState`) — one product rule, stated once per platform.
 */
typedef NS_ENUM(NSInteger, VLPlayTrackingState) {
    /// No trustworthy config yet: bootstrap AppsFlyer, hold VLPlay events.
    VLPlayTrackingStateUnknown = 0,
    /// Config says tracking is on.
    VLPlayTrackingStateOn,
    /// Config says tracking is off: nothing leaves the device.
    VLPlayTrackingStateOff,
};

/**
 What the switch becomes when a config is applied.

 @param authoritative whether this config is trustworthy evidence of what Ops
        wants — a completed fetch, or a cache still inside
        `kVLPlayTrackingCacheMaxAge`. The plist fallback and a stale cache are
        NOT: they are the absence of an answer, not an answer of "off".
 @param trackingFlag  the config's `features.appsFlyerTracking`. Read only when
        `authoritative`.

 Non-authoritative input changes nothing. That is the whole difference from the
 boolean: a first cold install whose fetch fails stays UNKNOWN rather than being
 told either way.
 */
static inline VLPlayTrackingState VLPlayTrackingResolveState(BOOL authoritative,
                                                             BOOL trackingFlag,
                                                             VLPlayTrackingState current) {
    if (!authoritative) return current;
    return trackingFlag ? VLPlayTrackingStateOn : VLPlayTrackingStateOff;
}

/// VLPlay-authored events go out only on a positive verdict.
static inline BOOL VLPlayTrackingCustomEventGated(BOOL initialized, VLPlayTrackingState state) {
    return !initialized || state != VLPlayTrackingStateOn;
}

/// Only an explicit OFF halts the AppsFlyer SDK's own traffic (`isStopped`).
static inline BOOL VLPlayTrackingShouldStopAppsFlyer(VLPlayTrackingState state) {
    return state == VLPlayTrackingStateOff;
}

/**
 Whether AppsFlyer may report at all. UNKNOWN may — that is the
 install-attribution exception, and it covers AppsFlyer's own
 lifecycle/session/install traffic only, never a VLPlay custom event.

 On iOS this axis is expressed through `AppsFlyerLib.isStopped` rather than by
 skipping `initWithDevKey:` (which `-initAppsFlyer` calls unconditionally), so
 this predicate is the negation of the one above. It is stated separately anyway
 because it is a separate clause of the rule, and because Android acts on it at
 a different site.
 */
static inline BOOL VLPlayTrackingShouldBootstrapAppsFlyer(VLPlayTrackingState state) {
    return state != VLPlayTrackingStateOff;
}

// Thin wrapper around AppsFlyerLib for SDK-side analytics. The helper owns
// init, customer-user-id, and a generic event dispatcher. Specific events
// are intentionally NOT defined here — call sites use trackCustomEvent so
// the event taxonomy can evolve without touching SDK internals.
@interface AppsFlyerHelper : NSObject

+ (instancetype)shared;

// Initialization
- (void)initWithDevKey:(NSString *)devKey appStoreId:(NSString *)appStoreId;
- (void)setCustomerUserId:(NSString *)userId;
- (void)setDevMode:(BOOL)devMode;
- (BOOL)isDevMode;

// Verbose AppsFlyer SDK logging (full network sends + responses). LOGGING
// ONLY — does not rename events or change data routing (that was devMode's
// `dev_` prefix). Safe to toggle any time, pre- or post-init. Do not leave
// enabled in App Store builds.
- (void)setDebugLogging:(BOOL)enabled;

// Master enable/disable for event dispatch. When set to NO, trackCustomEvent:
// queues rather than sending. Prefer -setMasterState: / -applyConfigVerdict:.
- (void)setTrackingEnabled:(BOOL)enabled;
// YES only on an explicit ON — UNKNOWN reads as not-enabled here.
- (BOOL)isTrackingEnabled;

// AF-Events dynamic layer (AF-EVENTS contract C1). Pushed from /sdk/config
// top-level `afEvents`. When enabled: drop events whose stable key is in
// `disabled`, and remap stable key → AF name via `map` (passthrough if absent).
// Default off → resolver no-op. `map`/`disabled` may be nil (treated as empty).
- (void)setAfEventsEnabled:(BOOL)enabled
                       map:(nullable NSDictionary<NSString *, NSString *> *)map
                  disabled:(nullable NSArray<NSString *> *)disabled;

// Hard kill-switch stated explicitly — an authoritative ON or OFF. A boolean IS
// a verdict, so it never yields UNKNOWN; the config path goes through
// -applyConfigVerdict:trackingFlag: instead, which is the only caller that can
// say "no verdict yet". Also flips AppsFlyerLib.isStopped, so the underlying SDK
// halts ALL outbound traffic (lifecycle, session, attribution, custom);
// `setTrackingEnabled:` alone only short-circuits our wrapper's logEvent path.
- (void)setMasterEnabled:(BOOL)enabled;

// Move the switch to `state` and make both gates agree with it: our logEvent
// gate, and AppsFlyerLib.isStopped. Only OFF stops AppsFlyer — UNKNOWN
// deliberately does not, because that is the state a first cold install sits in
// and stopping there throws away the install postback.
- (void)setMasterState:(VLPlayTrackingState)state;

// Apply what a config says about the switch. See VLPlayTrackingResolveState.
- (void)applyConfigVerdict:(BOOL)authoritative trackingFlag:(BOOL)trackingFlag;

// The switch's current state.
- (VLPlayTrackingState)masterState;

// Generic event dispatch — primary public API. Callers compose their own
// event name + params per the canonical AppsFlyer event list.
// `params` may be nil. Events posted before `initWithDevKey:appStoreId:`
// completes are queued (bounded) and replayed once init finishes.
- (void)trackCustomEvent:(NSString *)eventName params:(nullable NSDictionary *)params;

// Same as -trackCustomEvent:params: but reports whether the event was dispatched
// or queued. NO means it was dropped — master toggle off, pending queue full, or
// the stable key is absent from (or disabled in) the CMS afEvents whitelist.
// Callers that spend a one-shot budget on an event — a once-per-install flag, a
// de-dupe key — must gate on this so a dropped event doesn't consume that budget.
- (BOOL)trackCustomEventAccepted:(NSString *)eventName params:(nullable NSDictionary *)params;

// The outcome-reporting variant of the above is INTERNAL — declared in
// AppsFlyerHelper+SDKEvents.h, which is not a public header. Partners have no
// use for an event's dispatch verdict, and a method published here cannot be
// withdrawn later without breaking them.

// OneLink plumbing. A game owns its UIApplicationDelegate / UISceneDelegate, so
// the SDK cannot intercept these — the host forwards them and VLPlaySDKManager
// routes them here. Without them AppsFlyer never sees the tap and the OneLink
// resolves to nothing.
- (void)handleOpenURL:(nullable NSURL *)url options:(nullable NSDictionary *)options;
- (BOOL)continueUserActivity:(nullable NSUserActivity *)userActivity
          restorationHandler:(void (^ _Nullable)(NSArray * _Nullable))restorationHandler;

@end

NS_ASSUME_NONNULL_END
