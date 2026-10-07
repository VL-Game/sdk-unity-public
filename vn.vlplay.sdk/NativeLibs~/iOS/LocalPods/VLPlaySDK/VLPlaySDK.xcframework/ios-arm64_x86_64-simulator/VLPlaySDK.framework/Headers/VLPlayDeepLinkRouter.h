//
//  VLPlayDeepLinkRouter.h
//  VLPlaySDK
//
//  Deep-link parser + fan-out. Two producers feed it:
//    1. Push notifications — the `deeplink` field of an APNs payload.
//    2. AppsFlyer OneLink — direct (app already installed) and deferred
//       (route survives the install), via -[AppsFlyerHelper] delegates.
//
//  Both normalise to `vlplay://<action>[/<path>][?<query>]` and broadcast
//  VLPlayDeepLinkNotification. The SDK does not auto-navigate: a route can
//  land while the app is backgrounded or mid-flow, so the integrator owns the
//  dispatch decision.
//
//  Deferred OneLink routes resolve within a second or two of first launch —
//  usually BEFORE the game has finished booting and subscribed. A route that
//  arrives with no handler is therefore held (one slot) and replayed on the
//  first -setHandler: call, otherwise the whole deferred feature would drop
//  its one and only delivery.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString * const VLPlayDeepLinkNotification;
extern NSString * const VLPlayDeepLinkActionKey;
extern NSString * const VLPlayDeepLinkPathKey;
extern NSString * const VLPlayDeepLinkParamsKey;
extern NSString * const VLPlayDeepLinkRawURLKey;

extern NSString * const VLPlayDeepLinkActionShop;
extern NSString * const VLPlayDeepLinkActionAntiAddiction;
extern NSString * const VLPlayDeepLinkActionWeb;

/// Query-parameter names the SDK merges onto an AppsFlyer-sourced route.
/// They ride inside VLPlayDeepLinkParamsKey alongside the link's own params.
extern NSString * const VLPlayDeepLinkParamDeferred;    // "af_deferred" — @"true" / @"false"
extern NSString * const VLPlayDeepLinkParamMediaSource; // "media_source"
extern NSString * const VLPlayDeepLinkParamCampaign;    // "campaign"

typedef void (^VLPlayDeepLinkHandler)(NSDictionary<NSString *, id> *info);

@interface VLPlayDeepLinkRouter : NSObject

/// Parse + broadcast. Returns NO for an empty string or a non-`vlplay` scheme.
+ (BOOL)handle:(nullable NSString *)urlString;

/// Register a block that receives every parsed route. Pass nil to unregister.
/// A route that arrived while no handler was set is delivered immediately on
/// registration (one slot, then cleared). NSNotification observers keep
/// working unchanged — this is an additional, buffer-backed channel.
+ (void)setHandler:(nullable VLPlayDeepLinkHandler)handler;

/// Normalise an AppsFlyer OneLink payload into a `vlplay://` URL string.
///
/// `value` is the link's `deep_link_value`. Three shapes are accepted:
///   - a bare route      — @"shop"        -> @"vlplay://shop"
///   - a full SDK URL    — @"vlplay://shop?id=1" -> used as-is
///   - a foreign scheme  — @"https://..." -> nil (not ours to route)
/// When `value` is empty the click payload is searched for `deep_link_value`,
/// `af_dp`, then `link`.
///
/// Deep-link context (deep_link_sub1..10) and any attribution fields AppsFlyer
/// makes available are merged without overwriting keys already authored in `value`.
/// Attribution fields are not guaranteed on deferred links because UDL privacy can omit them.
///
/// Pure function — no SDK state, no side effects. Returns nil when nothing
/// routable is present.
+ (nullable NSString *)urlFromAppsFlyerValue:(nullable NSString *)value
                                  clickEvent:(nullable NSDictionary *)clickEvent
                                    deferred:(BOOL)deferred;

/// UDL arbitration. AppsFlyer can describe ONE deferred click twice — once through
/// -didResolveDeepLink: (UDL) and once through conversion data — and the two payloads
/// carry different key sets, so the URLs they normalise to are not byte-identical and
/// the repeat-URL dedupe cannot collapse them. UDL is the primary source; the
/// conversion route is a fallback for links authored without a UDL payload, and it
/// must only fire once UDL has said NOT_FOUND for this launch.
///
///   +noteUdlFound          — UDL delivered the route; drop any held conversion route.
///   +noteUdlNotFound       — UDL has nothing (or failed); release a held conversion
///                            route, and let a later one through directly.
///   +routeFromConversion:  — found → dropped; not-found → routed now;
///                            UDL not heard from yet → held (one slot).
+ (void)noteUdlFound;
+ (void)noteUdlNotFound;
+ (void)routeFromConversion:(nullable NSString *)urlString;

@end

NS_ASSUME_NONNULL_END
