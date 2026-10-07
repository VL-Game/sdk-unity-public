//
//  AntiAddictionManager.h
//  VLPlaySDK
//
//  Server-driven anti-addiction manager (Decree 147/2024/NĐ-CP).
//  SDK sends heartbeats, polls status, shows warning/kick popups.
//  All time-tracking logic is server-side.
//

#import <Foundation/Foundation.h>
#import "AntiAddictionConfig.h"
#import "AntiAddictionStatus.h"

NS_ASSUME_NONNULL_BEGIN

@protocol AntiAddictionDelegate <NSObject>
- (void)antiAddictionDidLoadConfig:(AntiAddictionConfig *)config;
- (void)antiAddictionDidReceiveStatus:(AntiAddictionStatus *)status;
@optional
/// SDK-internal (the SDK is this delegate). A polled status names the session it was fetched
/// for, so a kick acts on that session only — never on a player who signed in after the poll
/// went out. When implemented it is called instead of -antiAddictionDidReceiveStatus: for polls.
- (void)antiAddictionDidReceiveStatus:(AntiAddictionStatus *)status forSession:(NSUInteger)session;
@end

@interface AntiAddictionManager : NSObject

+ (instancetype)shared;

@property (nonatomic, weak, nullable) id<AntiAddictionDelegate> delegate;
@property (nonatomic, strong, readonly, nullable) AntiAddictionConfig *config;
@property (nonatomic, strong, readonly, nullable) AntiAddictionStatus *lastStatus;
@property (nonatomic, assign, readonly) BOOL sessionActive;

- (void)initWithContext;
- (void)startSession;
- (void)stopSession;
- (void)checkStatusNow;

#pragma mark - Warning popup state

// VLPlaySDKManager checks this flag to debounce per-session warning popups (avoid spam every poll).
@property (atomic, assign) BOOL warningShownThisSession;

#pragma mark - Debug controls (demo / SDK debugger only)
// Inject a mock status with shouldWarn=YES → fires delegate → host shows warn popup.
- (void)debugForceWarnPopup;
// Inject a mock status with shouldKick=YES → fires delegate → host shows kick popup
// (and signs the user out, mirroring real kick behaviour).
- (void)debugForceKickPopup;

@end

NS_ASSUME_NONNULL_END
