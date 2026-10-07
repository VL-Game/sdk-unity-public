//
//  VLPlayAdsModels.h
//  VLPlaySDK
//
//  IAA-01 (plans/IAA-MEDIATION-DESIGN.md §3.3) — provider-neutral ad models +
//  game-facing delegate protocols. PUBLIC header: games import this via the
//  umbrella; nothing here may reference an ad vendor or an internal SDK type.
//  Mirrors Android sdk.vlplay.vn.ads.model.* + ads.listener.* one-to-one.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Provider-neutral error codes (design §3.3) — never a raw vendor code; the
// originating provider/network error text is preserved in `message`.
extern NSString * const VLPlayAdErrorNoFill;            // "NO_FILL"
extern NSString * const VLPlayAdErrorTimeout;           // "TIMEOUT"
extern NSString * const VLPlayAdErrorNetworkError;      // "NETWORK_ERROR"
extern NSString * const VLPlayAdErrorNotReady;          // "NOT_READY"
extern NSString * const VLPlayAdErrorShowFailed;        // "SHOW_FAILED"
extern NSString * const VLPlayAdErrorAgeRestricted;     // "AGE_RESTRICTED"
extern NSString * const VLPlayAdErrorDisabled;          // "DISABLED"
extern NSString * const VLPlayAdErrorConsentRequired;   // "CONSENT_REQUIRED"
/// Placement id unknown to the remote config, or called with the wrong format —
/// an integration bug the game dev must see.
extern NSString * const VLPlayAdErrorInvalidPlacement;  // "INVALID_PLACEMENT"

/// Provider-neutral ad error. `code` is one of the VLPlayAdError* constants.
@interface VLPlayAdError : NSObject
@property (nonatomic, copy, readonly) NSString *code;
@property (nonatomic, copy, readonly) NSString *provider;
@property (nonatomic, copy, readonly) NSString *networkName;
@property (nonatomic, copy, readonly) NSString *message;
+ (instancetype)errorWithCode:(NSString *)code message:(NSString *)message;
+ (instancetype)errorWithCode:(NSString *)code provider:(nullable NSString *)provider
                      network:(nullable NSString *)networkName message:(nullable NSString *)message;
@end

/// Reward payload. INTEGRITY RULE (design §6.1): the reward passed to
/// `vlplayAdUserRewarded:` is ADVISORY (client-side provider signal) — the game
/// must NOT grant currency on it. Real crediting happens server-side via SSV;
/// the SDK then fires `vlplayAdRewardConfirmed:` with the server-confirmed
/// reward, whose `nonce` identifies the SSV transaction.
@interface VLPlayAdReward : NSObject
@property (nonatomic, copy, readonly) NSString *currency;
@property (nonatomic, assign, readonly) double amount;
/// SSV transaction nonce — empty on the advisory (client-side) reward.
@property (nonatomic, copy, readonly) NSString *nonce;
+ (instancetype)rewardWithCurrency:(nullable NSString *)currency amount:(double)amount
                             nonce:(nullable NSString *)nonce;
@end

/// Impression-level revenue data (ILRD) from the mediation provider.
@interface VLPlayAdRevenue : NSObject
@property (nonatomic, assign, readonly) double revenue;
@property (nonatomic, copy, readonly) NSString *currency;
/// Winning ad network inside the mediation waterfall/auction.
@property (nonatomic, copy, readonly) NSString *networkName;
/// Provider precision label (e.g. "exact", "estimated", "publisher_defined").
@property (nonatomic, copy, readonly) NSString *precision;
+ (instancetype)revenueWithAmount:(double)revenue currency:(nullable NSString *)currency
                          network:(nullable NSString *)networkName precision:(nullable NSString *)precision;
@end

/// Preload/load result callbacks. Fired on the main thread.
@protocol VLPlayAdLoadDelegate <NSObject>
/// Fill obtained — `+[VLPlayAds isReady:]` is now true.
- (void)vlplayAdLoaded:(NSString *)placementId;
/// No fill / network / timeout (after the retry budget is exhausted).
- (void)vlplayAdLoadFailed:(NSString *)placementId error:(VLPlayAdError *)error;
@end

/// Rewarded ad lifecycle. All callbacks fire on the main thread.
/// INTEGRITY: grant in-game currency ONLY in `vlplayAdRewardConfirmed:` (BE SSV
/// verified). `vlplayAdUserRewarded:` is the provider's client-side signal —
/// advisory, show a spinner at most. The SDK keeps a strong reference to the
/// delegate until the reward flow reaches a terminal state.
@protocol VLPlayRewardedAdDelegate <NSObject>
/// Client-side reward signal from the ad network — ADVISORY ONLY.
- (void)vlplayAdUserRewarded:(NSString *)placementId reward:(VLPlayAdReward *)reward;
/// BE SSV confirmed + credited (reconcile flow). Safe to grant.
- (void)vlplayAdRewardConfirmed:(NSString *)placementId reward:(VLPlayAdReward *)serverReward;
/// Full-screen ad closed → resume the game.
- (void)vlplayAdDismissed:(NSString *)placementId;
/// Show attempted but failed (not ready / capped / expired / render error).
- (void)vlplayAdShowFailed:(NSString *)placementId error:(VLPlayAdError *)error;
@optional
- (void)vlplayAdShown:(NSString *)placementId;
- (void)vlplayAdClicked:(NSString *)placementId;
@end

/// Interstitial ad lifecycle. Fired on the main thread.
@protocol VLPlayInterstitialAdDelegate <NSObject>
/// Full-screen ad closed → resume the game.
- (void)vlplayAdDismissed:(NSString *)placementId;
/// Show attempted but failed (not ready / capped / expired / render error).
- (void)vlplayAdShowFailed:(NSString *)placementId error:(VLPlayAdError *)error;
@optional
- (void)vlplayAdShown:(NSString *)placementId;
- (void)vlplayAdClicked:(NSString *)placementId;
@end

NS_ASSUME_NONNULL_END
