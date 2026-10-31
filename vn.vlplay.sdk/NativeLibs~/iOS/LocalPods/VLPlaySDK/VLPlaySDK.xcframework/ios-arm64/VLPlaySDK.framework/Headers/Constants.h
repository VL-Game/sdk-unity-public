//
//  Constants.h
//  VLPlaySDK
//
//   11/22/16.
//  
//

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

//! Project version number for VLPlaySDK

#define kVLPlaySDKVersionString @"1.9.6"

#ifdef DEBUG
#define VLPlayLog(FORMAT, ...) NSLog((@"[VLPlaySDK] [%s:%d] " FORMAT), __PRETTY_FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define VLPlayLog(FORMAT, ...)
#endif

#define ColorFromHEX(rgbValue) [UIColor colorWithRed:((float)((rgbValue & 0xFF0000) >> 16))/255.0 green:((float)((rgbValue & 0xFF00) >> 8))/255.0 blue:((float)(rgbValue & 0xFF))/255.0 alpha:1.0]
#define ColorFromAlphaHEX(rgbValue, alphaValue) [UIColor colorWithRed:((float)((rgbValue & 0xFF0000) >> 16))/255.0 green:((float)((rgbValue & 0xFF00) >> 8))/255.0 blue:((float)(rgbValue & 0xFF))/255.0 alpha:(float)alphaValue]

#define VLPlayLocalizeString(string) [Utilities localizeStringForString:string]

static NSString *kUserDefaultLoadedBufferLink = @"UserDefaultLoadedBufferLink";
static NSString *kUserDefaultFirstOpen = @"UserDefaultFirstOpenKey";
static NSString *kUserDefaultCachedUser = @"UserDefaultCachedUser";
static NSString *kUserDefaultCachedUTM = @"UserDefaultCachedUTM";
static NSString *kUserDefaultCachedNewRegisteredUser = @"UserDefaultCachedNewRegisteredUser";
static NSString *kUserDefaultStackOfIAPPayment = @"UserDefaultStackOfIAPPayment";
static NSString *kUserDefaultStackOfIAPFinishLog = @"UserDefaultStackOfIAPFinishLog";
static NSString *kUserDefaultSdkConfig = @"vlplay_sdk_config_v2";

static NSString *kKeychainAdvertisingIdentify = @"Ads_Id";
static NSString *kKeychainLoggedInstall = @"cachedInstall";
// BE-GUEST-02: stable per-device fingerprint sent on POST /signup/guest so BE
// can dedup repeated guest creation from the same device. Keychain survives
// app uninstall on iOS — keeps the mapping stable across reinstalls.
static NSString *kKeychainDeviceFingerprint = @"DeviceFingerprint";

static NSString *kNotificationCheckingWithFireBase = @"NotificationCheckingWithFireBase";
static NSString *kNotificationOpenBufferLink = @"NotificationOpenBufferLink";
static NSString *kNotificationGotFacebookGoogleAuthenConfig = @"NotificationGotFacebookGoogleAuthenConfig";

static NSString *generalDeviceType = @"3";

static CGFloat kWidthOfLandscapeSocialLoginButton = 50.0;

static NSString *kStartLogInApp = @"START_PAY_STORE";
static NSString *kFinishLogInApp = @"FINISH_PAY_STORE";

typedef NS_ENUM(NSInteger, SDKLanguage) {
    SDKLanguageViet = 0,
    SDKLanguageEnglish,
    SDKLanguagePhilipine,
    SDKLanguageThailand,
    SDKLanguageLaos,
    SDKLanguageCambodia,
    SDKLanguageMyanmar,
    SDKLanguageMalaysia,
    SDKLanguageIndonesia
};

typedef NS_ENUM(NSInteger, TrackingHitType) {
    TrackingHitTypeEvent = 0,
    TrackingHitTypeScreen
};

typedef NS_ENUM(NSInteger, GACategoryType) {
    GACategoryTypeInAppEvent = 0,
    GACategoryTypeAuthen,
    GACategoryTypeTracking,
    GACategoryTypePayment
};

typedef NS_ENUM(NSInteger, GAActionType) {
    GAActionTypeFirstOpen = 0,
    GAActionTypeOpenApp,
    GAActionTypeCloseApp,
    GAActionTypeCrashApp,
    GAActionTypeInstalledApp,
    GAActionTypeLogin,
    GAActionTypeRegister,
    GAActionTypeUTMCampaign,
    GAActionTypeUTMMedium,
    GAActionTypeUTMSource,
    GAActionTypeInGame
};

typedef NS_ENUM(NSInteger, GALabelType) {
    GALabelTypeNone = 0,
    GALabelTypeVLPlayID,
    GALabelTypeFacebook,
    GALabelTypeApple,
    GALabelTypeGoogle,
    GALabelTypeVLPlay,
    GALabelTypeUTM,
    GALabelTypeVLPlayPay,
    GALabelTypeInAppPurchase
};

typedef NS_ENUM(NSInteger, APICategoryType) {
    APICategoryTypeFirstOpen = 1,
    APICategoryTypeOpen = 2,
    APICategoryTypeCloseApp = 3,
    APICategoryTypeCrashApp = 4,
    APICategoryTypeClick = 5,
    APICategoryTypePayment = 6,
    APICategoryTypeInstall = 7,
    APICategoryTypeLoginVLPlay = 8,
    APICategoryTypeQuickStartLoginVLPlay = 12,
    APICategoryTypeLoginGoogle = 9,
    APICategoryTypeLoginFacebook = 10,
    APICategoryTypeLoginApple= 11,
    APICategoryTypeRegisterVLPlay = 13,
    APICategoryTypeOther = 0
};

/*
 * Legacy AppsFlyer name constants — DEPRECATED, and shipped only for source
 * compatibility.
 *
 * Nothing in this SDK reads them: measured 2026-08-31, the only four references
 * outside this file are inside commented-out code in VLPlaySignUpViewController.m.
 * They are kept because Constants.h is a PUBLIC header — it is listed in
 * VLPlaySDK.podspec's public_header_files and imported by the umbrella
 * VLPlaySDK.h — so deleting them is a source-breaking change for any integrator
 * who referenced one, not a mechanical cleanup. Removal is scheduled for the
 * next MAJOR release.
 *
 * They are also why af_login / af_purchase / af_complete_registration show up in
 * every grep as though iOS fires them. It does not; that is what made the
 * 2026-05-31 parity matrix wrong in the other direction. Canonical event names
 * and the AppsFlyer key each platform actually sends are generated from
 * vlgame-ads-hub into VLPlayTrackingEvents — read them there.
 */
#ifndef VLPLAY_DEPRECATED_AF_CONST
#define VLPLAY_DEPRECATED_AF_CONST __attribute__((deprecated("Unused by the VLPlay SDK; scheduled for removal in the next major release. Use the generated VLPlayTrackingEvents (VLPlayAppsFlyerKeyMap) instead.")))
#endif

static NSString *kAppsFlyerEventTypeLogin VLPLAY_DEPRECATED_AF_CONST = @"af_login";
static NSString *kAppsFlyerEventTypeGMOLogin VLPLAY_DEPRECATED_AF_CONST = @"gmo_login";
static NSString *kAppsFlyerEventTypeRegister VLPLAY_DEPRECATED_AF_CONST = @"af_complete_registration";
static NSString *kAppsFlyerEventTypeCompleteTutorial VLPLAY_DEPRECATED_AF_CONST = @"af_tutorial_completion";
static NSString *kAppsFlyerEventTypeLevelAchieved VLPLAY_DEPRECATED_AF_CONST = @"af_level_achieved";
static NSString *kAppsFlyerEventTypeInitialCheckout VLPLAY_DEPRECATED_AF_CONST = @"af_initiated_checkout";
static NSString *kAppsFlyerEventTypePurchase VLPLAY_DEPRECATED_AF_CONST = @"af_purchase";

// on Apple
static NSString *kAppsFlyerEventTypeAppleRegister VLPLAY_DEPRECATED_AF_CONST = @"af_Apple_complete_registration";
// on QuickStart
static NSString *kAppsFlyerEventTypeQuickStartRegister VLPLAY_DEPRECATED_AF_CONST = @"af_quickstart_complete_registration";
// on facebook
static NSString *kAppsFlyerEventTypeFBRegister VLPLAY_DEPRECATED_AF_CONST = @"af_fb_complete_registration";
// Google
static NSString *kAppsFlyerEventTypeGGRegister VLPLAY_DEPRECATED_AF_CONST = @"af_gg_complete_registration";

static NSString *kAppsFlyerEventTypeLoginValueKey VLPLAY_DEPRECATED_AF_CONST = @"af_login";
static NSString *kAppsFlyerEventTypeGMOLoginValueKey VLPLAY_DEPRECATED_AF_CONST = @"gmo_login";
static NSString *kAppsFlyerEventTypeRegisterValueKey VLPLAY_DEPRECATED_AF_CONST = @"af_complete_registration";
static NSString *kAppsFlyerEventTypeCompleteTutorialValueKey VLPLAY_DEPRECATED_AF_CONST = @"af_tutorial_completion";
static NSString *kAppsFlyerEventTypeLevelAchievedValueKey VLPLAY_DEPRECATED_AF_CONST = @"af_level_achieved";
static NSString *kAppsFlyerEventTypeInitialCheckoutValueKey VLPLAY_DEPRECATED_AF_CONST = @"af_initiated_checkout";
static NSString *kAppsFlyerEventTypePurchaseValueKey VLPLAY_DEPRECATED_AF_CONST = @"af_purchase";

NS_ASSUME_NONNULL_END
