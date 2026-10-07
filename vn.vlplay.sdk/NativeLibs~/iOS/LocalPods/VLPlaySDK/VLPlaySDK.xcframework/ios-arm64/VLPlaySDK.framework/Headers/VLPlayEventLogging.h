//
//  VLPlayEventLogging.h
//  VLPlaySDK
//
//   7/29/16.
//  
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface VLPlayEventLogging : NSObject

@property (nonatomic, copy) NSString *currentGAId;
@property (nonatomic, copy) NSString *currentGAClientId;
@property (nonatomic) TrackingHitType hitType;

@property (nonatomic, copy) NSString *gcId;
@property (nonatomic, copy) NSString *gcLabel;
@property (nonatomic, copy) NSString *gcValue;

+ (VLPlayEventLogging *)manager;

- (void)hitActivity:(APICategoryType)activity
            hitType:(TrackingHitType)hitType
           category:(GACategoryType)category
             action:(GAActionType)action
              label:(GALabelType)label
              value:(NSNumber *)value
         completion:(void (^)(BOOL status, id _Nullable responsedObject, NSError * _Nullable error))completionBlock;

- (void)hitCustomActivity:(NSInteger)activity
                 category:(NSString *)category
                   action:(NSString *)action
                    label:(NSString *)label
                    value:(NSNumber *)value
               completion:(void (^)(BOOL status, id _Nullable responsedObject, NSError * _Nullable error))completionBlock;

- (void)hitActivity:(APICategoryType)activity extendData:(NSString *)extend amount:(NSInteger)amountNumber forUser:(NSString *)userName userId:(NSString *)userId completion:(void (^)(BOOL status, id _Nullable responsedObject, NSError * _Nullable error))completionBlock;
- (void)logStartInAppWithOrderNo:(NSString *)orderNo forUser:(NSString *)userName userId:(NSString *)userId completion:(void (^)(BOOL status, id _Nullable responsedObject, NSError * _Nullable error))completionBlock;
- (void)logFinishInAppWithOrderNo:(NSString *)orderNo forUser:(NSString *)userName userId:(NSString *)userId completion:(void (^)(BOOL status, id _Nullable responsedObject, NSError * _Nullable error))completionBlock;

- (NSString *)getCurrentGAId;
- (NSString *)getCurrentGAClientId;

- (void)startGoogleConversionWithID:(NSString *)idString label:(NSString *)label value:(NSString *)value;

+ (NSString *)getGACategoryTypeString:(GACategoryType)type;
+ (NSString *)getGAActionTypeString:(GAActionType)type;
+ (NSString *)getGALabelTypeString:(GALabelType)type;
+ (NSString *)getAPICategoryTypeString:(APICategoryType)type;

- (void)handleApplication:(UIApplication *)application openURL:(NSURL *)url
        sourceApplication:(NSString *)sourceApplication annotation:(id)annotation;

- (void)configAppsFlyer;
- (void)setAppsFlyerInAppEvent:(NSString *)eventName values:(NSDictionary *)valueOfEvent;

// MARK: Checking With FireBase
+ (void)checkingByFireBaseWithEventName:(NSString *)eventName;

@end

NS_ASSUME_NONNULL_END
