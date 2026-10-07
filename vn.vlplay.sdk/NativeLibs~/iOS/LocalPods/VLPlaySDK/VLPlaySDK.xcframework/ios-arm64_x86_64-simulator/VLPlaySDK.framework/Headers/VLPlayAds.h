//
//  VLPlayAds.h
//  VLPlaySDK
//
//  Layer 1 — the public, provider-agnostic ads facade (IAA-01 design §3).
//  PUBLIC header. The API never names a mediation vendor — the provider is
//  server-driven per game via the /sdk/config `ads` block. Mirror of the
//  Android VLPlayAds facade.
//
//  Lifecycle: the module initializes itself from remote config after the SDK
//  bootstrap (no game-side init call). Every method is safe to call at any
//  time — when ads are disabled/not ready the delegates receive a
//  provider-neutral VLPlayAdError instead of anything throwing into game code.
//
//  Enabling ads in a game (DP-2a): the SDK framework does NOT link any ad
//  vendor. Add the vendor pod to the APP (e.g. `pod 'AppLovinSDK'`), have Ops
//  enable the CMS ads config — the SDK detects the vendor at runtime. Remove
//  the pod (or Ops flips ads.enabled off) to remove ads; nothing crashes.
//
//  Rewarded integrity (design §6.1): grant currency ONLY in
//  `vlplayAdRewardConfirmed:` (server SSV verified) — never in
//  `vlplayAdUserRewarded:` (advisory).
//
//      [VLPlayAds preload:@"rewarded_shop"];
//      if ([VLPlayAds isReady:@"rewarded_shop"]) {
//          [VLPlayAds showRewarded:@"rewarded_shop" fromViewController:vc delegate:self];
//      }
//

#import <UIKit/UIKit.h>
#import <VLPlaySDK/VLPlayAdsModels.h>

NS_ASSUME_NONNULL_BEGIN

@interface VLPlayAds : NSObject

/// YES when the remote config enables ads for this game.
+ (BOOL)isEnabled;

/// YES when the mediation provider finished initializing.
+ (BOOL)isInitialized;

/// Preloads a fill for the placement (recommended before show). Fire-and-forget.
+ (void)preload:(NSString *)placementId;

/// Preloads a fill with load-result callbacks.
+ (void)preload:(NSString *)placementId delegate:(nullable id<VLPlayAdLoadDelegate>)delegate;

/// Preloads every placement flagged preload:true in the remote config.
+ (void)preloadAll;

/// YES when a fill for the placement is loaded and ready to show.
+ (BOOL)isReady:(NSString *)placementId;

/// Shows a rewarded ad. The delegate receives the full lifecycle on the main
/// thread; failures (disabled / unknown placement / not ready / this placement's
/// previous ad not dismissed yet) arrive as `vlplayAdShowFailed:` — nothing throws. The SDK holds the delegate strongly
/// until the reward flow reaches a terminal state.
+ (void)showRewarded:(NSString *)placementId
  fromViewController:(nullable UIViewController *)viewController
            delegate:(nullable id<VLPlayRewardedAdDelegate>)delegate;

/// Shows an interstitial ad (frequency-capped per the remote config). A call while this
/// placement's previous ad is still on screen answers `vlplayAdShowFailed:` SHOW_FAILED.
+ (void)showInterstitial:(NSString *)placementId
      fromViewController:(nullable UIViewController *)viewController
                delegate:(nullable id<VLPlayInterstitialAdDelegate>)delegate;

/// Ties impressions + SSV reward callbacks to the VLPlay account. Buffered
/// until the provider is ready. The SDK does NOT set it on sign-in — call it once
/// the player is signed in. It clears it on sign-out and session expiry, so the
/// next player's ads never report the last one.
+ (void)setUserId:(nullable NSString *)userId;

/// QA ONLY — overrides the remote ads config with a locally supplied one, so a
/// build can exercise the ad pipeline before Ops provisions the CMS (e.g. with
/// Google's public AdMob test unit ids, which fill on any device without an
/// account). Mirrors the Android demo path (`AdsConfig.fromJson` +
/// `AdManager.applyConfig`), which Android exposes as public classes.
///
/// `config` is the same shape as the `/sdk/config` → `ads` block. Pass nil to
/// tear the provider down. The next real config refresh overwrites this.
///
/// ⚠️ A locally injected `ssv:true` placement can never confirm — the BE issues
/// no reward ticket for a placement it does not know. Use `ssv:false` here and
/// accept that `vlplayAdRewardConfirmed:` will not fire.
///
///     [VLPlayAds debugApplyConfig:@{ @"enabled": @YES, @"provider": @"admob",
///                                    @"testMode": @YES, @"placements": @[ … ] }];
+ (void)debugApplyConfig:(nullable NSDictionary *)config;

#pragma mark - Backend bridge for a host that owns mediation itself

/// (IAA-UNITY-FIRST-MEDIATION-PLAN §6 — B2.) When the game ships its own mediation plugin and
/// sets `VLPLAY_ADS_HOST_OWNED` = <true/> in VLPlaySDK-Info.plist, this module stays silent (no
/// provider, no preload, no ticket, no events) but still lends the host the pieces that must
/// have ONE owner: the server's ads config verbatim and the BE transport with the SDK's session
/// + refresh-token handling. Usable regardless of the switch. No vendor SDK involved.
/// Completions fire on the main thread, exactly once.

/// The `ads` block of /sdk/config exactly as the server sent it (unknown keys such as a v2
/// `unitIds` intact), serialized as JSON — or nil when the server sent no block (= ads off).
+ (nullable NSString *)adsConfigSnapshotJSON;

/// Issues a BE reward ticket for an ssv:true placement. `ticket` = @{ placementId, nonce,
/// signature (empty for AdMob), expiresAtMs }. Hand `nonce` (AdMob) or `nonce:signature` (MAX)
/// to the vendor SDK as SSV custom data, then confirm with +checkRewardStatusForNonce:.
/// `provider` / `adsConfigRevision` are reserved for the v2 ticket contract (I-BE-6), not sent yet.
+ (void)requestRewardTicketForPlacement:(NSString *)placementId
                               provider:(nullable NSString *)provider
                      adsConfigRevision:(nullable NSString *)adsConfigRevision
                             completion:(void (^)(NSDictionary * _Nullable ticket, NSString * _Nullable error))completion;

/// Reports one normalized ad event to POST /ads/events through the SDK's batch. Keys: `event`
/// (ad_load / ad_load_failed / ad_impression / ad_click / ad_rewarded / ad_reward_confirmed /
/// ad_dismissed), `placementId`, and optionally format / provider / network / revenue /
/// currency / precision / nonce / error / ts. Other keys are dropped. Fire-and-forget.
/// Returns NO when the event was rejected and nothing was buffered.
+ (BOOL)reportAdEvent:(NSDictionary *)normalizedEvent;

/// Reconciles a reward ticket with the BE (same poll as the SDK's own rewarded path — 10
/// attempts, 2 s → 30 s) and reports ONE terminal outcome. Grant ONLY when `confirmed` is YES.
+ (void)checkRewardStatusForNonce:(NSString *)nonce
                       completion:(void (^)(BOOL confirmed, double creditedAmount,
                                            NSString * _Nullable rewardCurrency, NSString * _Nullable reason))completion;

@end

NS_ASSUME_NONNULL_END
