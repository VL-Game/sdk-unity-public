//
//  VLPlayServiceBanner.h
//  VLPlaySDK
//
//  Service Banner (spec §10A v1.2.2) — top-aligned dismissible card showing
//  a game-wide singleton announcement. Authored by Ops via CMS Notification.
//  NOT part of the FAB / HUD; renders in its own UIWindow above the game
//  but below modal popups.
//
//  Lifecycle:
//   1. SDK init success → +[VLPlayServiceBanner fetchAndShow] (fire-and-
//      forget; called by VLPlaySdkConfigManager right after +[VLPlayFAB
//      applyConfigFromCMS]).
//   2. BE GET /api/v1/notification/{gameId} → render top card if non-empty.
//   3. User taps × → persist `vlsdk_seen_banner_<_id>` flag in
//      NSUserDefaults, dismiss with fade-out.
//   4. Singleton — second fetchAndShow while a banner is mounted is a no-op.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VLPlayServiceBanner : NSObject

/// Fire-and-forget banner fetch + render. Safe to call multiple times —
/// in-flight or mounted banner short-circuits subsequent calls.
+ (void)fetchAndShow;

/// Tear down the banner UI (logout / sessionExpired). Idempotent.
+ (void)dismiss;

@end

NS_ASSUME_NONNULL_END
