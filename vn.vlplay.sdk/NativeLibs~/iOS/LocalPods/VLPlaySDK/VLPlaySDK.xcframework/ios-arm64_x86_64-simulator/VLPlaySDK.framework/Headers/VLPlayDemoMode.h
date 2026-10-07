//
//  VLPlayDemoMode.h
//  VLPlaySDK
//
//  Demo mode for smoke testing without a live backend.
//  Returns mock API responses for non-purchase SDK flows.
//  Purchases are never mocked: iOS IAP always uses Apple StoreKit.
//
//  Only works in debug builds. Enable via VLPlaySDK-Info.plist: sdk_demo_mode = YES
//  or programmatically: [VLPlayDemoMode setEnabled:YES]
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VLPlayDemoMode : NSObject

+ (BOOL)isEnabled;
+ (void)setEnabled:(BOOL)enabled;

/// Returns a mock response dictionary for the given API path, or nil if not mocked.
+ (nullable NSDictionary *)mockResponseForPath:(NSString *)requestPath;

/// Returns a mock server config response dictionary.
+ (NSDictionary *)mockConfigResponse;

// Anti-addiction mocks
+ (NSDictionary *)mockAAConfigData;
+ (NSDictionary *)mockAAStatusData;
+ (NSDictionary *)mockAAStatusWarnData;
+ (NSDictionary *)mockAAStatusKickDataWithReason:(NSString *)reason;

@end

NS_ASSUME_NONNULL_END
