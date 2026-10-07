// To parse this JSON:
//
//   NSError *error;
//   VLPlayDetailAccountInfoModel *detailAccountInfoModel = [VLPlayDetailAccountInfoModel fromJSON:json encoding:NSUTF8Encoding error:&error];

#import <Foundation/Foundation.h>

@class VLPlayDetailAccountInfoModel;
@class VLPlayData;
@class VLPlayPlayer;

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Object interfaces

@interface VLPlayDetailAccountInfoModel : NSObject
@property (nonatomic, nullable, strong) VLPlayData *data;
@property (nonatomic, nullable, copy)   NSString *message;
@property (nonatomic, nullable, strong) NSNumber *statusCode;

+ (_Nullable instancetype)fromJSON:(NSString *)json encoding:(NSStringEncoding)encoding error:(NSError *_Nullable *)error;
+ (instancetype)fromJSONDictionary:(NSDictionary *)dict;
+ (_Nullable instancetype)fromData:(NSData *)data error:(NSError *_Nullable *)error;
- (NSString *_Nullable)toJSON:(NSStringEncoding)encoding error:(NSError *_Nullable *)error;
- (NSData *_Nullable)toData:(NSError *_Nullable *)error;
@end

@interface VLPlayData : NSObject
@property (nonatomic, nullable, strong) VLPlayPlayer *player;
@end

@interface VLPlayPlayer : NSObject
@property (nonatomic, nullable, copy)   NSString *theID;
@property (nonatomic, nullable, copy)   NSString *dob;
@property (nonatomic, nullable, copy)   NSString *email;
@property (nonatomic, nullable, copy)   NSString *fullName;
@property (nonatomic, nullable, copy)   NSString *identifier;
@property (nonatomic, nullable, copy)   NSString *identityCard;
@property (nonatomic, nullable, strong) NSNumber *identityVerified;   // BE durable flag (PR #94) — authoritative "has verified"
/// Backend's contact-verification flags, straight off `GET /detail`.
///
/// `isVerified` is the EMAIL one and `isPhoneVerified` the phone one — named
/// that way in the Player schema, not by us. They are what the backend itself
/// branches on: `account/contact/change/request-old` refuses with 1066 unless
/// `contact != nil && flag`, and `bind/request` refuses with 1067 when it is.
/// Nil means the field was absent from this response, which is not the same as
/// false — see -vlp_hasVerifiedContactOfType: for how that is treated.
@property (nonatomic, nullable, strong) NSNumber *isVerified;
@property (nonatomic, nullable, strong) NSNumber *isPhoneVerified;
@property (nonatomic, nullable, strong) NSNumber *isBanned;
@property (nonatomic, nullable, strong) NSNumber *isUpdatePassword;
// Straight answer to "does this account have a password", computed backend-side
// from the (select:false, never emitted) password hash. Prefer it over inferring
// from isUpdatePassword + the social ids: that inference is wrong for an
// email+password account that later linked a social provider, which keeps
// isUpdatePassword == false while genuinely having a password.
// nil when talking to a backend from before the field shipped — fall back then.
@property (nonatomic, nullable, strong) NSNumber *hasPassword;
// Social provider ids. Present (select:true) on every /detail payload; combined
// with isUpdatePassword they answer "does this account have a password at all".
@property (nonatomic, nullable, copy)   NSString *ggId;
@property (nonatomic, nullable, copy)   NSString *fbId;
@property (nonatomic, nullable, copy)   NSString *appleId;
@property (nonatomic, nullable, copy)   NSString *issueDate;
@property (nonatomic, nullable, copy)   NSString *phone;
@property (nonatomic, nullable, copy)   NSString *placeOfGrant;
@property (nonatomic, nullable, copy)   NSString *address;
@property (nonatomic, nullable, copy)   NSString *username;



@end

NS_ASSUME_NONNULL_END
