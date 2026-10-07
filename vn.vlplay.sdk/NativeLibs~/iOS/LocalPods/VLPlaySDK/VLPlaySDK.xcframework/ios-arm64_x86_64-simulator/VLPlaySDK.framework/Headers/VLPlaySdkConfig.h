//
//  VLPlaySdkConfig.h
//  VLPlaySDK

#import <Foundation/Foundation.h>

@class VLPlayAdsConfig;

NS_ASSUME_NONNULL_BEGIN

// iOS supports Apple StoreKit purchases only.
extern NSString * const VLPlayPaymentMethodAppleIAP;     // "apple_iap"

@interface VLPlaySdkConfig : NSObject

// Top-level
@property (nonatomic, copy, readonly) NSString *appsflyerDevKey;
@property (nonatomic, copy, readonly) NSString *appsflyerAppId;
// SDKCONFIG-APPSFLYER-PER-PLATFORM (locked 2026-05-15): per-platform pair
// shipped by BE in addition to the legacy flat fields. Native reads the
// platform-specific value with strict no-cross-fallback per §5.2 — empty
// iOS field falls to Info.plist baseline, not Android field.
@property (nonatomic, copy, readonly) NSString *appsflyerIosDevKey;
@property (nonatomic, copy, readonly) NSString *appsflyerAndroidDevKey;
@property (nonatomic, copy, readonly) NSString *appsflyerIosAppId;
@property (nonatomic, copy, readonly) NSString *appsflyerAndroidAppId;
@property (nonatomic, copy, readonly) NSString *sdkMinVersion;
@property (nonatomic, assign, readonly) BOOL forceUpdate;
@property (nonatomic, assign, readonly) BOOL maintenanceMode;
@property (nonatomic, copy, readonly) NSString *maintenanceMessage;

// AGE-RATING (min_age, 2026-06-17): per-game age floor (VN decree rating),
// TOP-LEVEL sibling of `features` in /sdk/config — NOT nested. Resolved at
// parse: absent ⇒ 18 (safe default), 0 ⇒ 00+/no gate, >0 ⇒ floor (12/16/18).
// The gate lives in identity verification. Callers MUST treat <= 0 as "no
// gate" — do NOT fall back to 18 on a value of 0. Parity web-sdk resolveMinAge
// + Android SdkConfig.getMinAge().
@property (nonatomic, assign, readonly) NSInteger minAge;

// Features (6 toggles per CMS — guestLogin/appsFlyerTracking/otpRequired/
// emailVerification are CMS-side reality but absent in openapi.yaml v2.1.0;
// see GAP-2 in my-notes/REMOTE_CONFIG_IMPL.md)
@property (nonatomic, assign, readonly) BOOL identityVerification;
@property (nonatomic, assign, readonly) BOOL antiAddiction;
@property (nonatomic, assign, readonly) BOOL guestLogin;
@property (nonatomic, assign, readonly) BOOL appsFlyerTracking;
@property (nonatomic, assign, readonly) BOOL otpRequired;
// emailVerification gates the post-signup OTP screen (Web SDK alignment
// 2026-05-09 LATE+1). Default NO — legacy CMS without the toggle keeps the
// no-OTP path; flip on in CMS to drive Option A flow.
@property (nonatomic, assign, readonly) BOOL emailVerification;
// IDENTITY-REQUIRE-IDCARD (features.identityRequireIdCard, 2026-07-02): per-game Ops
// toggle. Default YES = current behavior. When NO the Identity screen hides the
// CMND/CCCD field + skips its validation + omits it from the payload. Web-sdk parity
// `serverConfig.features.identityRequireIdCard !== false`.
@property (nonatomic, assign, readonly) BOOL identityRequireIdCard;

// Payment methods — filtered on parse so iOS can expose only Apple IAP even if
// CMS sends cross-platform or legacy payment identifiers.
@property (nonatomic, copy, readonly) NSArray<NSString *> *paymentMethods;

// Social login — nested per-provider (matches openapi.yaml v2.1.0 spec)
@property (nonatomic, copy, readonly) NSString *facebookAppId;
@property (nonatomic, copy, readonly) NSString *facebookClientToken;
@property (nonatomic, copy, readonly) NSString *googleWebClientId;
@property (nonatomic, copy, readonly) NSString *googleIosClientId;
@property (nonatomic, copy, readonly) NSString *appleBundleId;
@property (nonatomic, copy, readonly) NSString *appleServicesId;

// Floating Action Ball (FAB) — CMS-driven overlay config. `fab` block is
// optional; all keys default off / empty when absent.
//   fab.enabled  : BOOL — global on/off per game.
//   fab.position : NSString — `left` | `right` (default right).
//   fab.iconUrl  : NSString — HTTPS URL for ball glyph; SDK bundles default.
//   fab.tabs     : NSArray<NSString *> — render order from
//                   `account` | `support` | `identity`.
/// YES iff the CMS payload contained a `fab` object. Distinguishes "CMS
/// has no opinion on FAB" (local override drives) from "CMS explicitly says
/// fab.enabled=false" (CMS overrides local). Spec §8.2 + Web SDK behaviour:
/// `{...local, ...cms.fab}` only spreads CMS when the block is present.
@property (nonatomic, assign, readonly) BOOL fabBlockPresent;
@property (nonatomic, assign, readonly) BOOL fabEnabled;
@property (nonatomic, copy, readonly) NSString *fabPosition;
@property (nonatomic, copy, readonly) NSString *fabIconUrl;
@property (nonatomic, copy, readonly) NSArray<NSString *> *fabTabs;

// Push provider — OneSignal v5 SDK App ID per D6.5 (Chen 2026-05-16, supersedes
// D6.3 FCM-only). Empty when CMS hasn't provisioned the field → -[VLPlaySDKManager
// bootstrapOneSignalIfNeeded] no-ops. Per-game; same field Web SDK consumes
// via `web-sdk/src/notifications/onesignal.ts` initOneSignal(appId).
@property (nonatomic, copy, readonly) NSString *onesignalAppId;

// AF-Events dynamic layer (AF-EVENTS contract C1, 2026-05-29). TOP-LEVEL
// sibling of `features` in /sdk/config — NOT nested. Lets Ops rename / disable
// / enable AppsFlyer events at runtime without an SDK rebuild. Default off +
// empty → resolver no-op for legacy BE. Mirrors Android SdkConfig.
//   afEvents.enabled  : BOOL — master toggle for the dynamic layer.
//   afEvents.map      : { stableKey : afEventName } — remap; else passthrough.
//   afEvents.disabled : [stableKey] — dropped before dispatch.
@property (nonatomic, assign, readonly) BOOL afEventsEnabled;
@property (nonatomic, copy, readonly) NSDictionary<NSString *, NSString *> *afEventsMap;
@property (nonatomic, copy, readonly) NSArray<NSString *> *afEventsDisabled;

// IAA ads (IAA-01, P3-1). TOP-LEVEL sibling of `features` in /sdk/config —
// NOT nested. Tri-state like fab: nil = no ads block on the wire (absent ⇒
// absent in cache) — the AdManager treats nil as the kill-switch. The parsed
// object resolves `platformOverrides.ios` client-side (G3/N-10).
@property (nonatomic, strong, readonly, nullable) VLPlayAdsConfig *ads;

// Support
@property (nonatomic, copy, readonly) NSString *hotline;
@property (nonatomic, copy, readonly) NSString *fanpage;
@property (nonatomic, copy, readonly) NSString *emailSupport;

/// Returns YES only for the Apple IAP identifier when CMS enables it.
- (BOOL)isPaymentMethodEnabled:(NSString *)methodId;

/// Parse from API response "data" dictionary (nested: features, socialLogin, support)
+ (nullable instancetype)fromDictionary:(NSDictionary *)dict;

/// Build partial config from VLPlaySDK-Info.plist (fallback)
+ (nullable instancetype)fromPlist;

/// Serialize for NSUserDefaults cache (same nested structure as API)
- (NSDictionary *)toDictionary;

@end

NS_ASSUME_NONNULL_END
