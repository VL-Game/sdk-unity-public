// VLPlayLoginInfoModel.h

// To parse this JSON:
//
//   NSError *error;
//   VLPlayLoginInfoModel *loginInfoModel = [VLPlayLoginInfoModel fromJSON:json encoding:NSUTF8Encoding error:&error];

#import <Foundation/Foundation.h>

@class VLPlayLoginInfoModel;
@class VLPlayDataLogin;
@class VLPlayGameInfo;
@class VLPlayGame;
@class VLPlayNotification;
@class VLPlayPlayerLogin;
@class VLPlayTokens;

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Object interfaces

@interface VLPlayLoginInfoModel : NSObject

@property (nonatomic, nullable, strong) VLPlayDataLogin *data;
@property (nonatomic, nullable, copy)   NSString *message;
@property (nonatomic, nullable, strong) NSNumber *statusCode;

+ (_Nullable instancetype)fromJSON:(NSString *)json encoding:(NSStringEncoding)encoding error:(NSError *_Nullable *)error;
+ (_Nullable instancetype)fromData:(NSData *)data error:(NSError *_Nullable *)error;
+ (instancetype)fromJSONDictionary:(NSDictionary *)dict;
- (NSString *_Nullable)toJSON:(NSStringEncoding)encoding error:(NSError *_Nullable *)error;
- (NSData *_Nullable)toData:(NSError *_Nullable *)error;
@end

@interface VLPlayDataLogin : NSObject
@property (nonatomic, nullable, strong) VLPlayGameInfo *gameInfo;
@property (nonatomic, nullable, strong) VLPlayNotification *notification;
@property (nonatomic, nullable, strong) VLPlayPlayerLogin *player;
@property (nonatomic, nullable, strong) VLPlayTokens *tokens;
@property (nonatomic, nullable, copy)   NSString *type;
@end

@interface VLPlayGameInfo : NSObject
@property (nonatomic, nullable, strong) NSNumber *isShow;
@property (nonatomic, nullable, strong) NSNumber *gameState;
@property (nonatomic, nullable, copy)   NSString *fanPage;
@property (nonatomic, nullable, copy)   NSString *hotline;
@property (nonatomic, nullable, copy)   NSString *theID;
@property (nonatomic, nullable, strong) NSNumber *code;
@property (nonatomic, nullable, strong) VLPlayGame *game;
@property (nonatomic, nullable, strong) NSNumber *identifier;
@property (nonatomic, nullable, copy)   NSString *message;
@property (nonatomic, nullable, copy)   NSString *name;
@property (nonatomic, nullable, copy)   NSString *status;
@end

@interface VLPlayGame : NSObject
@property (nonatomic, nullable, copy)   NSString *theID;
@property (nonatomic, nullable, strong) NSNumber *status;
@end

@interface VLPlayNotification : NSObject
@property (nonatomic, nullable, copy) NSString *theID;
@property (nonatomic, nullable, copy) NSString *errorType;
@property (nonatomic, nullable, copy) NSString *message;
@property (nonatomic, nullable, copy) NSString *title;
@end

@interface VLPlayPlayerLogin : NSObject
@property (nonatomic, nullable, copy)   NSString *theID;
@property (nonatomic, nullable, strong) NSNumber *identifier;
@property (nonatomic, nullable, copy)   NSNumber *isUpdatePassword;
@property (nonatomic, nullable, copy)   NSString *password __deprecated_msg("Plaintext password from the login response — do not persist or display.");
@property (nonatomic, nullable, copy)   NSString *username;
@end

@interface VLPlayTokens : NSObject
@property (nonatomic, nullable, copy) NSString *accessToken;
@property (nonatomic, nullable, copy) NSString *refreshToken;
@end

NS_ASSUME_NONNULL_END
