// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include "atom/common/mouse_util.h"

using Cursor = blink::WebCursorInfo::Type;

namespace atom {

std::string CursorTypeToString(const content::CursorInfo& info) {
  switch (info.type) {
    case Cursor::kTypePointer: return "default";
    case Cursor::kTypeCross: return "crosshair";
    case Cursor::kTypeHand: return "pointer";
    case Cursor::kTypeIBeam: return "text";
    case Cursor::kTypeWait: return "wait";
    case Cursor::kTypeHelp: return "help";
    case Cursor::kTypeEastResize: return "e-resize";
    case Cursor::kTypeNorthResize: return "n-resize";
    case Cursor::kTypeNorthEastResize: return "ne-resize";
    case Cursor::kTypeNorthWestResize: return "nw-resize";
    case Cursor::kTypeSouthResize: return "s-resize";
    case Cursor::kTypeSouthEastResize: return "se-resize";
    case Cursor::kTypeSouthWestResize: return "sw-resize";
    case Cursor::kTypeWestResize: return "w-resize";
    case Cursor::kTypeNorthSouthResize: return "ns-resize";
    case Cursor::kTypeEastWestResize: return "ew-resize";
    case Cursor::kTypeNorthEastSouthWestResize: return "nesw-resize";
    case Cursor::kTypeNorthWestSouthEastResize: return "nwse-resize";
    case Cursor::kTypeColumnResize: return "col-resize";
    case Cursor::kTypeRowResize: return "row-resize";
    case Cursor::kTypeMiddlePanning: return "m-panning";
    case Cursor::kTypeEastPanning: return "e-panning";
    case Cursor::kTypeNorthPanning: return "n-panning";
    case Cursor::kTypeNorthEastPanning: return "ne-panning";
    case Cursor::kTypeNorthWestPanning: return "nw-panning";
    case Cursor::kTypeSouthPanning: return "s-panning";
    case Cursor::kTypeSouthEastPanning: return "se-panning";
    case Cursor::kTypeSouthWestPanning: return "sw-panning";
    case Cursor::kTypeWestPanning: return "w-panning";
    case Cursor::kTypeMove: return "move";
    case Cursor::kTypeVerticalText: return "vertical-text";
    case Cursor::kTypeCell: return "cell";
    case Cursor::kTypeContextMenu: return "context-menu";
    case Cursor::kTypeAlias: return "alias";
    case Cursor::kTypeProgress: return "progress";
    case Cursor::kTypeNoDrop: return "nodrop";
    case Cursor::kTypeCopy: return "copy";
    case Cursor::kTypeNone: return "none";
    case Cursor::kTypeNotAllowed: return "not-allowed";
    case Cursor::kTypeZoomIn: return "zoom-in";
    case Cursor::kTypeZoomOut: return "zoom-out";
    case Cursor::kTypeGrab: return "grab";
    case Cursor::kTypeGrabbing: return "grabbing";
    case Cursor::kTypeCustom: return "custom";
    default: return "default";
  }
}

}  // namespace atom
