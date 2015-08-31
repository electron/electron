// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_EVENT_TYPES_H_
#define ATOM_COMMON_EVENT_TYPES_H_

namespace atom {

namespace event_types {

extern const char kMouseDown[];
extern const char kMouseUp[];
extern const char kMouseMove[];
extern const char kMouseEnter[];
extern const char kMouseLeave[];
extern const char kContextMenu[];
extern const char kMouseWheel[];

extern const char kKeyDown[];
extern const char kKeyUp[];
extern const char kChar[];

extern const char kMouseLeftButton[];
extern const char kMouseRightButton[];
extern const char kMouseMiddleButton[];

extern const char kModifierLeftButtonDown[];
extern const char kModifierMiddleButtonDown[];
extern const char kModifierRightButtonDown[];

extern const char kModifierShiftKey[];
extern const char kModifierControlKey[];
extern const char kModifierAltKey[];
extern const char kModifierMetaKey[];
extern const char kModifierCapsLockOn[];
extern const char kModifierNumLockOn[];

extern const char kModifierIsKeyPad[];
extern const char kModifierIsAutoRepeat[];
extern const char kModifierIsLeft[];
extern const char kModifierIsRight[];

}  // namespace switches

}  // namespace atom

#endif  // ATOM_COMMON_EVENT_TYPES_H_
