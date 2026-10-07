//
//  VIDUser.h
//  VLPlaySDK
//
//   7/25/16.
//  
//

#import <Foundation/Foundation.h>
#import "VLPlayLoginInfoModel.h"
#import "VLPlayDetailAccountInfoModel.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, AuthenType) {
    AuthenTypeNormal = 0,
    AuthenTypeFacebook = 1,
    AuthenTypeGoogle = 2,
    AuthenTypeYahoo = 3,
    AuthenTypeApple = 7,
    AuthenTypeQuickStart = 8,
    AuthenTypeNone = -1
};

// Sprint E-1 (2026-05-25): NS_ASSUME_NONNULL applied; nearly every NSString
// here is conceptually nullable (token absent pre-signin, mobile/email may
// be empty per account). Marked explicitly with `nullable` qualifier.
@interface VIDUser : NSObject

@property (nonatomic, copy, nullable) NSString *quickPlayUser;
@property (nonatomic, copy, nullable) NSString *quickPlayPassword;
@property (nonatomic, copy, nullable) NSString *googleSignInToken;
@property (nonatomic, copy, nullable) NSString *appleSignInToken;
@property (nonatomic) BOOL signedIn;
@property (nonatomic, copy, nullable) NSString *accountUsingMobile;
@property (nonatomic, copy, nullable) NSString *extend;
@property (nonatomic, copy, nullable) NSString *userId;
@property (nonatomic, copy, nullable) NSString *userId2;
@property (nonatomic, copy, nullable) NSString *accessToken;
@property (nonatomic, copy, nullable) NSString *refreshToken;
@property (nonatomic, copy, nullable) NSString *userName;
@property (nonatomic) NSTimeInterval expiration;
@property (nonatomic) AuthenType authenType;
//@property (nonatomic) NSInteger vcoinBalance;
@property (nonatomic, copy, nullable) NSString *avatarURL;
@property (nonatomic, copy, nullable) NSString *currentGameVer;
@property (nonatomic) long currentGameVerNumber;
@property (nonatomic, copy, nullable) NSString *email;
@property (nonatomic, copy, nullable) NSString *mobile;
@property (nonatomic) NSInteger userStatus;
@property (nonatomic, nullable) VLPlayLoginInfoModel *loginInfoModel;
@property (nonatomic, nullable) VLPlayDetailAccountInfoModel *detailAccountInfoModel;
+ (VIDUser *)currentUser;

- (void)loadData:(nullable NSDictionary *)userInfo;
- (void)resetData;
- (NSDictionary *)exportUserInfo;

@end

NS_ASSUME_NONNULL_END
