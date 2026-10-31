Pod::Spec.new do |s|
  s.name         = 'VLPlaySDK'
  s.version      = '1.9.6'
  s.summary      = 'VLPlaySDK core — prebuilt xcframework consumed as a binary (Unity wrapper vendored pod).'
  s.description  = <<-DESC
    Binary-distribution podspec for the core variant of VLPlaySDK, vendored inside
    the vn.vlplay.sdk Unity package (NativeLibs~/iOS/LocalPods). Mirrors the proven
    g01-ios-game consumer pattern. The Unity iOS post-processor (U1) injects this
    pod into the EDM4U-generated Podfile with a resolved absolute path.

    Bump recipe (keep filename VLPlaySDK.xcframework — inner framework name match):
      unzip vlgame-lib-sdk/sdk/ios-sdk/core/<ver>/VLPlaySDK-core-<ver>.xcframework.zip
      mv VLPlaySDK-core.xcframework VLPlaySDK.xcframework
  DESC
  s.homepage     = 'https://sdk.vlplay.vn'
  s.license      = { :type => 'Proprietary', :text => 'Copyright (c) 2016-2026 VL Play. All rights reserved.' }
  s.author       = { 'VL Play' => 'dev@vlplay.vn' }
  # Ignored for :path pods; kept for documentation / spec validation.
  s.source       = { :git => 'https://github.com/VL-Game/vlgame-sdk-main-ios.git', :tag => "core/v#{s.version}" }

  s.ios.deployment_target = '15.0'
  s.swift_versions        = ['5.0']

  # The prebuilt binary. Since native 1.5.0 this is a STATIC framework (U-6):
  # it bakes ZERO vendor classes — AppsFlyer / FirebaseCore / GoogleSignIn
  # arrive through the s.dependency list below and CocoaPods resolves exactly
  # one copy for the whole app, which is the point of the split. On ≤1.4.5 the
  # binary was dynamic with those baked in; do not mix this podspec with an old
  # xcframework or vice versa. (ZMJTipView was a fourth such dependency until
  # native 1.5.1 dropped it — it declared an iOS 8.0 deployment target, below
  # the libarclite floor Xcode 14.3+ removed, so it broke the Pods build.)
  #
  # NOTE: filename MUST match the inner framework name (VLPlaySDK.framework) or
  # CocoaPods derives a wrong -framework flag → "framework not found" (GAP2).
  s.vendored_frameworks = 'VLPlaySDK.xcframework'

  # A static library's Objective-C categories are discarded by the linker unless
  # the CONSUMING target links with -ObjC. This SDK has 28 of them, including
  # the networking layer's NetworkModal (Authen)/(Account) — without the flag
  # the build is green and the first sign-in dies with `unrecognized selector`.
  # user_target_xcconfig puts the flag on UnityFramework (the target EDM4U's
  # Podfile integrates), which is exactly where the link happens.
  s.user_target_xcconfig = { 'OTHER_LDFLAGS' => '-ObjC' }

  # The xcframework nests VLPlaySDKResource.bundle (nibs, pngs, Roboto,
  # ErrorCode/LocalizableString, SVProgressHUD.bundle) inside the framework —
  # but a STATIC framework's wrapper never reaches the app bundle, so CocoaPods
  # must copy the bundle out explicitly. At runtime NSBundle+Additions falls
  # back to the main bundle and finds it there. Resources are arch-agnostic;
  # the device slice's copy serves both.
  s.resources = 'VLPlaySDK.xcframework/ios-arm64/VLPlaySDK.framework/VLPlaySDKResource.bundle'

  s.frameworks = [
    'UIKit', 'Foundation', 'StoreKit', 'WebKit',
    'UserNotifications', 'SafariServices', 'SystemConfiguration',
    'CoreData', 'AddressBook', 'Security', 'AdSupport',
    'AuthenticationServices', 'CoreTelephony',
  ]

  # Every dependency the binary links against — since 1.5.0 NONE of them is
  # baked in, so all must resolve here. Constraints mirror the source podspec
  # (vlgame-sdk-ios/VLPlaySDK.podspec); keep them in lockstep on re-vendor.
  s.dependency 'FBSDKCoreKit',         '~> 18.0'
  s.dependency 'FBSDKLoginKit',        '~> 18.0'
  s.dependency 'FBSDKShareKit',        '~> 18.0'
  s.dependency 'OneSignalXCFramework', '5.5.1'
  s.dependency 'AppsFlyerFramework',   '~> 6.18'
  s.dependency 'FirebaseCore',         '~> 12.12'
  s.dependency 'GoogleSignIn',         '~> 9.1'
end
