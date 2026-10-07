//
//  VLPlayFAB.h
//  VLPlaySDK
//
//  Native Floating Action Ball + HUD overlay (Option B hybrid).
//  Parity Web SDK FAB (vlgame-project-hub/docs/fe/FLOATING-BALL-IMPL-PLAN.md).
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/// Tab identifiers for FAB HUD panel. Match spec §3.1.
///
typedef NS_ENUM(NSInteger, VLPlayFABTab) {
    VLPlayFABTabAccount  = 0,
    VLPlayFABTabSupport  = 1,
    VLPlayFABTabIdentity = 3,
};

/// Public entry for the SDK-owned floating ball + HUD overlay.
///
/// Lives on its own retained `UIWindow` (parity `VLPlayLoading`) bound to the
/// foreground-active `UIWindowScene`, `windowLevel = UIWindowLevelNormal + 1`
/// so it sits above the game but BELOW the SDK's auth/IAP popups + the
/// `VLPlayLoading` overlay (`UIWindowLevelAlert + 1`).
///
/// Lifecycle: SDK auto-shows the ball after a successful sign-in (when
/// `sdkConfig.fab.enabled == YES`) and auto-dismisses on sign-out. Game hosts
/// can also drive it explicitly via the +show/+dismiss class methods.
@interface VLPlayFAB : NSObject

#pragma mark - Visibility

/// Show the floating ball. Idempotent — no-op when already visible. Safe to
/// call before any window exists (will materialise on the next main-loop tick).
+ (void)show;

/// Hide the floating ball + collapse the HUD if open. Idempotent.
+ (void)dismiss;

/// Show the HUD panel with a specific initial tab. Auto-shows the ball first
/// if not yet visible.
+ (void)showHudWithInitialTab:(VLPlayFABTab)tab;

/// Collapse the HUD panel only; ball stays visible.
+ (void)dismissHud;

/// YES while the ball is on screen.
+ (BOOL)isVisible;

/// YES while the HUD panel is open.
+ (BOOL)isHudVisible;

/// YES when the FAB is enabled AND at least one tab survives the filter.
/// Used by `+[VLPlaySDKManager showHud:]` to decide between "open HUD" and the
/// standalone-popup fallback (spec §9.1).
+ (BOOL)canOpenHud;

/// Re-evaluate CMS state and create / tear down the ball accordingly. Call
/// after every successful `/sdk/config` refresh + every sign-in / sign-out.
/// Idempotent.
+ (void)applyConfigFromCMS;

#pragma mark - Configuration

/// Partner-game context forwarded into Support and analytics.
/// Recognised keys: `serverGameId`, `serverName`, `roleId`, `roleName`.
///
/// Parity Web SDK `window.VLGameSDK.setFABContext({...})`.
+ (void)setContext:(nullable NSDictionary<NSString *, NSString *> *)context;

/// Local override for CMS `sdkConfig.fab` block. Spec §8.2 merge order is
/// **local < CMS** (CMS wins). This method seeds defaults for offline / dev
/// runs (e.g. before CMS ships the `fab` block to BE staging).
///
/// Recognised keys (all optional):
///   `enabled` (NSNumber bool), `position` (NSString `left`|`right`),
///   `iconUrl` (NSString), `tabs` (NSArray<NSString *> in render order:
///   `account` / `support` / `identity`).
///
/// Pass nil or an empty dict to clear the cached override.
+ (void)configure:(nullable NSDictionary *)config;

@end

NS_ASSUME_NONNULL_END
