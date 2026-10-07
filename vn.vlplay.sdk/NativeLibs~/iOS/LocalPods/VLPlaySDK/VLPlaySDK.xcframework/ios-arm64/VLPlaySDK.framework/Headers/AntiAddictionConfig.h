//
//  AntiAddictionConfig.h
//  VLPlaySDK
//
//  Server-driven anti-addiction configuration from GET /api/v1/anti-addiction/config.
//  Immutable after construction.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AntiAddictionAgeGroup : NSObject
@property (nonatomic, copy, readonly) NSString *name;
@property (nonatomic, assign, readonly) NSInteger minAge;
@property (nonatomic, assign, readonly) NSInteger maxAge; // -1 = no upper limit
- (instancetype)initWithName:(NSString *)name minAge:(NSInteger)minAge maxAge:(NSInteger)maxAge;
@end

@interface AntiAddictionConfig : NSObject

@property (nonatomic, assign, readonly) BOOL enabled;
@property (nonatomic, assign, readonly) NSInteger underageMaxMinutesPerDay;
@property (nonatomic, assign, readonly) NSInteger underageMaxMinutesPerSession;
@property (nonatomic, assign, readonly) NSInteger teenMaxMinutesPerDay;
@property (nonatomic, assign, readonly) NSInteger teenMaxMinutesPerSession;
@property (nonatomic, assign, readonly) NSInteger adultMaxMinutesPerDay; // -1 = unlimited
@property (nonatomic, copy, readonly) NSArray<AntiAddictionAgeGroup *> *ageGroups;
@property (nonatomic, assign, readonly) BOOL curfewEnabled;
@property (nonatomic, assign, readonly) NSInteger curfewStartHour;
@property (nonatomic, assign, readonly) NSInteger curfewEndHour;
@property (nonatomic, copy, readonly) NSArray<NSString *> *curfewAppliesTo;
@property (nonatomic, assign, readonly) NSInteger warningIntervalMinutes;
@property (nonatomic, copy, readonly, nullable) NSString *warningMessage;
@property (nonatomic, assign, readonly) NSInteger cooldownHours;

+ (nullable instancetype)fromDictionary:(NSDictionary *)dict;
- (NSDictionary *)toDictionary;

@end

NS_ASSUME_NONNULL_END
