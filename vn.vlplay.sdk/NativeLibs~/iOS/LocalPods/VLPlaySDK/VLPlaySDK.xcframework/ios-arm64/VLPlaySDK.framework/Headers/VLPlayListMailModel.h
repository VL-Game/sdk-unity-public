// To parse this JSON:
//
//   NSError *error;
//   VLPlayListMailModel *listMailModel = [VLPlayListMailModel fromJSON:json encoding:NSUTF8Encoding error:&error];

#import <Foundation/Foundation.h>

@class VLPlayListMailModel;
@class VLPlayMailData;
@class VLPlayResult;
@class VLPlayMailID;

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Object interfaces

@interface VLPlayListMailModel : NSObject
@property (nonatomic, nullable, strong) NSNumber *statusCode;
@property (nonatomic, nullable, copy)   NSString *message;
@property (nonatomic, nullable, strong) VLPlayMailData *data;

+ (_Nullable instancetype)fromJSON:(NSString *)json encoding:(NSStringEncoding)encoding error:(NSError *_Nullable *)error;
+ (instancetype)fromJSONDictionary:(NSDictionary *)dict;
+ (_Nullable instancetype)fromData:(NSData *)data error:(NSError *_Nullable *)error;
- (NSString *_Nullable)toJSON:(NSStringEncoding)encoding error:(NSError *_Nullable *)error;
- (NSData *_Nullable)toData:(NSError *_Nullable *)error;
@end

@interface VLPlayMailData : NSObject
@property (nonatomic, nullable, copy)   NSArray<VLPlayResult *> *result;
@property (nonatomic, nullable, strong) NSNumber *unreadCount;
@end

@interface VLPlayResult : NSObject
@property (nonatomic, nullable, copy)   NSString *theID;
@property (nonatomic, nullable, copy)   NSString *deviceType;
@property (nonatomic, nullable, strong) VLPlayMailID *mailID;
@property (nonatomic, nullable, strong) NSNumber *isRead;
@property (nonatomic, nullable, strong) NSNumber *isShow;
@property (nonatomic, nullable, copy)   NSString *createdAt;
@property (nonatomic, nullable, copy)   NSString *updatedAt;
@end

@interface VLPlayMailID : NSObject
@property (nonatomic, nullable, copy) NSString *theID;
@property (nonatomic, nullable, copy) NSString *title;
@property (nonatomic, nullable, copy) NSString *message;
@property (nonatomic, nullable, copy) NSString *userName;
@end

NS_ASSUME_NONNULL_END
