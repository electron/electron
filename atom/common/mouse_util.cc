// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include "atom/common/mouse_util.h"

using Cursor = blink::WebCursorInfo::Type;

namespace atom {

std::string CursorTypeToString(const content::WebCursor::CursorInfo& info) {
  switch (info.type) {
    case Cursor::TypePointer: return "default";
    case Cursor::TypeCross: return "crosshair";
    case Cursor::TypeHand: return "pointer";
    case Cursor::TypeIBeam: return "text";
    case Cursor::TypeWait: return "wait";
    case Cursor::TypeHelp: return "help";
    case Cursor::TypeEastResize: return "e-resize";
    case Cursor::TypeNorthResize: return "n-resize";
    case Cursor::TypeNorthEastResize: return "ne-resize";
    case Cursor::TypeNorthWestResize: return "nw-resize";
    case Cursor::TypeSouthResize: return "s-resize";
    case Cursor::TypeSouthEastResize: return "se-resize";
    case Cursor::TypeSouthWestResize: return "sw-resize";
    case Cursor::TypeWestResize: return "w-resize";
    case Cursor::TypeNorthSouthResize: return "ns-resize";
    case Cursor::TypeEastWestResize: return "ew-resize";
    case Cursor::TypeNorthEastSouthWestResize: return "nesw-resize";
    case Cursor::TypeNorthWestSouthEastResize: return "nwse-resize";
    case Cursor::TypeColumnResize: return "col-resize";
    case Cursor::TypeRowResize: return "row-resize";
    case Cursor::TypeMiddlePanning: return "m-panning";
    case Cursor::TypeEastPanning: return "e-panning";
    case Cursor::TypeNorthPanning: return "n-panning";
    case Cursor::TypeNorthEastPanning: return "ne-panning";
    case Cursor::TypeNorthWestPanning: return "nw-panning";
    case Cursor::TypeSouthPanning: return "s-panning";
    case Cursor::TypeSouthEastPanning: return "se-panning";
    case Cursor::TypeSouthWestPanning: return "sw-panning";
    case Cursor::TypeWestPanning: return "w-panning";
    case Cursor::TypeMove: return "move";
    case Cursor::TypeVerticalText: return "vertical-text";
    case Cursor::TypeCell: return "cell";
    case Cursor::TypeContextMenu: return "context-menu";
    case Cursor::TypeAlias: return "alias";
    case Cursor::TypeProgress: return "progress";
    case Cursor::TypeNoDrop: return "nodrop";
    case Cursor::TypeCopy: return "copy";
    case Cursor::TypeNone: return "none";
    case Cursor::TypeNotAllowed: return "not-allowed";
    case Cursor::TypeZoomIn: return "zoom-in";
    case Cursor::TypeZoomOut: return "zoom-out";
    case Cursor::TypeGrab: return "grab";
    case Cursor::TypeGrabbing: return "grabbing";
    case Cursor::TypeCustom: return "custom";
    default: return "default";
  }
}

}  // namespace atom
