// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora, Ltd.  All rights reserved.
 * Copyright (C) 2008, 2009 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// WindowsKeyCodeForGdkKeyCode is copied from platform/gtk/KeyEventGtk.cpp

#include "ui/events/keycodes/keyboard_code_conversion_gtk.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <X11/keysym.h>

#include "base/basictypes.h"
#include "build/build_config.h"
#include "ui/events/keycodes/keyboard_code_conversion_x.h"
#include "ui/events/keycodes/keyboard_codes_posix.h"

namespace ui {

KeyboardCode WindowsKeyCodeForGdkKeyCode(int keycode) {
  // Gdk key codes (e.g. GDK_BackSpace) and X keysyms (e.g. XK_BackSpace) share
  // the same values.
  return KeyboardCodeFromXKeysym(keycode);
}

int GdkKeyCodeForWindowsKeyCode(KeyboardCode keycode, bool shift) {
  // Gdk key codes and X keysyms share the same values.
  return XKeysymForWindowsKeyCode(keycode, shift);
}

// Just in case, test whether Gdk key codes match X ones.
COMPILE_ASSERT(GDK_KP_0 == XK_KP_0, keycode_check);
COMPILE_ASSERT(GDK_A == XK_A, keycode_check);
COMPILE_ASSERT(GDK_Escape == XK_Escape, keycode_check);
COMPILE_ASSERT(GDK_F1 == XK_F1, keycode_check);
COMPILE_ASSERT(GDK_Kanji == XK_Kanji, keycode_check);
COMPILE_ASSERT(GDK_Page_Up == XK_Page_Up, keycode_check);
COMPILE_ASSERT(GDK_Tab == XK_Tab, keycode_check);
COMPILE_ASSERT(GDK_a == XK_a, keycode_check);
COMPILE_ASSERT(GDK_space == XK_space, keycode_check);

int GdkNativeKeyCodeForWindowsKeyCode(KeyboardCode keycode, bool shift) {
  int keyval = GdkKeyCodeForWindowsKeyCode(keycode, shift);
  GdkKeymapKey* keys;
  gint n_keys;

  int native_keycode = 0;
  if (keyval && gdk_keymap_get_entries_for_keyval(0, keyval, &keys, &n_keys)) {
    native_keycode = keys[0].keycode;
    g_free(keys);
  }

  return native_keycode;
}

}  // namespace ui
