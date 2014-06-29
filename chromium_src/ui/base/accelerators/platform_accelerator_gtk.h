// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_ACCELERATORS_PLATFORM_ACCELERATOR_GTK_H_
#define UI_BASE_ACCELERATORS_PLATFORM_ACCELERATOR_GTK_H_

#include <gdk/gdk.h>

#include "base/compiler_specific.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/accelerators/platform_accelerator.h"

namespace ui {

class Accelerator;

// This is a GTK specific class for specifing accelerator keys.
class UI_BASE_EXPORT PlatformAcceleratorGtk : public PlatformAccelerator {
 public:
  PlatformAcceleratorGtk();
  PlatformAcceleratorGtk(guint gdk_key_code, GdkModifierType gdk_modifier);
  virtual ~PlatformAcceleratorGtk();

  // PlatformAccelerator:
  virtual scoped_ptr<PlatformAccelerator> CreateCopy() const OVERRIDE;
  virtual bool Equals(const PlatformAccelerator& rhs) const OVERRIDE;

  guint gdk_key_code() const { return gdk_key_code_; }
  GdkModifierType gdk_modifier() const { return gdk_modifier_; }

 private:
  guint gdk_key_code_;
  GdkModifierType gdk_modifier_;

  DISALLOW_COPY_AND_ASSIGN(PlatformAcceleratorGtk);
};

UI_BASE_EXPORT Accelerator AcceleratorForGdkKeyCodeAndModifier(
    guint gdk_key_code,
    GdkModifierType gdk_modifier);
UI_BASE_EXPORT guint
    GetGdkKeyCodeForAccelerator(const Accelerator& accelerator);
UI_BASE_EXPORT GdkModifierType GetGdkModifierForAccelerator(
    const Accelerator& accelerator);

}  // namespace ui

#endif  // UI_BASE_ACCELERATORS_PLATFORM_ACCELERATOR_GTK_H_
