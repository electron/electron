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

#ifndef UI_EVENTS_KEYCODES_KEYBOARD_CODE_CONVERSION_GTK_H_
#define UI_EVENTS_KEYCODES_KEYBOARD_CODE_CONVERSION_GTK_H_

#include "ui/events/events_base_export.h"
#include "ui/events/keycodes/keyboard_codes_posix.h"

typedef struct _GdkEventKey GdkEventKey;

namespace ui {

EVENTS_BASE_EXPORT KeyboardCode WindowsKeyCodeForGdkKeyCode(int keycode);

EVENTS_BASE_EXPORT int GdkKeyCodeForWindowsKeyCode(KeyboardCode keycode,
                                                   bool shift);

// For WebKit DRT testing: simulate the native keycode for the given
// input |keycode|.  Return the native keycode.
EVENTS_BASE_EXPORT int GdkNativeKeyCodeForWindowsKeyCode(KeyboardCode keycode,
                                                         bool shift);

} // namespace ui

#endif  // UI_EVENTS_KEYCODES_KEYBOARD_CODE_CONVERSION_GTK_H_
