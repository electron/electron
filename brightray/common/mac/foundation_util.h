#ifndef BRIGHTRAY_COMMON_MAC_FOUNDATION_UTIL_H_
#define BRIGHTRAY_COMMON_MAC_FOUNDATION_UTIL_H_

// This header exists to work around incompatibilities between
// base/mac/foundation_util.h and the 10.8 SDK.

#import <Foundation/Foundation.h>

// base/mac/foundation_util.h contains an incompatible declaration of
// NSSearchPathDirectory, so here we #define it to be something else.
#define NSSearchPathDirectory NSSearchPathDirectory___PRE_10_8
#import "base/mac/foundation_util.h"
#undef NSSearchPathDirectory

#endif
