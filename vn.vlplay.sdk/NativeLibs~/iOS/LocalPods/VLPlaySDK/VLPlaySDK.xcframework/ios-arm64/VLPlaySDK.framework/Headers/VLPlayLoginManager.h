//
//  VLPlayLoginManager.h
//  VLPlaySDK
//
//  Created by Admin on 10/23/18.
//  Copyright © 2018 VLPlay. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface VLPlayLoginManager : NSObject
+ (instancetype) sharedManager;
- (void) autoSignInFrom:(nullable UIViewController *)viewController;
- (void) autoSignInFacebookFrom:(nullable UIViewController *)viewController;
- (void) autoSignInGoogleFrom:(nullable UIViewController *)viewController;
- (void) autosignInGoogleFrom:(nullable UIViewController *)viewController __deprecated_msg("Use -autoSignInGoogleFrom: (Sprint E-2 rename for camel-case parity)");
- (void) autoQuickStartFrom:(nullable UIViewController *)viewController;
@end

NS_ASSUME_NONNULL_END
