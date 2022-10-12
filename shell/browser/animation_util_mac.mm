// Copyright (c) 2022 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/animation_util.h"

#import <QuartzCore/QuartzCore.h>

// Disables actions within a scope.
ScopedCAActionDisabler::ScopedCAActionDisabler() {
  [CATransaction begin];
  [CATransaction setDisableActions:YES];
}

ScopedCAActionDisabler::~ScopedCAActionDisabler() {
  [CATransaction commit];
}
