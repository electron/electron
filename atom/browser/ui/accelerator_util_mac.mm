// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/accelerator_util.h"

#include "ui/base/accelerators/accelerator.h"
#import "ui/base/accelerators/platform_accelerator_cocoa.h"
#import "ui/events/keycodes/keyboard_code_conversion_mac.h"

namespace accelerator_util {

void SetPlatformAccelerator(ui::Accelerator* accelerator) {
  unichar character;
  unichar characterIgnoringModifiers;

  NSUInteger modifiers =
      (accelerator->IsCtrlDown() ? NSControlKeyMask : 0) |
      (accelerator->IsCmdDown() ? NSCommandKeyMask : 0) |
      (accelerator->IsAltDown() ? NSAlternateKeyMask : 0) |
      (accelerator->IsShiftDown() ? NSShiftKeyMask : 0);

  ui::MacKeyCodeForWindowsKeyCode(accelerator->key_code(),
                                  modifiers,
                                  &character,
                                  &characterIgnoringModifiers);

  if (character != characterIgnoringModifiers) {
    modifiers ^= NSShiftKeyMask;
  }

  if (character == NSDeleteFunctionKey) {
    character = NSDeleteCharacter;
  }

  NSString* characters =
      [[[NSString alloc] initWithCharacters:&character length:1] autorelease];

  std::unique_ptr<ui::PlatformAccelerator> platform_accelerator(
      new ui::PlatformAcceleratorCocoa(characters, modifiers));
  accelerator->set_platform_accelerator(std::move(platform_accelerator));
}

}  // namespace accelerator_util
