// SPDX-License-Identifier: MIT
// Objective-C bridge for iOS. Import in bridging header.
#pragma once
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HymtTranslator : NSObject
- (nullable instancetype)initWithModel:(NSString *)path threads:(int)t gpuLayers:(int)g error:(NSError **)err;
- (nullable NSString *)translate:(NSString *)text from:(nullable NSString *)src to:(NSString *)tgt error:(NSError **)err;
@end

NS_ASSUME_NONNULL_END
