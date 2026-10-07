//
//  AntiAddictionStatus.h
//  VLPlaySDK
//
//  Server-driven anti-addiction status from GET /api/v1/anti-addiction/status.
//  Immutable after construction.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface AntiAddictionStatus : NSObject

@property (nonatomic, copy, readonly) NSString *ageGroup; // "underage", "teen", "adult"
@property (nonatomic, assign, readonly) NSInteger totalPlayedTodayMinutes;
@property (nonatomic, assign, readonly) NSInteger remainingMinutesToday;
@property (nonatomic, assign, readonly) NSInteger currentSessionMinutes;
@property (nonatomic, assign, readonly) NSInteger remainingSessionMinutes;
@property (nonatomic, assign, readonly) BOOL curfewActive;
@property (nonatomic, copy, readonly, nullable) NSString *nextCurfewStart;
@property (nonatomic, assign, readonly) BOOL shouldWarn;
@property (nonatomic, assign, readonly) BOOL shouldKick;
@property (nonatomic, copy, readonly, nullable) NSString *kickReason;
// Server-authoritative fields (Decree 147 cooldown/warning batch, 2026-07).
@property (nonatomic, copy, readonly, nullable) NSString *warningMessage; // custom warn text
@property (nonatomic, copy, readonly, nullable) NSString *cooldownUntil;  // ISO8601 forced-break end

+ (nullable instancetype)fromDictionary:(NSDictionary *)dict;

@end

NS_ASSUME_NONNULL_END
